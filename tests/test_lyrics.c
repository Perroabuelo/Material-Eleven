// Pruebas en PC del parser de letras (source/lyrics.c), con los .lrc de
// fixtures/lyrics/ pasados por source/text_encoding.c como en la consola.
// Compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lyrics.h"
#include "text_encoding.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

typedef struct {
	uint32_t ms;
	const char *text;
} Expected;

static const char *line_text(const Lyrics *l, uint32_t i) {
	return l->text + l->lines[i].off;
}

static void parse_text(const char *text, Lyrics *out) {
	CHECK(Lyrics_Parse(text, strlen(text), out) == 0, "Lyrics_Parse devolvio error");
}

// Lee el fixture y lo pasa por TextEncoding_ToUtf8 y Lyrics_Parse, como hace
// lyrics_load.c con un .lrc.
static void parse_fixture(const char *name, Lyrics *out) {
	char path[160];
	snprintf(path, sizeof(path), "fixtures/lyrics/%s", name);
	memset(out, 0, sizeof(*out));

	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		CHECK(0, "no se pudo abrir %s", path);
		return;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char *raw = malloc(size > 0 ? size : 1);
	size_t len = fread(raw, 1, size, f);
	fclose(f);

	char *utf8 = NULL;
	size_t utf8_len = 0;
	CHECK(TextEncoding_ToUtf8(raw, len, &utf8, &utf8_len) == 0, "%s: ToUtf8 fallo", name);
	free(raw);

	CHECK(Lyrics_Parse(utf8, utf8_len, out) == 0, "%s: Lyrics_Parse devolvio error", name);
	free(utf8);
}

static void expect_lines(const char *name, const Lyrics *l, int synced, const Expected *want, uint32_t n) {
	CHECK(l->synced == synced, "%s: synced=%d, se esperaba %d", name, l->synced, synced);
	CHECK(l->count == n, "%s: %u lineas, se esperaban %u", name, l->count, n);

	for (uint32_t i = 0; i < n && i < l->count; i++) {
		CHECK(l->lines[i].ms == want[i].ms, "%s: linea %u a %u ms, se esperaba %u", name, i, l->lines[i].ms, want[i].ms);
		CHECK(strcmp(line_text(l, i), want[i].text) == 0 && l->lines[i].len == strlen(want[i].text),
			"%s: linea %u es \"%s\", se esperaba \"%s\"", name, i, line_text(l, i), want[i].text);
	}
}

static const Expected good[] = {
	{ 0, "Primera línea" },
	{ 4000, "Segunda línea" },
	{ 8500, "Tercera, con acentos: ñandú" },
	{ 12000, "Una línea con dos marcas" },
	{ 16000, "Una línea con dos marcas" },
};

static void test_good(void) {
	Lyrics l;
	parse_fixture("01 bien.lrc", &l);
	expect_lines("01 bien", &l, 1, good, 5);

	// Las dos marcas de la misma linea apuntan al mismo texto.
	if (l.count == 5)
		CHECK(l.lines[3].off == l.lines[4].off, "01 bien: las dos marcas no comparten el texto");

	Lyrics_Free(&l);
}

static void test_same_as_good(void) {
	// BOM de UTF-8, CRLF y UTF-16 LE y BE dan las mismas lineas que 01 bien.lrc.
	static const char *names[] = { "06 bom utf8.lrc", "08 crlf.lrc", "09 utf16 le.lrc", "10 utf16 be.lrc" };

	for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		Lyrics l;
		parse_fixture(names[i], &l);
		expect_lines(names[i], &l, 1, good, 5);

		for (uint32_t k = 0; k < l.count; k++)
			CHECK(strchr(line_text(&l, k), '\r') == NULL, "%s: la linea %u tiene un \\r", names[i], k);

		Lyrics_Free(&l);
	}
}

