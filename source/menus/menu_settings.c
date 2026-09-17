#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_audioplayer.h"
#include "menu_displayfiles.h"
#include "menu_settings.h"
#include "nav_rail.h"
#include "status_bar.h"
#include "textures.h"
#include "touch.h"
#include "ui_theme.h"
#include "utils.h"
#include "vitaaudiolib.h"

#define CAT_COL_X    (UI_RAIL_WIDTH)
#define CAT_COL_W    252
#define DETAIL_X     (CAT_COL_X + CAT_COL_W)
#define HEADER_H     60
#define CAT_ROW_H    44
#define ITEM_ROW_H   46

typedef enum {
	SETTINGS_ITEM_RADIO,
	SETTINGS_ITEM_TOGGLE
} SettingsItemKind;

typedef struct {
	const char *label;
	int item_count;
	int divider_before; // index a divider is drawn above, or -1 for none
	void (*get_hint)(char *buf, int size);
	const char *(*item_label)(int index);
	SettingsItemKind (*item_kind)(int index);
	SceBool (*item_active)(int index);
	void (*item_activate)(int index);
} SettingsCategory;

// ---- Almacenamiento (device) ----
static const char *device_items[] = { "ux0:/", "ur0:/", "uma0:/" };
static void device_hint(char *buf, int size) { snprintf(buf, size, "%s", device_items[config.device]); }
static const char *device_item_label(int i) { return device_items[i]; }
static SettingsItemKind device_item_kind(int i) { (void)i; return SETTINGS_ITEM_RADIO; }
static SceBool device_item_active(int i) { return config.device == i; }
static void device_item_activate(int i) {
	if (FS_DirExists(device_items[i])) {
		config.device = i;
		Config_Save(config);
		strcpy(root_path, device_items[config.device]);
		strcpy(cwd, root_path);
		sceIoRemove("ux0:data/ElevenMPV/lastdir.txt");
		Dirbrowse_PopulateFiles(SCE_TRUE);
	}
}

// ---- Orden (sort) ----
static const char *sort_items[] = { "Nombre (A-Z)", "Nombre (Z-A)", "Tamaño (mayor primero)", "Tamaño (menor primero)" };
static const char *sort_hints[] = { "A-Z", "Z-A", "Tam. v", "Tam. ^" };
static void sort_hint(char *buf, int size) { snprintf(buf, size, "%s", sort_hints[config.sort]); }
static const char *sort_item_label(int i) { return sort_items[i]; }
static SettingsItemKind sort_item_kind(int i) { (void)i; return SETTINGS_ITEM_RADIO; }
static SceBool sort_item_active(int i) { return config.sort == i; }
static void sort_item_activate(int i) { config.sort = i; Config_Save(config); Dirbrowse_PopulateFiles(SCE_TRUE); }

// ---- Metadatos ----
static const char *meta_items[] = { "Metadatos FLAC", "Metadatos MP3", "Metadatos OPUS" };
static void meta_hint(char *buf, int size) {
	int enabled = (config.meta_flac ? 1 : 0) + (config.meta_mp3 ? 1 : 0) + (config.meta_opus ? 1 : 0);
	snprintf(buf, size, "%d/3", enabled);
}
static const char *meta_item_label(int i) { return meta_items[i]; }
static SettingsItemKind meta_item_kind(int i) { (void)i; return SETTINGS_ITEM_TOGGLE; }
static SceBool meta_item_active(int i) {
	switch (i) {
		case 0: return config.meta_flac;
		case 1: return config.meta_mp3;
		case 2: return config.meta_opus;
	}
	return SCE_FALSE;
}
static void meta_item_activate(int i) {
	switch (i) {
		case 0: config.meta_flac = !config.meta_flac; break;
		case 1: config.meta_mp3 = !config.meta_mp3; break;
		case 2: config.meta_opus = !config.meta_opus; break;
	}
	Config_Save(config);
}

// ---- Normalizador (ALC) ----
static const char *alc_items[] = { "Normalizador desactivado", "Normalizador activado" };
static void alc_hint(char *buf, int size) { snprintf(buf, size, "%s", config.alc_mode == 0 ? "Off" : "On"); }
static const char *alc_item_label(int i) { return alc_items[i]; }
static SettingsItemKind alc_item_kind(int i) { (void)i; return SETTINGS_ITEM_RADIO; }
static SceBool alc_item_active(int i) { return config.alc_mode == i; }
static void alc_item_activate(int i) { config.alc_mode = i; Config_Save(config); Dirbrowse_PopulateFiles(SCE_TRUE); }

