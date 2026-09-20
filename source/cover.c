#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FLAC/metadata.h>
#include <mpg123.h>

#include "common.h"
#include "cover.h"
#include "fs.h"
#include "opus/opusfile.h"
#include "ui_gpu.h"

#define COVER_DIR      "ux0:data/ElevenMPV/covers"
#define COVER_HEADER   128
#define COVER_KEY_MAX  (COVER_HEADER - 12)
#define COVER_PIXELS   (COVER_SIZE * COVER_SIZE * 4)

// Cuantas miniaturas se tienen decodificadas a la vez. A 64 KB cada una son
// 1 MB: cabe una pagina de lista con margen para desplazarse sin releer.
#define COVER_CACHE_SLOTS 16

typedef struct {
	char key[COVER_KEY_MAX];
	vita2d_texture *texture;
	SceBool empty;   // la entrada existe y dice que no hay caratula
	unsigned int age;
} Cover_Slot;

static Cover_Slot cover_slots[COVER_CACHE_SLOTS];
static unsigned int cover_clock = 0;
static SceBool cover_mpg123_ready = SCE_FALSE;

// ---------------------------------------------------------------------------
// la clave y su archivo

// La clave es el album cuando lo hay, y la ruta cuando no. El archivo se nombra
// por un hash de la clave porque un album puede llamarse cualquier cosa, incluida
// alguna que no valga como nombre de archivo; la clave entera va dentro para que
// una colision se note en vez de servir la caratula de otro disco.
static void Cover_Key(char *dst, size_t cap, const char *album, const char *path) {
	const char *key = (album != NULL && album[0] != '\0') ? album : path;
	snprintf(dst, cap, "%s", (key != NULL) ? key : "");
}

static unsigned int Cover_Hash(const char *key) {
	unsigned int h = 2166136261u;

	for (const char *p = key; *p != '\0'; p++) {
		h ^= (unsigned char)*p;
		h *= 16777619u;
	}

	return h;
}

static void Cover_FilePath(char *dst, size_t cap, const char *key) {
	snprintf(dst, cap, "%s/%08x.cv", COVER_DIR, Cover_Hash(key));
}

// ---------------------------------------------------------------------------
// el archivo de cache

static void Cover_WriteHeader(unsigned char *h, const char *key, SceBool has_image) {
	memset(h, 0, COVER_HEADER);
	memcpy(h, "EMPVCV", 6);
	h[6] = 1;                      // version
	h[7] = has_image ? 1 : 0;
	h[8] = COVER_SIZE & 0xFF;
	h[9] = (COVER_SIZE >> 8) & 0xFF;
	h[10] = COVER_SIZE & 0xFF;
	h[11] = (COVER_SIZE >> 8) & 0xFF;
	snprintf((char *)h + 12, COVER_KEY_MAX, "%s", key);
}

// Devuelve 1 con imagen, 0 sin imagen, -1 si no hay entrada utilizable.
static int Cover_ReadEntry(const char *key, unsigned char *pixels) {
	char file[512];
	Cover_FilePath(file, sizeof(file), key);

	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);

	if (fd < 0)
		return -1;

	unsigned char header[COVER_HEADER];

	if (sceIoRead(fd, header, COVER_HEADER) != COVER_HEADER || memcmp(header, "EMPVCV", 6) != 0 || header[6] != 1) {
		sceIoClose(fd);
		return -1;
	}

	// La colision se detecta aqui: mismo hash, otra clave.
	if (strncmp((const char *)header + 12, key, COVER_KEY_MAX - 1) != 0) {
		sceIoClose(fd);
		return -1;
	}

	if (header[7] == 0) {
		sceIoClose(fd);
		return 0;
	}

	int read = (pixels != NULL) ? sceIoRead(fd, pixels, COVER_PIXELS) : COVER_PIXELS;
	sceIoClose(fd);
	return (read == COVER_PIXELS) ? 1 : -1;
}

