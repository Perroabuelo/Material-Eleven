#include <psp2/io/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "fs.h"
#include "lang.h"

// v2 -> v3 added "language" and v3 -> v4 "group_end". Each version appends its lines at the end of the
// format, so an older file is a prefix of the newer format and sscanf reads it
// up to where it ends; CONFIG_FIELDS_V* is how many fields that prefix holds,
// config_ver included.
#define CONFIG_VERSION   4
#define CONFIG_FIELDS_V2 9
#define CONFIG_FIELDS_V3 10
#define CONFIG_FIELDS_V4 11

#define CONFIG_BUFFER_SIZE 256

config_t config;
static int config_version_holder = 0;

const char *config_file =
	"config_ver = %d\n"
	"metadata_flac = %d\n"
	"metadata_mp3 = %d\n"
	"metadata_opus = %d\n"
	"sort = %d\n"
	"alc_mode = %d\n"
	"device = %d\n"
	"eq_mode = %d\n"
	"eq_volume = %d\n"
	"language = %d\n"
	"group_end = %d";

static void Config_SetDefaults(void) {
	config.meta_flac = SCE_FALSE;
	config.meta_mp3 = SCE_TRUE;
	config.meta_opus = SCE_TRUE;
	config.sort = 0;
	config.alc_mode = 0;
	config.device = 0;
	config.eq_mode = 0;
	config.eq_volume = SCE_FALSE;
	config.language = LANG_PREF_SYSTEM;
	config.group_end = CONFIG_GROUP_END_REPEAT;
}

int Config_Save(config_t config) {
	int ret = 0;

	char *buf = malloc(CONFIG_BUFFER_SIZE);
	int len = snprintf(buf, CONFIG_BUFFER_SIZE, config_file, CONFIG_VERSION, config.meta_flac, config.meta_mp3, config.meta_opus, config.sort,
		config.alc_mode, config.device, config.eq_mode, config.eq_volume, config.language,
		config.group_end);

	// A truncated file would lose its last lines and, with them, the settings
	// they hold: better to keep the previous file than to write half of this one.
	if (len < 0 || len >= CONFIG_BUFFER_SIZE) {
		free(buf);
		return -1;
	}

	if (R_FAILED(ret = FS_WriteFile("ux0:data/ElevenMPV/config.cfg", buf, len))) {
		free(buf);
		return ret;
	}

	free(buf);
	return 0;
}

int Config_Load(void) {
	int ret = 0;

	if (!FS_FileExists("ux0:data/ElevenMPV/config.cfg")) {
		Config_SetDefaults();
		return Config_Save(config);
	}

	SceOff size = 0;
	FS_GetFileSize("ux0:data/ElevenMPV/config.cfg", &size);
	char *buf = malloc(size + 1);

	if (R_FAILED(ret = FS_ReadFile("ux0:data/ElevenMPV/config.cfg", buf, size))) {
		free(buf);
		return ret;
	}

	buf[size] = '\0';
	int fields = sscanf(buf, config_file, &config_version_holder, &config.meta_flac, &config.meta_mp3, &config.meta_opus, &config.sort,
		&config.alc_mode, &config.device, &config.eq_mode, &config.eq_volume, &config.language,
		&config.group_end);
	free(buf);

	if (config_version_holder >= CONFIG_VERSION && fields >= CONFIG_FIELDS_V4)
		return 0;

	// v3 has everything but group_end: keep what it has and write it back as v4.
	if (config_version_holder == 3 && fields >= CONFIG_FIELDS_V3) {
		config.group_end = CONFIG_GROUP_END_REPEAT;
		return Config_Save(config);
	}

	// v2 also lacks the language. Updating used to reset every setting here.
	if (config_version_holder == 2 && fields >= CONFIG_FIELDS_V2) {
		config.language = LANG_PREF_SYSTEM;
		config.group_end = CONFIG_GROUP_END_REPEAT;
		return Config_Save(config);
	}

	// Older than v2, or not the fields its version promises (a file edited by
	// hand, say): nothing half-read is kept.
	sceIoRemove("ux0:data/ElevenMPV/config.cfg");
	Config_SetDefaults();
	return Config_Save(config);
}

int Config_GetLastDirectory(void) {
	int ret = 0;
	const char *root_paths[] = {
		"ux0:/",
		"ur0:/",
		"uma0:/"
	};

	if (!FS_FileExists("ux0:data/ElevenMPV/lastdir.txt")) {
		snprintf(root_path, 8, "ux0:/");
		FS_WriteFile("ux0:data/ElevenMPV/lastdir.txt", root_path, strlen(root_path) + 1);
		strcpy(cwd, root_path); // Set Start Path to "sdmc:/" if lastDir.txt hasn't been created.
	}
	else {
		strcpy(root_path, root_paths[config.device]);
		SceOff size = 0;

		FS_GetFileSize("ux0:data/ElevenMPV/lastdir.txt", &size);
		char *buf = malloc(size + 1);

		if (R_FAILED(ret = FS_ReadFile("ux0:data/ElevenMPV/lastdir.txt", buf, size))) {
			free(buf);
			return ret;
		}

		buf[size] = '\0';
		char path[512];
		sscanf(buf, "%[^\n]s", path);

		if (FS_DirExists(path)) // Incase a directory previously visited had been deleted, set start path to sdmc:/ to avoid errors.
			strcpy(cwd, path);
		else
			strcpy(cwd, root_path);

		free(buf);
	}

	return 0;
}
