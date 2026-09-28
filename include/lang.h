#ifndef _ELEVENMPV_LANG_H_
#define _ELEVENMPV_LANG_H_

// Los idiomas de la interfaz y el texto de cada pantalla en el idioma activo.
//
// Es logica pura: no incluye vita2d ni headers de SCE, para poder probarla en PC
// (tests/test_lang.c). Quien lee el idioma de la consola es main.c.

typedef enum {
#define X(id, en, es) id,
#include "lang_strings.h"
#undef X
	STR_COUNT
} LangString;

// El idioma efectivo: en cual se dibuja la interfaz.
typedef enum {
	LANG_EN = 0,
	LANG_ES,
	LANG_COUNT
} Lang;

// La preferencia guardada en config.language. Un valor fuera de rango se trata
// como Sistema.
#define LANG_PREF_SYSTEM  0
#define LANG_PREF_ENGLISH 1
#define LANG_PREF_SPANISH 2
#define LANG_PREF_COUNT   3

// Replica de SCE_SYSTEM_PARAM_LANG_SPANISH (psp2/system_param.h), para no
// incluir headers de SCE aqui.
#define LANG_SYSTEM_SPANISH 3

// El texto `id` en el idioma activo. Nunca devuelve NULL.
const char *Lang_Get(LangString id);

// El texto `id` en un idioma dado. Si falta, cae al ingles.
const char *Lang_GetIn(Lang lang, LangString id);

// El idioma efectivo para una preferencia y un idioma de sistema (un
// SceSystemParamLang). Pura: no toca el idioma activo.
Lang Lang_Resolve(int preference, int system_lang);

// Guarda el idioma de la consola leido al arrancar, para poder recalcular cuando
// el usuario vuelve a elegir Sistema.
void Lang_SetSystemLanguage(int system_lang);

// Resuelve `preference` contra el idioma de sistema guardado y lo deja activo.
// El siguiente fotograma ya se dibuja en ese idioma.
void Lang_Apply(int preference);

Lang Lang_Current(void);

#endif
