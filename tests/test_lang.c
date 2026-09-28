// Pruebas en PC del modulo de idioma (source/lang.c), compiladas con el gcc del
// host. lang.c es logica pura, asi que no hace falta el VitaSDK.
//
// Correrlas desde la raiz del repo, en WSL o en cualquier Linux con gcc y make:
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "lang.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

static const char *const string_ids[STR_COUNT] = {
#define X(id, en, es) [id] = #id,
#include "lang_strings.h"
#undef X
};

// Idiomas de sistema (SceSystemParamLang) que no son espanol: japones, ingles
// de EE. UU., frances, aleman, ruso, chino simplificado e ingles britanico.
static const int non_spanish_systems[] = { 0, 1, 2, 4, 8, 11, 18 };
#define NON_SPANISH_COUNT (int)(sizeof(non_spanish_systems) / sizeof(non_spanish_systems[0]))

static void test_resolve(void) {
	for (int i = 0; i < NON_SPANISH_COUNT; i++) {
		int sys = non_spanish_systems[i];
		CHECK(Lang_Resolve(LANG_PREF_SYSTEM, sys) == LANG_EN, "Sistema con idioma %d deberia dar ingles", sys);
		CHECK(Lang_Resolve(LANG_PREF_ENGLISH, sys) == LANG_EN, "English con idioma %d deberia dar ingles", sys);
		CHECK(Lang_Resolve(LANG_PREF_SPANISH, sys) == LANG_ES, "Español con idioma %d deberia dar espanol", sys);
	}

	CHECK(Lang_Resolve(LANG_PREF_SYSTEM, LANG_SYSTEM_SPANISH) == LANG_ES, "Sistema en espanol deberia dar espanol");
	CHECK(Lang_Resolve(LANG_PREF_ENGLISH, LANG_SYSTEM_SPANISH) == LANG_EN, "English gana al sistema en espanol");
	CHECK(Lang_Resolve(LANG_PREF_SPANISH, LANG_SYSTEM_SPANISH) == LANG_ES, "Español con sistema en espanol");

	// Un idioma de sistema que no se pudo leer.
	CHECK(Lang_Resolve(LANG_PREF_SYSTEM, -1) == LANG_EN, "Sistema sin idioma leido deberia dar ingles");

	// Preferencias fuera de rango: se tratan como Sistema.
	const int invalid[] = { -1, LANG_PREF_COUNT, 7, 1000 };
	for (int i = 0; i < (int)(sizeof(invalid) / sizeof(invalid[0])); i++) {
		CHECK(Lang_Resolve(invalid[i], LANG_SYSTEM_SPANISH) == LANG_ES, "preferencia %d con sistema en espanol", invalid[i]);
		CHECK(Lang_Resolve(invalid[i], 1) == LANG_EN, "preferencia %d con sistema en ingles", invalid[i]);
	}
}

static void test_apply(void) {
	Lang_SetSystemLanguage(LANG_SYSTEM_SPANISH);
	Lang_Apply(LANG_PREF_SYSTEM);
	CHECK(Lang_Current() == LANG_ES, "Apply(Sistema) con consola en espanol");
	CHECK(strcmp(Lang_Get(STR_SETTINGS_TITLE), "Ajustes") == 0, "Lang_Get deberia seguir al idioma activo");

	Lang_Apply(LANG_PREF_ENGLISH);
	CHECK(Lang_Current() == LANG_EN, "Apply(English)");
	CHECK(strcmp(Lang_Get(STR_SETTINGS_TITLE), "Settings") == 0, "Lang_Get en ingles");

	// Volver a Sistema recalcula con el idioma guardado.
	Lang_Apply(LANG_PREF_SYSTEM);
	CHECK(Lang_Current() == LANG_ES, "volver a Sistema deberia recalcular");

	Lang_SetSystemLanguage(2);
	Lang_Apply(99);
	CHECK(Lang_Current() == LANG_EN, "preferencia invalida con consola en frances");
}

static void test_no_empty(void) {
	for (int lang = 0; lang < LANG_COUNT; lang++) {
		for (int id = 0; id < STR_COUNT; id++) {
			const char *text = Lang_GetIn((Lang)lang, (LangString)id);
			CHECK(text != NULL && text[0] != '\0', "%s vacio en el idioma %d", string_ids[id], lang);
		}
	}

	CHECK(Lang_GetIn(LANG_EN, STR_COUNT) != NULL, "un id fuera de rango no deberia dar NULL");
}

// Copia a `out` los especificadores de conversion de `fmt`, separados por
// espacios: "%d de %d" da "%d %d". Un "%%" cuenta, porque cambia lo que se
// imprime. Devuelve 0 si hay un % sin terminar.
static int extract_specs(const char *fmt, char *out, int cap) {
	int used = 0;
	out[0] = '\0';

	for (const char *p = fmt; *p; p++) {
		if (*p != '%')
			continue;

		const char *start = p++;
		while (*p && strchr("-+ #0", *p)) p++;
		while (*p && ((*p >= '0' && *p <= '9') || *p == '*' || *p == '.')) p++;
		while (*p && strchr("hlLqjzt", *p)) p++;

		if (!*p || !strchr("diouxXeEfFgGaAcspn%", *p))
			return 0;

		int len = (int)(p - start) + 1;
		if (used + len + 2 > cap)
			return 0;

		memcpy(out + used, start, len);
		used += len;
		out[used++] = ' ';
		out[used] = '\0';
	}

	return 1;
}

static void test_specifiers(void) {
	for (int id = 0; id < STR_COUNT; id++) {
		char en[128], es[128];
		const char *en_text = Lang_GetIn(LANG_EN, (LangString)id);
		const char *es_text = Lang_GetIn(LANG_ES, (LangString)id);

		CHECK(extract_specs(en_text, en, sizeof(en)), "%s: especificador mal formado en ingles: \"%s\"", string_ids[id], en_text);
		CHECK(extract_specs(es_text, es, sizeof(es)), "%s: especificador mal formado en espanol: \"%s\"", string_ids[id], es_text);
		CHECK(strcmp(en, es) == 0, "%s: especificadores distintos: \"%s\" frente a \"%s\"", string_ids[id], en_text, es_text);
	}
}

int main(void) {
	test_resolve();
	test_apply();
	test_no_empty();
	test_specifiers();

	if (failures) {
		printf("%d falla(s)\n", failures);
		return 1;
	}

	printf("test_lang: todo bien (%d textos por idioma)\n", STR_COUNT);
	return 0;
}
