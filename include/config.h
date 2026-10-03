#ifndef _ELEVENMPV_CONFIG_H_
#define _ELEVENMPV_CONFIG_H_

#include <psp2/types.h>

typedef struct {
	SceBool meta_flac;
	SceBool meta_mp3;
	SceBool meta_opus;
	int sort;
	int alc_mode;
	int device;
	int eq_mode;      // 0 = Off, 1 = Heavy, 2 = Pop, 3 = Jazz, 4 = Unique
	SceBool eq_volume; // Halve output volume while an EQ preset is active
	int language;     // 0 = System, 1 = English, 2 = Español (LANG_PREF_*)
	int group_end;    // When an album or artist ends: CONFIG_GROUP_END_*
} config_t;

// Any value other than NEXT behaves as REPEAT, the default.
#define CONFIG_GROUP_END_REPEAT 0
#define CONFIG_GROUP_END_NEXT   1

extern config_t config;

int Config_Save(config_t config);
int Config_Load(void);
int Config_GetLastDirectory(void);

#endif
