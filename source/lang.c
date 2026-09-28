#include <stddef.h>

#include "lang.h"

static const char *const lang_table[LANG_COUNT][STR_COUNT] = {
	[LANG_EN] = {
#define X(id, en, es) [id] = en,
#include "lang_strings.h"
#undef X
	},
	[LANG_ES] = {
#define X(id, en, es) [id] = es,
#include "lang_strings.h"
#undef X
	},
};

static Lang lang_current = LANG_EN;
static int lang_system = -1;

const char *Lang_GetIn(Lang lang, LangString id) {
	if ((int)id < 0 || id >= STR_COUNT)
		return "";

	if ((int)lang < 0 || lang >= LANG_COUNT)
		lang = LANG_EN;

	const char *text = lang_table[lang][id];

	if (text == NULL || text[0] == '\0')
		text = lang_table[LANG_EN][id];

	return text ? text : "";
}

const char *Lang_Get(LangString id) {
	return Lang_GetIn(lang_current, id);
}

Lang Lang_Resolve(int preference, int system_lang) {
	switch (preference) {
		case LANG_PREF_ENGLISH:
			return LANG_EN;
		case LANG_PREF_SPANISH:
			return LANG_ES;
		default:
			return (system_lang == LANG_SYSTEM_SPANISH) ? LANG_ES : LANG_EN;
	}
}

void Lang_SetSystemLanguage(int system_lang) {
	lang_system = system_lang;
}

void Lang_Apply(int preference) {
	lang_current = Lang_Resolve(preference, lang_system);
}

Lang Lang_Current(void) {
	return lang_current;
}
