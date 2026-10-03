#include <math.h>

#include "common.h"
#include "nav_rail.h"
#include "touch.h"
#include "ui_theme.h"

#define RAIL_BUTTON_SIZE      44
#define RAIL_BUTTON_RADIUS    16
#define RAIL_SETTINGS_SIZE    40
#define RAIL_TOP_PAD          18
#define RAIL_GAP              26
// Drawn at its final size - no 28px texture rescaled down to 19px any more.
#define RAIL_ICON_SIZE        22
#define RAIL_GEAR_TEETH       8
#define RAIL_BUTTONS          4

typedef struct {
	UI_Screen screen;
	float x, y, size;
} NavRail_Button;

static void NavRail_GetButtons(NavRail_Button out[RAIL_BUTTONS]) {
	float x = (UI_RAIL_WIDTH - RAIL_BUTTON_SIZE) / 2.0f;
	float pitch = RAIL_BUTTON_SIZE + RAIL_GAP;

	// El paso es 70 px y UI_TOUCH_MIN es 66, asi que las areas tactiles
	// crecidas de dos botones seguidos siguen sin solaparse con cuatro.
	out[0] = (NavRail_Button){ UI_SCREEN_NOW_PLAYING, x, RAIL_TOP_PAD, RAIL_BUTTON_SIZE };
	out[1] = (NavRail_Button){ UI_SCREEN_FOLDERS, x, RAIL_TOP_PAD + pitch, RAIL_BUTTON_SIZE };
	out[2] = (NavRail_Button){ UI_SCREEN_LIBRARY, x, RAIL_TOP_PAD + 2 * pitch, RAIL_BUTTON_SIZE };

	float settings_x = (UI_RAIL_WIDTH - RAIL_SETTINGS_SIZE) / 2.0f;
	out[3] = (NavRail_Button){ UI_SCREEN_SETTINGS, settings_x, 544 - RAIL_TOP_PAD - RAIL_SETTINGS_SIZE, RAIL_SETTINGS_SIZE };
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

// Library: a quaver - filled head, stem and flag. Unmistakable next to the play
// triangle, the folder outline and the cog, which are the three silhouettes it
// has to stay apart from.
static void NavRail_DrawLibraryGlyph(float cx, float cy, float size, unsigned int color) {
	float t = size * 0.11f;
	float head_d = size * 0.36f;
	float head_cx = cx - size * 0.14f, head_cy = cy + size * 0.24f;
	float stem_x = head_cx + head_d / 2.0f - t / 2.0f;
	float stem_top = cy - size * 0.46f;

	// Un pill cuadrado es un circulo: el radio es h/2.
	UI_DrawPill(head_cx - head_d / 2.0f, head_cy - head_d / 2.0f, head_d, head_d, color);
	UI_DrawStroke(stem_x, head_cy, stem_x, stem_top, t, color);
	UI_DrawStroke(stem_x, stem_top, cx + size * 0.38f, stem_top + size * 0.22f, t, color);
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
		case UI_SCREEN_LIBRARY: NavRail_DrawLibraryGlyph(cx, cy, RAIL_ICON_SIZE, color); break;
		case UI_SCREEN_SETTINGS: NavRail_DrawSettingsGlyph(cx, cy, RAIL_ICON_SIZE, color); break;
		default: break;
	}
}

UI_Screen NavRail_DrawAndHitTest(UI_Screen active) {
	NavRail_Button buttons[RAIL_BUTTONS];
	NavRail_GetButtons(buttons);

	vita2d_draw_rectangle(0, 0, UI_RAIL_WIDTH, 544, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(UI_RAIL_WIDTH - 1, 0, 1, 544, UI_COLOR_HAIRLINE);

	UI_Screen tapped = UI_SCREEN_NONE;

	for (int i = 0; i < RAIL_BUTTONS; i++) {
		NavRail_Button *b = &buttons[i];
		SceBool is_active = (b->screen == active);

		if (is_active)
			UI_DrawRoundedRect(b->x, b->y, b->size, b->size, RAIL_BUTTON_RADIUS, ui_color_accent_wash);

		NavRail_DrawIcon(b->screen, b->x + b->size / 2.0f, b->y + b->size / 2.0f,
			is_active ? ui_color_accent : UI_COLOR_TEXT_TERTIARY);

		// The drawn button keeps its size; only what answers a touch grows.
		if (UI_TouchTarget(b->x, b->y, b->size, b->size))
			tapped = b->screen;
	}

	return tapped;
}

void NavRail_DrawHintBar(float y, const char **segments, int count) {
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, UI_HINT_BAR_HEIGHT, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, 1, UI_COLOR_HAIRLINE);

	float x = UI_RAIL_WIDTH + 22;
	float baseline = UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, y, UI_HINT_BAR_HEIGHT);

	for (int i = 0; i < count; i++) {
		if (!segments[i])
			continue;

		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, baseline, UI_COLOR_TEXT_SECONDARY, segments[i]);
		x += UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, segments[i]) + 26;
	}
}

