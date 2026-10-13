// Pruebas en PC del modelo de playlist (source/playlist.c), compiladas con el
// gcc del host. playlist.c es logica pura, asi que no hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "playlist.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// --- reservas que fallan a pedido ------------------------------------------
// playlist.c se compila con -DPLAYLIST_TEST_ALLOC y reserva todo por aqui.
// Con fail_at = n, la reserva numero n (desde 0) devuelve NULL; con -1 ninguna.

static int fail_at = -1;
static int alloc_calls = 0;

void *Playlist_TestRealloc(void *ptr, size_t size) {
	if (fail_at >= 0 && alloc_calls++ == fail_at)
		return NULL;

	return realloc(ptr, size);
}

static void fail_reset(int at) {
	fail_at = at;
	alloc_calls = 0;
}

// --- ayudas ----------------------------------------------------------------

static Playlist_Result parse_str(const char *text, Playlist *pl) {
	return Playlist_Parse(text, strlen(text), pl);
}

static void set_entry(Playlist_Entry *e, const char *path, const char *title, const char *artist, const char *album) {
	memset(e, 0, sizeof(*e));
	snprintf(e->path, sizeof(e->path), "%s", path);
	snprintf(e->title, sizeof(e->title), "%s", title);
	snprintf(e->artist, sizeof(e->artist), "%s", artist);
	snprintf(e->album, sizeof(e->album), "%s", album);
}

// Una playlist de `n` entradas con rutas "ux0:music/<prefix><i>.flac".
static void make_playlist(Playlist *pl, const char *name, const char *prefix, int n) {
	memset(pl, 0, sizeof(*pl));
	snprintf(pl->name, sizeof(pl->name), "%s", name);
	if (n == 0)
		return;

	pl->entries = calloc((size_t)n, sizeof(Playlist_Entry));
	pl->cap = n;
	for (int i = 0; i < n; i++) {
		char path[64];
		snprintf(path, sizeof(path), "ux0:music/%s%d.flac", prefix, i);
		set_entry(&pl->entries[i], path, "t", "a", "al");
	}
	pl->count = n;
}

// --- formato ---------------------------------------------------------------

static void test_round_trip(void) {
	Playlist pl;
	memset(&pl, 0, sizeof(pl));
	snprintf(pl.name, sizeof(pl.name), "%s", "Canción de \xF0\x9F\x8E\xB8 viaje");

	Playlist_Entry entries[4];
	set_entry(&entries[0], "ux0:music/Daft Punk/Discovery/03 Digital Love.flac", "Digital Love", "Daft Punk", "Discovery");
	set_entry(&entries[1], "ux0:music/ñandú/canción.mp3", "Canción \xF0\x9F\x98\x80", "Él y ñ", "Álbum");
	set_entry(&entries[2], "ux0:music/sin/tags.ogg", "", "", "");
	set_entry(&entries[3], "ux0:music/raro.opus", "Uno - Dos", "", "");
	pl.entries = entries;
	pl.count = pl.cap = 4;

	char *text = NULL;
	size_t len = 0;
	CHECK(Playlist_Serialize(&pl, &text, &len) == PLAYLIST_OK, "serializar fallo");
	CHECK(text != NULL && strlen(text) == len, "el largo no coincide con el texto");
	CHECK(text != NULL && strncmp(text, "#EXTM3U\n#PLAYLIST:Canción", 25) == 0, "no empieza con la cabecera: %s", text);
	CHECK(text != NULL && strstr(text, "#EXTINF:-1,Daft Punk - Digital Love\n#EXTALB:Discovery\nux0:music/Daft Punk/Discovery/03 Digital Love.flac\n") != NULL,
		"la entrada no tiene el formato de design.md:\n%s", text);
	CHECK(text != NULL && strchr(text, '\r') == NULL && (unsigned char)text[0] != 0xEF, "escribio CR o BOM");

	Playlist back;
	CHECK(Playlist_Parse(text, len, &back) == PLAYLIST_OK, "releer lo escrito fallo");
	CHECK(strcmp(back.name, pl.name) == 0, "el nombre no volvio igual: %s", back.name);
	CHECK(back.count == 4, "volvieron %d entradas y no 4", back.count);
	for (int i = 0; i < 4 && i < back.count; i++) {
		CHECK(strcmp(back.entries[i].path, entries[i].path) == 0, "ruta %d: %s", i, back.entries[i].path);
		CHECK(strcmp(back.entries[i].title, entries[i].title) == 0, "titulo %d: %s", i, back.entries[i].title);
		CHECK(strcmp(back.entries[i].artist, entries[i].artist) == 0, "artista %d: %s", i, back.entries[i].artist);
		CHECK(strcmp(back.entries[i].album, entries[i].album) == 0, "album %d: %s", i, back.entries[i].album);
	}

	Playlist_Free(&back);
	free(text);
}

