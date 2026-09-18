#include <psp2/ime_dialog.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "dirbrowse.h"
#include "menu_audioplayer.h"
#include "menu_settings.h"
#include "nav_rail.h"
#include "status_bar.h"
#include "touch.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

#define CONTENT_X     (UI_RAIL_WIDTH)
#define TOPBAR_H      64
#define FILTER_W      240
#define FILTER_H      40
#define MINI_PLAYER_H 72
// Glyph boxes for the mini-player transport controls.
#define MINI_GLYPH_SIZE      24
#define MINI_PLAY_GLYPH_SIZE 18
#define MINI_PLAY_R          22

static float Menu_FilterBoxX(void) { return 960 - 22 - FILTER_W; }
static float Menu_FilterBoxY(void) { return (TOPBAR_H - FILTER_H) / 2.0f; }

static void Menu_PromptFilter(void) {
	SceWChar16 title[] = u"Buscar en esta carpeta";
	SceWChar16 initial[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
	SceWChar16 input[SCE_IME_DIALOG_MAX_TEXT_LENGTH];

	const char *current = Dirbrowse_GetFilter();
	int i = 0;
	for (; current[i] != '\0' && i < SCE_IME_DIALOG_MAX_TEXT_LENGTH - 1; i++)
		initial[i] = (SceWChar16)(unsigned char)current[i];
	initial[i] = 0;
	memcpy(input, initial, sizeof(initial));

	SceImeDialogParam param;
	sceImeDialogParamInit(&param);
	param.dialogMode = SCE_IME_DIALOG_DIALOG_MODE_WITH_CANCEL;
	param.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_WITH_CLEAR;
	param.title = title;
	param.maxTextLength = 63;
	param.initialText = initial;
	param.inputTextBuffer = input;

	if (sceImeDialogInit(&param) < 0)
		return;

	while (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) {
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_common_dialog_update();
		vita2d_end_drawing();
		vita2d_swap_buffers();
	}

	if (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_FINISHED) {
		SceImeDialogResult result;
		memset(&result, 0, sizeof(result));
		sceImeDialogGetResult(&result);

		if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
			char narrow[64];
			int j = 0;
			for (; input[j] != 0 && j < 63; j++)
				narrow[j] = (char)input[j];
			narrow[j] = '\0';

			if (narrow[0] != '\0')
				Dirbrowse_SetFilter(narrow);
			else
				Dirbrowse_ClearFilter();
		}
	}

	sceImeDialogTerm();
}

static void Menu_DrawTopBar(void) {
	vita2d_draw_rectangle(CONTENT_X, TOPBAR_H - 1, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);

	const char *device_label = root_path;
	float chip_pad = 12.0f;
	int chip_text_w = UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, device_label);
	float chip_x = CONTENT_X + 22, chip_h = 38, chip_y = (TOPBAR_H - chip_h) / 2, chip_w = chip_text_w + chip_pad * 2;

	UI_DrawRoundedRect(chip_x, chip_y, chip_w, chip_h, 10, UI_COLOR_SURFACE);
	UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, chip_x + chip_pad, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, chip_y, chip_h), UI_COLOR_TRACKER, device_label);

	const char *relative = cwd + strlen(root_path);
	if (relative[0] != '\0')
		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, chip_x + chip_w + 14, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, 0, TOPBAR_H), Menu_FilterBoxX() - 16 - (chip_x + chip_w + 14), UI_COLOR_TEXT_SECONDARY, relative);

	float fx = Menu_FilterBoxX(), fy = Menu_FilterBoxY();
	UI_DrawPill(fx, fy, FILTER_W, FILTER_H, UI_COLOR_SURFACE);

	const char *filter_text = Dirbrowse_HasFilter() ? Dirbrowse_GetFilter() : "Buscar en esta carpeta";
	unsigned int filter_color = Dirbrowse_HasFilter() ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_MUTED;
	UI_DrawText(UI_FACE_UI, UI_TS_LABEL, fx + 14, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, fy, FILTER_H), filter_color, filter_text);
}

static float Menu_MiniPlayerY(void) { return 544 - UI_HINT_BAR_HEIGHT - MINI_PLAYER_H; }

// One source for the mini-player transport, used by both the drawing and the
// hit testing so the active areas cannot drift away from the drawn controls.
// Centres, not left edges: the three controls sit on a UI_TOUCH_MIN pitch, so
// each active area is already the minimum size and none of them overlaps its
// neighbour. The drawn glyphs are smaller and centred inside their area.
typedef struct {
	float prev_cx, play_cx, next_cx, cy;
} Menu_MiniTransport;

#define MINI_TRANSPORT_GAP 8

static Menu_MiniTransport Menu_MiniTransportGeometry(void) {
	Menu_MiniTransport t;

	t.next_cx = 960 - 22 - UI_TOUCH_MIN / 2.0f;
	t.play_cx = t.next_cx - UI_TOUCH_MIN - MINI_TRANSPORT_GAP;
	t.prev_cx = t.play_cx - UI_TOUCH_MIN - MINI_TRANSPORT_GAP;
	t.cy = Menu_MiniPlayerY() + MINI_PLAYER_H / 2.0f;

	return t;
}

// Where the title and artist have to stop so they never reach the controls.
static float Menu_MiniTextRight(void) {
	return Menu_MiniTransportGeometry().prev_cx - UI_TOUCH_MIN / 2.0f - 18;
}

