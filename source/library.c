#include <psp2/ctrl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/rtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fs.h"
#include "library.h"
#include "nav_rail.h"
#include "ui_theme.h"
#include "utils.h"

#define LIBRARY_ROOT_FILE   "ux0:data/ElevenMPV/libraryroot.txt"
#define LIBRARY_INDEX_FILE  "ux0:data/ElevenMPV/library.txt"

// Una version que no se reconoce no se migra: se descarta y se ofrece
// reescanear. Sale gratis porque no hay nada del usuario que perder.
#define LIBRARY_INDEX_MAGIC   "ELEVENMPV_LIBRARY"
#define LIBRARY_INDEX_VERSION 1

// Entradas de directorio por fotograma. A los 595 us por entrada que midio el
// grupo 0 sobre una coleccion real, dieciseis salen a unos 9,5 ms: cabe en el
// fotograma con margen. Es una constante de ajuste, no una decision de diseño.
#define LIBRARY_ENTRIES_PER_FRAME 16

#define LIBRARY_INITIAL_CAPACITY 64
#define LIBRARY_WRITE_BUFFER (32 * 1024)

typedef struct {
	char path[LIBRARY_PATH_MAX];
	int depth;
} Library_Pending;

static char library_root[LIBRARY_PATH_MAX] = "";
static SceBool library_has_root = SCE_FALSE;

static Library_Track *library_tracks = NULL;
static int library_count = 0;
static int library_capacity = 0;
static SceBool library_built = SCE_FALSE;
static SceBool library_truncated = SCE_FALSE;

// ---------------------------------------------------------------------------
// la lista

static SceBool Library_Grow(void) {
	int next = (library_capacity == 0) ? LIBRARY_INITIAL_CAPACITY : library_capacity * 2;

	if (next > LIBRARY_MAX_TRACKS)
		next = LIBRARY_MAX_TRACKS;

	if (next <= library_capacity)
		return SCE_FALSE;

	Library_Track *grown = (Library_Track *)realloc(library_tracks, (size_t)next * sizeof(Library_Track));

	if (grown == NULL)
		return SCE_FALSE;

	library_tracks = grown;
	library_capacity = next;
	return SCE_TRUE;
}

static Library_Track *Library_Append(void) {
	if (library_count >= LIBRARY_MAX_TRACKS) {
		library_truncated = SCE_TRUE;
		return NULL;
	}

	if ((library_count == library_capacity) && !Library_Grow())
		return NULL;

	Library_Track *track = &library_tracks[library_count];
	memset(track, 0, sizeof(*track));
	library_count++;
	return track;
}

void Library_Free(void) {
	free(library_tracks);
	library_tracks = NULL;
	library_count = 0;
	library_capacity = 0;
	library_built = SCE_FALSE;
	library_truncated = SCE_FALSE;
}

int Library_Count(void) {
	return library_count;
}

const Library_Track *Library_GetTrack(int index) {
	if (index < 0 || index >= library_count)
		return NULL;

	return &library_tracks[index];
}

SceBool Library_IsTruncated(void) {
	return library_truncated;
}

SceBool Library_IsBuilt(void) {
	return library_built;
}

// ---------------------------------------------------------------------------
// la carpeta de escaneo

void Library_LoadRoot(void) {
	library_root[0] = '\0';
	library_has_root = SCE_FALSE;

	SceOff size = 0;

	if (R_FAILED(FS_GetFileSize(LIBRARY_ROOT_FILE, &size)) || size <= 0 || size >= LIBRARY_PATH_MAX)
		return;

	char buf[LIBRARY_PATH_MAX];

	if (FS_ReadFile(LIBRARY_ROOT_FILE, buf, (int)size) != (int)size)
		return;

	buf[size] = '\0';

	// Una linea, como lastdir.txt.
	char *end = strpbrk(buf, "\r\n");

	if (end != NULL)
		*end = '\0';

	if (buf[0] == '\0')
		return;

	snprintf(library_root, LIBRARY_PATH_MAX, "%s", buf);
	library_has_root = SCE_TRUE;
}

SceBool Library_HasRoot(void) {
	return library_has_root;
}

SceBool Library_RootAvailable(void) {
	return library_has_root && FS_DirExists(library_root);
}

const char *Library_GetRoot(void) {
	return library_root;
}

