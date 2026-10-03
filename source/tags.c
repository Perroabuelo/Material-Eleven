#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <string.h>

#include <FLAC/metadata.h>
#include <mpg123.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "common.h"
#include "fs.h"
#include "opus/opusfile.h"
#include "tags.h"
#include "track_meta.h"
#include "xmp.h"

// mpg123 pide una inicializacion global antes del primer handle. La ruta de
// reproduccion la hace por su cuenta en MP3_Init; aqui se hace una vez y se
// deja hecha, porque la pasada de tags abre y cierra un handle por pista.
static SceBool tags_mpg123_ready = SCE_FALSE;

static void Tags_Set(char *dst, const char *src) {
	if (src == NULL)
		return;

	// Los saltos y tabuladores se sustituyen aqui y no al escribir el indice,
	// para que lo que se guarda y lo que se muestra sean lo mismo.
	size_t n = 0;

	for (const char *p = src; *p != '\0' && n + 1 < TAGS_FIELD_MAX; p++)
		dst[n++] = ((unsigned char)*p < 0x20) ? ' ' : *p;

	// Sin espacios de cola: un tag rellenado a lo ancho ordenaria distinto que
	// el mismo tag sin rellenar.
	while (n > 0 && dst[n - 1] == ' ')
		n--;

	dst[n] = '\0';
}

// --- Vorbis comments, que comparten FLAC, OGG y OPUS -----------------------

static void Tags_TakeVorbisComment(Tags *out, const char *comment) {
	if (!strncasecmp("TITLE=", comment, 6))
		Tags_Set(out->title, comment + 6);
	else if (!strncasecmp("ARTIST=", comment, 7))
		Tags_Set(out->artist, comment + 7);
	else if (!strncasecmp("ALBUM=", comment, 6))
		Tags_Set(out->album, comment + 6);
	else if (!strncasecmp("TRACKNUMBER=", comment, 12))
		out->track = TrackMeta_ParseNumber(comment + 12);
	else if (!strncasecmp("DISCNUMBER=", comment, 11))
		out->disc = TrackMeta_ParseNumber(comment + 11);
}

static SceBool Tags_ReadFlac(const char *path, Tags *out) {
	FLAC__StreamMetadata *tags = NULL;

	// Lee solo el bloque de metadatos: no abre el decoder ni toca el audio.
	if (!FLAC__metadata_get_tags(path, &tags) || tags == NULL)
		return SCE_FALSE;

	for (unsigned int i = 0; i < tags->data.vorbis_comment.num_comments; i++)
		Tags_TakeVorbisComment(out, (const char *)tags->data.vorbis_comment.comments[i].entry);

	FLAC__metadata_object_delete(tags);
	return SCE_TRUE;
}

static size_t Tags_OggRead(void *ptr, size_t size, size_t count, void *stream) {
	return sceIoRead(*(SceUID *)stream, ptr, size * count);
}

static int Tags_OggSeek(void *stream, ogg_int64_t offset, int whence) {
	return sceIoLseek32(*(SceUID *)stream, (unsigned int)offset, whence);
}

static int Tags_OggClose(void *stream) {
	return sceIoClose(*(SceUID *)stream);
}

static long Tags_OggTell(void *stream) {
	return sceIoLseek32(*(SceUID *)stream, 0, SCE_SEEK_CUR);
}

static SceBool Tags_ReadOgg(const char *path, Tags *out) {
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);

	if (fd < 0)
		return SCE_FALSE;

	ov_callbacks cb;
	cb.read_func = Tags_OggRead;
	cb.seek_func = Tags_OggSeek;
	cb.close_func = Tags_OggClose;
	cb.tell_func = Tags_OggTell;

	OggVorbis_File vf;

	// Una cabecera dañada sale por aqui, y ov_open_callbacks se ha quedado con
	// el descriptor solo si tuvo exito.
	if (ov_open_callbacks(&fd, &vf, NULL, 0, cb) < 0) {
		sceIoClose(fd);
		return SCE_FALSE;
	}

	vorbis_comment *vc = ov_comment(&vf, -1);

	if (vc != NULL) {
		for (int i = 0; i < vc->comments; i++)
			Tags_TakeVorbisComment(out, vc->user_comments[i]);
	}

	ov_clear(&vf);
	return SCE_TRUE;
}

static SceBool Tags_ReadOpus(const char *path, Tags *out) {
	int error = 0;
	OggOpusFile *of = op_open_file(path, &error);

	if (of == NULL)
		return SCE_FALSE;

	const OpusTags *tags = op_tags(of, 0);

	if (tags != NULL) {
		if (opus_tags_query_count(tags, "title") > 0)
			Tags_Set(out->title, opus_tags_query(tags, "title", 0));
		if (opus_tags_query_count(tags, "artist") > 0)
			Tags_Set(out->artist, opus_tags_query(tags, "artist", 0));
		if (opus_tags_query_count(tags, "album") > 0)
			Tags_Set(out->album, opus_tags_query(tags, "album", 0));
		out->track = TrackMeta_ParseNumber(opus_tags_query(tags, "tracknumber", 0));
		out->disc = TrackMeta_ParseNumber(opus_tags_query(tags, "discnumber", 0));
	}

	op_free(of);
	return SCE_TRUE;
}

