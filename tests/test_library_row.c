// Pruebas en PC de las filas del indice de la biblioteca
// (source/library_row.c), compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "library_row.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Parte una fila por tabuladores como Library_SplitFields: el ultimo campo se
// queda con lo que sobre.
static int split(char *line, char **fields, int max_fields) {
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

static const char *field(char **fields, int pos) {
	return (pos >= 0) ? fields[pos] : NULL;
}

static void test_v2(void) {
	LibraryRow_Layout l;
	char line[] = "1234\t5678\t1\tflac\tAngel\tLed Zeppelin\tIV\tux0:music/a\tb.flac";
	char *fields[LIBRARY_ROW_MAX_FIELDS];

	CHECK(LibraryRow_GetLayout(2, &l), "la version 2 no se reconoce");
	CHECK(l.fields == 8, "v2 tiene %d campos, se esperaban 8", l.fields);
	CHECK(split(line, fields, l.fields) == l.fields, "la fila v2 no tiene sus campos");

	CHECK(!strcmp(field(fields, l.size), "1234"), "v2: tamaño mal ubicado");
	CHECK(!strcmp(field(fields, l.mtime), "5678"), "v2: fecha mal ubicada");
	CHECK(!strcmp(field(fields, l.ext), "flac"), "v2: extension mal ubicada");
	CHECK(!strcmp(field(fields, l.title), "Angel"), "v2: titulo mal ubicado");
	CHECK(!strcmp(field(fields, l.artist), "Led Zeppelin"), "v2: artista mal ubicado");
	CHECK(!strcmp(field(fields, l.album), "IV"), "v2: album mal ubicado");
	CHECK(!strcmp(field(fields, l.path), "ux0:music/a\tb.flac"), "v2: la ruta no se queda con el resto");

	// Sin numeros y pendiente, aunque la fila diga tagged = 1.
	CHECK(l.track == -1 && l.disc == -1, "v2 trae numeros de pista o de disco");
	CHECK(l.tagged == -1, "una fila v2 no queda pendiente de releer tags");
}

static void test_v3(void) {
	LibraryRow_Layout l;
	char line[] = "1234\t5678\t1\tmp3\t7\t2\tAngel\tLed Zeppelin\tIV\tux0:music/a.mp3";
	char *fields[LIBRARY_ROW_MAX_FIELDS];

	CHECK(LibraryRow_GetLayout(3, &l), "la version 3 no se reconoce");
	CHECK(l.fields == 10, "v3 tiene %d campos, se esperaban 10", l.fields);
	CHECK(l.fields <= LIBRARY_ROW_MAX_FIELDS, "v3 no cabe en LIBRARY_ROW_MAX_FIELDS");
	CHECK(split(line, fields, l.fields) == l.fields, "la fila v3 no tiene sus campos");

	CHECK(!strcmp(field(fields, l.size), "1234"), "v3: tamaño mal ubicado");
	CHECK(!strcmp(field(fields, l.mtime), "5678"), "v3: fecha mal ubicada");
	CHECK(l.tagged >= 0 && !strcmp(field(fields, l.tagged), "1"), "v3: tagged mal ubicado");
	CHECK(!strcmp(field(fields, l.ext), "mp3"), "v3: extension mal ubicada");
	CHECK(l.track >= 0 && !strcmp(field(fields, l.track), "7"), "v3: pista mal ubicada");
	CHECK(l.disc >= 0 && !strcmp(field(fields, l.disc), "2"), "v3: disco mal ubicado");
	CHECK(!strcmp(field(fields, l.title), "Angel"), "v3: titulo mal ubicado");
	CHECK(!strcmp(field(fields, l.artist), "Led Zeppelin"), "v3: artista mal ubicado");
	CHECK(!strcmp(field(fields, l.album), "IV"), "v3: album mal ubicado");
	CHECK(!strcmp(field(fields, l.path), "ux0:music/a.mp3"), "v3: ruta mal ubicada");
	CHECK(l.path == l.fields - 1, "v3: la ruta no es el ultimo campo");
}

static void test_unknown(void) {
	LibraryRow_Layout l;
	int versions[] = { 0, 1, 4, -1 };

	for (unsigned int i = 0; i < sizeof(versions) / sizeof(versions[0]); i++)
		CHECK(!LibraryRow_GetLayout(versions[i], &l), "la version %d se acepta", versions[i]);

	CHECK(!LibraryRow_GetLayout(3, NULL), "acepta un destino NULL");
}

int main(void) {
	test_v2();
	test_v3();
	test_unknown();

	if (failures == 0)
		printf("test_library_row: todo bien\n");

	return failures == 0 ? 0 : 1;
}
