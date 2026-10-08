// Pruebas en PC del ajuste de renglones de la letra (source/lyrics_layout.c),
// con una medida fija de 10 por punto de codigo en lugar de UI_TextWidth.
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
#include "lyrics_layout.h"
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
	const Lyrics *lyrics;
	int bad_line; // la medida recibio un texto fuera de la linea que decia
} MeasureCtx;

static float measure(const char *text, size_t len, uint32_t line, void *ctx) {
	MeasureCtx *m = ctx;
	const Lyrics_Line *l = &m->lyrics->lines[line];
	const char *begin = m->lyrics->text + l->off;

	if (line >= m->lyrics->count || text < begin || text + len > begin + l->len)
		m->bad_line = 1;

	float w = 0;
	for (size_t i = 0; i < len; i++) {
		if (((unsigned char)text[i] & 0xC0) != 0x80)
			w += 10;
	}
	return w;
}

static int is_boundary(const char *s, uint32_t off) {
	return ((unsigned char)s[off] & 0xC0) != 0x80;
}

// Arma la letra sin tiempos de `text` y su layout con `width`, y comprueba lo
// que vale siempre: renglones dentro de su linea, en orden, cortados en
// limites de UTF-8 y con a lo sumo `width` salvo que tengan un solo caracter.
static void build(const char *text, float width, Lyrics *l, LyricsLayout *layout) {
	CHECK(Lyrics_Parse(text, strlen(text), l) == 0, "Lyrics_Parse fallo");

	MeasureCtx m = { l, 0 };
	CHECK(LyricsLayout_Build(l, width, measure, &m, layout) == 0, "LyricsLayout_Build fallo");
	CHECK(!m.bad_line, "la medida recibio un texto de otra linea");

	uint32_t prev_line = 0;
	for (uint32_t r = 0; r < layout->count; r++) {
		const LyricsLayout_Row *row = &layout->rows[r];
		const Lyrics_Line *line = &l->lines[row->line];

		CHECK(row->line >= prev_line && row->line < l->count, "renglon %u con linea %u fuera de orden", r, row->line);
		CHECK(row->off >= line->off && row->off + row->len <= line->off + line->len, "renglon %u fuera de su linea", r);
		CHECK(row->len <= LYRICS_LAYOUT_MAX_ROW_BYTES, "renglon %u de %u bytes", r, row->len);

		if (row->len > 0) {
			CHECK(is_boundary(l->text, row->off) && (row->off + row->len == line->off + line->len || is_boundary(l->text, row->off + row->len)),
				"renglon %u cortado dentro de una secuencia UTF-8", r);

			float w = measure(l->text + row->off, row->len, row->line, &m);
			size_t first = TextEncoding_Utf8SeqLen((const unsigned char *)l->text + row->off, row->len);
			CHECK(w <= width || first == row->len, "renglon %u mide %.0f con ancho %.0f", r, w, width);
		}

		prev_line = row->line;
	}

	for (uint32_t i = 0; i < l->count; i++)
		CHECK(LyricsLayout_FirstRow(layout, i) < layout->count, "la linea %u no tiene renglones", i);
}

static void expect_rows(const char *name, const Lyrics *l, const LyricsLayout *layout, const char *const *want, uint32_t n) {
	CHECK(layout->count == n, "%s: %u renglones, se esperaban %u", name, layout->count, n);

	for (uint32_t r = 0; r < n && r < layout->count; r++) {
		const LyricsLayout_Row *row = &layout->rows[r];
		CHECK(row->len == strlen(want[r]) && memcmp(l->text + row->off, want[r], row->len) == 0,
			"%s: renglon %u es \"%.*s\", se esperaba \"%s\"", name, r, (int)row->len, l->text + row->off, want[r]);
	}
}

static void test_spaces(void) {
	static const char *const want[] = { "uno dos", "tres", "cuatro" };
	Lyrics l;
	LyricsLayout layout;

	build("uno dos tres cuatro", 100, &l, &layout);
	expect_rows("espacios", &l, &layout, want, 3);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);

	// Varios espacios donde se corta no quedan al principio del renglon.
	static const char *const many[] = { "abcd", "efgh" };
	build("abcd      efgh", 50, &l, &layout);
	expect_rows("varios espacios", &l, &layout, many, 2);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);

	// Una linea que cabe justo es un solo renglon.
	static const char *const exact[] = { "abcde fghi" };
	build("abcde fghi", 100, &l, &layout);
	expect_rows("justo", &l, &layout, exact, 1);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);
}

static void test_long_word(void) {
	static const char *const want[] = { "abcdefghij", "klmnopqrst", "uvwxyz" };
	Lyrics l;
	LyricsLayout layout;

	build("abcdefghijklmnopqrstuvwxyz", 100, &l, &layout);
	expect_rows("palabra larga", &l, &layout, want, 3);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);

	// Con caracteres de dos bytes, el corte cae entre puntos de codigo.
	static const char *const multi[] = { "ññññññññññ", "ñññ" };
	build("ñññññññññññññ", 100, &l, &layout);
	expect_rows("palabra larga multibyte", &l, &layout, multi, 2);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);

	// Un ancho menor que un caracter igual deja uno por renglon.
	static const char *const tiny[] = { "a", "b", "c" };
	build("abc", 5, &l, &layout);
	expect_rows("ancho minimo", &l, &layout, tiny, 3);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);
}

