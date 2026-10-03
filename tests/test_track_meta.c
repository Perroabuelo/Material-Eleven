// Pruebas en PC de los numeros de pista y del orden del disco
// (source/track_meta.c), compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "track_meta.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

static int sign(int v) {
	return (v > 0) - (v < 0);
}

static void test_parse_number(void) {
	static const struct { const char *in; int out; } cases[] = {
		{ "3", 3 },
		{ "03", 3 },
		{ "3/12", 3 },
		{ "2/2", 2 },
		{ "12", 12 },
		{ "", 0 },
		{ "abc", 0 },
		{ "/12", 0 },
		{ "A1", 0 },
	};

	for (unsigned int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		int got = TrackMeta_ParseNumber(cases[i].in);
		CHECK(got == cases[i].out, "ParseNumber(\"%s\") dio %d, se esperaba %d", cases[i].in, got, cases[i].out);
	}

	CHECK(TrackMeta_ParseNumber(NULL) == 0, "ParseNumber(NULL) no dio 0");
}

static void test_id3v1_track(void) {
	unsigned char comment[30];

	// ID3v1.0: el comentario ocupa los 30 bytes, no hay numero.
	memset(comment, 'x', sizeof(comment));
	CHECK(TrackMeta_Id3v1Track(comment) == 0, "ID3v1.0 dio numero de pista");

	// ID3v1.0 con un comentario corto: todo ceros al final, tampoco hay numero.
	memset(comment, 0, sizeof(comment));
	CHECK(TrackMeta_Id3v1Track(comment) == 0, "ID3v1.0 vacio dio numero de pista");

	// ID3v1.1: cero en el byte 28 y la pista en el 29.
	memset(comment, 0, sizeof(comment));
	memcpy(comment, "hola", 4);
	comment[29] = 7;
	CHECK(TrackMeta_Id3v1Track(comment) == 7, "ID3v1.1 con pista 7 dio %d", TrackMeta_Id3v1Track(comment));
}

static TrackMeta_Key key(const char *group, int disc, int track, const char *title, const char *path) {
	TrackMeta_Key k = { group, disc, track, title, path };
	return k;
}

static void test_compare_disc(void) {
	TrackMeta_Key a, b;

	// Los numeros mandan sobre el titulo.
	a = key("X", 0, 2, "Angel", "a");
	b = key("X", 0, 10, "Black Dog", "b");
	CHECK(TrackMeta_CompareDisc(&a, &b) < 0, "pista 2 no va antes que la 10");
	b = key("X", 0, 1, "Zoo", "b");
	CHECK(TrackMeta_CompareDisc(&a, &b) > 0, "pista 2 no va despues que la 1");

	// El disco manda sobre la pista.
	a = key("X", 1, 8, "a", "a");
	b = key("X", 2, 1, "b", "b");
	CHECK(TrackMeta_CompareDisc(&a, &b) < 0, "disco 1 pista 8 no va antes que disco 2 pista 1");

	// Sin disco cuenta como disco 1.
	a = key("X", 0, 3, "a", "a");
	b = key("X", 1, 2, "b", "b");
	CHECK(TrackMeta_CompareDisc(&a, &b) > 0, "sin disco no cuenta como disco 1");
	b = key("X", 2, 1, "b", "b");
	CHECK(TrackMeta_CompareDisc(&a, &b) < 0, "sin disco no va antes que el disco 2");

	// Las sin numero van al final de su disco, por titulo.
	a = key("X", 0, 0, "Aaa", "a");
	b = key("X", 0, 12, "Zzz", "b");
	CHECK(TrackMeta_CompareDisc(&a, &b) > 0, "sin numero no va al final");
	b = key("X", 0, 0, "Bbb", "b");
	CHECK(TrackMeta_CompareDisc(&a, &b) < 0, "dos sin numero no se ordenan por titulo");

	// Desempate por titulo, sin distinguir mayusculas, y luego por ruta.
	a = key("X", 1, 4, "alfa", "z");
	b = key("X", 1, 4, "BETA", "a");
	CHECK(TrackMeta_CompareDisc(&a, &b) < 0, "el titulo no desempata");
	a = key("X", 1, 4, "Igual", "ux0:music/a.flac");
	b = key("X", 1, 4, "igual", "ux0:music/b.flac");
	CHECK(TrackMeta_CompareDisc(&a, &b) < 0, "la ruta no desempata");
	CHECK(TrackMeta_CompareDisc(&b, &a) > 0, "la ruta no desempata al reves");
	CHECK(TrackMeta_CompareDisc(&a, &a) == 0, "una clave no es igual a si misma");
}

static void test_compare_grouped(void) {
	TrackMeta_Key a, b;

	// Grupos por orden alfabetico, aunque la pista diga otra cosa.
	a = key("Alfa", 0, 9, "x", "a");
	b = key("Beta", 0, 1, "x", "b");
	CHECK(TrackMeta_CompareGrouped(&a, &b) < 0, "Alfa no va antes que Beta");

	// Sin distinguir mayusculas: mismo grupo, decide el orden del disco.
	a = key("alfa", 0, 2, "x", "a");
	b = key("ALFA", 0, 1, "x", "b");
	CHECK(TrackMeta_CompareGrouped(&a, &b) > 0, "el grupo distingue mayusculas");
	CHECK(sign(TrackMeta_CompareGrouped(&a, &b)) == sign(TrackMeta_CompareDisc(&a, &b)), "dentro del grupo no manda el disco");

	// El grupo vacio, y NULL, van al final.
	a = key("", 0, 1, "a", "a");
	b = key("Zzz", 0, 5, "z", "z");
	CHECK(TrackMeta_CompareGrouped(&a, &b) > 0, "el grupo vacio no va al final");
	a = key(NULL, 0, 1, "a", "a");
	CHECK(TrackMeta_CompareGrouped(&a, &b) > 0, "el grupo NULL no va al final");
	CHECK(TrackMeta_CompareGrouped(&b, &a) < 0, "el grupo NULL no va al final al reves");
}

int main(void) {
	test_parse_number();
	test_id3v1_track();
	test_compare_disc();
	test_compare_grouped();

	if (failures == 0)
		printf("test_track_meta: todo bien\n");

	return failures == 0 ? 0 : 1;
}
