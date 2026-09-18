#include <psp2/audioout.h>
#include <psp2/io/dirent.h>
#include <psp2/power.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "audio.h"
#include "common.h"
#include "config.h"
#include "fs.h"
#include "menu_displayfiles.h"
#include "menu_settings.h"
#include "nav_rail.h"
#include "status_bar.h"
#include "touch.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

typedef enum {
	MUSIC_STATE_NONE,   // 0
	MUSIC_STATE_REPEAT, // 1
	MUSIC_STATE_SHUFFLE // 2
} Music_State;

static char playlist[1024][512];
static int count = 0, selection = 0, state = 0;
static int length_time_width = 0;
static char *position_time = NULL, *length_time = NULL, *filename = NULL;

static int Menu_GetMusicList(void) {
	SceUID dir = 0;

	if (R_SUCCEEDED(dir = sceIoDopen(cwd))) {
		int entryCount = 0, i = 0;
		SceIoDirent *entries = (SceIoDirent *)calloc(MAX_FILES, sizeof(SceIoDirent));

		while (sceIoDread(dir, &entries[entryCount]) > 0)
			entryCount++;

		sceIoDclose(dir);
		qsort(entries, entryCount, sizeof(SceIoDirent), Utils_Alphasort);

		for (i = 0; i < entryCount; i++) {
			if ((!strncasecmp(FS_GetFileExt(entries[i].d_name), "flac", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "it", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "mod", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "mp3", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "ogg", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "opus", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "s3m", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].d_name), "wav", 4)) ||
				(!strncasecmp(FS_GetFileExt(entries[i].d_name), "xm", 4))) {
				strcpy(playlist[count], cwd);
				strcpy(playlist[count] + strlen(playlist[count]), entries[i].d_name);
				count++;
			}
		}

		free(entries);
	}
	else {
		sceIoDclose(dir);
		return dir;
	}

	return 0;
}

static int Music_GetCurrentIndex(char *path) {
	for(int i = 0; i < count; ++i) {
		if (!strcmp(playlist[i], path))
			return i;
	}

	return 0;
}

static void Menu_ConvertSecondsToString(char *string, SceUInt64 seconds) {
	int h = 0, m = 0, s = 0;
	h = (seconds / 3600);
	m = (seconds - (3600 * h)) / 60;
	s = (seconds - (3600 * h) - (m * 60));

	if (h > 0)
		snprintf(string, 35, "%02d:%02d:%02d", h, m, s);
	else
		snprintf(string, 35, "%02d:%02d", m, s);
}

static void Menu_InitMusic(char *path) {
	Audio_Init(path);
	if (sceAudioOutSetAlcMode(config.alc_mode) < 0)
		return;

	filename = malloc(128);
	snprintf(filename, 128, Utils_Basename(path));
	position_time = malloc(35);
	length_time = malloc(35);
	length_time_width = 0;

	Menu_ConvertSecondsToString(length_time, Audio_GetLengthSeconds());
	length_time_width = UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, length_time);
	selection = Music_GetCurrentIndex(path);
}

static void Music_FreeCurrentTrack(void) {
	// Tearing a track down stalls the render loop for a long time - Audio_Term
	// sleeps 100ms and the next track decodes its cover art - so the frame still
	// in flight has to be retired first. Otherwise the next vita2d_start_drawing
	// resets the vertex pool out from under vertices the GPU is still reading.
	// This used to sit inside the branch below, which meant it only ran when the
	// outgoing track happened to have cover art.
	//
	// It stays here even though UI_GpuFreeTexture synchronises too: that one
	// only covers the cover-art path, and what has to be retired before the
	// stall is the whole frame, cover art or not.
	vita2d_wait_rendering_done();

	free(filename);
	free(length_time);
	free(position_time);
	filename = length_time = position_time = NULL;

	UI_GpuFreeTexture(&metadata.cover_image);
}

static void Music_HandleNext(SceBool forward, int next_state) {
	if (next_state == MUSIC_STATE_NONE) {
		if (forward)
			selection++;
		else
			selection--;
	}
	else if (next_state == MUSIC_STATE_SHUFFLE) {
		int old_selection = selection;
		time_t t;
		srand((unsigned) time(&t));
		selection = rand() % (count - 1);

		if (selection == old_selection)
			selection--;
	}

	Utils_SetMax(&selection, 0, (count - 1));
	Utils_SetMin(&selection, (count - 1), 0);

	Audio_Stop();
	Music_FreeCurrentTrack();
	Audio_Term();
	Menu_InitMusic(playlist[selection]);
}