static void test_invalid_stamps(void) {
	static const Expected want[] = {
		{ 0, "Esta marca es válida" },
		{ 5000, "Sin centésimas" },
		{ 8123, "Demasiados decimales" },
	};
	Lyrics l;
	parse_fixture("02 marcas invalidas.lrc", &l);
	expect_lines("02 marcas invalidas", &l, 1, want, 3);
	Lyrics_Free(&l);
}

static void test_unclosed(void) {
	static const Expected want[] = {
		{ 0, "Bien" },
		{ 8000, "Después del error" },
	};
	Lyrics l;
	parse_fixture("03 corchete sin cerrar.lrc", &l);
	expect_lines("03 corchete sin cerrar", &l, 1, want, 2);
	Lyrics_Free(&l);
}

static void test_long_line(void) {
	Lyrics l;
	parse_fixture("04 linea larga.lrc", &l);
	CHECK(l.synced && l.count == 2, "04 linea larga: %u lineas", l.count);
	if (l.count == 2) {
		CHECK(l.lines[0].len > 2500, "04 linea larga: la linea larga mide %u bytes", l.lines[0].len);
		CHECK(strcmp(line_text(&l, 1), "Línea normal después") == 0, "04 linea larga: segunda linea \"%s\"", line_text(&l, 1));
	}
	Lyrics_Free(&l);
}

static void test_no_lyrics(void) {
	static const char *names[] = { "05 vacio.lrc", "17 solo etiquetas.lrc" };

	for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		Lyrics l;
		parse_fixture(names[i], &l);
		CHECK(l.count == 0 && l.text == NULL && l.lines == NULL, "%s: deberia ser sin letra", names[i]);
		Lyrics_Free(&l);
	}

	// Solo lineas en blanco, o marcas sin texto, tampoco es letra.
	Lyrics l;
	parse_text("\n  \n\t\n", &l);
	CHECK(l.count == 0, "lineas en blanco dieron letra");
	Lyrics_Free(&l);

	parse_text("[00:01.00]\n[00:02.00]   \n", &l);
	CHECK(l.count == 0, "marcas sin texto dieron letra");
	Lyrics_Free(&l);

	CHECK(Lyrics_Parse(NULL, 0, &l) == 0 && l.count == 0, "NULL dio letra");
}

static void test_latin1(void) {
	Lyrics l;
	parse_fixture("07 no utf8.lrc", &l);
	CHECK(l.count == 2 && strcmp(line_text(&l, 0), "Línea en Latin-1: canción, ñandú") == 0,
		"07 no utf8: primera linea \"%s\"", l.count ? line_text(&l, 0) : "");
	Lyrics_Free(&l);
}

static void test_offsets(void) {
	static const Expected positive[] = { { 9500, "Diez segundos menos medio" } };
	static const Expected negative[] = { { 10250, "Diez segundos más un cuarto" } };
	static const Expected below[] = { { 0, "No baja de cero" }, { 4000, "Cuatro segundos" } };
	Lyrics l;

	parse_fixture("11 offset positivo.lrc", &l);
	expect_lines("11 offset positivo", &l, 1, positive, 1);
	Lyrics_Free(&l);

	parse_fixture("12 offset negativo.lrc", &l);
	expect_lines("12 offset negativo", &l, 1, negative, 1);
	Lyrics_Free(&l);

	parse_fixture("13 offset bajo cero.lrc", &l);
	expect_lines("13 offset bajo cero", &l, 1, below, 2);
	Lyrics_Free(&l);

	// Un desfase que no es un numero se ignora, y la etiqueta no es texto.
	static const Expected ignored[] = { { 10000, "Sin desfase" } };
	parse_text("[offset:abc]\n[00:10.00]Sin desfase\n", &l);
	expect_lines("offset no numerico", &l, 1, ignored, 1);
	Lyrics_Free(&l);

	parse_text("[offset:]\n[OFFSET:+]\n[00:10.00]Sin desfase\n", &l);
	expect_lines("offset vacio", &l, 1, ignored, 1);
	Lyrics_Free(&l);
}