static void test_cjk(void) {
	// Hangul sin espacios: se corta entre dos caracteres.
	static const char *const want[] = { "어두워진하늘아래어두", "워진하늘" };
	Lyrics l;
	LyricsLayout layout;

	build("어두워진하늘아래어두워진하늘", 100, &l, &layout);
	expect_rows("hangul sin espacios", &l, &layout, want, 2);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);

	// Mezcla de latin y hangul: el latin no se parte si hay otro corte.
	static const char *const mixed[] = { "Look at my", "toe, 나의 ex 이름", "tattoo" };
	build("Look at my toe, 나의 ex 이름 tattoo", 130, &l, &layout);
	expect_rows("latin y hangul", &l, &layout, mixed, 3);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);

	// Kana y kanji.
	static const char *const kana[] = { "きみの名前を", "よんだ" };
	build("きみの名前をよんだ", 60, &l, &layout);
	expect_rows("kana", &l, &layout, kana, 2);
	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);
}

static void test_lines(void) {
	// Cada linea empieza en su propio renglon, y una linea vacia ocupa uno.
	static const char *const want[] = { "uno dos", "tres", "", "cuatro" };
	Lyrics l;
	LyricsLayout layout;

	build("uno dos tres\n\ncuatro\n", 100, &l, &layout);
	expect_rows("lineas", &l, &layout, want, 4);

	CHECK(LyricsLayout_FirstRow(&layout, 0) == 0, "primera fila de la linea 0");
	CHECK(LyricsLayout_FirstRow(&layout, 1) == 2, "primera fila de la linea 1: %u", LyricsLayout_FirstRow(&layout, 1));
	CHECK(LyricsLayout_FirstRow(&layout, 2) == 3, "primera fila de la linea 2: %u", LyricsLayout_FirstRow(&layout, 2));
	CHECK(LyricsLayout_FirstRow(&layout, 3) == layout.count, "una linea que no existe tuvo fila");
	CHECK(layout.rows[2].len == 0 && layout.rows[2].line == 1, "la linea vacia no tiene su renglon");

	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);

	// Una letra vacia da un layout vacio.
	Lyrics empty = { 0 };
	MeasureCtx m = { &empty, 0 };
	CHECK(LyricsLayout_Build(&empty, 100, measure, &m, &layout) == 0 && layout.count == 0 && layout.rows == NULL, "letra vacia");
	LyricsLayout_Free(&layout);
}

static void test_long_line_fixture(void) {
	// La linea de casi 3 KB de 04 linea larga.lrc, con el ancho de la vista.
	FILE *f = fopen("fixtures/lyrics/04 linea larga.lrc", "rb");
	if (f == NULL) {
		CHECK(0, "no se pudo abrir 04 linea larga.lrc");
		return;
	}

	char buf[8192];
	size_t len = fread(buf, 1, sizeof(buf) - 1, f);
	buf[len] = '\0';
	fclose(f);

	Lyrics l;
	LyricsLayout layout;
	build(buf, 768, &l, &layout);
	CHECK(layout.count > 20, "04 linea larga: solo %u renglones", layout.count);

	// Juntos, los renglones de la linea larga son todo su texto menos los
	// espacios de los cortes.
	size_t chars = 0;
	for (uint32_t r = 0; r < layout.count; r++) {
		if (layout.rows[r].line == 0)
			chars += layout.rows[r].len;
	}
	size_t spaces_dropped = LyricsLayout_FirstRow(&layout, 1) - 1;
	CHECK(chars + spaces_dropped == l.lines[0].len, "04 linea larga: se perdio texto (%zu + %zu de %u)", chars, spaces_dropped, l.lines[0].len);

	LyricsLayout_Free(&layout);
	Lyrics_Free(&l);
}

// Textos pseudoaleatorios con espacios, latin, hangul y bytes sueltos.
static void test_random(void) {
	static const char *const pieces[] = { " ", "  ", "a", "bc", "ñ", "어", "두워", "名", "x\xC3", "\xE2\x82" };
	unsigned int seed = 4242;
	char text[600];

	for (int n = 0; n < 3000; n++) {
		size_t len = 0;
		seed = seed * 1103515245u + 12345u;
		int parts = (seed >> 16) % 120;

		for (int p = 0; p < parts; p++) {
			seed = seed * 1103515245u + 12345u;
			const char *piece = pieces[(seed >> 16) % (sizeof(pieces) / sizeof(pieces[0]))];
			if (len + strlen(piece) + 2 >= sizeof(text))
				break;
			memcpy(text + len, piece, strlen(piece));
			len += strlen(piece);
			if ((seed >> 8) % 23 == 0)
				text[len++] = '\n';
		}
		text[len] = '\0';

		char *utf8 = NULL;
		size_t utf8_len = 0;
		TextEncoding_SanitizeUtf8((const unsigned char *)text, len, &utf8, &utf8_len);

		Lyrics l;
		LyricsLayout layout;
		seed = seed * 1103515245u + 12345u;
		build(utf8, 5 + (seed >> 16) % 200, &l, &layout);
		LyricsLayout_Free(&layout);
		Lyrics_Free(&l);
		free(utf8);
	}
}

int main(void) {
	test_spaces();
	test_long_word();
	test_cjk();
	test_lines();
	test_long_line_fixture();
	test_random();

	if (failures == 0)
		printf("test_lyrics_layout: todo bien\n");

	return failures == 0 ? 0 : 1;
}