const char *Music_GetDisplayTitle(void) {
	if ((metadata.has_meta) && (metadata.title[0] != '\0'))
		return metadata.title;

	return filename;
}

const char *Music_GetDisplayArtist(void) {
	if ((metadata.has_meta) && (metadata.artist[0] != '\0'))
		return metadata.artist;

	return "";
}

void Music_TogglePlayPause(void) {
	if (Audio_HasTrack())
		Audio_Pause();
}

void Music_Previous(void) {
	if (Audio_HasTrack() && count != 0)
		Music_HandleNext(SCE_FALSE, MUSIC_STATE_NONE);
}

void Music_Next(void) {
	if (Audio_HasTrack() && count != 0)
		Music_HandleNext(SCE_TRUE, MUSIC_STATE_NONE);
}

// ---- Now Playing rendering ----

#define CONTENT_X       (UI_RAIL_WIDTH)
#define STATUS_H        32
#define LEFT_PANEL_X     (CONTENT_X + 32)
#define LEFT_PANEL_W     372
#define COVER_SIZE       258
#define RIGHT_PANEL_X    (LEFT_PANEL_X + LEFT_PANEL_W + 20)
#define RIGHT_PANEL_R    (960 - 34)
#define TRANSPORT_CY     318
#define PLAY_BTN_R       38
#define SIDE_BTN_SIZE    44
#define SIDE_BTN_GAP     26
#define TOGGLE_ICON_SIZE 34
// Wide enough that the shuffle and repeat touch areas, once grown to
// UI_TOUCH_MIN, clear the skip buttons beside them; at 26 they overlapped by a
// pixel and the skip button, tested first, swallowed the edge of the toggle.
#define TOGGLE_ICON_GAP  30
#define SEEK_Y           220
#define SEEK_H           5
#define UPNEXT_ROW_H     48

// Glyph boxes for the vector transport icons (drawn, not rescaled textures),
// each comfortably inside the control it sits in.
#define PLAY_GLYPH_SIZE   32
#define SKIP_GLYPH_SIZE   30
#define STATE_GLYPH_SIZE  24

// Shuffle: two paths that cross, each ending in a right-pointing arrow head.
static void Menu_DrawShuffleGlyph(float cx, float cy, float size, unsigned int color) {
	float x = cx - size / 2.0f, y = cy - size / 2.0f;
	float t = size * 0.085f;
	float top = y + size * 0.25f, bottom = y + size * 0.75f;
	float in_x = x + size * 0.11f, bend_x = x + size * 0.30f;
	float out_x = x + size * 0.58f, tip_x = x + size * 0.72f;
	float head = size * 0.13f;

	// Lower-left -> upper-right.
	UI_DrawStroke(in_x, bottom, bend_x, bottom, t, color);
	UI_DrawStroke(bend_x, bottom, out_x, top, t, color);
	UI_DrawStroke(out_x, top, tip_x, top, t, color);

	// Upper-left -> lower-right.
	UI_DrawStroke(in_x, top, bend_x, top, t, color);
	UI_DrawStroke(bend_x, top, out_x, bottom, t, color);
	UI_DrawStroke(out_x, bottom, tip_x, bottom, t, color);

	UI_DrawTriangle(tip_x, top - head, x + size * 0.92f, top, tip_x, top + head, color);
	UI_DrawTriangle(tip_x, bottom - head, x + size * 0.92f, bottom, tip_x, bottom + head, color);
}

// Repeat: a broken loop, arrow head at each open end.
static void Menu_DrawRepeatGlyph(float cx, float cy, float size, unsigned int color) {
	float x = cx - size / 2.0f, y = cy - size / 2.0f;
	float t = size * 0.085f;
	float top = y + size * 0.28f, bottom = y + size * 0.72f, mid = y + size * 0.50f;
	float left = x + size * 0.15f, right = x + size * 0.85f;
	float head = size * 0.13f;

	// Top run, left side going down.
	UI_DrawStroke(left, top, x + size * 0.70f, top, t, color);
	UI_DrawStroke(left, top, left, mid, t, color);
	UI_DrawTriangle(x + size * 0.70f, top - head, x + size * 0.93f, top, x + size * 0.70f, top + head, color);

	// Bottom run, right side going up.
	UI_DrawStroke(right, bottom, x + size * 0.30f, bottom, t, color);
	UI_DrawStroke(right, bottom, right, mid, t, color);
	UI_DrawTriangle(x + size * 0.30f, bottom - head, x + size * 0.07f, bottom, x + size * 0.30f, bottom + head, color);
}

