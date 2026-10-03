#include "audio.h"
#include "common.h"
#include "menu_audioplayer.h"
#include "mini_player.h"
#include "touch.h"
#include "ui_theme.h"

// Glyph boxes for the mini-player transport controls.
#define MINI_GLYPH_SIZE      24
#define MINI_PLAY_GLYPH_SIZE 18
#define MINI_PLAY_R          22

float MiniPlayer_Top(void) { return 544 - UI_HINT_BAR_HEIGHT - MINI_PLAYER_H; }

// One source for the mini-player transport, used by both the drawing and the
// hit testing so the active areas cannot drift away from the drawn controls.
// Centres, not left edges: the three controls sit on a UI_TOUCH_MIN pitch, so
// each active area is already the minimum size and none of them overlaps its
// neighbour. The drawn glyphs are smaller and centred inside their area.
typedef struct {
	float prev_cx, play_cx, next_cx, cy;
} MiniPlayer_Transport;

#define MINI_TRANSPORT_GAP 8

static MiniPlayer_Transport MiniPlayer_TransportGeometry(void) {
	MiniPlayer_Transport t;

	t.next_cx = 960 - 22 - UI_TOUCH_MIN / 2.0f;
	t.play_cx = t.next_cx - UI_TOUCH_MIN - MINI_TRANSPORT_GAP;
	t.prev_cx = t.play_cx - UI_TOUCH_MIN - MINI_TRANSPORT_GAP;
	t.cy = MiniPlayer_Top() + MINI_PLAYER_H / 2.0f;

	return t;
}

// Where the title and artist have to stop so they never reach the controls.
static float MiniPlayer_TextRight(void) {
	return MiniPlayer_TransportGeometry().prev_cx - UI_TOUCH_MIN / 2.0f - 18;
}

void MiniPlayer_Draw(void) {
	if (!Audio_HasTrack())
		return;

	float y = MiniPlayer_Top();
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, MINI_PLAYER_H, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(UI_RAIL_WIDTH, y, 960 - UI_RAIL_WIDTH, 1, UI_COLOR_HAIRLINE);

	float cover_size = 46, cover_x = UI_RAIL_WIDTH + 22, cover_y = y + (MINI_PLAYER_H - cover_size) / 2;
	if ((metadata.has_meta) && (metadata.cover_image))
		vita2d_draw_texture_scale(metadata.cover_image, cover_x, cover_y,
			cover_size / vita2d_texture_get_width(metadata.cover_image), cover_size / vita2d_texture_get_height(metadata.cover_image));
	else
		UI_DrawRoundedRect(cover_x, cover_y, cover_size, cover_size, 12, UI_COLOR_LOSSLESS);

	const char *title = Music_GetDisplayTitle();
	const char *artist = Music_GetDisplayArtist();
	float text_x = cover_x + cover_size + 14;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, text_x, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y + 10, 22), MiniPlayer_TextRight() - text_x, UI_COLOR_TEXT_PRIMARY, title);
	if (artist[0] != '\0')
		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, text_x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y + 36, 20), MiniPlayer_TextRight() - text_x, UI_COLOR_TEXT_SECONDARY, artist);

	MiniPlayer_Transport t = MiniPlayer_TransportGeometry();

	UI_DrawSkipGlyph(t.prev_cx, t.cy, MINI_GLYPH_SIZE, SCE_FALSE, UI_COLOR_TEXT_SECONDARY);

	UI_DrawRoundedRect(t.play_cx - MINI_PLAY_R, t.cy - MINI_PLAY_R, MINI_PLAY_R * 2, MINI_PLAY_R * 2, MINI_PLAY_R, ui_color_accent);
	if (Audio_IsPaused())
		UI_DrawPlayGlyph(t.play_cx, t.cy, MINI_PLAY_GLYPH_SIZE, ui_color_on_accent);
	else
		UI_DrawPauseGlyph(t.play_cx, t.cy, MINI_PLAY_GLYPH_SIZE, ui_color_on_accent);

	UI_DrawSkipGlyph(t.next_cx, t.cy, MINI_GLYPH_SIZE, SCE_TRUE, UI_COLOR_TEXT_SECONDARY);
}

SceBool MiniPlayer_HandleTouch(void) {
	if (!Audio_HasTrack())
		return SCE_FALSE;

	MiniPlayer_Transport t = MiniPlayer_TransportGeometry();

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