static void test_empty_playlist_round_trip(void) {
	Playlist pl;
	memset(&pl, 0, sizeof(pl));
	snprintf(pl.name, sizeof(pl.name), "%s", "Vacia");

	char *text = NULL;
	size_t len = 0;
	CHECK(Playlist_Serialize(&pl, &text, &len) == PLAYLIST_OK, "serializar una vacia fallo");

	Playlist back;
	CHECK(Playlist_Parse(text, len, &back) == PLAYLIST_OK, "una playlist vacia con nombre no es valida");
	CHECK(back.count == 0 && back.entries == NULL && strcmp(back.name, "Vacia") == 0, "la vacia no volvio igual");
	Playlist_Free(&back);
	free(text);
}

static void test_tolerant_reader(void) {
	const char *text =
		"\xEF\xBB\xBF" "#EXTM3U\r\n"
		"\r\n"
		"#PLAYLIST:A mano\r\n"
		"#EXTGRP:algo que no conozco\r\n"
		"\r\n"
		"#EXTINF:123,Artista - Titulo\r\n"
		"#EXTALB:Disco\r\n"
		"ux0:music/a.flac\r\n"
		"ux0:music/sin extinf.mp3\r\n"
		"   \r\n"
		"#EXTINF:-1,Solo titulo\n"
		"ux0:music/c.ogg";
	Playlist pl;

	CHECK(parse_str(text, &pl) == PLAYLIST_OK, "BOM, CRLF y lineas raras no se aceptaron");
	CHECK(strcmp(pl.name, "A mano") == 0, "nombre: '%s'", pl.name);
	CHECK(pl.count == 3, "%d entradas y no 3", pl.count);
	if (pl.count == 3) {
		CHECK(strcmp(pl.entries[0].path, "ux0:music/a.flac") == 0, "ruta 0 con CR: '%s'", pl.entries[0].path);
		CHECK(strcmp(pl.entries[0].artist, "Artista") == 0 && strcmp(pl.entries[0].title, "Titulo") == 0, "EXTINF mal separado");
		CHECK(strcmp(pl.entries[0].album, "Disco") == 0, "EXTALB: '%s'", pl.entries[0].album);
		CHECK(pl.entries[1].title[0] == '\0' && pl.entries[1].album[0] == '\0', "la ruta sin EXTINF heredo datos de la anterior");
		CHECK(strcmp(pl.entries[2].title, "Solo titulo") == 0 && pl.entries[2].artist[0] == '\0', "titulo sin artista mal leido");
		CHECK(strcmp(pl.entries[2].path, "ux0:music/c.ogg") == 0, "la ultima linea sin salto se perdio");
	}
	Playlist_Free(&pl);
}

static void test_no_name_with_entries(void) {
	Playlist pl;
	CHECK(parse_str("#EXTM3U\nux0:music/a.flac\n", &pl) == PLAYLIST_OK, "sin #PLAYLIST pero con entradas deberia ser valida");
	CHECK(pl.name[0] == '\0' && pl.count == 1, "nombre o cantidad mal");
	Playlist_Free(&pl);
}

static void test_invalid(void) {
	static const char *bad[] = {
		"",
		"\n\n",
		"\xEF\xBB\xBF",
		"ux0:music/a.flac\n",                // sin #EXTM3U
		"hola, esto no es una playlist\n",
		"#EXTM3U\n",                         // ni nombre ni entradas
		"#EXTM3U\n#PLAY",                    // truncado en la cabecera
		"#EXTM3U\n#EXTINF:-1,A - B\n",       // truncado antes de la ruta
		"#EXTINF:-1,A - B\nux0:music/a.flac\n#EXTM3U\n",
	};

	for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		Playlist pl;
		Playlist_Result r = parse_str(bad[i], &pl);
		CHECK(r == PLAYLIST_INVALID, "el caso %zu dio %d y no INVALID", i, (int)r);
		CHECK(pl.entries == NULL && pl.count == 0, "el caso %zu dejo entradas", i);
	}

	Playlist pl;
	CHECK(Playlist_Parse(NULL, 0, &pl) == PLAYLIST_INVALID, "NULL de largo 0 no dio INVALID");

	// Bytes nulos y binario al azar tampoco rompen nada.
	char junk[300];
	for (size_t i = 0; i < sizeof(junk); i++)
		junk[i] = (char)(i * 37 + 11);
	CHECK(Playlist_Parse(junk, sizeof(junk), &pl) == PLAYLIST_INVALID, "basura binaria no dio INVALID");
}

