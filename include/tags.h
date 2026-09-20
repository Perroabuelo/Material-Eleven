#ifndef _ELEVENMPV_TAGS_H_
#define _ELEVENMPV_TAGS_H_

#include <psp2/types.h>

// Lectura de metadatos por el camino mas barato de cada formato, separada de la
// ruta de reproduccion.
//
// Los decoders leen los tags como parte de abrir el archivo para sonarlo, y eso
// cuesta abrir el decoder entero. El escaner no va a reproducir nada, asi que
// pide solo lo que necesita: la cabecera de metadatos de FLAC, el ID3 de MP3,
// los comentarios Vorbis de OGG y OPUS, y xmp_test_module para los modulos de
// tracker, que lee la cabecera y nada mas. WAV no lleva nada que leer.

#define TAGS_FIELD_MAX 64

typedef struct {
	char title[TAGS_FIELD_MAX];
	char artist[TAGS_FIELD_MAX];
	char album[TAGS_FIELD_MAX];
} Tags;

// Rellena `out` con lo que ese archivo traiga. Devuelve SCE_FALSE si el formato
// no lleva tags, si el archivo no se pudo abrir o si su cabecera esta dañada; en
// todos esos casos `out` queda vacio y quien llama se queda con su respaldo, que
// es el nombre del archivo.
//
// No es error que un campo salga vacio: significa que ese archivo no lo trae.
SceBool Tags_Read(const char *path, const char *ext, Tags *out);

#endif
