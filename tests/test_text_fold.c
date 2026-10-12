// Pruebas en PC de la comparacion sin mayusculas ni tildes (source/text_fold.c),
// compiladas con el gcc del host. text_fold.c es logica pura, asi que no hace
// falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "text_fold.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

static void expect_fold(const char *in, const char *want) {
	char out[TEXT_FOLD_MAX];
	size_t len = TextFold_Fold(in, out, sizeof(out));

	CHECK(len == strlen(want) && strcmp(out, want) == 0, "\"%s\" se plego como \"%s\" y no como \"%s\"", in, out, want);
}

// Pliega la aguja como lo hace quien llama, una vez, y busca.
static int contains(const char *haystack, const char *needle) {
	char folded[TEXT_FOLD_MAX];
	TextFold_Fold(needle, folded, sizeof(folded));
	return TextFold_Contains(haystack, folded);
}

static void test_fold(void) {
	expect_fold("ABC xyz 123", "abc xyz 123");
	expect_fold("Canción", "cancion");
	expect_fold("CANCIÓN", "cancion");
	expect_fold("ÁÉÍÓÚ áéíóú", "aeiou aeiou");
	expect_fold("Pingüino Über", "pinguino uber");
	expect_fold("Ñandú año", "nandu ano");
	expect_fold("Straße", "strasse");
	expect_fold("Æsir Œuvre Ĳssel", "aesir oeuvre ijssel");
	expect_fold("Çà Ø ÿ Þ", "ca o y th");
	expect_fold("Łódź Dvořák Škoda", "lodz dvorak skoda");
	// × y ÷ no son letras: se quedan como estan.
	expect_fold("2×3÷1", "2×3÷1");
	expect_fold("", "");
}

static void test_passthrough(void) {
	// Texto no latino: se copia tal cual y sigue encontrandose por su texto exacto.
	expect_fold("東京事変", "東京事変");
	expect_fold("Привет", "Привет");
	expect_fold("a\xF0\x9F\x8E\xB5" "B", "a\xF0\x9F\x8E\xB5" "b");
	CHECK(contains("Ringo 東京事変 live", "東京"), "no encontro el texto japones exacto");

	// UTF-8 invalido o truncado: los bytes pasan uno a uno y lo demas se pliega.
	expect_fold("A\xC3" "B\x80" "C", "a\xC3" "b\x80" "c");
	expect_fold("x\xE2\x82", "x\xE2\x82");
	expect_fold("\xC3", "\xC3");
}

static void test_buffers(void) {
	char out[8];

	// "Straße" plegado son 7 bytes: cabe justo con el NUL.
	CHECK(TextFold_Fold("Straße", out, 8) == 7 && strcmp(out, "strasse") == 0, "no cupo en el buffer justo");
	// Con 5 bytes libres, "ss" no cabe entero y se detiene antes.
	CHECK(TextFold_Fold("Straße", out, 6) == 4 && strcmp(out, "stra") == 0, "partio el ss");
	// Un caracter sin plegar tampoco se parte.
	CHECK(TextFold_Fold("ab東", out, 4) == 2 && strcmp(out, "ab") == 0, "partio un caracter de tres bytes");

	out[0] = 'z';
	CHECK(TextFold_Fold("abc", out, 0) == 0 && out[0] == 'z', "escribio con capacidad 0");
}

static void test_contains(void) {
	CHECK(contains("Canción del Mariachi", "cancion"), "cancion no encontro Canción");
	CHECK(contains("Canción del Mariachi", "CANCIÓN"), "CANCIÓN no encontro Canción");
	CHECK(contains("Canción.flac", "CIÓN.FLAC"), "no encontro la coincidencia al final");
	CHECK(contains("El Niño", "nino"), "nino no encontro Niño");
	CHECK(contains("Die Straße", "strasse"), "strasse no encontro Straße");
	CHECK(!contains("Canción", "canciones"), "una aguja mas larga que el pajar coincidio");
	CHECK(!contains("Yellow", "blue"), "coincidio algo que no esta");
	CHECK(!contains("", "a"), "un pajar vacio contuvo algo");

	// La aguja vacia esta en todo, tambien en un pajar vacio.
	CHECK(contains("Yellow", ""), "la aguja vacia no coincidio");
	CHECK(contains("", ""), "la aguja vacia no coincidio con el vacio");
}

int main(void) {
	test_fold();
	test_passthrough();
	test_buffers();
	test_contains();

	if (failures == 0)
		printf("test_text_fold: todo bien\n");

	return failures == 0 ? 0 : 1;
}
