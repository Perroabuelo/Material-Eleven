#include "common.h"
#include "nav_rail.h"
#include "textures.h"
#include "touch.h"
#include "ui_theme.h"

#define RAIL_BUTTON_SIZE      44
#define RAIL_BUTTON_RADIUS    16
#define RAIL_SETTINGS_SIZE    40
#define RAIL_TOP_PAD          18
#define RAIL_GAP              22

typedef struct {
	UI_Screen screen;
	vita2d_texture *icon;
	float x, y, size;
} NavRail_Button;

static void NavRail_GetButtons(NavRail_Button out[3]) {
	float x = (UI_RAIL_WIDTH - RAIL_BUTTON_SIZE) / 2.0f;

	out[0] = (NavRail_Button){ UI_SCREEN_NOW_PLAYING, icon_nav_now_playing, x, RAIL_TOP_PAD, RAIL_BUTTON_SIZE };
	out[1] = (NavRail_Button){ UI_SCREEN_FOLDERS, icon_nav_folders, x, RAIL_TOP_PAD + RAIL_BUTTON_SIZE + RAIL_GAP, RAIL_BUTTON_SIZE };

	float settings_x = (UI_RAIL_WIDTH - RAIL_SETTINGS_SIZE) / 2.0f;
	out[2] = (NavRail_Button){ UI_SCREEN_SETTINGS, icon_nav_settings, settings_x, 544 - RAIL_TOP_PAD - RAIL_SETTINGS_SIZE, RAIL_SETTINGS_SIZE };
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
			UI_DrawRoundedRect(b->x, b->y, b->size, b->size, RAIL_BUTTON_RADIUS, UI_COLOR_ACCENT_WASH);

		float icon_size = 19;
		float icon_x = b->x + (b->size - icon_size) / 2;
		float icon_y = b->y + (b->size - icon_size) / 2;
		vita2d_draw_texture_tint_scale(b->icon, icon_x, icon_y, icon_size / vita2d_texture_get_width(b->icon),
			icon_size / vita2d_texture_get_height(b->icon), is_active ? UI_COLOR_ACCENT : UI_COLOR_TEXT_TERTIARY);

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