static void test_unordered(void) {
	static const Expected want[] = {
		{ 2000, "A los dos" },
		{ 5000, "A los cinco" },
		{ 10000, "A los diez" },
	};
	Lyrics l;
	parse_fixture("14 desordenadas.lrc", &l);
	expect_lines("14 desordenadas", &l, 1, want, 3);
	Lyrics_Free(&l);

	// A igual marca se conserva el orden del archivo.
	static const Expected ties[] = {
		{ 1000, "uno" }, { 3000, "b" }, { 3000, "a" }, { 3000, "c" },
	};
	parse_text("[00:03.00]b\n[00:03.00]a\n[00:01.00]uno\n[00:03.00]c\n", &l);
	expect_lines("empates", &l, 1, ties, 4);
	Lyrics_Free(&l);
}

static void test_word_marks(void) {
	static const Expected want[] = { { 0, "Antes" }, { 12300, "Hola mundo" } };
	Lyrics l;
	parse_fixture("15 marcas por palabra.lrc", &l);
	expect_lines("15 marcas por palabra", &l, 1, want, 2);
	Lyrics_Free(&l);

	// Una marca por palabra al final no deja espacios colgando.
	static const Expected tail[] = { { 1000, "Fin" } };
	parse_text("[00:01.00]<00:01.00>Fin <00:01.50>\n", &l);
	expect_lines("marca al final", &l, 1, tail, 1);
	Lyrics_Free(&l);
}

static void test_genius(void) {
	static const Expected want[] = {
		{ 0, "[헌트릭스 \"Golden\" 가사]" },
		{ 0, "" },
		{ 0, "[Verse: Rumi, Zoey, Mira, All]" },
		{ 0, "Primera estrofa, primera línea" },
		{ 0, "Primera estrofa, segunda línea" },
		{ 0, "" },
		{ 0, "[Chorus: All]" },
		{ 0, "어두워진 하늘 아래" },
		{ 0, "Segunda estrofa, última línea" },
	};
	Lyrics l;
	parse_fixture("16 sin tiempos genius.lrc", &l);
	expect_lines("16 sin tiempos genius", &l, 0, want, 9);

	if (l.count == 9) {
		static const int header[9] = { 1, 0, 1, 0, 0, 0, 1, 0, 0 };
		for (int i = 0; i < 9; i++)
			CHECK(((l.lines[i].flags & LYRICS_LINE_HEADER) != 0) == header[i], "16 genius: linea %d encabezado=%d", i, l.lines[i].flags);
	}

	CHECK(Lyrics_LineAt(&l, 0) == -1 && Lyrics_LineAt(&l, 600000) == -1, "una letra sin tiempos dio linea actual");
	Lyrics_Free(&l);
}

static void test_stanzas(void) {
	static const Expected want[] = {
		{ 0, "Uno" }, { 0, "Dos" }, { 0, "" }, { 0, "Tres" }, { 0, "Cuatro" },
	};
	Lyrics l;
	parse_text("Uno\nDos  \n\nTres\nCuatro\n", &l);
	expect_lines("estrofas", &l, 0, want, 5);
	Lyrics_Free(&l);
}

static void test_lrclib_header(void) {
	static const Expected want[] = { { 1500, "Ah-ah-ah-ah-ah" } };
	Lyrics l;
	parse_text("[ti:TOMBOY]\n[ar:i-dle]\n[al:I NEVER DIE]\n[by:LRCLIB]\n[length: 02:54]\n[00:01.50] Ah-ah-ah-ah-ah\n", &l);
	expect_lines("cabecera LRCLIB", &l, 1, want, 1);
	Lyrics_Free(&l);

	// Sin tiempos, las etiquetas tambien se quitan; cualquier otro corchete queda.
	static const Expected plain[] = { { 0, "[Intro]" }, { 0, "Texto" } };
	parse_text("[Ti:Titulo]\n[Intro]\nTexto\n", &l);
	expect_lines("etiquetas sin tiempos", &l, 0, plain, 2);
	Lyrics_Free(&l);
}