// ---- Legend with button glyphs ----

#define HINT_PAD_X        22
#define HINT_ENTRY_GAP    26
#define HINT_LABEL_GAP    8
#define HINT_BUTTON_GAP   4
#define HINT_GLYPH_BOX    16
#define HINT_GLYPH_STROKE 2.2f
#define HINT_CHIP_PAD_X   6
#define HINT_CHIP_H       20

typedef enum {
	HINT_SYM_CROSS,
	HINT_SYM_CIRCLE,
	HINT_SYM_SQUARE,
	HINT_SYM_TRIANGLE,
	HINT_SYM_CHIP
} NavRail_HintSymbol;

static NavRail_HintSymbol NavRail_HintSymbolOf(NavRail_HintButton button) {
	SceBool cross_confirms = (SCE_CTRL_ENTER == SCE_CTRL_CROSS);

	switch (button) {
		case HINT_BTN_CONFIRM: return cross_confirms ? HINT_SYM_CROSS : HINT_SYM_CIRCLE;
		case HINT_BTN_CANCEL: return cross_confirms ? HINT_SYM_CIRCLE : HINT_SYM_CROSS;
		case HINT_BTN_TRIANGLE: return HINT_SYM_TRIANGLE;
		case HINT_BTN_SQUARE: return HINT_SYM_SQUARE;
		default: return HINT_SYM_CHIP;
	}
}

static const char *NavRail_HintChipName(NavRail_HintButton button) {
	switch (button) {
		case HINT_BTN_L: return "L";
		case HINT_BTN_R: return "R";
		case HINT_BTN_SELECT: return "SELECT";
		case HINT_BTN_START: return "START";
		default: return "";
	}
}

static float NavRail_HintButtonWidth(NavRail_HintButton button) {
	if (NavRail_HintSymbolOf(button) != HINT_SYM_CHIP)
		return HINT_GLYPH_BOX;

	return UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, NavRail_HintChipName(button)) + HINT_CHIP_PAD_X * 2;
}

// The four symbols in outline, as the console's face buttons print them,
// centered on (cx, cy) inside a HINT_GLYPH_BOX square.
static void NavRail_DrawHintSymbol(NavRail_HintSymbol symbol, float cx, float cy) {
	float half = HINT_GLYPH_BOX / 2.0f;

	switch (symbol) {
		case HINT_SYM_CROSS: {
			float r = half * 0.72f;
			UI_DrawStroke(cx - r, cy - r, cx + r, cy + r, HINT_GLYPH_STROKE, UI_COLOR_BTN_CROSS);
			UI_DrawStroke(cx - r, cy + r, cx + r, cy - r, HINT_GLYPH_STROKE, UI_COLOR_BTN_CROSS);
			break;
		}
		case HINT_SYM_CIRCLE:
			UI_DrawRing(cx, cy, half * 0.9f, HINT_GLYPH_STROKE, UI_COLOR_BTN_CIRCLE);
			break;
		case HINT_SYM_SQUARE: {
			float s = half * 0.74f;
			UI_DrawStroke(cx - s, cy - s, cx + s, cy - s, HINT_GLYPH_STROKE, UI_COLOR_BTN_SQUARE);
			UI_DrawStroke(cx + s, cy - s, cx + s, cy + s, HINT_GLYPH_STROKE, UI_COLOR_BTN_SQUARE);
			UI_DrawStroke(cx + s, cy + s, cx - s, cy + s, HINT_GLYPH_STROKE, UI_COLOR_BTN_SQUARE);
			UI_DrawStroke(cx - s, cy + s, cx - s, cy - s, HINT_GLYPH_STROKE, UI_COLOR_BTN_SQUARE);
			break;
		}
		case HINT_SYM_TRIANGLE: {
			float top = cy - half * 0.82f, base = cy + half * 0.62f, w = half * 0.86f;
			UI_DrawStroke(cx, top, cx + w, base, HINT_GLYPH_STROKE, UI_COLOR_BTN_TRIANGLE);
			UI_DrawStroke(cx + w, base, cx - w, base, HINT_GLYPH_STROKE, UI_COLOR_BTN_TRIANGLE);
			UI_DrawStroke(cx - w, base, cx, top, HINT_GLYPH_STROKE, UI_COLOR_BTN_TRIANGLE);
			break;
		}
		case HINT_SYM_CHIP:
			break;
	}
}