static void Menu_DrawUpNext(void) {
	float x = RIGHT_PANEL_X, y = 544 - UI_HINT_BAR_HEIGHT - 14 - (2 * UPNEXT_ROW_H) - 30;

	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, 24), UI_COLOR_TEXT_MUTED, "A CONTINUACION");
	y += 30;

	int upcoming = count - selection - 1;

	if (upcoming <= 0) {
		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, UPNEXT_ROW_H), RIGHT_PANEL_R - x, UI_COLOR_TEXT_TERTIARY, "No hay mas pistas en esta carpeta");
		return;
	}

	for (int i = 0; i < 2 && i < upcoming; i++) {
		char *path = playlist[selection + 1 + i];
		char *name = Utils_Basename(path);

		UI_DrawRoundedRect(x, y + 7, 34, 34, 9, UI_COLOR_SURFACE_2);
		UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, x + 48, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y, UPNEXT_ROW_H), RIGHT_PANEL_R - (x + 48), UI_COLOR_TEXT_PRIMARY, name);

		y += UPNEXT_ROW_H;
	}
}

static void Menu_DrawTransportControls(void) {
	float cx = (RIGHT_PANEL_X + RIGHT_PANEL_R) / 2.0f;

	float prev_x = cx - PLAY_BTN_R - SIDE_BTN_GAP - SIDE_BTN_SIZE;
	float next_x = cx + PLAY_BTN_R + SIDE_BTN_GAP;
	float side_y = TRANSPORT_CY - SIDE_BTN_SIZE / 2;

	UI_DrawRoundedRect(prev_x, side_y, SIDE_BTN_SIZE, SIDE_BTN_SIZE, 16, UI_COLOR_SURFACE);
	UI_DrawSkipGlyph(prev_x + SIDE_BTN_SIZE / 2.0f, TRANSPORT_CY, SKIP_GLYPH_SIZE, SCE_FALSE, UI_COLOR_TEXT_PRIMARY);

	UI_DrawRoundedRect(next_x, side_y, SIDE_BTN_SIZE, SIDE_BTN_SIZE, 16, UI_COLOR_SURFACE);
	UI_DrawSkipGlyph(next_x + SIDE_BTN_SIZE / 2.0f, TRANSPORT_CY, SKIP_GLYPH_SIZE, SCE_TRUE, UI_COLOR_TEXT_PRIMARY);

	UI_DrawRoundedRect(cx - PLAY_BTN_R, TRANSPORT_CY - PLAY_BTN_R, PLAY_BTN_R * 2, PLAY_BTN_R * 2, 26, ui_color_accent);
	if (Audio_IsPaused())
		UI_DrawPlayGlyph(cx, TRANSPORT_CY, PLAY_GLYPH_SIZE, UI_COLOR_TEXT_PRIMARY);
	else
		UI_DrawPauseGlyph(cx, TRANSPORT_CY, PLAY_GLYPH_SIZE, UI_COLOR_TEXT_PRIMARY);

	float shuffle_x = prev_x - TOGGLE_ICON_GAP - TOGGLE_ICON_SIZE;
	Menu_DrawShuffleGlyph(shuffle_x + TOGGLE_ICON_SIZE / 2.0f, TRANSPORT_CY, STATE_GLYPH_SIZE,
		(state == MUSIC_STATE_SHUFFLE) ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_TERTIARY);

	float repeat_x = next_x + SIDE_BTN_SIZE + TOGGLE_ICON_GAP;
	Menu_DrawRepeatGlyph(repeat_x + TOGGLE_ICON_SIZE / 2.0f, TRANSPORT_CY, STATE_GLYPH_SIZE,
		(state == MUSIC_STATE_REPEAT) ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_TERTIARY);
}