static void test_line_at(void) {
	Lyrics l;
	parse_fixture("01 bien.lrc", &l);

	CHECK(Lyrics_LineAt(&l, 0) == 0, "LineAt(0) dio %d", Lyrics_LineAt(&l, 0));
	CHECK(Lyrics_LineAt(&l, 3999) == 0, "LineAt(3999) dio %d", Lyrics_LineAt(&l, 3999));
	CHECK(Lyrics_LineAt(&l, 4000) == 1, "LineAt(4000) dio %d", Lyrics_LineAt(&l, 4000));
	CHECK(Lyrics_LineAt(&l, 12000) == 3, "LineAt(12000) dio %d", Lyrics_LineAt(&l, 12000));
	CHECK(Lyrics_LineAt(&l, 15999) == 3, "LineAt(15999) dio %d", Lyrics_LineAt(&l, 15999));
	CHECK(Lyrics_LineAt(&l, 16000) == 4, "LineAt(16000) dio %d", Lyrics_LineAt(&l, 16000));
	CHECK(Lyrics_LineAt(&l, UINT32_MAX) == 4, "LineAt(max) dio %d", Lyrics_LineAt(&l, UINT32_MAX));
	Lyrics_Free(&l);

	// Antes de la primera marca no hay linea actual.
	parse_fixture("18 primera a los 3 s.lrc", &l);
	CHECK(Lyrics_LineAt(&l, 0) == -1 && Lyrics_LineAt(&l, 2999) == -1, "antes de la primera marca hubo linea actual");
	CHECK(Lyrics_LineAt(&l, 3000) == 0, "a los 3 s no hubo linea actual");
	Lyrics_Free(&l);

	Lyrics empty = { 0 };
	CHECK(Lyrics_LineAt(&empty, 1000) == -1, "una letra vacia dio linea actual");
}

static void test_choose(void) {
	Lyrics lrc, emb, out;

	// Gana la letra con tiempos.
	parse_text("[00:01.00]del lrc\n", &lrc);
	parse_text("embebida\n", &emb);
	Lyrics_Choose(&lrc, &emb, &out);
	CHECK(out.count == 1 && strcmp(out.text, "del lrc") == 0, "con tiempos contra sin tiempos no gano el .lrc");
	CHECK(lrc.count == 0 && emb.count == 0 && lrc.text == NULL && emb.text == NULL, "Choose no vacio las fuentes");
	Lyrics_Free(&out);

	// Tambien cuando la que tiene tiempos es la embebida.
	parse_text("del lrc\n", &lrc);
	parse_text("[00:01.00]embebida\n", &emb);
	Lyrics_Choose(&lrc, &emb, &out);
	CHECK(out.count == 1 && strcmp(out.text, "embebida") == 0, "la embebida con tiempos no gano");
	Lyrics_Free(&out);

	// A igualdad gana el .lrc: las dos sin tiempos y las dos con tiempos.
	parse_text("del lrc\n", &lrc);
	parse_text("embebida\n", &emb);
	Lyrics_Choose(&lrc, &emb, &out);
	CHECK(out.count == 1 && strcmp(out.text, "del lrc") == 0, "sin tiempos las dos, no gano el .lrc");
	Lyrics_Free(&out);

	parse_text("[00:01.00]del lrc\n", &lrc);
	parse_text("[00:01.00]embebida\n", &emb);
	Lyrics_Choose(&lrc, &emb, &out);
	CHECK(out.count == 1 && strcmp(out.text, "del lrc") == 0, "con tiempos las dos, no gano el .lrc");
	Lyrics_Free(&out);

	// Un .lrc vacio o con solo etiquetas no tapa a la embebida.
	parse_fixture("17 solo etiquetas.lrc", &lrc);
	parse_text("embebida\n", &emb);
	Lyrics_Choose(&lrc, &emb, &out);
	CHECK(out.count == 1 && strcmp(out.text, "embebida") == 0, "un .lrc con solo etiquetas tapo a la embebida");
	Lyrics_Free(&out);

	parse_fixture("05 vacio.lrc", &lrc);
	parse_text("embebida\n", &emb);
	Lyrics_Choose(&lrc, &emb, &out);
	CHECK(out.count == 1 && strcmp(out.text, "embebida") == 0, "un .lrc vacio tapo a la embebida");
	Lyrics_Free(&out);

	// Ninguna.
	memset(&lrc, 0, sizeof(lrc));
	memset(&emb, 0, sizeof(emb));
	Lyrics_Choose(&lrc, &emb, &out);
	CHECK(out.count == 0 && out.text == NULL, "sin ninguna letra dio letra");
}