// ---- Ecualizador ----
static const char *eq_items[] = { "Apagado", "Heavy", "Pop", "Jazz", "Unique", "Limitar volumen con EQ" };
static void eq_hint(char *buf, int size) { snprintf(buf, size, "%s", eq_items[config.eq_mode]); }
static const char *eq_item_label(int i) { return eq_items[i]; }
static SettingsItemKind eq_item_kind(int i) { return i == 5 ? SETTINGS_ITEM_TOGGLE : SETTINGS_ITEM_RADIO; }
static SceBool eq_item_active(int i) { return i == 5 ? config.eq_volume : (config.eq_mode == i); }
static void eq_item_activate(int i) {
	if (i == 5) {
		config.eq_volume = !config.eq_volume;
		Config_Save(config);
	}
	else {
		config.eq_mode = i;
		sceAudioOutSetEffectType(config.eq_mode);
		Config_Save(config);
	}
}

static const SettingsCategory categories[] = {
	{ "Almacenamiento", 3, -1, device_hint, device_item_label, device_item_kind, device_item_active, device_item_activate },
	{ "Orden", 4, -1, sort_hint, sort_item_label, sort_item_kind, sort_item_active, sort_item_activate },
	{ "Metadatos", 3, -1, meta_hint, meta_item_label, meta_item_kind, meta_item_active, meta_item_activate },
	{ "Normalizador", 2, -1, alc_hint, alc_item_label, alc_item_kind, alc_item_active, alc_item_activate },
	{ "Ecualizador", 6, 5, eq_hint, eq_item_label, eq_item_kind, eq_item_active, eq_item_activate },
};
#define CATEGORY_COUNT (sizeof(categories) / sizeof(categories[0]))

static void SettingsUI_DrawRadio(float cx, float cy, SceBool active) {
	if (active) {
		vita2d_draw_fill_circle(cx, cy, 9.0f, UI_COLOR_ACCENT);
		vita2d_draw_fill_circle(cx, cy, 6.6f, UI_COLOR_BG);
		vita2d_draw_fill_circle(cx, cy, 4.5f, UI_COLOR_ACCENT);
	}
	else {
		vita2d_draw_fill_circle(cx, cy, 9.0f, RGBA8(0x4B, 0x45, 0x60, 255));
		vita2d_draw_fill_circle(cx, cy, 7.4f, UI_COLOR_BG);
	}
}

static void Menu_DrawSettingsCategoryColumn(int category_index) {
	vita2d_draw_rectangle(CAT_COL_X + CAT_COL_W - 1, 0, 1, 544, UI_COLOR_HAIRLINE);
	vita2d_draw_rectangle(CAT_COL_X, HEADER_H - 1, CAT_COL_W, 1, UI_COLOR_HAIRLINE);

	vita2d_font_draw_text(font_ui, CAT_COL_X + 18, UI_TextBaselineY(font_ui, UI_FONT_SIZE_TITLE, "Ajustes", 0, HEADER_H),
		UI_COLOR_TEXT_PRIMARY, UI_FONT_SIZE_TITLE, "Ajustes");

	float y = HEADER_H + 10;

	for (int i = 0; i < (int)CATEGORY_COUNT; i++) {
		SceBool is_active = (i == category_index);
		float row_x = CAT_COL_X + 10, row_w = CAT_COL_W - 20;

		if (is_active)
			UI_DrawRoundedRect(row_x, y, row_w, CAT_ROW_H, 12, UI_COLOR_ACCENT_WASH);

		unsigned int text_color = is_active ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_SECONDARY;
		vita2d_font_draw_text(font_ui, row_x + 12, UI_TextBaselineY(font_ui, UI_FONT_SIZE_BODY, categories[i].label, y, CAT_ROW_H),
			text_color, UI_FONT_SIZE_BODY, categories[i].label);

		char hint[32];
		categories[i].get_hint(hint, sizeof(hint));
		int hint_w = vita2d_font_text_width(font_mono, UI_FONT_SIZE_LABEL_SMALL, hint);
		vita2d_font_draw_text(font_mono, row_x + row_w - 12 - hint_w,
			UI_TextBaselineY(font_mono, UI_FONT_SIZE_LABEL_SMALL, hint, y, CAT_ROW_H),
			is_active ? UI_COLOR_ACCENT : UI_COLOR_TEXT_MUTED, UI_FONT_SIZE_LABEL_SMALL, hint);

		y += CAT_ROW_H;
	}
}

