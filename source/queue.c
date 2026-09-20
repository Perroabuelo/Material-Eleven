#include <stdio.h>
#include <string.h>

#include "queue.h"

// La reserva es, por ahora, exactamente la que tenia el playlist[1024][512]
// estatico de menu_audioplayer.c, para que mudar la cola aqui no cambie ni el
// consumo ni el comportamiento. Dimensionarla a la cola real es el paso
// siguiente, y queda dentro de este archivo.
#define QUEUE_MAX_TRACKS 1024
#define QUEUE_PATH_MAX   512

static char queue_paths[QUEUE_MAX_TRACKS][QUEUE_PATH_MAX];
static int queue_count = 0;
static int queue_position = 0;

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
