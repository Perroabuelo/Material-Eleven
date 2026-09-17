#include <math.h>

#include "common.h"
#include "nav_rail.h"
#include "touch.h"
#include "ui_theme.h"

#define RAIL_BUTTON_SIZE      44
#define RAIL_BUTTON_RADIUS    16
#define RAIL_SETTINGS_SIZE    40
#define RAIL_TOP_PAD          18
#define RAIL_GAP              22
// Drawn at its final size - no 28px texture rescaled down to 19px any more.
#define RAIL_ICON_SIZE        22
#define RAIL_GEAR_TEETH       8

typedef struct {
	UI_Screen screen;
	float x, y, size;
} NavRail_Button;

static void NavRail_GetButtons(NavRail_Button out[3]) {
	float x = (UI_RAIL_WIDTH - RAIL_BUTTON_SIZE) / 2.0f;

	out[0] = (NavRail_Button){ UI_SCREEN_NOW_PLAYING, x, RAIL_TOP_PAD, RAIL_BUTTON_SIZE };
	out[1] = (NavRail_Button){ UI_SCREEN_FOLDERS, x, RAIL_TOP_PAD + RAIL_BUTTON_SIZE + RAIL_GAP, RAIL_BUTTON_SIZE };

	float settings_x = (UI_RAIL_WIDTH - RAIL_SETTINGS_SIZE) / 2.0f;
	out[2] = (NavRail_Button){ UI_SCREEN_SETTINGS, settings_x, 544 - RAIL_TOP_PAD - RAIL_SETTINGS_SIZE, RAIL_SETTINGS_SIZE };
}

// Now Playing: filled play triangle, nudged right so it reads optically centered.
static void NavRail_DrawNowPlayingGlyph(float cx, float cy, float size, unsigned int color) {
	float h = size * 0.88f, w = size * 0.76f;
	float left = cx - w * 0.38f;

	UI_DrawTriangle(left, cy - h / 2.0f, left + w, cy, left, cy + h / 2.0f, color);
}

// Folders: outlined folder with a tab on the left.
static void NavRail_DrawFoldersGlyph(float cx, float cy, float size, unsigned int color) {
	float x = cx - size / 2.0f, y = cy - size / 2.0f;
	float t = size * 0.10f;
	float l = x + size * 0.08f, r = x + size * 0.92f;
	float top = y + size * 0.24f, fold = y + size * 0.36f, bottom = y + size * 0.78f;

	UI_DrawStroke(l, top, x + size * 0.40f, top, t, color);
	UI_DrawStroke(x + size * 0.40f, top, x + size * 0.50f, fold, t, color);
	UI_DrawStroke(x + size * 0.50f, fold, r, fold, t, color);
	UI_DrawStroke(r, fold, r, bottom, t, color);
	UI_DrawStroke(r, bottom, l, bottom, t, color);
	UI_DrawStroke(l, bottom, l, top, t, color);
}

// Settings: cog - an open ring plus radial teeth, so it stays correct over both
// the plain rail background and the accent wash of the active button.
static void NavRail_DrawSettingsGlyph(float cx, float cy, float size, unsigned int color) {
	float tooth_inner = size * 0.30f, tooth_outer = size * 0.50f, tooth_half_w = size * 0.085f;

	UI_DrawRing(cx, cy, size * 0.375f, size * 0.175f, color);

	for (int i = 0; i < RAIL_GEAR_TEETH; i++) {
		float a = (2.0f * UI_PI * (float)i) / (float)RAIL_GEAR_TEETH;
		float c = cosf(a), s = sinf(a);
		// Perpendicular to the tooth axis.
		float pc = -s * tooth_half_w, ps = c * tooth_half_w;

		UI_DrawQuad(cx + c * tooth_inner + pc, cy + s * tooth_inner + ps,
			cx + c * tooth_outer + pc, cy + s * tooth_outer + ps,
			cx + c * tooth_outer - pc, cy + s * tooth_outer - ps,
			cx + c * tooth_inner - pc, cy + s * tooth_inner - ps, color);
	}
}

static void NavRail_DrawIcon(UI_Screen screen, float cx, float cy, unsigned int color) {
	switch (screen) {
		case UI_SCREEN_NOW_PLAYING: NavRail_DrawNowPlayingGlyph(cx, cy, RAIL_ICON_SIZE, color); break;
		case UI_SCREEN_FOLDERS: NavRail_DrawFoldersGlyph(cx, cy, RAIL_ICON_SIZE, color); break;
		case UI_SCREEN_SETTINGS: NavRail_DrawSettingsGlyph(cx, cy, RAIL_ICON_SIZE, color); break;
		default: break;
	}
}

UI_Screen NavRail_DrawAndHitTest(UI_Screen active) {
	NavRail_Button buttons[3];
	NavRail_GetButtons(buttons);

	vita2d_draw_rectangle(0, 0, UI_RAIL_WIDTH, 544, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(UI_RAIL_WIDTH - 1, 0, 1, 544, UI_COLOR_HAIRLINE);

	UI_Screen tapped = UI_SCREEN_NONE;

	for (int i = 0; i < 3; i++) {
		NavRail_Button *b = &buttons[i];
		SceBool is_active = (b->screen == active);

		if (is_active)
			UI_DrawRoundedRect(b->x, b->y, b->size, b->size, RAIL_BUTTON_RADIUS, ui_color_accent_wash);

		NavRail_DrawIcon(b->screen, b->x + b->size / 2.0f, b->y + b->size / 2.0f,
			is_active ? ui_color_accent : UI_COLOR_TEXT_TERTIARY);

		if (Touch_Position(b->x, b->y, b->x + b->size, b->y + b->size))
			tapped = b->screen;
	}

	return tapped;
}

void NavRail_DrawHintBar(float y, const char **segments, int count) {
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, UI_HINT_BAR_HEIGHT, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, 1, UI_COLOR_HAIRLINE);

	float x = UI_RAIL_WIDTH + 22;
	float baseline = UI_TextBaselineY(font_mono, UI_FONT_SIZE_HINT, "Ag", y, UI_HINT_BAR_HEIGHT);

	for (int i = 0; i < count; i++) {
		if (!segments[i])
			continue;

		vita2d_font_draw_text(font_mono, x, baseline, UI_COLOR_TEXT_SECONDARY, UI_FONT_SIZE_HINT, segments[i]);
		x += vita2d_font_text_width(font_mono, UI_FONT_SIZE_HINT, segments[i]) + 26;
	}
}