static void Menu_DrawSettingsDetail(int category_index, int item_index) {
	const SettingsCategory *cat = &categories[category_index];

	vita2d_draw_rectangle(DETAIL_X, HEADER_H - 1, 960 - DETAIL_X, 1, UI_COLOR_HAIRLINE);
	vita2d_font_draw_text(font_ui, DETAIL_X + 26, UI_TextBaselineY(font_ui, UI_FONT_SIZE_TITLE_LARGE, cat->label, 0, HEADER_H),
		UI_COLOR_TEXT_PRIMARY, UI_FONT_SIZE_TITLE_LARGE, cat->label);

	float y = HEADER_H + 18;

	for (int i = 0; i < cat->item_count; i++) {
		if (cat->divider_before == i) {
			vita2d_draw_rectangle(DETAIL_X + 26, y + 8, 960 - DETAIL_X - 52, 1, UI_COLOR_HAIRLINE);
			y += 22;
		}

		SceBool row_selected = (i == item_index);
		if (row_selected)
			UI_DrawRoundedRect(DETAIL_X + 12, y, 960 - DETAIL_X - 24, ITEM_ROW_H, 12, UI_COLOR_ACCENT_WASH);

		const char *label = cat->item_label(i);
		SettingsItemKind kind = cat->item_kind(i);
		SceBool active = cat->item_active(i);
		unsigned int text_color = row_selected ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_SECONDARY;

		if (kind == SETTINGS_ITEM_RADIO) {
			SettingsUI_DrawRadio(DETAIL_X + 26 + 9, y + ITEM_ROW_H / 2, active);
			vita2d_font_draw_text(font_ui, DETAIL_X + 26 + 30, UI_TextBaselineY(font_ui, UI_FONT_SIZE_BODY, label, y, ITEM_ROW_H),
				text_color, UI_FONT_SIZE_BODY, label);
		}
		else {
			vita2d_font_draw_text(font_ui, DETAIL_X + 26, UI_TextBaselineY(font_ui, UI_FONT_SIZE_BODY, label, y, ITEM_ROW_H),
				text_color, UI_FONT_SIZE_BODY, label);

			vita2d_texture *toggle_tex = active ? toggle_on : toggle_off;
			float toggle_x = 960 - 26 - vita2d_texture_get_width(toggle_tex);
			float toggle_y = y + (ITEM_ROW_H - vita2d_texture_get_height(toggle_tex)) / 2;
			vita2d_draw_texture(toggle_tex, toggle_x, toggle_y);
		}

		y += ITEM_ROW_H;
	}
}

void Menu_DisplaySettings(void) {
	int category_index = 0, item_index = 0;

	vita2d_set_clear_color(UI_COLOR_BG);

	while (SCE_TRUE) {
		const SettingsCategory *cat = &categories[category_index];

		vita2d_start_drawing();
		vita2d_clear_screen();

		Menu_DrawSettingsCategoryColumn(category_index);
		Menu_DrawSettingsDetail(category_index, item_index);

		const char *hints[] = { "Seleccionar", "Atrás", NULL, "L . R - cambiar de categoria" };
		NavRail_DrawHintBar(544 - UI_HINT_BAR_HEIGHT, hints, 4);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_SETTINGS);

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();

		if (tapped == UI_SCREEN_FOLDERS) {
			Menu_DisplayFiles();
			return;
		}
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack()) {
			Menu_ShowNowPlaying();
			return;
		}

		if (pressed & SCE_CTRL_CANCEL) {
			Menu_DisplayFiles();
			return;
		}

		if (pressed & SCE_CTRL_LTRIGGER) {
			category_index--;
			item_index = 0;
		}
		else if (pressed & SCE_CTRL_RTRIGGER) {
			category_index++;
			item_index = 0;
		}
		Utils_SetMax(&category_index, 0, (int)CATEGORY_COUNT - 1);
		Utils_SetMin(&category_index, (int)CATEGORY_COUNT - 1, 0);

		if (pressed & SCE_CTRL_UP)
			item_index--;
		else if (pressed & SCE_CTRL_DOWN)
			item_index++;
		Utils_SetMax(&item_index, 0, cat->item_count - 1);
		Utils_SetMin(&item_index, cat->item_count - 1, 0);

		if (pressed & SCE_CTRL_ENTER)
			cat->item_activate(item_index);
	}
}
