/*
 * The audio settings menu (EQ preset and limiter toggle) is derived from
 * ElevenMPV-A (https://github.com/GrapheneCt/ElevenMPV-A), (C) 2020-2022
 * GrapheneCt and contributors, GPL-3.0-or-later. See NOTICE.
 */

#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "lang.h"
#include "menu_audioplayer.h"
#include "menu_displayfiles.h"
#include "menu_library.h"
#include "menu_settings.h"
#include "nav_rail.h"
#include "status_bar.h"
#include "touch.h"
#include "touch_list.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"
#include "vitaaudiolib.h"

#define CAT_COL_X    (UI_RAIL_WIDTH)
#define CAT_COL_W    286
#define DETAIL_X     (CAT_COL_X + CAT_COL_W)
#define HEADER_H     64
#define CAT_ROW_H    56
#define ITEM_ROW_H   58
#define TOGGLE_W     56
#define TOGGLE_H     30
#define TOGGLE_KNOB_R 12
#define CAT_LIST_TOP (HEADER_H + 10)
// The extra room a divider takes above its item.
#define DIVIDER_GAP  22
// Both lists end at the hint bar: Settings has no mini player.
#define LIST_BOTTOM  (544 - UI_HINT_BAR_HEIGHT)

typedef enum {
	SETTINGS_ITEM_RADIO,
	SETTINGS_ITEM_TOGGLE
} SettingsItemKind;

