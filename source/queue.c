#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fs.h"
#include "queue.h"
#include "utils.h"

// Techo declarado, no tamaño reservado: la cola crece hasta aqui y no mas, pero
// una carpeta de doce canciones cuesta doce rutas y no el techo. Coincide con el
// techo de pistas de la biblioteca que fijo la medicion del grupo 0, para que una
// vista de biblioteca llena quepa entera en la cola.
#define QUEUE_MAX_TRACKS 4000

// Cuanto se reserva la primera vez, y desde donde se duplica. Una carpeta de
// album cabe entera sin un solo realloc.
#define QUEUE_INITIAL_CAPACITY 32

// Lo que admite una ruta al componerla. Es el mismo limite que tenia la fila del
// arreglo estatico, de modo que lo que se truncaba antes se trunca igual.
#define QUEUE_PATH_MAX 512

static char **queue_paths = NULL;
// En paralelo a las rutas, y con NULL donde el productor no trajo nombre.
static char **queue_titles = NULL;
static int queue_count = 0;
static int queue_capacity = 0;
static int queue_position = 0;

// Une carpeta y nombre acotando a mano, que es lo mismo que hace el navegador
// de carpetas y evita que el compilador tenga que razonar sobre el truncado de
// snprintf entre dos buffers del mismo tamaño.
static void Queue_JoinPath(char *dst, size_t cap, const char *dir, const char *name) {
	size_t n = 0;

	for (const char *p = dir; *p != '\0' && n + 1 < cap; p++)
		dst[n++] = *p;

	for (const char *p = name; *p != '\0' && n + 1 < cap; p++)
		dst[n++] = *p;

	dst[n] = '\0';
}

static SceBool Queue_Grow(void) {
	int next = (queue_capacity == 0) ? QUEUE_INITIAL_CAPACITY : queue_capacity * 2;

	if (next > QUEUE_MAX_TRACKS)
		next = QUEUE_MAX_TRACKS;

	if (next <= queue_capacity)
		return SCE_FALSE;

	char **grown = (char **)realloc(queue_paths, (size_t)next * sizeof(char *));

	if (grown == NULL)
		return SCE_FALSE;

	queue_paths = grown;

	char **grown_titles = (char **)realloc(queue_titles, (size_t)next * sizeof(char *));

	if (grown_titles == NULL)
		return SCE_FALSE;

	queue_titles = grown_titles;
	queue_capacity = next;
	return SCE_TRUE;
}

void Queue_Clear(void) {
	for (int i = 0; i < queue_count; i++) {
		free(queue_paths[i]);
		free(queue_titles[i]);
	}

	free(queue_paths);
	free(queue_titles);
	queue_paths = NULL;
	queue_titles = NULL;
	queue_count = 0;
	queue_capacity = 0;
	queue_position = 0;
}

static char *Queue_Dup(const char *s) {
	if (s == NULL || s[0] == '\0')
		return NULL;

	size_t len = strlen(s) + 1;
	char *copy = (char *)malloc(len);

	if (copy != NULL)
		memcpy(copy, s, len);

	return copy;
}

SceBool Queue_Add(const char *path, const char *title) {
	if (path == NULL || queue_count >= QUEUE_MAX_TRACKS)
		return SCE_FALSE;

	if ((queue_count == queue_capacity) && !Queue_Grow())
		return SCE_FALSE;

	// Cada ruta ocupa lo que mide, no lo que midan las demas: donde el arreglo
	// estatico gastaba 512 bytes por pista tuviera la ruta el largo que tuviera,
	// aqui una ruta de 78 bytes - que es la media medida en una coleccion real -
	// cuesta 79.
	char *copy = Queue_Dup(path);

	if (copy == NULL)
		return SCE_FALSE;

	queue_paths[queue_count] = copy;
	queue_titles[queue_count] = Queue_Dup(title);
	queue_count++;
	return SCE_TRUE;
}

int Queue_Count(void) {
	return queue_count;
}

const char *Queue_GetPath(int index) {
	if (index < 0 || index >= queue_count)
		return NULL;

	return queue_paths[index];
}

const char *Queue_GetTitle(int index) {
	if (index < 0 || index >= queue_count)
		return NULL;

	return queue_titles[index];
}

int Queue_GetPosition(void) {
	return queue_position;
}

void Queue_SetPosition(int index) {
	queue_position = index;
}

int Queue_IndexOf(const char *path) {
	if (path == NULL)
		return 0;

	for (int i = 0; i < queue_count; i++) {
		if (!strcmp(queue_paths[i], path))
			return i;
	}

	return 0;
}

// Productor de carpeta. Es el Menu_GetMusicList de menu_audioplayer.c movido
// tal cual: mismas extensiones reconocidas, mismo Utils_Alphasort, mismo no
// mirar si la entrada es carpeta. Lo que entra en la cola y en que orden no
// cambia; lo unico que cambia es quien es el dueño de la lista.
int Queue_FillFromFolder(const char *dir) {
	SceUID fd = 0;

	if (R_SUCCEEDED(fd = sceIoDopen(dir))) {
		int entryCount = 0, i = 0;
		SceIoDirent *entries = (SceIoDirent *)calloc(MAX_FILES, sizeof(SceIoDirent));

		while ((entryCount < MAX_FILES) && (sceIoDread(fd, &entries[entryCount]) > 0))
			entryCount++;

		sceIoDclose(fd);
		qsort(entries, entryCount, sizeof(SceIoDirent), Utils_Alphasort);

		for (i = 0; i < entryCount; i++) {
			if ((!strncasecmp(FS_GetFileExt(entries[i].d_name), "flac", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "it", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "mod", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "mp3", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "ogg", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "opus", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "s3m", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "wav", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "xm", 4))) {
				char path[QUEUE_PATH_MAX];
				Queue_JoinPath(path, sizeof(path), dir, entries[i].d_name);
				// Sin nombre: una carpeta no sabe mas que sus rutas.
				Queue_Add(path, NULL);
			}
		}

		free(entries);
	}
	else
		return fd;

	return 0;
}