static SceBool Menu_HandleTransportTouch(void) {
	float cx = (RIGHT_PANEL_X + RIGHT_PANEL_R) / 2.0f;
	float prev_x = cx - PLAY_BTN_R - SIDE_BTN_GAP - SIDE_BTN_SIZE;
	float next_x = cx + PLAY_BTN_R + SIDE_BTN_GAP;
	float side_y = TRANSPORT_CY - SIDE_BTN_SIZE / 2;
	float shuffle_x = prev_x - TOGGLE_ICON_GAP - TOGGLE_ICON_SIZE;
	float repeat_x = next_x + SIDE_BTN_SIZE + TOGGLE_ICON_GAP;
	float toggle_y = TRANSPORT_CY - TOGGLE_ICON_SIZE / 2;

	if (UI_TouchTarget(cx - PLAY_BTN_R, TRANSPORT_CY - PLAY_BTN_R, PLAY_BTN_R * 2, PLAY_BTN_R * 2)) {
		Audio_Pause();
		return SCE_TRUE;
	}

	if (UI_TouchTarget(prev_x, side_y, SIDE_BTN_SIZE, SIDE_BTN_SIZE)) {
		if (count != 0)
			Music_HandleNext(SCE_FALSE, MUSIC_STATE_NONE);
		return SCE_TRUE;
	}

	if (UI_TouchTarget(next_x, side_y, SIDE_BTN_SIZE, SIDE_BTN_SIZE)) {
		if (count != 0)
			Music_HandleNext(SCE_TRUE, MUSIC_STATE_NONE);
		return SCE_TRUE;
	}

	if (UI_TouchTarget(shuffle_x, toggle_y, TOGGLE_ICON_SIZE, TOGGLE_ICON_SIZE)) {
		state = (state == MUSIC_STATE_SHUFFLE) ? MUSIC_STATE_NONE : MUSIC_STATE_SHUFFLE;
		return SCE_TRUE;
	}

	if (UI_TouchTarget(repeat_x, toggle_y, TOGGLE_ICON_SIZE, TOGGLE_ICON_SIZE)) {
		state = (state == MUSIC_STATE_REPEAT) ? MUSIC_STATE_NONE : MUSIC_STATE_REPEAT;
		return SCE_TRUE;
	}

	return SCE_FALSE;
}

