#ifndef _ELEVENMPV_COVER_H_
#define _ELEVENMPV_COVER_H_

#include <psp2/types.h>
#include <vita2d.h>

// Miniaturas de caratula, cacheadas en disco.
//
// El tamaño no lo decidio el gusto sino la medicion del grupo 0: decodificar un
// JPEG de 600x600 cuesta 93 ms de CPU, porque vita2d_load_JPEG_buffer va por
// libjpeg y no por el decodificador por hardware de la consola - los simbolos
// indefinidos de vita2d_image_jpeg.o son libjpeg puro, sin una sola referencia a
// sceJpeg. Ese costo es fijo y no baja por pedir una miniatura mas pequeña, asi
// que se paga una vez, se reescala, y lo que se guarda son los pixeles ya
// reducidos: leerlos despues no cuesta ni decodificador ni parseo de cabecera.
#define COVER_SIZE 128

// La clave es el album y no la pista: un disco de doce canciones lleva doce
// copias de la misma imagen, y extraerla una vez baja el trabajo en un orden de
// magnitud - medido, 32,5 s por pista frente a 8,0 s por album. Las pistas sin
// album no participan del reparto y se cachean por su ruta.

// Extrae, reescala y guarda. Tambien guarda la marca de "no hay caratula", que
// es lo que evita reintentarlo en cada arranque.
//
// SCE_TRUE si quedo entrada escrita, tenga imagen o no.
SceBool Cover_Build(const char *album, const char *path, const char *ext);

// Si ya hay entrada, con imagen o sin ella.
SceBool Cover_IsCached(const char *album, const char *path);

// La textura de esa entrada, o NULL si no hay imagen o si no cabia traerla en
// esta llamada. `budget` es cuantas se admiten de disco ahora mismo, y se
// descuenta: recorrer una lista larga no puede quedarse esperando caratulas, asi
// que las que no llegan se dibujan con el marcador por defecto y llegan luego.
vita2d_texture *Cover_Get(const char *album, const char *path, int *budget);

// Lo que dice la entrada de esa clave, sin traer sus pixeles: solo la cabecera
// de 128 bytes, o nada si ya esta en RAM. Sirve para elegir que caratula
// representa a un artista sin pagar la lectura de las que no la tienen.
typedef enum {
	COVER_PROBE_MISSING = -1, // no hay entrada: la pasada de caratulas no llego
	COVER_PROBE_EMPTY = 0,    // hay entrada y dice que no hay caratula
	COVER_PROBE_IMAGE = 1     // hay caratula
} Cover_ProbeResult;

Cover_ProbeResult Cover_Probe(const char *album, const char *path);

// Suelta las texturas cacheadas en RAM. El cache en disco no se toca.
void Cover_Free(void);

#endif