void Library_SetRoot(const char *path) {
	if (path == NULL || path[0] == '\0')
		return;

	// La carpeta nueva no hereda nada de la anterior: el spec pide que ninguna
	// pista de la vieja permanezca. Tambien en disco, para no depender solo de
	// que la comprobacion de cabecera lo atrape al arrancar.
	Library_Free();
	sceIoRemove(LIBRARY_INDEX_FILE);

	snprintf(library_root, LIBRARY_PATH_MAX, "%s", path);

	// Con barra final, que es como el productor de carpeta compone rutas.
	size_t len = strlen(library_root);

	if (len > 0 && library_root[len - 1] != '/' && len + 1 < LIBRARY_PATH_MAX) {
		library_root[len] = '/';
		library_root[len + 1] = '\0';
	}

	library_has_root = SCE_TRUE;

	char line[LIBRARY_PATH_MAX + 2];
	int written = snprintf(line, sizeof(line), "%s\n", library_root);
	FS_WriteFile(LIBRARY_ROOT_FILE, line, written);
}

// ---------------------------------------------------------------------------
// persistencia

// Los tags pueden traer cualquier cosa; un tabulador o un salto de linea
// romperia el formato. Se sustituyen al escribir, no al indexar, porque lo que
// esta limitado es el archivo y no el dato.
static void Library_WriteField(char *dst, size_t cap, const char *src) {
	size_t n = 0;

	for (const char *p = src; *p != '\0' && n + 1 < cap; p++)
		dst[n++] = ((unsigned char)*p < 0x20) ? ' ' : *p;

	dst[n] = '\0';
}

SceBool Library_Save(void) {
	SceUID fd = sceIoOpen(LIBRARY_INDEX_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);

	if (fd < 0)
		return SCE_FALSE;

	char *buf = (char *)malloc(LIBRARY_WRITE_BUFFER);

	if (buf == NULL) {
		sceIoClose(fd);
		return SCE_FALSE;
	}

	int used = 0;
	SceBool ok = SCE_TRUE;

	// La cabecera lleva version, carpeta y numero de pistas. El numero es lo que
	// luego permite decir si el archivo quedo a medias.
	used += snprintf(buf + used, LIBRARY_WRITE_BUFFER - used, "%s\t%d\n", LIBRARY_INDEX_MAGIC, LIBRARY_INDEX_VERSION);
	used += snprintf(buf + used, LIBRARY_WRITE_BUFFER - used, "root\t%s\n", library_root);
	used += snprintf(buf + used, LIBRARY_WRITE_BUFFER - used, "count\t%d\n", library_count);

	for (int i = 0; i < library_count && ok; i++) {
		const Library_Track *t = &library_tracks[i];
		char title[LIBRARY_TAG_MAX], artist[LIBRARY_TAG_MAX], album[LIBRARY_TAG_MAX];

		Library_WriteField(title, sizeof(title), t->title);
		Library_WriteField(artist, sizeof(artist), t->artist);
		Library_WriteField(album, sizeof(album), t->album);

		// La ruta va la ultima: si alguna vez trae un tabulador, no corre los
		// campos de detras.
		char line[LIBRARY_PATH_MAX + 4 * LIBRARY_TAG_MAX + 64];
		int len = snprintf(line, sizeof(line), "%llu\t%llu\t%s\t%s\t%s\t%s\t%s\n",
			(unsigned long long)t->size, (unsigned long long)t->mtime, t->ext, title, artist, album, t->path);

		if (len >= (int)sizeof(line))
			len = (int)sizeof(line) - 1;

		if (used + len >= LIBRARY_WRITE_BUFFER) {
			if (sceIoWrite(fd, buf, used) != used)
				ok = SCE_FALSE;
			used = 0;
		}

		memcpy(buf + used, line, len);
		used += len;
	}

	if (ok && used > 0 && sceIoWrite(fd, buf, used) != used)
		ok = SCE_FALSE;

	free(buf);
	sceIoClose(fd);
	return ok;
}

// Parte la linea por tabuladores en sitio. Devuelve cuantos campos salieron;
// el ultimo se queda con todo lo que sobre, que es donde va la ruta.
static int Library_SplitFields(char *line, char **fields, int max_fields) {
	int n = 0;
	char *p = line;

	while (n < max_fields) {
		fields[n++] = p;

		if (n == max_fields)
			break;

		char *tab = strchr(p, '\t');

		if (tab == NULL)
			break;

		*tab = '\0';
		p = tab + 1;
	}

	return n;
}

