#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fs.h"
#include "queue.h"
#include "utils.h"

// La reserva es, por ahora, exactamente la que tenia el playlist[1024][512]
// estatico de menu_audioplayer.c, para que mudar la cola aqui no cambie ni el
// consumo ni el comportamiento. Dimensionarla a la cola real es el paso
// siguiente, y queda dentro de este archivo.
#define QUEUE_MAX_TRACKS 1024
#define QUEUE_PATH_MAX   512

static char queue_paths[QUEUE_MAX_TRACKS][QUEUE_PATH_MAX];
static int queue_count = 0;
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

void Queue_Clear(void) {
	queue_count = 0;
	queue_position = 0;
}

SceBool Queue_Add(const char *path) {
	if (path == NULL || queue_count >= QUEUE_MAX_TRACKS)
		return SCE_FALSE;

	// Acotado, a diferencia del strcpy que esto releva: una ruta mas larga que
	// la fila se queda truncada en vez de escribir en la fila siguiente.
	snprintf(queue_paths[queue_count], QUEUE_PATH_MAX, "%s", path);
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
				Queue_Add(path);
			}
		}

		free(entries);
	}
	else
		return fd;

	return 0;
}