static void test_long_path_and_ceiling(void) {
	// Una ruta de 300 bytes no cabe y se salta; las demas quedan.
	char text[1024];
	char longpath[301];
	memset(longpath, 'x', 300);
	longpath[300] = '\0';
	snprintf(text, sizeof(text), "#EXTM3U\n#PLAYLIST:L\nux0:a\n%s\nux0:b\n", longpath);

	Playlist pl;
	CHECK(parse_str(text, &pl) == PLAYLIST_OK && pl.count == 2, "la ruta larga no se salto bien (%d)", pl.count);
	Playlist_Free(&pl);

	// Mas de 4000 entradas en el archivo: se leen las primeras 4000.
	size_t cap = 32 + 4100 * 24;
	char *big = malloc(cap);
	size_t n = (size_t)snprintf(big, cap, "#EXTM3U\n");
	for (int i = 0; i < 4100; i++)
		n += (size_t)snprintf(big + n, cap - n, "ux0:music/%d.flac\n", i);

	CHECK(Playlist_Parse(big, n, &pl) == PLAYLIST_OK && pl.count == PLAYLIST_MAX_ENTRIES, "el techo no se respeto (%d)", pl.count);
	CHECK(pl.count == PLAYLIST_MAX_ENTRIES && strcmp(pl.entries[3999].path, "ux0:music/3999.flac") == 0, "no son las primeras 4000");
	Playlist_Free(&pl);
	free(big);
}

static void test_utf8_cut(void) {
	// Un titulo mas largo que el campo se corta sin partir una ñ.
	char text[512];
	char title[200];
	memset(title, 'a', 62);
	strcpy(title + 62, "ñññ");
	snprintf(text, sizeof(text), "#EXTM3U\n#EXTINF:-1,%s\nux0:a\n", title);

	Playlist pl;
	CHECK(parse_str(text, &pl) == PLAYLIST_OK && pl.count == 1, "no leyo la entrada");
	if (pl.count == 1)
		CHECK(strlen(pl.entries[0].title) == 62, "el corte partio un caracter (%zu bytes)", strlen(pl.entries[0].title));
	Playlist_Free(&pl);
}

static void test_newlines_in_fields(void) {
	Playlist pl;
	make_playlist(&pl, "Dos\nlineas", "n", 1);
	snprintf(pl.entries[0].title, sizeof(pl.entries[0].title), "%s", "Ti\r\ntulo");

	char *text = NULL;
	size_t len = 0;
	CHECK(Playlist_Serialize(&pl, &text, &len) == PLAYLIST_OK, "serializar fallo");

	Playlist back;
	CHECK(Playlist_Parse(text, len, &back) == PLAYLIST_OK && back.count == 1, "no se releyo");
	CHECK(strcmp(back.name, "Dos lineas") == 0, "nombre: '%s'", back.name);
	CHECK(back.count == 1 && strcmp(back.entries[0].title, "Ti  tulo") == 0, "titulo: '%s'", back.count ? back.entries[0].title : "");
	Playlist_Free(&back);
	Playlist_Free(&pl);
	free(text);
}

static void test_parse_nomem(void) {
	const char *text = "#EXTM3U\n#PLAYLIST:X\nux0:a\nux0:b\n";
	Playlist pl;

	fail_reset(0);
	CHECK(parse_str(text, &pl) == PLAYLIST_NOMEM, "sin memoria no dio NOMEM");
	CHECK(pl.entries == NULL && pl.count == 0, "sin memoria dejo algo a medias");
	fail_reset(-1);

	Playlist src;
	make_playlist(&src, "S", "s", 2);
	char *out = NULL;
	size_t len = 0;
	fail_reset(0);
	CHECK(Playlist_Serialize(&src, &out, &len) == PLAYLIST_NOMEM && out == NULL, "serializar sin memoria no dio NOMEM");
	fail_reset(-1);
	Playlist_Free(&src);
}

// --- operaciones -----------------------------------------------------------

// Si las rutas de la playlist son, en orden, "ux0:music/<prefix><ids[i]>.flac".
static int order_is(const Playlist *pl, const char *prefix, const int *ids, int n) {
	if (pl->count != n)
		return 0;

	for (int i = 0; i < n; i++) {
		char path[64];
		snprintf(path, sizeof(path), "ux0:music/%s%d.flac", prefix, ids[i]);
		if (strcmp(pl->entries[i].path, path) != 0)
			return 0;
	}

	return 1;
}