typedef struct {
	LangString label;
	LangString caption; // drawn above the items, or STR_COUNT for none
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
static const LangString sort_items[] = { STR_SORT_NAME_AZ, STR_SORT_NAME_ZA, STR_SORT_SIZE_DESC, STR_SORT_SIZE_ASC };
static const LangString sort_hints[] = { STR_SORT_HINT_NAME_AZ, STR_SORT_HINT_NAME_ZA, STR_SORT_HINT_SIZE_DESC, STR_SORT_HINT_SIZE_ASC };
static void sort_hint(char *buf, int size) { snprintf(buf, size, "%s", Lang_Get(sort_hints[config.sort])); }
static const char *sort_item_label(int i) { return Lang_Get(sort_items[i]); }
static SettingsItemKind sort_item_kind(int i) { (void)i; return SETTINGS_ITEM_RADIO; }
static SceBool sort_item_active(int i) { return config.sort == i; }
static void sort_item_activate(int i) { config.sort = i; Config_Save(config); Dirbrowse_PopulateFiles(SCE_TRUE); }

// ---- Metadatos ----
static const LangString meta_items[] = { STR_META_FLAC, STR_META_MP3, STR_META_OPUS };
static void meta_hint(char *buf, int size) {
	int enabled = (config.meta_flac ? 1 : 0) + (config.meta_mp3 ? 1 : 0) + (config.meta_opus ? 1 : 0);
	snprintf(buf, size, "%d/3", enabled);
}
static const char *meta_item_label(int i) { return Lang_Get(meta_items[i]); }
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
static const LangString alc_items[] = { STR_ALC_OFF, STR_ALC_ON };
static void alc_hint(char *buf, int size) { snprintf(buf, size, "%s", Lang_Get(config.alc_mode == 0 ? STR_ALC_HINT_OFF : STR_ALC_HINT_ON)); }
static const char *alc_item_label(int i) { return Lang_Get(alc_items[i]); }
static SettingsItemKind alc_item_kind(int i) { (void)i; return SETTINGS_ITEM_RADIO; }
static SceBool alc_item_active(int i) { return config.alc_mode == i; }
static void alc_item_activate(int i) { config.alc_mode = i; Config_Save(config); Dirbrowse_PopulateFiles(SCE_TRUE); }

// ---- Ecualizador ----
// The preset names are proper names and read the same in both languages; only
// "Off" and the volume toggle go through the string table.
static const char *eq_presets[] = { NULL, "Heavy", "Pop", "Jazz", "Unique" };
static const char *eq_item_label(int i) {
	if (i == 0)
		return Lang_Get(STR_EQ_OFF);
	if (i == 5)
		return Lang_Get(STR_EQ_LIMIT_VOLUME);
	return eq_presets[i];
}
static void eq_hint(char *buf, int size) { snprintf(buf, size, "%s", eq_item_label(config.eq_mode)); }
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

// ---- Idioma ----
// "English" and "Español" are written in their own language whatever the active
// one is, so anyone can find their way back to theirs; only "System" is
// translated. The index is the LANG_PREF_* value stored in config.language.
static const char *language_names[] = { NULL, "English", "Español" };
static int language_choice(void) {
	return (config.language == LANG_PREF_ENGLISH || config.language == LANG_PREF_SPANISH) ? config.language : LANG_PREF_SYSTEM;
}
static const char *language_item_label(int i) { return (i == LANG_PREF_SYSTEM) ? Lang_Get(STR_LANG_SYSTEM) : language_names[i]; }
static void language_hint(char *buf, int size) { snprintf(buf, size, "%s", language_item_label(language_choice())); }
static SettingsItemKind language_item_kind(int i) { (void)i; return SETTINGS_ITEM_RADIO; }
// An out-of-range value is not corrected on load: it behaves as System, and so
// it is shown as System.
static SceBool language_item_active(int i) { return language_choice() == i; }
static void language_item_activate(int i) {
	config.language = i;
	Config_Save(config);
	Lang_Apply(config.language);
}

// ---- Reproduccion ----
// What the Library queues when playing from inside an album or an artist. It
// only takes effect on the next play from the Library: the queue already
// playing is not rebuilt.
static const LangString group_end_items[] = { STR_GROUP_END_REPEAT, STR_GROUP_END_NEXT };
static int group_end_choice(void) {
	return (config.group_end == CONFIG_GROUP_END_NEXT) ? CONFIG_GROUP_END_NEXT : CONFIG_GROUP_END_REPEAT;
}
static void group_end_hint(char *buf, int size) {
	snprintf(buf, size, "%s", Lang_Get(group_end_choice() == CONFIG_GROUP_END_NEXT ? STR_GROUP_END_HINT_NEXT : STR_GROUP_END_HINT_REPEAT));
}
static const char *group_end_item_label(int i) { return Lang_Get(group_end_items[i]); }
static SettingsItemKind group_end_item_kind(int i) { (void)i; return SETTINGS_ITEM_RADIO; }
static SceBool group_end_item_active(int i) { return group_end_choice() == i; }
static void group_end_item_activate(int i) { config.group_end = i; Config_Save(config); }

static const SettingsCategory categories[] = {
	{ STR_SETTINGS_STORAGE, STR_COUNT, 3, -1, device_hint, device_item_label, device_item_kind, device_item_active, device_item_activate },
	{ STR_SETTINGS_SORT, STR_COUNT, 4, -1, sort_hint, sort_item_label, sort_item_kind, sort_item_active, sort_item_activate },
	{ STR_SETTINGS_METADATA, STR_COUNT, 3, -1, meta_hint, meta_item_label, meta_item_kind, meta_item_active, meta_item_activate },
	{ STR_SETTINGS_NORMALIZER, STR_COUNT, 2, -1, alc_hint, alc_item_label, alc_item_kind, alc_item_active, alc_item_activate },
	{ STR_SETTINGS_EQUALIZER, STR_COUNT, 6, 5, eq_hint, eq_item_label, eq_item_kind, eq_item_active, eq_item_activate },
	{ STR_SETTINGS_LANGUAGE, STR_COUNT, LANG_PREF_COUNT, -1, language_hint, language_item_label, language_item_kind, language_item_active, language_item_activate },
	{ STR_SETTINGS_PLAYBACK, STR_GROUP_END_TITLE, 2, -1, group_end_hint, group_end_item_label, group_end_item_kind, group_end_item_active, group_end_item_activate },
};
#define CATEGORY_COUNT (sizeof(categories) / sizeof(categories[0]))

// The category column fits whole, so it never scrolls, but it takes taps the
// same way as every list. The item list scrolls only if a category ever has
// more items than fit.
static TouchList category_list;
static TouchList item_list;

// Where item `i` starts in the item list, counting the divider above it; with
// i == item_count, the list's height.
static float Settings_ItemTop(const SettingsCategory *cat, int i) {
	float top = i * ITEM_ROW_H;

	if (cat->divider_before >= 0 && i >= cat->divider_before)
		top += DIVIDER_GAP;

	return top;
}

// The item at `y` in the item list, or -1 (the divider's gap, or past the end).
static int Settings_ItemAt(const SettingsCategory *cat, float y) {
	if (y < 0.0f)
		return -1;

	for (int i = 0; i < cat->item_count; i++) {
		float top = Settings_ItemTop(cat, i);

		if (y >= top && y < top + ITEM_ROW_H)
			return i;
	}

	return -1;
}

// The item list starts under the title, and under the caption if it has one.
static float Settings_ItemListTop(const SettingsCategory *cat) {
	return HEADER_H + 18 + ((cat->caption < STR_COUNT) ? 36 : 0);
}

static void SettingsUI_DrawRadio(float cx, float cy, SceBool active) {
	if (active) {
		vita2d_draw_fill_circle(cx, cy, 11.0f, ui_color_accent);
		vita2d_draw_fill_circle(cx, cy, 8.1f, UI_COLOR_BG);
		vita2d_draw_fill_circle(cx, cy, 5.5f, ui_color_accent);
	}
	else {
		vita2d_draw_fill_circle(cx, cy, 11.0f, RGBA8(0x4B, 0x45, 0x60, 255));
		vita2d_draw_fill_circle(cx, cy, 9.1f, UI_COLOR_BG);
	}
}

// Track via UI_DrawPill (already vector) plus a drawn knob - the toggle PNGs it
// replaces were solid black, which read as an empty pill on the dark surface.
static void SettingsUI_DrawToggle(float x, float y, SceBool active) {
	float knob_cx = active ? (x + TOGGLE_W - TOGGLE_H / 2.0f) : (x + TOGGLE_H / 2.0f);

	UI_DrawPill(x, y, TOGGLE_W, TOGGLE_H, active ? ui_color_accent : UI_COLOR_SURFACE_2);
	vita2d_draw_fill_circle(knob_cx, y + TOGGLE_H / 2.0f, TOGGLE_KNOB_R,
		active ? UI_COLOR_BG : UI_COLOR_TEXT_TERTIARY);
}

static void Menu_DrawSettingsCategoryColumn(int category_index) {
	vita2d_draw_rectangle(CAT_COL_X + CAT_COL_W - 1, 0, 1, 544, UI_COLOR_HAIRLINE);
	vita2d_draw_rectangle(CAT_COL_X, HEADER_H - 1, CAT_COL_W, 1, UI_COLOR_HAIRLINE);

	UI_DrawText(UI_FACE_UI, UI_TS_TITLE, CAT_COL_X + 18, UI_TextBaselineY(UI_FACE_UI, UI_TS_TITLE, 0, HEADER_H), UI_COLOR_TEXT_PRIMARY, Lang_Get(STR_SETTINGS_TITLE));

	TouchList_Layout(&category_list, CAT_COL_X, CAT_LIST_TOP, CAT_COL_X + CAT_COL_W, LIST_BOTTOM,
		(int)CATEGORY_COUNT, CAT_ROW_H, category_index);

	int touched = TouchList_PressedRow(&category_list);
	TouchList_BeginClip(&category_list);

	for (int i = TouchList_FirstRow(&category_list); i < (int)CATEGORY_COUNT; i++) {
		float y = TouchList_RowY(&category_list, i);

		if (y >= LIST_BOTTOM)
			break;

		SceBool is_active = (i == category_index);
		float row_x = CAT_COL_X + 10, row_w = CAT_COL_W - 20;

		if (is_active)
			UI_DrawRoundedRect(row_x, y, row_w, CAT_ROW_H, 14, ui_color_accent_wash);
		else if (i == touched)
			UI_DrawRowPressHighlight(row_x, y, row_w, CAT_ROW_H);

		unsigned int text_color = is_active ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_SECONDARY;
		char hint[32];
		categories[i].get_hint(hint, sizeof(hint));
		int hint_w = UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, hint);

		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, row_x + 14, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, CAT_ROW_H), row_w - 28 - hint_w - 12, text_color, Lang_Get(categories[i].label));
		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, row_x + row_w - 14 - hint_w, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, y, CAT_ROW_H), is_active ? ui_color_accent : UI_COLOR_TEXT_MUTED, hint);
	}

	TouchList_EndClip();
}