static void Menu_RunNowPlayingLoop(void) {
	while (SCE_TRUE) {
		vita2d_start_drawing();
		vita2d_clear_screen();

		vita2d_draw_rectangle(CONTENT_X, STATUS_H - 1, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);
		StatusBar_Display();

		// Cover art panel. vita2d does offer rectangular clipping, and the
		// text overflow policy uses it; what it has no equivalent for is a
		// stencil or an arbitrary-shape clip, so the artwork is still drawn as
		// a plain rect rather than one with rounded corners.
		float cover_y = STATUS_H + 22;
		if ((metadata.has_meta) && (metadata.cover_image))
			vita2d_draw_texture_scale(metadata.cover_image, LEFT_PANEL_X, cover_y,
				(float)COVER_SIZE / vita2d_texture_get_width(metadata.cover_image), (float)COVER_SIZE / vita2d_texture_get_height(metadata.cover_image));
		else
			UI_DrawRoundedRect(LEFT_PANEL_X, cover_y, COVER_SIZE, COVER_SIZE, UI_RADIUS_LG, UI_COLOR_SURFACE);

		const char *title = Music_GetDisplayTitle();
		const char *artist = Music_GetDisplayArtist();
		float info_y = cover_y + COVER_SIZE + 20;

		// The fallback rasterises near 18 px and is soft above the body token,
		// so a title the app's own face cannot cover is drawn one step down
		// rather than large and blurry. Only tracks that would otherwise be
		// unreadable are affected.
		UI_TextSize title_ts = UI_TextNeedsFallback(UI_FACE_UI, title) ? UI_TS_FALLBACK_MAX : UI_TS_DISPLAY;
		float title_box = (title_ts == UI_TS_DISPLAY) ? 38.0f : 30.0f;

		UI_DrawTextClipped(UI_FACE_UI, title_ts, LEFT_PANEL_X, UI_TextBaselineY(UI_FACE_UI, title_ts, info_y, title_box), LEFT_PANEL_W, UI_COLOR_TEXT_PRIMARY, title);
		info_y += title_box;

		if (artist[0] != '\0') {
			UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, LEFT_PANEL_X, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, info_y, 26), LEFT_PANEL_W, UI_COLOR_TEXT_SECONDARY, artist);
			info_y += 26;
		}

		const char *badge_label; unsigned int badge_color, badge_wash;
		if (UI_GetFormatBadge(FS_GetFileExt(filename), &badge_label, &badge_color, &badge_wash))
			UI_DrawBadge(LEFT_PANEL_X, info_y + 8, UI_TS_BADGE, badge_label, badge_wash, badge_color, badge_color);

		// Seek bar
		SceUInt64 length = Audio_GetLength();
		double ratio = length ? ((double)Audio_GetPosition() / (double)length) : 0.0;
		float seek_w = RIGHT_PANEL_R - RIGHT_PANEL_X;
		UI_DrawPill(RIGHT_PANEL_X, SEEK_Y, seek_w, SEEK_H, UI_COLOR_SURFACE);
		UI_DrawPill(RIGHT_PANEL_X, SEEK_Y, (float)(seek_w * ratio), SEEK_H, ui_color_accent);

		Menu_ConvertSecondsToString(position_time, Audio_GetPositionSeconds());
		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, RIGHT_PANEL_X, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, SEEK_Y + 12, 24), UI_COLOR_TEXT_TERTIARY, position_time);
		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, RIGHT_PANEL_R - length_time_width, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, SEEK_Y + 12, 24), UI_COLOR_TEXT_TERTIARY, length_time);

		Menu_DrawTransportControls();
		Menu_DrawUpNext();

		const char *hints[] = { "Menu", "Atras", "Reproducir / Pausa", NULL, "L . R - Anterior / Siguiente" };
		NavRail_DrawHintBar(544 - UI_HINT_BAR_HEIGHT, hints, 5);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_NOW_PLAYING);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		if (!playing) {
			if (state == MUSIC_STATE_NONE) {
				if (count != 0)
					Music_HandleNext(SCE_TRUE, MUSIC_STATE_NONE);
			}
			else if (state == MUSIC_STATE_REPEAT)
				Music_HandleNext(SCE_FALSE, MUSIC_STATE_REPEAT);
			else if (state == MUSIC_STATE_SHUFFLE) {
				if (count != 0)
					Music_HandleNext(SCE_FALSE, MUSIC_STATE_SHUFFLE);
			}
		}

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();

		if (tapped == UI_SCREEN_FOLDERS) {
			Touch_Reset();
			Menu_DisplayFiles();
			return;
		}
		else if (tapped == UI_SCREEN_SETTINGS) {
			Touch_Reset();
			Menu_DisplaySettings();
			return;
		}

		if (pressed & SCE_CTRL_ENTER)
			Audio_Pause();

		if (!Menu_HandleTransportTouch() && Touch_CheckHeld() && Touch_Position(RIGHT_PANEL_X, SEEK_Y - 12, RIGHT_PANEL_R, SEEK_Y + 12)) {
			SceBool was_paused = Audio_IsPaused();
			if (!was_paused)
				Audio_Pause();

			Audio_Seek((SceUInt64)(Touch_GetX() - RIGHT_PANEL_X));

			if (was_paused != Audio_IsPaused())
				Audio_Pause();
		}

		if (pressed & SCE_CTRL_TRIANGLE)
			state = (state == MUSIC_STATE_SHUFFLE) ? MUSIC_STATE_NONE : MUSIC_STATE_SHUFFLE;
		else if (pressed & SCE_CTRL_SQUARE)
			state = (state == MUSIC_STATE_REPEAT) ? MUSIC_STATE_NONE : MUSIC_STATE_REPEAT;

		if (pressed & SCE_CTRL_LTRIGGER) {
			if (count != 0)
				Music_HandleNext(SCE_FALSE, MUSIC_STATE_NONE);
		}
		else if (pressed & SCE_CTRL_RTRIGGER) {
			if (count != 0)
				Music_HandleNext(SCE_TRUE, MUSIC_STATE_NONE);
		}

		if (pressed & SCE_CTRL_START)
			scePowerRequestDisplayOff();

		if (pressed & SCE_CTRL_CANCEL) {
			Touch_Reset();
			Menu_DisplayFiles();
			return;
		}
	}
}

void Menu_PlayAudio(char *path) {
	if (Audio_HasTrack()) {
		Audio_Stop();
		Music_FreeCurrentTrack();
		Audio_Term();
		count = 0;
	}

	Menu_GetMusicList();
	Menu_InitMusic(path);

	Menu_RunNowPlayingLoop();
}

void Menu_ShowNowPlaying(void) {
	if (!Audio_HasTrack())
		return;

	Menu_RunNowPlayingLoop();
}