static void test_append_duplicates(void) {
	// "Viaje" ya tiene 3 de las 10 pistas de un album.
	Playlist viaje, album;
	make_playlist(&viaje, "Viaje", "d", 0);
	make_playlist(&album, "", "d", 10);

	Playlist_Entry three[3] = { album.entries[2], album.entries[5], album.entries[9] };
	int added = -1, skipped = -1, full = -1;
	CHECK(Playlist_Append(&viaje, three, 3, &added, &skipped, &full) == PLAYLIST_OK, "anadir a una vacia fallo");
	CHECK(added == 3 && skipped == 0 && full == 0, "a una vacia: %d anadidas, %d omitidas, llena %d", added, skipped, full);

	CHECK(Playlist_Append(&viaje, album.entries, 10, &added, &skipped, &full) == PLAYLIST_OK, "anadir el album fallo");
	CHECK(added == 7 && skipped == 3 && full == 0, "el album: %d anadidas, %d omitidas, llena %d", added, skipped, full);

	const int expect[] = { 2, 5, 9, 0, 1, 3, 4, 6, 7, 8 };
	CHECK(order_is(&viaje, "d", expect, 10), "no quedaron al final y en el orden del disco");

	// Repetidas dentro de lo que se anade tampoco entran dos veces.
	Playlist_Entry twice[2] = { album.entries[0], album.entries[0] };
	Playlist other;
	make_playlist(&other, "O", "z", 0);
	CHECK(Playlist_Append(&other, twice, 2, &added, &skipped, NULL) == PLAYLIST_OK && added == 1 && skipped == 1,
		"la repetida dentro del mismo anadido entro dos veces");

	// Los contadores pueden ser NULL, y anadir nada no cambia nada.
	CHECK(Playlist_Append(&other, NULL, 0, NULL, NULL, NULL) == PLAYLIST_OK && other.count == 1, "anadir 0 cambio algo");

	Playlist_Free(&other);
	Playlist_Free(&viaje);
	Playlist_Free(&album);
}

static void test_append_full(void) {
	Playlist pl, more;
	make_playlist(&pl, "Casi llena", "f", 3998);
	make_playlist(&more, "", "g", 5);

	int added = -1, skipped = -1, full = -1;
	CHECK(Playlist_Append(&pl, more.entries, 5, &added, &skipped, &full) == PLAYLIST_OK, "anadir a una casi llena fallo");
	CHECK(pl.count == PLAYLIST_MAX_ENTRIES, "quedaron %d y no 4000", pl.count);
	CHECK(added == 2 && full == 1, "%d anadidas, llena %d", added, full);
	CHECK(strcmp(pl.entries[3998].path, "ux0:music/g0.flac") == 0 && strcmp(pl.entries[3999].path, "ux0:music/g1.flac") == 0,
		"no entraron las primeras que cabian, en orden");

	// Llena del todo: no entra nada, pero las que ya estaban se cuentan.
	CHECK(Playlist_Append(&pl, more.entries, 5, &added, &skipped, &full) == PLAYLIST_OK, "anadir a una llena fallo");
	CHECK(added == 0 && skipped == 2 && full == 1, "a una llena: %d anadidas, %d omitidas, llena %d", added, skipped, full);

	Playlist_Free(&pl);
	Playlist_Free(&more);
}

static void test_append_nomem(void) {
	Playlist pl, more;
	make_playlist(&pl, "P", "p", 3);
	make_playlist(&more, "", "q", 4);

	fail_reset(0);
	int added = -1;
	CHECK(Playlist_Append(&pl, more.entries, 4, &added, NULL, NULL) == PLAYLIST_NOMEM, "sin memoria no dio NOMEM");
	fail_reset(-1);

	const int expect[] = { 0, 1, 2 };
	CHECK(order_is(&pl, "p", expect, 3) && pl.cap == 3, "sin memoria la playlist cambio");
	CHECK(added == -1, "sin memoria escribio los contadores");

	Playlist_Free(&pl);
	Playlist_Free(&more);
}