static void test_size_limit(void) {
	// Mas de 64 KB es como si no existiera; justo 64 KB todavia se lee.
	size_t big = 100 * 1024;
	char *text = malloc(big);
	for (size_t i = 0; i < big; i++)
		text[i] = (i % 40 == 39) ? '\n' : 'a';

	Lyrics l;
	CHECK(Lyrics_Parse(text, big, &l) == 0 && l.count == 0, "una letra de 100 KB se cargo");
	Lyrics_Free(&l);

	CHECK(Lyrics_Parse(text, LYRICS_MAX_BYTES, &l) == 0 && l.count > 0, "una letra de 64 KB no se cargo");
	Lyrics_Free(&l);

	CHECK(Lyrics_Parse(text, LYRICS_MAX_BYTES + 1, &l) == 0 && l.count == 0, "una letra de 64 KB + 1 se cargo");
	Lyrics_Free(&l);

	free(text);
}

// Bloques pseudoaleatorios con semilla fija, sesgados hacia los caracteres
// que el parser mira, y pasados por TextEncoding como un .lrc. ASan y UBSan
// cortan si algo se sale del buffer, y LeakSanitizer si algo no se libera.
static void test_random(void) {
	static const char alphabet[] = "[]<>:.0123456789+-\n\r \tabcxyz";
	unsigned int seed = 777;
	unsigned char block[400];

	for (int n = 0; n < 5000; n++) {
		seed = seed * 1103515245u + 12345u;
		size_t len = (seed >> 16) % sizeof(block);

		for (size_t i = 0; i < len; i++) {
			seed = seed * 1103515245u + 12345u;
			unsigned int r = seed >> 16;
			block[i] = (n % 3 == 0) ? (unsigned char)r : (unsigned char)alphabet[r % (sizeof(alphabet) - 1)];
		}

		char *utf8 = NULL;
		size_t utf8_len = 0;
		CHECK(TextEncoding_ToUtf8(block, len, &utf8, &utf8_len) == 0, "ToUtf8 fallo");

		Lyrics l;
		CHECK(Lyrics_Parse(utf8, utf8_len, &l) == 0, "Lyrics_Parse fallo");

		for (uint32_t i = 0; i < l.count; i++) {
			CHECK(l.lines[i].off + l.lines[i].len <= utf8_len && l.text[l.lines[i].off + l.lines[i].len] == '\0',
				"linea %u fuera del buffer", i);
			if (l.synced && i > 0)
				CHECK(l.lines[i - 1].ms <= l.lines[i].ms, "lineas sin ordenar");
		}

		(void)Lyrics_LineAt(&l, seed);
		Lyrics_Free(&l);
		free(utf8);
	}
}

int main(void) {
	test_good();
	test_same_as_good();
	test_invalid_stamps();
	test_unclosed();
	test_long_line();
	test_no_lyrics();
	test_latin1();
	test_offsets();
	test_unordered();
	test_word_marks();
	test_genius();
	test_stanzas();
	test_lrclib_header();
	test_line_at();
	test_choose();
	test_size_limit();
	test_random();

	if (failures == 0)
		printf("test_lyrics: todo bien\n");

	return failures == 0 ? 0 : 1;
}