SceBool Library_Load(void) {
	Library_Free();

	SceOff size = 0;

	if (R_FAILED(FS_GetFileSize(LIBRARY_INDEX_FILE, &size)) || size <= 0)
		return SCE_FALSE;

	char *buf = (char *)malloc((size_t)size + 1);

	if (buf == NULL)
		return SCE_FALSE;

	if (FS_ReadFile(LIBRARY_INDEX_FILE, buf, (int)size) != (int)size) {
		free(buf);
		return SCE_FALSE;
	}

	buf[size] = '\0';

	char *cursor = buf;
	char *line = NULL;
	int declared = -1;
	SceBool header_ok = SCE_FALSE;
	int header_lines = 0;

	while ((line = cursor) != NULL && *cursor != '\0') {
		char *nl = strchr(cursor, '\n');

		if (nl != NULL) {
			*nl = '\0';
			cursor = nl + 1;
		}
		else
			cursor += strlen(cursor);

		if (line[0] == '\0')
			continue;

		char *fields[7];

		if (header_lines < 3) {
			int n = Library_SplitFields(line, fields, 2);
			header_lines++;

			if (header_lines == 1) {
				// Version desconocida: se descarta entero.
				if (n < 2 || strcmp(fields[0], LIBRARY_INDEX_MAGIC) != 0 || atoi(fields[1]) != LIBRARY_INDEX_VERSION)
					break;
			}
			else if (header_lines == 2) {
				if (n < 2 || strcmp(fields[0], "root") != 0)
					break;

				// libraryroot.txt manda, no la cabecera: un indice que habla de
				// otra carpeta es de antes de cambiarla, y cargarlo dejaria pistas
				// de la carpeta vieja en la biblioteca nueva.
				if (!library_has_root || strcmp(fields[1], library_root) != 0)
					break;
			}
			else {
				if (n < 2 || strcmp(fields[0], "count") != 0)
					break;

				declared = atoi(fields[1]);
				header_ok = SCE_TRUE;
			}

			continue;
		}

		if (Library_SplitFields(line, fields, 7) < 7)
			break;

		Library_Track *t = Library_Append();

		if (t == NULL)
			break;

		t->size = (SceOff)strtoull(fields[0], NULL, 10);
		t->mtime = (SceUInt64)strtoull(fields[1], NULL, 10);
		snprintf(t->ext, LIBRARY_EXT_MAX, "%s", fields[2]);
		snprintf(t->title, LIBRARY_TAG_MAX, "%s", fields[3]);
		snprintf(t->artist, LIBRARY_TAG_MAX, "%s", fields[4]);
		snprintf(t->album, LIBRARY_TAG_MAX, "%s", fields[5]);
		snprintf(t->path, LIBRARY_PATH_MAX, "%s", fields[6]);
	}

	free(buf);

	// Incompleto es tan malo como ilegible: un corte de energia a mitad de
	// escritura deja menos lineas de las que la cabecera promete, y enseñar eso
	// como si fuera la coleccion es peor que ofrecer reescanear.
	if (!header_ok || declared != library_count) {
		Library_Free();
		return SCE_FALSE;
	}

	library_built = SCE_TRUE;
	return SCE_TRUE;
}

// ---------------------------------------------------------------------------
// el recorrido

static void Library_JoinPath(char *dst, size_t cap, const char *dir, const char *name, int trailing_slash) {
	size_t n = 0;

	for (const char *p = dir; *p != '\0' && n + 1 < cap; p++)
		dst[n++] = *p;

	for (const char *p = name; *p != '\0' && n + 1 < cap; p++)
		dst[n++] = *p;

	if (trailing_slash && n + 1 < cap)
		dst[n++] = '/';

	dst[n] = '\0';
}

// Cabe entero lo unido, o no se anota: una ruta truncada no abre ningun archivo,
// asi que apuntarla seria apuntar una pista rota.
static SceBool Library_FitsPath(const char *dir, const char *name, int trailing_slash) {
	return (strlen(dir) + strlen(name) + (trailing_slash ? 1 : 0) + 1) <= LIBRARY_PATH_MAX;
}

static void Library_DrawScanProgress(const char *folder, int found, int skipped) {
	char detail[128];

	vita2d_start_drawing();
	vita2d_clear_screen();

	float x = 80.0f, y = 200.0f;

	UI_DrawText(UI_FACE_UI, UI_TS_TITLE, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_TITLE, y, 30), UI_COLOR_TEXT_PRIMARY, "Escaneando la biblioteca");
	y += 44.0f;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, 26), 960.0f - x - 80.0f,
		UI_COLOR_TEXT_SECONDARY, folder);
	y += 34.0f;

	snprintf(detail, sizeof(detail), "%d pistas encontradas", found);
	UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, y, 24), UI_COLOR_TEXT_TERTIARY, detail);
	y += 28.0f;

	if (skipped > 0) {
		snprintf(detail, sizeof(detail), "%d omitidas por ruta demasiado larga", skipped);
		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, 22), UI_COLOR_TEXT_MUTED, detail);
		y += 26.0f;
	}

	if (library_truncated) {
		snprintf(detail, sizeof(detail), "Limite de %d pistas alcanzado", LIBRARY_MAX_TRACKS);
		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, 22), UI_COLOR_TEXT_MUTED, detail);
	}

	const char *hints[] = { NULL, "Abandonar", NULL, NULL, NULL };
	NavRail_DrawHintBar(544 - UI_HINT_BAR_HEIGHT, hints, 5);

	vita2d_end_drawing();
	vita2d_swap_buffers();
}