static void test_remove(void) {
	Playlist pl;
	make_playlist(&pl, "R", "r", 5);

	CHECK(Playlist_Remove(&pl, 0) == 0, "quitar la primera fallo");
	const int a[] = { 1, 2, 3, 4 };
	CHECK(order_is(&pl, "r", a, 4), "quitar la primera dejo otro orden");

	CHECK(Playlist_Remove(&pl, 1) == 0, "quitar una del medio fallo");
	const int b[] = { 1, 3, 4 };
	CHECK(order_is(&pl, "r", b, 3), "quitar una del medio dejo otro orden");

	CHECK(Playlist_Remove(&pl, 2) == 0, "quitar la ultima fallo");
	const int c[] = { 1, 3 };
	CHECK(order_is(&pl, "r", c, 2), "quitar la ultima dejo otro orden");

	CHECK(Playlist_Remove(&pl, 2) == -1 && Playlist_Remove(&pl, -1) == -1, "quitar fuera de rango no dio -1");
	CHECK(order_is(&pl, "r", c, 2), "quitar fuera de rango cambio algo");

	Playlist_Free(&pl);
}

static void test_move(void) {
	Playlist pl;
	make_playlist(&pl, "M", "m", 6);

	// La quinta, tres puestos arriba: queda segunda (spec "Mover una cancion").
	CHECK(Playlist_Move(&pl, 4, 1) == 0, "mover hacia arriba fallo");
	const int up[] = { 0, 4, 1, 2, 3, 5 };
	CHECK(order_is(&pl, "m", up, 6), "mover hacia arriba dejo otro orden");

	CHECK(Playlist_Move(&pl, 1, 3) == 0, "mover hacia abajo fallo");
	const int down[] = { 0, 1, 2, 4, 3, 5 };
	CHECK(order_is(&pl, "m", down, 6), "mover hacia abajo dejo otro orden");

	CHECK(Playlist_Move(&pl, 5, 0) == 0, "mover al principio fallo");
	const int first[] = { 5, 0, 1, 2, 4, 3 };
	CHECK(order_is(&pl, "m", first, 6), "mover al principio dejo otro orden");

	CHECK(Playlist_Move(&pl, 0, 5) == 0, "mover al final fallo");
	const int last[] = { 0, 1, 2, 4, 3, 5 };
	CHECK(order_is(&pl, "m", last, 6), "mover al final dejo otro orden");

	CHECK(Playlist_Move(&pl, 2, 2) == 0 && order_is(&pl, "m", last, 6), "mover a su mismo puesto cambio algo");

	CHECK(Playlist_Move(&pl, 0, -1) == -1 && Playlist_Move(&pl, 0, 6) == -1 && Playlist_Move(&pl, 6, 0) == -1,
		"mover fuera de rango no dio -1");
	CHECK(order_is(&pl, "m", last, 6), "mover fuera de rango cambio algo");

	Playlist_Free(&pl);
}

// --- nombres ---------------------------------------------------------------

static void test_name_valid(void) {
	CHECK(Playlist_NameValid("Favoritas"), "un nombre normal no fue valido");
	CHECK(!Playlist_NameValid(""), "el nombre vacio fue valido");
	CHECK(!Playlist_NameValid("   "), "un nombre de solo espacios fue valido");
	CHECK(!Playlist_NameValid(" \t "), "espacios y tabuladores fueron validos");
	CHECK(Playlist_NameValid("  a  "), "un nombre con espacios alrededor no fue valido");

	char name[PLAYLIST_NAME_MAX];
	memset(name, 'a', 64);
	name[64] = '\0';
	CHECK(Playlist_NameValid(name), "64 caracteres no fue valido");
	name[64] = 'a';
	name[65] = '\0';
	CHECK(!Playlist_NameValid(name), "65 caracteres fue valido");

	// 64 eñes son 128 bytes, pero 64 caracteres.
	name[0] = '\0';
	for (int i = 0; i < 64; i++)
		strcat(name, "ñ");
	CHECK(Playlist_NameValid(name), "64 enes no fueron validas");
	strcat(name, "ñ");
	CHECK(!Playlist_NameValid(name), "65 enes fueron validas");
}

static void test_names_equal(void) {
	CHECK(Playlist_NamesEqual("Favoritas", "FAVORÍTAS"), "Favoritas y FAVORÍTAS no fueron iguales");
	CHECK(Playlist_NamesEqual("favoritas", "Favoritas"), "solo mayusculas no fue igual");
	CHECK(Playlist_NamesEqual("Año", "ANO"), "la ñ no se trato como n");
	CHECK(!Playlist_NamesEqual("Viaje", "Viaje a la costa"), "un prefijo fue igual");
	CHECK(!Playlist_NamesEqual("Viaje", "Viajes"), "nombres distintos fueron iguales");
}

