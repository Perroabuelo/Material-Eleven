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
// El plan de reproduccion: queue_order[slot] es el indice natural de la pista
// que suena en ese puesto. Sin barajado es la identidad. Se guarda aparte en vez
// de barajar las rutas en sitio para que apagar el barajado no tenga que
// recuperar un orden natural que ya se habria perdido.
static int *queue_order = NULL;
static int queue_count = 0;
static int queue_capacity = 0;
// Puesto dentro del plan, no indice en queue_paths. Por eso no sale de aqui:
// fuera de este archivo la cola solo habla en rutas.
static int queue_slot = 0;
static SceBool queue_shuffled = SCE_FALSE;

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

	// Los tres arreglos crecen juntos o no crece ninguno a efectos de la cola.
	// Un realloc que sale bien ya movio su bloque, asi que se guarda aunque el
	// siguiente falle - soltarlo dejaria el puntero viejo colgando -, pero la
	// capacidad solo se publica cuando los tres la tienen. Antes la de rutas se
	// aplicaba sola si fallaba la de titulos, y los dos quedaban de tamaños
	// distintos bajo una misma capacidad.
	char **grown = (char **)realloc(queue_paths, (size_t)next * sizeof(char *));

	if (grown == NULL)
		return SCE_FALSE;

	queue_paths = grown;

	char **grown_titles = (char **)realloc(queue_titles, (size_t)next * sizeof(char *));

	if (grown_titles == NULL)
		return SCE_FALSE;

	queue_titles = grown_titles;

	int *grown_order = (int *)realloc(queue_order, (size_t)next * sizeof(int));

	if (grown_order == NULL)
		return SCE_FALSE;

	queue_order = grown_order;
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
	free(queue_order);
	queue_paths = NULL;
	queue_titles = NULL;
	queue_order = NULL;
	queue_count = 0;
	queue_capacity = 0;
	queue_slot = 0;
	// El barajado sobrevive: es un modo de la sesion, no de esta cola. La
	// siguiente se baraja cuando quien la llena salta a la pista elegida.
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
	// Entra al final del plan con su indice natural. Si el barajado esta
	// encendido, el salto a la pista elegida rebaraja la cola ya completa.
	queue_order[queue_count] = queue_count;
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
	return queue_slot;
}

// Indice natural de esa ruta, o -1 si no esta. A diferencia del viejo
// Queue_IndexOf, que devolvia 0, no puede disfrazar un fallo de "volver al
// principio".
static int Queue_Find(const char *path) {
	if (path == NULL)
		return -1;

	for (int i = 0; i < queue_count; i++) {
		if (!strcmp(queue_paths[i], path))
			return i;
	}

	return -1;
}

// Rehace el plan dejando `first` (indice natural) en el slot 0. Sin barajado
// es la identidad y `first` no se mueve de su sitio; con barajado va al frente
// y el resto se baraja detras con Fisher-Yates. El generador lo siembra una sola
// vez el reproductor, asi que aqui no se vuelve a sembrar.
static void Queue_Plan(int first) {
	for (int i = 0; i < queue_count; i++)
		queue_order[i] = i;

	if (!queue_shuffled || queue_count == 0) {
		queue_slot = (first >= 0 && first < queue_count) ? first : 0;
		return;
	}

	if (first >= 0 && first < queue_count) {
		queue_order[first] = 0;
		queue_order[0] = first;
	}

	for (int i = queue_count - 1; i > 1; i--) {
		int j = 1 + rand() % i;
		int tmp = queue_order[i];
		queue_order[i] = queue_order[j];
		queue_order[j] = tmp;
	}

	queue_slot = 0;
}

void Queue_SetShuffle(SceBool on) {
	int current = (queue_count > 0) ? queue_order[queue_slot] : -1;

	queue_shuffled = on;
	Queue_Plan(current);
}

SceBool Queue_IsShuffled(void) {
	return queue_shuffled;
}

const char *Queue_Advance(SceBool forward) {
	if (queue_count == 0)
		return NULL;

	queue_slot += forward ? 1 : -1;

	if (queue_slot >= queue_count)
		queue_slot = 0;
	else if (queue_slot < 0)
		queue_slot = queue_count - 1;

	return queue_paths[queue_order[queue_slot]];
}

SceBool Queue_SeekToPath(const char *path) {
	int natural = Queue_Find(path);

	if (natural < 0)
		return SCE_FALSE;

	Queue_Plan(natural);
	return SCE_TRUE;
}

SceBool Queue_PeekAhead(int n, const char **path, const char **title) {
	if (queue_count == 0 || n < 0)
		return SCE_FALSE;

	int natural = queue_order[(queue_slot + n) % queue_count];

	if (path != NULL)
		*path = queue_paths[natural];

	if (title != NULL)
		*title = queue_titles[natural];

	return SCE_TRUE;
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
