#ifndef _ELEVENMPV_TRACK_META_H_
#define _ELEVENMPV_TRACK_META_H_

// Numeros de pista y de disco, y el orden que deciden.
//
// Es logica pura, sin vita2d ni SCE, para que el orden de un album - que es
// justo lo que la biblioteca tiene que respetar - se pruebe en el PC con
// make -C tests. library.c arma las claves y llama desde sus comparadores.

// "3", "03" y "3/12" dan 3. Vacio, "abc", "/12" o NULL dan 0, que es ausente:
// el texto tiene que empezar por un digito.
int TrackMeta_ParseNumber(const char *text);

// ID3v1.1 guarda el numero de pista en el ultimo byte del comentario, con un
// cero justo antes. Sin esa marca es ID3v1.0 y no hay numero: devuelve 0.
int TrackMeta_Id3v1Track(const unsigned char comment[30]);

typedef struct {
	const char *group; // album o artista; vacio o NULL es "sin grupo"
	int disc;          // 0 es ausente, y cuenta como disco 1
	int track;         // 0 es ausente, y va al final de su disco
	const char *title;
	const char *path;
} TrackMeta_Key;

// El orden del disco: disco, luego pista (las sin numero al final), luego
// titulo y por ultimo ruta, para que el orden sea estable.
int TrackMeta_CompareDisc(const TrackMeta_Key *a, const TrackMeta_Key *b);

// Por grupo sin distinguir mayusculas, con el grupo vacio al final, y dentro
// del grupo el orden del disco.
int TrackMeta_CompareGrouped(const TrackMeta_Key *a, const TrackMeta_Key *b);

#endif