// Simula una carpeta con estos archivos, comparando sin mayusculas como la Vita.
typedef struct {
	const char **files;
	int count;
} Folder;

static int folder_has(const char *file, void *ctx) {
	const Folder *f = ctx;
	for (int i = 0; i < f->count; i++) {
		if (strcasecmp(f->files[i], file) == 0)
			return 1;
	}
	return 0;
}

static void test_file_name(void) {
	char out[PLAYLIST_FILE_MAX];

	CHECK(Playlist_FileName("Favoritas", NULL, NULL, out, sizeof(out)) == 0 && strcmp(out, "Favoritas.m3u8") == 0, "simple: '%s'", out);

	CHECK(Playlist_FileName("AC/DC: lo mejor?", NULL, NULL, out, sizeof(out)) == 0, "AC/DC fallo");
	CHECK(strcmp(out, "AC_DC_ lo mejor_.m3u8") == 0, "AC/DC: '%s'", out);

	CHECK(Playlist_FileName("a\\b*c\"d<e>f|g\th\x01", NULL, NULL, out, sizeof(out)) == 0, "caracteres raros fallo");
	CHECK(strcmp(out, "a_b_c_d_e_f_g_h_.m3u8") == 0, "caracteres raros: '%s'", out);

	CHECK(Playlist_FileName("Canción ñandú", NULL, NULL, out, sizeof(out)) == 0 && strcmp(out, "Canción ñandú.m3u8") == 0,
		"las tildes no se conservaron: '%s'", out);

	// 64 bytes justos quedan enteros; mas se cortan a 64.
	char name[PLAYLIST_NAME_MAX];
	memset(name, 'x', 64);
	name[64] = '\0';
	CHECK(Playlist_FileName(name, NULL, NULL, out, sizeof(out)) == 0 && strlen(out) == 64 + 5, "64 bytes: %zu", strlen(out));

	// 63 equis y una ñ: la ñ no cabe entera y sale, sin dejar medio caracter.
	memset(name, 'x', 63);
	strcpy(name + 63, "ñyy");
	CHECK(Playlist_FileName(name, NULL, NULL, out, sizeof(out)) == 0, "corte UTF-8 fallo");
	CHECK(strlen(out) == 63 + 5 && strncmp(out + 63, ".m3u8", 5) == 0, "el corte partio la ñ: '%s'", out);

	// Colision: "a/b" y "a_b" dan el mismo archivo.
	const char *taken[] = { "a_b.m3u8" };
	Folder one = { taken, 1 };
	CHECK(Playlist_FileName("a/b", folder_has, &one, out, sizeof(out)) == 0 && strcmp(out, "a_b (2).m3u8") == 0, "colision: '%s'", out);

	const char *taken3[] = { "A_B.m3u8", "a_b (2).m3u8", "a_b (3).M3U8" };
	Folder three = { taken3, 3 };
	CHECK(Playlist_FileName("a:b", folder_has, &three, out, sizeof(out)) == 0 && strcmp(out, "a_b (4).m3u8") == 0, "tres colisiones: '%s'", out);

	// Con el sufijo tampoco se pasa de 64 bytes antes de la extension.
	memset(name, 'x', 64);
	name[64] = '\0';
	char first[PLAYLIST_FILE_MAX];
	memcpy(first, name, 64);
	strcpy(first + 64, ".m3u8");
	const char *long_taken[] = { first };
	Folder lf = { long_taken, 1 };
	CHECK(Playlist_FileName(name, folder_has, &lf, out, sizeof(out)) == 0, "colision larga fallo");
	CHECK(strlen(out) == 64 + 5 && strstr(out, " (2).m3u8") != NULL, "colision larga: '%s' (%zu)", out, strlen(out));

	// Un buffer chico no se desborda.
	char tiny[8];
	CHECK(Playlist_FileName("Favoritas", NULL, NULL, tiny, sizeof(tiny)) == -1, "un buffer chico no dio -1");
}

// --- recuperacion tras un corte --------------------------------------------