SceBool Library_RunScan(void) {
	// Sin carpeta, o con una que ya no esta, no se toca nada: recorrer una
	// raiz ausente dejaria cero pistas y se llevaria por delante un indice
	// que sigue siendo bueno para cuando la tarjeta vuelva.
	if (!Library_RootAvailable())
		return SCE_FALSE;

	Library_Free();

	// Una pila explicita y no recursion: el arbol lo elige el usuario y puede
	// ser arbitrariamente profundo, y la pila de un hilo aqui es limitada. Es
	// ademas lo que permite suspender el escaneo entre fotogramas, porque el
	// estado del escaneo es esta pila.
	Library_Pending *stack = (Library_Pending *)malloc(sizeof(Library_Pending) * LIBRARY_MAX_DEPTH * 8);

	if (stack == NULL)
		return SCE_FALSE;

	const int stack_max = LIBRARY_MAX_DEPTH * 8;
	int top = 0;
	int skipped = 0;
	SceBool abandoned = SCE_FALSE;

	snprintf(stack[top].path, LIBRARY_PATH_MAX, "%s", library_root);
	stack[top].depth = 0;
	top++;

	SceUID dir = -1;
	char dir_path[LIBRARY_PATH_MAX] = "";
	int dir_depth = 0;

	while (SCE_TRUE) {
		// Un trozo acotado por fotograma, y despues dibujar y leer el pad. Es la
		// forma que Menu_PromptFilter ya establecio para correr un lazo propio
		// sin perder ni el dibujado ni la salida.
		for (int budget = 0; budget < LIBRARY_ENTRIES_PER_FRAME; budget++) {
			if (dir < 0) {
				if (top == 0)
					break;

				top--;
				snprintf(dir_path, LIBRARY_PATH_MAX, "%s", stack[top].path);
				dir_depth = stack[top].depth;
				dir = sceIoDopen(dir_path);

				// Una carpeta que no abre no detiene el escaneo: se deja fuera
				// solo ella y se sigue con las demas.
				if (dir < 0) {
					dir = -1;
					continue;
				}
			}

			SceIoDirent entry;
			memset(&entry, 0, sizeof(entry));

			// Nunca hay mas de una SceIoDirent viva: el escaner no reserva un
			// arreglo de entradas, asi que no tiene techo por carpeta y no paga
			// los ~360 KB que cuesta cada lectura de directorio del navegador.
			if (sceIoDread(dir, &entry) <= 0) {
				sceIoDclose(dir);
				dir = -1;
				continue;
			}

			if (SCE_S_ISDIR(entry.d_stat.st_mode)) {
				if (!strcmp(entry.d_name, ".") || !strcmp(entry.d_name, ".."))
					continue;

				if (dir_depth + 1 >= LIBRARY_MAX_DEPTH || top >= stack_max)
					continue;

				if (!Library_FitsPath(dir_path, entry.d_name, 1)) {
					skipped++;
					continue;
				}

				Library_JoinPath(stack[top].path, LIBRARY_PATH_MAX, dir_path, entry.d_name, 1);
				stack[top].depth = dir_depth + 1;
				top++;
				continue;
			}

			if (!FS_IsPlayableExt(FS_GetFileExt(entry.d_name)))
				continue;

			if (!Library_FitsPath(dir_path, entry.d_name, 0)) {
				skipped++;
				continue;
			}

			Library_Track *track = Library_Append();

			if (track == NULL) {
				// Techo alcanzado: se deja de anotar, pero el escaneo termina
				// ordenadamente y lo dice.
				break;
			}

			Library_JoinPath(track->path, LIBRARY_PATH_MAX, dir_path, entry.d_name, 0);
			snprintf(track->ext, LIBRARY_EXT_MAX, "%s", FS_GetFileExt(entry.d_name));

			// d_stat viene relleno por el propio dread: tamaño y fecha no
			// cuestan una llamada aparte.
			track->size = entry.d_stat.st_size;
			sceRtcGetTime64_t(&entry.d_stat.st_mtime, &track->mtime);
		}

		if (library_truncated || (dir < 0 && top == 0))
			break;

		Library_DrawScanProgress(dir_path, library_count, skipped);
		Utils_ReadControls();

		if (pressed & SCE_CTRL_CANCEL) {
			abandoned = SCE_TRUE;
			break;
		}
	}

	if (dir >= 0)
		sceIoDclose(dir);

	free(stack);

	if (abandoned) {
		// Abandonar deja la biblioteca como estaba, no a medias: el indice en
		// disco sigue siendo el de antes y se recarga.
		Library_Free();
		Library_Load();
		return SCE_FALSE;
	}

	library_built = SCE_TRUE;
	Library_Save();
	return SCE_TRUE;
}