static void Menu_DrawSettingsDetail(int category_index, int item_index) {
	const SettingsCategory *cat = &categories[category_index];

	vita2d_draw_rectangle(DETAIL_X, HEADER_H - 1, 960 - DETAIL_X, 1, UI_COLOR_HAIRLINE);
	UI_DrawText(UI_FACE_UI, UI_TS_TITLE, DETAIL_X + 26, UI_TextBaselineY(UI_FACE_UI, UI_TS_TITLE, 0, HEADER_H), UI_COLOR_TEXT_PRIMARY, Lang_Get(cat->label));

	float y = HEADER_H + 18;

	if (cat->caption < STR_COUNT) {
		UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, DETAIL_X + 26, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y, 32), 960 - 26 - (DETAIL_X + 26),
			UI_COLOR_TEXT_TERTIARY, Lang_Get(cat->caption));
		y += 36;
	}

	float list_top = Settings_ItemListTop(cat);
	float show_top = Settings_ItemTop(cat, item_index);

	TouchList_LayoutContent(&item_list, DETAIL_X, list_top, 960, LIST_BOTTOM,
		Settings_ItemTop(cat, cat->item_count), show_top, show_top + ITEM_ROW_H);

	int touched = Settings_ItemAt(cat, TouchList_PressedY(&item_list));
	TouchList_BeginClip(&item_list);

	for (int i = 0; i < cat->item_count; i++) {
		y = TouchList_ContentTop(&item_list) + Settings_ItemTop(cat, i);

		if (y >= LIST_BOTTOM)
			break;

		if (y + ITEM_ROW_H <= list_top)
			continue;

		if (cat->divider_before == i)
			vita2d_draw_rectangle(DETAIL_X + 26, y - DIVIDER_GAP + 8, 960 - DETAIL_X - 52, 1, UI_COLOR_HAIRLINE);

		SceBool row_selected = (i == item_index);
		if (row_selected)
			UI_DrawRoundedRect(DETAIL_X + 12, y, 960 - DETAIL_X - 24, ITEM_ROW_H, 14, ui_color_accent_wash);
		else if (i == touched)
			UI_DrawRowPressHighlight(DETAIL_X + 12, y, 960 - DETAIL_X - 24, ITEM_ROW_H);

		const char *label = cat->item_label(i);
		SettingsItemKind kind = cat->item_kind(i);
		SceBool active = cat->item_active(i);
		unsigned int text_color = row_selected ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_SECONDARY;

		if (kind == SETTINGS_ITEM_RADIO) {
			SettingsUI_DrawRadio(DETAIL_X + 26 + 11, y + ITEM_ROW_H / 2, active);
			UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, DETAIL_X + 26 + 36, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, ITEM_ROW_H), 960 - 26 - (DETAIL_X + 26 + 36), text_color, label);
		}
		else {
			UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, DETAIL_X + 26, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, ITEM_ROW_H), (960 - 26 - TOGGLE_W - 16) - (DETAIL_X + 26), text_color, label);

			SettingsUI_DrawToggle(960 - 26 - TOGGLE_W, y + (ITEM_ROW_H - TOGGLE_H) / 2.0f, active);
		}
	}

	TouchList_EndClip();
}