static void test_recover_table(void) {
	// Las tres filas de la tabla de design.md.
	CHECK(Playlist_Recover(1, 1, 0) == PLAYLIST_RECOVER_DELETE_TMP, "corte en 1");
	CHECK(Playlist_Recover(0, 1, 1) == (PLAYLIST_RECOVER_RESTORE_BAK | PLAYLIST_RECOVER_DELETE_TMP), "corte entre 2 y 3");
	CHECK(Playlist_Recover(1, 0, 1) == PLAYLIST_RECOVER_DELETE_BAK, "corte en 4");
	// Solo la playlist, y nada.
	CHECK(Playlist_Recover(1, 0, 0) == 0, "solo main hizo algo");
	CHECK(Playlist_Recover(0, 0, 0) == 0, "nada hizo algo");
	// Las combinaciones que no salen de un solo guardado tambien quedan bien.
	CHECK(Playlist_Recover(0, 1, 0) == PLAYLIST_RECOVER_DELETE_TMP, "solo tmp (una nueva que no llego a existir)");
	CHECK(Playlist_Recover(0, 0, 1) == PLAYLIST_RECOVER_RESTORE_BAK, "solo bak");
	CHECK(Playlist_Recover(1, 1, 1) == (PLAYLIST_RECOVER_DELETE_TMP | PLAYLIST_RECOVER_DELETE_BAK), "los tres");
}

// Un disco de mentira con la terna de archivos de una playlist. NULL es que el
// archivo no existe.
typedef struct {
	const char *main, *tmp, *bak;
} Disk;

// Corre los pasos del guardado de design.md sobre el disco, y se corta antes
// del paso `cut` (1 a 4; 5 no corta). Un corte en el paso 1 deja el .tmp a medias.
static void save_with_cut(Disk *d, const char *content, int cut) {
	if (cut == 1) {
		d->tmp = "#EXTM3U\n#PLAYL";
		return;
	}
	d->tmp = content;
	if (cut == 2)
		return;
	if (d->main != NULL) {
		d->bak = d->main;
		d->main = NULL;
	}
	if (cut == 3)
		return;
	d->main = d->tmp;
	d->tmp = NULL;
	if (cut == 4)
		return;
	d->bak = NULL;
}

static void recover(Disk *d) {
	int a = Playlist_Recover(d->main != NULL, d->tmp != NULL, d->bak != NULL);

	if (a & PLAYLIST_RECOVER_RESTORE_BAK) {
		d->main = d->bak;
		d->bak = NULL;
	}
	if (a & PLAYLIST_RECOVER_DELETE_TMP)
		d->tmp = NULL;
	if (a & PLAYLIST_RECOVER_DELETE_BAK)
		d->bak = NULL;
}

static void test_interrupted_save(void) {
	const char *old_text = "#EXTM3U\n#PLAYLIST:V\nux0:a\nux0:b\n";
	const char *new_text = "#EXTM3U\n#PLAYLIST:V\nux0:a\n";

	for (int cut = 1; cut <= 5; cut++) {
		Disk d = { old_text, NULL, NULL };
		save_with_cut(&d, new_text, cut);
		recover(&d);

		CHECK(d.tmp == NULL && d.bak == NULL, "corte %d: quedaron restos", cut);
		CHECK(d.main == old_text || d.main == new_text, "corte %d: la playlist no esta en ninguna de sus versiones", cut);
		if (cut == 1)
			CHECK(d.main == old_text, "escribir a medias no dejo el contenido anterior");

		Playlist pl;
		CHECK(d.main != NULL && parse_str(d.main, &pl) == PLAYLIST_OK && pl.count > 0, "corte %d: la playlist quedo vacia o ilegible", cut);
		Playlist_Free(&pl);
	}

	// Una playlist nueva que se corta antes de existir no deja nada.
	Disk d = { NULL, NULL, NULL };
	save_with_cut(&d, new_text, 1);
	recover(&d);
	CHECK(d.main == NULL && d.tmp == NULL && d.bak == NULL, "una nueva cortada dejo restos");
}

// --- mosaico ---------------------------------------------------------------

// Que dice el cache de cada album: una cadena con una letra por album
// ('A' + i) que vale '1' con caratula, '0' sin ella y '-' sin mirar.
typedef struct {
	const char *covers;
	int calls;
} Probe;

static int probe_table(const char *album, const char *path, void *ctx) {
	Probe *p = ctx;
	p->calls++;
	if (album[0] == '\0')
		return strstr(path, "con") != NULL ? 1 : 0;

	char c = p->covers[album[0] - 'A'];
	return c == '1' ? 1 : c == '0' ? 0 : -1;
}

// Una playlist cuyas entradas son de los albumes de `albums`, una letra por
// pista; '.' es una pista sin album.
static void make_albums(Playlist *pl, const char *albums) {
	int n = (int)strlen(albums);
	make_playlist(pl, "", "m", n);
	for (int i = 0; i < n; i++) {
		if (albums[i] == '.')
			pl->entries[i].album[0] = '\0';
		else
			snprintf(pl->entries[i].album, sizeof(pl->entries[i].album), "%c", albums[i]);
	}
}