static SceBool Cover_WriteEntry(const char *key, const unsigned char *pixels) {
	sceIoMkdir(COVER_DIR, 0777);

	char file[512];
	Cover_FilePath(file, sizeof(file), key);

	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);

	if (fd < 0)
		return SCE_FALSE;

	unsigned char header[COVER_HEADER];
	Cover_WriteHeader(header, key, pixels != NULL);

	SceBool ok = (sceIoWrite(fd, header, COVER_HEADER) == COVER_HEADER);

	if (ok && pixels != NULL)
		ok = (sceIoWrite(fd, pixels, COVER_PIXELS) == COVER_PIXELS);

	sceIoClose(fd);
	return ok;
}

SceBool Cover_IsCached(const char *album, const char *path) {
	char key[COVER_KEY_MAX];
	Cover_Key(key, sizeof(key), album, path);
	return (Cover_ReadEntry(key, NULL) >= 0) ? SCE_TRUE : SCE_FALSE;
}

// ---------------------------------------------------------------------------
// reescalado

// vita2d decodifica siempre en disposicion lineal, asi que los pixeles se leen
// desde la CPU. Los formatos que produce son ABGR de 4 bytes, BGR de 3, o R de 1
// para un JPEG en escala de grises; en los tres, el byte 0 de un pixel es el
// rojo. Es la misma lectura que hace el acento dinamico en ui_theme.c.
static SceBool Cover_BytesPerPixel(SceGxmTextureFormat format, unsigned int *out) {
	switch (format) {
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR: *out = 4; return SCE_TRUE;
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR: *out = 3; return SCE_TRUE;
		case SCE_GXM_TEXTURE_FORMAT_U8_R: *out = 1; return SCE_TRUE;
		default: return SCE_FALSE;
	}
}

// Promedia el rectangulo de origen que cae en cada pixel de salida. Recorre la
// imagen entera una vez - unos 360.000 pixeles para una caratula de 600x600 -,
// que al lado de los 93 ms que costo decodificarla no se nota.
static SceBool Cover_Downscale(const vita2d_texture *src, unsigned char *dst) {
	unsigned int bpp = 0;

	if (src == NULL || !Cover_BytesPerPixel(vita2d_texture_get_format(src), &bpp))
		return SCE_FALSE;

	const unsigned char *pixels = (const unsigned char *)vita2d_texture_get_datap(src);
	unsigned int w = vita2d_texture_get_width(src);
	unsigned int h = vita2d_texture_get_height(src);
	unsigned int stride = vita2d_texture_get_stride(src);

	if (pixels == NULL || w == 0 || h == 0)
		return SCE_FALSE;

	for (int oy = 0; oy < COVER_SIZE; oy++) {
		unsigned int y0 = (unsigned int)((SceUInt64)oy * h / COVER_SIZE);
		unsigned int y1 = (unsigned int)((SceUInt64)(oy + 1) * h / COVER_SIZE);

		if (y1 <= y0)
			y1 = y0 + 1;

		for (int ox = 0; ox < COVER_SIZE; ox++) {
			unsigned int x0 = (unsigned int)((SceUInt64)ox * w / COVER_SIZE);
			unsigned int x1 = (unsigned int)((SceUInt64)(ox + 1) * w / COVER_SIZE);

			if (x1 <= x0)
				x1 = x0 + 1;

			unsigned int r = 0, g = 0, b = 0, n = 0;

			for (unsigned int y = y0; y < y1 && y < h; y++) {
				const unsigned char *row = pixels + (size_t)y * stride;

				for (unsigned int x = x0; x < x1 && x < w; x++) {
					const unsigned char *p = row + (size_t)x * bpp;

					r += p[0];
					g += (bpp >= 3) ? p[1] : p[0];
					b += (bpp >= 3) ? p[2] : p[0];
					n++;
				}
			}

			if (n == 0)
				n = 1;

			unsigned char *out = dst + ((size_t)oy * COVER_SIZE + ox) * 4;
			out[0] = (unsigned char)(r / n);
			out[1] = (unsigned char)(g / n);
			out[2] = (unsigned char)(b / n);
			out[3] = 255;
		}
	}

	return SCE_TRUE;
}