// --- ID3 -------------------------------------------------------------------

static void Tags_TakeMpgString(char *dst, mpg123_string *s) {
	if (s != NULL && s->p != NULL)
		Tags_Set(dst, s->p);
}

// Los numeros no tienen puntero propio en mpg123_id3v2 como el titulo: hay que
// buscarlos entre los frames de texto. El id son 4 bytes sin terminador.
static int Tags_Id3v2Number(const mpg123_id3v2 *v2, const char *id) {
	for (size_t i = 0; i < v2->texts; i++) {
		const mpg123_text *t = &v2->text[i];

		if (!memcmp(t->id, id, 4) && t->text.p != NULL)
			return TrackMeta_ParseNumber(t->text.p);
	}

	return 0;
}

static SceBool Tags_ReadMp3(const char *path, Tags *out) {
	if (!tags_mpg123_ready) {
		if (mpg123_init() != MPG123_OK)
			return SCE_FALSE;

		tags_mpg123_ready = SCE_TRUE;
	}

	int error = 0;
	mpg123_handle *h = mpg123_new(NULL, &error);

	if (h == NULL)
		return SCE_FALSE;

	// Sin MPG123_PICTURE: la caratula es la fase 3 y se paga una vez por album,
	// no una vez por pista aqui.
	if (mpg123_open(h, path) != MPG123_OK) {
		mpg123_delete(h);
		return SCE_FALSE;
	}

	mpg123_id3v1 *v1 = NULL;
	mpg123_id3v2 *v2 = NULL;
	SceBool ok = SCE_FALSE;

	if ((mpg123_meta_check(h) & MPG123_ID3) && mpg123_id3(h, &v1, &v2) == MPG123_OK) {
		if (v2 != NULL) {
			Tags_TakeMpgString(out->title, v2->title);
			Tags_TakeMpgString(out->artist, v2->artist);
			Tags_TakeMpgString(out->album, v2->album);
			out->track = Tags_Id3v2Number(v2, "TRCK");
			out->disc = Tags_Id3v2Number(v2, "TPOS");
		}

		// ID3v1 rellena solo lo que v2 no trajo, y sus campos no llevan
		// terminador: son 30 bytes a lo sumo.
		if (v1 != NULL) {
			if (out->title[0] == '\0') {
				char buf[31];
				snprintf(buf, sizeof(buf), "%.30s", v1->title);
				Tags_Set(out->title, buf);
			}
			if (out->artist[0] == '\0') {
				char buf[31];
				snprintf(buf, sizeof(buf), "%.30s", v1->artist);
				Tags_Set(out->artist, buf);
			}
			if (out->album[0] == '\0') {
				char buf[31];
				snprintf(buf, sizeof(buf), "%.30s", v1->album);
				Tags_Set(out->album, buf);
			}
			if (out->track == 0)
				out->track = TrackMeta_Id3v1Track((const unsigned char *)v1->comment);
		}

		ok = SCE_TRUE;
	}

	mpg123_close(h);
	mpg123_delete(h);
	return ok;
}

// --- modulos de tracker ----------------------------------------------------

static SceBool Tags_ReadModule(const char *path, Tags *out) {
	struct xmp_test_info info;
	char mutable_path[512];

	memset(&info, 0, sizeof(info));

	// xmp_test_module pide char* y no const char*.
	snprintf(mutable_path, sizeof(mutable_path), "%s", path);

	// Lee la cabecera y nada mas: xmp_load_module, que es lo que usa la ruta de
	// reproduccion, cargaria patrones y muestras que aqui no hacen falta.
	if (xmp_test_module(mutable_path, &info) != 0)
		return SCE_FALSE;

	Tags_Set(out->title, info.name);
	return SCE_TRUE;
}

// ---------------------------------------------------------------------------

SceBool Tags_Read(const char *path, const char *ext, Tags *out) {
	if (out == NULL)
		return SCE_FALSE;

	memset(out, 0, sizeof(*out));

	if (path == NULL || ext == NULL)
		return SCE_FALSE;

	if (!strcasecmp(ext, "flac"))
		return Tags_ReadFlac(path, out);
	if (!strcasecmp(ext, "mp3"))
		return Tags_ReadMp3(path, out);
	if (!strcasecmp(ext, "ogg"))
		return Tags_ReadOgg(path, out);
	if (!strcasecmp(ext, "opus"))
		return Tags_ReadOpus(path, out);
	if (!strcasecmp(ext, "mod") || !strcasecmp(ext, "xm") ||
		!strcasecmp(ext, "s3m") || !strcasecmp(ext, "it"))
		return Tags_ReadModule(path, out);

	// WAV llega aqui: no lleva metadatos embebidos que esta aplicacion sepa
	// leer, y su respaldo es el nombre del archivo.
	return SCE_FALSE;
}