static int mosaic_is(const Playlist *pl, const int *out, int n, const char *expect) {
	if ((int)strlen(expect) != n)
		return 0;
	for (int i = 0; i < n; i++) {
		if (pl->entries[out[i]].album[0] != expect[i])
			return 0;
	}
	return 1;
}

static void test_mosaic(void) {
	Playlist pl;
	int out[4], pending;
	Probe p = { "111111", 0 };

	make_albums(&pl, "AABCBD");
	int n = Playlist_MosaicAlbums(&pl, probe_table, &p, out, &pending);
	CHECK(n == 4 && mosaic_is(&pl, out, n, "ABCD") && pending == 0, "A,A,B,C,B,D no dio A,B,C,D (%d)", n);
	CHECK(p.calls == 4, "pregunto %d veces y no 4", p.calls);
	Playlist_Free(&pl);

	// B y D no tienen caratula: se saltan.
	Probe skip = { "101011", 0 };
	make_albums(&pl, "ABBCDEF");
	n = Playlist_MosaicAlbums(&pl, probe_table, &skip, out, &pending);
	CHECK(n == 4 && mosaic_is(&pl, out, n, "ACEF"), "no salto los albumes sin caratula (%d)", n);
	CHECK(skip.calls == 6, "pregunto %d veces por 6 albumes", skip.calls);
	Playlist_Free(&pl);

	// 0, 1, 2 y 3 albumes distintos.
	const char *few[] = { "", "AAA", "ABAB", "ABCCBA" };
	for (int k = 0; k < 4; k++) {
		Probe all = { "111111", 0 };
		make_albums(&pl, few[k]);
		n = Playlist_MosaicAlbums(&pl, probe_table, &all, out, NULL);
		CHECK(n == k, "'%s' dio %d albumes y no %d", few[k], n, k);
		Playlist_Free(&pl);
	}

	// Ninguno con caratula.
	Probe none = { "000000", 0 };
	make_albums(&pl, "ABC");
	CHECK(Playlist_MosaicAlbums(&pl, probe_table, &none, out, &pending) == 0 && pending == 0, "sin caratulas eligio algo");
	Playlist_Free(&pl);

	// Un album sin mirar se salta por ahora, y lo avisa.
	Probe waiting = { "1-1111", 0 };
	make_albums(&pl, "ABCDE");
	n = Playlist_MosaicAlbums(&pl, probe_table, &waiting, out, &pending);
	CHECK(n == 4 && mosaic_is(&pl, out, n, "ACDE") && pending == 1, "el album sin mirar no marco pendiente");
	Playlist_Free(&pl);

	// Album vacio: cada pista es su propio album, con clave por ruta.
	Probe by_path = { "111111", 0 };
	make_albums(&pl, "..A");
	snprintf(pl.entries[0].path, sizeof(pl.entries[0].path), "%s", "ux0:music/con caratula.flac");
	snprintf(pl.entries[1].path, sizeof(pl.entries[1].path), "%s", "ux0:music/sin.flac");
	n = Playlist_MosaicAlbums(&pl, probe_table, &by_path, out, NULL);
	CHECK(n == 2 && out[0] == 0 && out[1] == 2, "el album vacio no se trato por ruta (%d)", n);
	Playlist_Free(&pl);

	// Dos pistas sin album, cada una con su caratula, son dos albumes.
	Probe two_empty = { "111111", 0 };
	make_albums(&pl, "..");
	snprintf(pl.entries[0].path, sizeof(pl.entries[0].path), "%s", "ux0:music/con 1.flac");
	snprintf(pl.entries[1].path, sizeof(pl.entries[1].path), "%s", "ux0:music/con 2.flac");
	CHECK(Playlist_MosaicAlbums(&pl, probe_table, &two_empty, out, NULL) == 2, "dos pistas sin album no fueron dos albumes");
	Playlist_Free(&pl);
}

int main(void) {
	test_round_trip();
	test_empty_playlist_round_trip();
	test_tolerant_reader();
	test_no_name_with_entries();
	test_invalid();
	test_long_path_and_ceiling();
	test_utf8_cut();
	test_newlines_in_fields();
	test_parse_nomem();
	test_append_duplicates();
	test_append_full();
	test_append_nomem();
	test_remove();
	test_move();
	test_name_valid();
	test_names_equal();
	test_file_name();
	test_recover_table();
	test_interrupted_save();
	test_mosaic();

	if (failures == 0)
		printf("test_playlist: todo bien\n");

	return failures == 0 ? 0 : 1;
}