static int NavRail_HintButtonCount(const NavRail_Hint *hint) {
	int n = 0;
	while (n < NAV_RAIL_HINT_MAX_BUTTONS && hint->buttons[n] != HINT_BTN_NONE)
		n++;
	return n;
}

static float NavRail_HintJoinWidth(const NavRail_Hint *hint) {
	return hint->combo ? (HINT_BUTTON_GAP * 2 + UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, "+")) : HINT_BUTTON_GAP;
}

static float NavRail_HintWidth(const NavRail_Hint *hint) {
	int n = NavRail_HintButtonCount(hint);
	float w = 0;

	for (int i = 0; i < n; i++)
		w += NavRail_HintButtonWidth(hint->buttons[i]) + ((i > 0) ? NavRail_HintJoinWidth(hint) : 0);

	if (n > 0)
		w += HINT_LABEL_GAP;

	return w + UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, hint->label);
}

void NavRail_DrawHints(float y, const NavRail_Hint *hints, int count) {
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, UI_HINT_BAR_HEIGHT, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, 1, UI_COLOR_HAIRLINE);

	float x = UI_RAIL_WIDTH + HINT_PAD_X;
	float right = 960 - HINT_PAD_X;
	float mid = y + UI_HINT_BAR_HEIGHT / 2.0f;
	float baseline = UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, y, UI_HINT_BAR_HEIGHT);
	float chip_y = mid - HINT_CHIP_H / 2.0f;
	float chip_baseline = UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, chip_y, HINT_CHIP_H);

	for (int i = 0; i < count; i++) {
		const NavRail_Hint *hint = &hints[i];

		if (!hint->label)
			continue;

		// Lo que no entra no se dibuja, y lo que venia despues tampoco: las
		// pantallas listan primero lo mas importante.
		if (x + NavRail_HintWidth(hint) > right)
			break;

		int n = NavRail_HintButtonCount(hint);

		for (int b = 0; b < n; b++) {
			if (b > 0) {
				if (hint->combo) {
					x += HINT_BUTTON_GAP;
					UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, baseline, UI_COLOR_TEXT_TERTIARY, "+");
					x += UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, "+") + HINT_BUTTON_GAP;
				}
				else
					x += HINT_BUTTON_GAP;
			}

			NavRail_HintSymbol symbol = NavRail_HintSymbolOf(hint->buttons[b]);
			float w = NavRail_HintButtonWidth(hint->buttons[b]);

			if (symbol == HINT_SYM_CHIP) {
				UI_DrawPill(x, chip_y, w, HINT_CHIP_H, UI_COLOR_SURFACE_2);
				UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x + HINT_CHIP_PAD_X, chip_baseline, UI_COLOR_TEXT_SECONDARY,
					NavRail_HintChipName(hint->buttons[b]));
			}
			else
				NavRail_DrawHintSymbol(symbol, x + w / 2.0f, mid);

			x += w;
		}

		if (n > 0)
			x += HINT_LABEL_GAP;

		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, baseline, UI_COLOR_TEXT_SECONDARY, hint->label);
		x += UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, hint->label) + HINT_ENTRY_GAP;
	}
}