// ---------------------------------------------------------------------------
// extraccion por formato
//
// Reaprovecha lo que ya existia por formato en la ruta de reproduccion
// (source/audio/flac.c, mp3.c y opus.c) en vez de reescribirlo, pero sin abrir
// el decoder: aqui no se va a sonar nada.

static vita2d_texture *Cover_FromFlac(const char *path) {
	FLAC__StreamMetadata *pic = NULL;
	const char *mimes[] = { "image/jpeg", "image/jpg", "image/png" };

	for (int i = 0; i < 3; i++) {
		if (!FLAC__metadata_get_picture(path, &pic, FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER,
			mimes[i], NULL, (unsigned)(-1), (unsigned)(-1), (unsigned)(-1), (unsigned)(-1)) || pic == NULL)
			continue;

		// data_length y no length: length es el tamaño del bloque de metadatos
		// entero, cabecera incluida, no el de la imagen.
		vita2d_texture *tex = (i == 2)
			? vita2d_load_PNG_buffer(pic->data.picture.data)
			: vita2d_load_JPEG_buffer(pic->data.picture.data, pic->data.picture.data_length);

		FLAC__metadata_object_delete(pic);
		return tex;
	}

	return NULL;
}

static vita2d_texture *Cover_FromMp3(const char *path) {
	if (!cover_mpg123_ready) {
		if (mpg123_init() != MPG123_OK)
			return NULL;

		cover_mpg123_ready = SCE_TRUE;
	}

	int error = 0;
	mpg123_handle *h = mpg123_new(NULL, &error);

	if (h == NULL)
		return NULL;

	mpg123_param(h, MPG123_ADD_FLAGS, MPG123_PICTURE, 0.0);

	if (mpg123_open(h, path) != MPG123_OK) {
		mpg123_delete(h);
		return NULL;
	}

	mpg123_id3v1 *v1 = NULL;
	mpg123_id3v2 *v2 = NULL;
	vita2d_texture *tex = NULL;

	if ((mpg123_meta_check(h) & MPG123_ID3) && mpg123_id3(h, &v1, &v2) == MPG123_OK && v2 != NULL) {
		for (size_t i = 0; i < v2->pictures && tex == NULL; i++) {
			mpg123_picture *pic = &v2->picture[i];

			if ((pic->type != 3 && pic->type != 0) || pic->mime_type.p == NULL)
				continue;

			if (!strcasecmp(pic->mime_type.p, "image/jpg") || !strcasecmp(pic->mime_type.p, "image/jpeg"))
				tex = vita2d_load_JPEG_buffer(pic->data, pic->size);
			else if (!strcasecmp(pic->mime_type.p, "image/png"))
				tex = vita2d_load_PNG_buffer(pic->data);
		}
	}

	mpg123_close(h);
	mpg123_delete(h);
	return tex;
}

static vita2d_texture *Cover_FromOpus(const char *path) {
	int error = 0;
	OggOpusFile *of = op_open_file(path, &error);

	if (of == NULL)
		return NULL;

	const OpusTags *tags = op_tags(of, 0);
	vita2d_texture *tex = NULL;

	if (tags != NULL && opus_tags_query_count(tags, "METADATA_BLOCK_PICTURE") > 0) {
		OpusPictureTag pic = { 0 };
		opus_picture_tag_init(&pic);

		if (opus_picture_tag_parse(&pic, opus_tags_query(tags, "METADATA_BLOCK_PICTURE", 0)) == 0 && pic.type == 3) {
			if (pic.format == OP_PIC_FORMAT_JPEG)
				tex = vita2d_load_JPEG_buffer(pic.data, pic.data_length);
			else if (pic.format == OP_PIC_FORMAT_PNG)
				tex = vita2d_load_PNG_buffer(pic.data);
		}

		opus_picture_tag_clear(&pic);
	}

	op_free(of);
	return tex;
}