UI_Screen Menu_DisplaySettings(void) {
	int category_index = 0, item_index = 0;

	vita2d_set_clear_color(UI_COLOR_BG);

	TouchList_Reset(&category_list);
	TouchList_Reset(&item_list);

	while (SCE_TRUE) {
		const SettingsCategory *cat = &categories[category_index];

		UI_GpuBeginFrame();
		vita2d_clear_screen();

		Menu_DrawSettingsCategoryColumn(category_index);
		Menu_DrawSettingsDetail(category_index, item_index);

		const NavRail_Hint hints[] = {
			{ { HINT_BTN_CONFIRM }, 0, Lang_Get(STR_HINT_SELECT) },
			{ { HINT_BTN_CANCEL }, 0, Lang_Get(STR_HINT_BACK) },
			{ { HINT_BTN_L, HINT_BTN_R }, 0, Lang_Get(STR_HINT_SETTINGS_CATEGORY) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};
		NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, 4);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_SETTINGS);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		// Each list takes only the touches that start in it.
		float item_tap_y = -1.0f;
		int category_tap = TouchList_Update(&category_list, SCE_TRUE, NULL);
		TouchList_Update(&item_list, SCE_TRUE, &item_tap_y);
		int item_tap = Settings_ItemAt(cat, item_tap_y);

		if (tapped == UI_SCREEN_FOLDERS || tapped == UI_SCREEN_LIBRARY)
			return tapped;
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack())
			return tapped;

		if (pressed & SCE_CTRL_CANCEL)
			return UI_SCREEN_FOLDERS;

		// Tapping a category does what L and R do: open it on its first item.
		if (category_tap >= 0) {
			category_index = category_tap;
			item_index = 0;
			TouchList_Reset(&item_list);
			continue;
		}

		// Tapping an item selects it and does what Confirm does.
		if (item_tap >= 0) {
			item_index = item_tap;
			cat->item_activate(item_index);
			continue;
		}

		int before_category = category_index, before_item = item_index;

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

		if (category_index != before_category) {
			TouchList_Reset(&item_list);
			TouchList_Follow(&category_list);
		}

		Utils_ListStep(&item_index, cat->item_count, cat->item_count);

		// A direction button stops any glide and brings the selection back
		// into view.
		if (item_index != before_item || (pressed & (SCE_CTRL_UP | SCE_CTRL_DOWN)))
			TouchList_Follow(&item_list);

		if (pressed & SCE_CTRL_ENTER)
			cat->item_activate(item_index);
	}
}