static void Menu_DrawMiniPlayer(void) {
	if (!Audio_HasTrack())
		return;

	float y = Menu_MiniPlayerY();
	vita2d_draw_rectangle(CONTENT_X, y, 960 - CONTENT_X, MINI_PLAYER_H, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(CONTENT_X, y, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);

	float cover_size = 46, cover_x = CONTENT_X + 22, cover_y = y + (MINI_PLAYER_H - cover_size) / 2;
	if ((metadata.has_meta) && (metadata.cover_image))
		vita2d_draw_texture_scale(metadata.cover_image, cover_x, cover_y,
			cover_size / vita2d_texture_get_width(metadata.cover_image), cover_size / vita2d_texture_get_height(metadata.cover_image));
	else
		UI_DrawRoundedRect(cover_x, cover_y, cover_size, cover_size, 12, UI_COLOR_LOSSLESS);

	const char *title = Music_GetDisplayTitle();
	const char *artist = Music_GetDisplayArtist();
	float text_x = cover_x + cover_size + 14;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, text_x, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y + 10, 22), Menu_MiniTextRight() - text_x, UI_COLOR_TEXT_PRIMARY, title);
	if (artist[0] != '\0')
		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, text_x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y + 36, 20), Menu_MiniTextRight() - text_x, UI_COLOR_TEXT_SECONDARY, artist);

	Menu_MiniTransport t = Menu_MiniTransportGeometry();

	UI_DrawSkipGlyph(t.prev_cx, t.cy, MINI_GLYPH_SIZE, SCE_FALSE, UI_COLOR_TEXT_SECONDARY);

	UI_DrawRoundedRect(t.play_cx - MINI_PLAY_R, t.cy - MINI_PLAY_R, MINI_PLAY_R * 2, MINI_PLAY_R * 2, MINI_PLAY_R, ui_color_accent);
	if (Audio_IsPaused())
		UI_DrawPlayGlyph(t.play_cx, t.cy, MINI_PLAY_GLYPH_SIZE, UI_COLOR_TEXT_PRIMARY);
	else
		UI_DrawPauseGlyph(t.play_cx, t.cy, MINI_PLAY_GLYPH_SIZE, UI_COLOR_TEXT_PRIMARY);

	UI_DrawSkipGlyph(t.next_cx, t.cy, MINI_GLYPH_SIZE, SCE_TRUE, UI_COLOR_TEXT_SECONDARY);
}

static SceBool Menu_HandleMiniPlayerTouch(void) {
	if (!Audio_HasTrack())
		return SCE_FALSE;

	Menu_MiniTransport t = Menu_MiniTransportGeometry();

	// Same geometry the drawing used, grown to the minimum touch size. The two
	// used to be written out twice and could drift apart.
	if (UI_TouchTarget(t.play_cx - UI_TOUCH_MIN / 2.0f, t.cy - UI_TOUCH_MIN / 2.0f, UI_TOUCH_MIN, UI_TOUCH_MIN)) {
		Music_TogglePlayPause();
		return SCE_TRUE;
	}
	if (UI_TouchTarget(t.prev_cx - UI_TOUCH_MIN / 2.0f, t.cy - UI_TOUCH_MIN / 2.0f, UI_TOUCH_MIN, UI_TOUCH_MIN)) {
		Music_Previous();
		return SCE_TRUE;
	}
	if (UI_TouchTarget(t.next_cx - UI_TOUCH_MIN / 2.0f, t.cy - UI_TOUCH_MIN / 2.0f, UI_TOUCH_MIN, UI_TOUCH_MIN)) {
		Music_Next();
		return SCE_TRUE;
	}

	return SCE_FALSE;
}

static void Menu_HandleControls(void) {
	int visible_count = Dirbrowse_GetVisibleCount();

	if (visible_count > 0) {
		if (pressed & SCE_CTRL_UP)
			position--;
		else if (pressed & SCE_CTRL_DOWN)
			position++;

		Utils_SetMax(&position, 0, visible_count - 1);
		Utils_SetMin(&position, visible_count - 1, 0);

		if (pressed & SCE_CTRL_LEFT)
			position = 0;
		else if (pressed & SCE_CTRL_RIGHT)
			position = visible_count - 1;

		if (pressed & SCE_CTRL_ENTER)
			Dirbrowse_OpenFile();
	}

	if ((strcmp(cwd, root_path) != 0) && (pressed & SCE_CTRL_CANCEL)) {
		Dirbrowse_Navigate(SCE_TRUE);
		Dirbrowse_PopulateFiles(SCE_TRUE);
	}
}

void Menu_DisplayFiles(void) {
	Dirbrowse_PopulateFiles(SCE_FALSE);
	vita2d_set_clear_color(UI_COLOR_BG);

	while (SCE_TRUE) {
		vita2d_start_drawing();
		vita2d_clear_screen();

		StatusBar_Display();
		Menu_DrawTopBar();
		Dirbrowse_DisplayFiles();
		Menu_DrawMiniPlayer();

		const char *hints[] = { "Abrir / Reproducir", "Carpeta superior", NULL, NULL, "SELECT - Ajustes", "START - Salir" };
		NavRail_DrawHintBar(544 - UI_HINT_BAR_HEIGHT, hints, 6);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_FOLDERS);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		if (tapped == UI_SCREEN_SETTINGS) {
			Menu_DisplaySettings();
			return;
		}
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack()) {
			Menu_ShowNowPlaying();
			return;
		}

		if (Menu_HandleMiniPlayerTouch())
			continue;

		if (Touch_Position(Menu_FilterBoxX(), Menu_FilterBoxY(), Menu_FilterBoxX() + FILTER_W, Menu_FilterBoxY() + FILTER_H)) {
			Menu_PromptFilter();
			continue;
		}

		Menu_HandleControls();

		if (pressed & SCE_CTRL_SELECT) {
			Menu_DisplaySettings();
			return;
		}

		if (pressed & SCE_CTRL_START)
			break;
	}
}