// OGG, WAV y los modulos de tracker no llevan caratula que esta aplicacion sepa
// leer, y llegan aqui para quedarse con la marca de "no hay".
static vita2d_texture *Cover_Extract(const char *path, const char *ext) {
	if (ext == NULL)
		return NULL;

	if (!strcasecmp(ext, "flac"))
		return Cover_FromFlac(path);
	if (!strcasecmp(ext, "mp3"))
		return Cover_FromMp3(path);
	if (!strcasecmp(ext, "opus"))
		return Cover_FromOpus(path);

	return NULL;
}

SceBool Cover_Build(const char *album, const char *path, const char *ext) {
	char key[COVER_KEY_MAX];
	Cover_Key(key, sizeof(key), album, path);

	vita2d_texture *full = Cover_Extract(path, ext);

	if (full == NULL) {
		// La marca de que no hay es tan util como la imagen: sin ella se
		// reintentaria la extraccion en cada arranque.
		return Cover_WriteEntry(key, NULL);
	}

	unsigned char *pixels = (unsigned char *)malloc(COVER_PIXELS);
	SceBool ok = SCE_FALSE;

	if (pixels != NULL) {
		if (Cover_Downscale(full, pixels))
			ok = Cover_WriteEntry(key, pixels);

		free(pixels);
	}

	UI_GpuFreeTexture(&full);

	if (!ok)
		ok = Cover_WriteEntry(key, NULL);

	return ok;
}

// ---------------------------------------------------------------------------
// las texturas vivas

static Cover_Slot *Cover_FindSlot(const char *key) {
	for (int i = 0; i < COVER_CACHE_SLOTS; i++) {
		if (cover_slots[i].key[0] != '\0' && !strcmp(cover_slots[i].key, key))
			return &cover_slots[i];
	}

	return NULL;
}

static Cover_Slot *Cover_ClaimSlot(void) {
	Cover_Slot *oldest = &cover_slots[0];

	for (int i = 0; i < COVER_CACHE_SLOTS; i++) {
		if (cover_slots[i].key[0] == '\0')
			return &cover_slots[i];

		if (cover_slots[i].age < oldest->age)
			oldest = &cover_slots[i];
	}

	UI_GpuFreeTexture(&oldest->texture);
	oldest->key[0] = '\0';
	oldest->empty = SCE_FALSE;
	return oldest;
}

vita2d_texture *Cover_Get(const char *album, const char *path, int *budget) {
	char key[COVER_KEY_MAX];
	Cover_Key(key, sizeof(key), album, path);

	Cover_Slot *slot = Cover_FindSlot(key);

	if (slot != NULL) {
		slot->age = ++cover_clock;
		return slot->texture;
	}

	// Sin presupuesto no se va a disco: la lista se dibuja con el marcador y la
	// caratula llega en un fotograma proximo.
	if (budget != NULL && *budget <= 0)
		return NULL;

	if (budget != NULL)
		(*budget)--;

	unsigned char *pixels = (unsigned char *)malloc(COVER_PIXELS);

	if (pixels == NULL)
		return NULL;

	int state = Cover_ReadEntry(key, pixels);
	slot = Cover_ClaimSlot();
	snprintf(slot->key, sizeof(slot->key), "%s", key);
	slot->age = ++cover_clock;
	slot->texture = NULL;
	slot->empty = SCE_TRUE;

	if (state == 1) {
		vita2d_texture *tex = vita2d_create_empty_texture(COVER_SIZE, COVER_SIZE);

		if (tex != NULL) {
			unsigned char *dst = (unsigned char *)vita2d_texture_get_datap(tex);
			unsigned int stride = vita2d_texture_get_stride(tex);

			for (int y = 0; y < COVER_SIZE; y++)
				memcpy(dst + (size_t)y * stride, pixels + (size_t)y * COVER_SIZE * 4, COVER_SIZE * 4);

			slot->texture = tex;
			slot->empty = SCE_FALSE;
		}
	}

	free(pixels);
	return slot->texture;
}

void Cover_Free(void) {
	for (int i = 0; i < COVER_CACHE_SLOTS; i++) {
		UI_GpuFreeTexture(&cover_slots[i].texture);
		cover_slots[i].key[0] = '\0';
		cover_slots[i].empty = SCE_FALSE;
		cover_slots[i].age = 0;
	}
}
