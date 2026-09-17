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
#include "textures.h"
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
	length_time_width = vita2d_font_text_width(font_mono, UI_FONT_SIZE_HINT, length_time);
	selection = Music_GetCurrentIndex(path);
}

static void Music_FreeCurrentTrack(void) {
	free(filename);
	free(length_time);
	free(position_time);

	if ((metadata.has_meta) && (metadata.cover_image)) {
		vita2d_wait_rendering_done();
		vita2d_free_texture(metadata.cover_image);
	}
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
#define STATUS_H        28
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
#define TOGGLE_ICON_GAP  26
#define SEEK_Y           220
#define SEEK_H           5
#define UPNEXT_ROW_H     40

static void Menu_DrawUpNext(void) {
	float x = RIGHT_PANEL_X, y = 544 - UI_HINT_BAR_HEIGHT - 18 - (2 * UPNEXT_ROW_H) - 22;

	vita2d_font_draw_text(font_mono, x, UI_TextBaselineY(font_mono, UI_FONT_SIZE_BADGE, "A CONTINUACION", y, 20),
		UI_COLOR_TEXT_MUTED, UI_FONT_SIZE_BADGE, "A CONTINUACION");
	y += 26;

	int upcoming = count - selection - 1;

	if (upcoming <= 0) {
		vita2d_font_draw_text(font_ui, x, UI_TextBaselineY(font_ui, UI_FONT_SIZE_BODY, "No hay mas pistas en esta carpeta", y, UPNEXT_ROW_H),
			UI_COLOR_TEXT_TERTIARY, UI_FONT_SIZE_BODY, "No hay mas pistas en esta carpeta");
		return;
	}

	for (int i = 0; i < 2 && i < upcoming; i++) {
		char *path = playlist[selection + 1 + i];
		char *name = Utils_Basename(path);

		UI_DrawRoundedRect(x, y + 5, 30, 30, 8, UI_COLOR_SURFACE_2);
		vita2d_font_draw_text(font_ui, x + 42, UI_TextBaselineY(font_ui, UI_FONT_SIZE_LABEL_SMALL, name, y, UPNEXT_ROW_H),
			UI_COLOR_TEXT_PRIMARY, UI_FONT_SIZE_LABEL_SMALL, name);

		y += UPNEXT_ROW_H;
	}
}

static void Menu_DrawTransportControls(void) {
	float cx = (RIGHT_PANEL_X + RIGHT_PANEL_R) / 2.0f;

	float prev_x = cx - PLAY_BTN_R - SIDE_BTN_GAP - SIDE_BTN_SIZE;
	float next_x = cx + PLAY_BTN_R + SIDE_BTN_GAP;
	float side_y = TRANSPORT_CY - SIDE_BTN_SIZE / 2;

	UI_DrawRoundedRect(prev_x, side_y, SIDE_BTN_SIZE, SIDE_BTN_SIZE, 16, UI_COLOR_SURFACE);
	vita2d_draw_texture(btn_rewind, prev_x + (SIDE_BTN_SIZE - vita2d_texture_get_width(btn_rewind)) / 2,
		side_y + (SIDE_BTN_SIZE - vita2d_texture_get_height(btn_rewind)) / 2);

	UI_DrawRoundedRect(next_x, side_y, SIDE_BTN_SIZE, SIDE_BTN_SIZE, 16, UI_COLOR_SURFACE);
	vita2d_draw_texture(btn_forward, next_x + (SIDE_BTN_SIZE - vita2d_texture_get_width(btn_forward)) / 2,
		side_y + (SIDE_BTN_SIZE - vita2d_texture_get_height(btn_forward)) / 2);

	UI_DrawRoundedRect(cx - PLAY_BTN_R, TRANSPORT_CY - PLAY_BTN_R, PLAY_BTN_R * 2, PLAY_BTN_R * 2, 26, UI_COLOR_ACCENT);
	vita2d_texture *play_tex = Audio_IsPaused() ? btn_play : btn_pause;
	vita2d_draw_texture(play_tex, cx - vita2d_texture_get_width(play_tex) / 2, TRANSPORT_CY - vita2d_texture_get_height(play_tex) / 2);

	float shuffle_x = prev_x - TOGGLE_ICON_GAP - TOGGLE_ICON_SIZE;
	vita2d_texture *shuffle_tex = (state == MUSIC_STATE_SHUFFLE) ? btn_shuffle_overlay : btn_shuffle;
	vita2d_draw_texture(shuffle_tex, shuffle_x + (TOGGLE_ICON_SIZE - vita2d_texture_get_width(shuffle_tex)) / 2,
		TRANSPORT_CY - vita2d_texture_get_height(shuffle_tex) / 2);

	float repeat_x = next_x + SIDE_BTN_SIZE + TOGGLE_ICON_GAP;
	vita2d_texture *repeat_tex = (state == MUSIC_STATE_REPEAT) ? btn_repeat_overlay : btn_repeat;
	vita2d_draw_texture(repeat_tex, repeat_x + (TOGGLE_ICON_SIZE - vita2d_texture_get_width(repeat_tex)) / 2,
		TRANSPORT_CY - vita2d_texture_get_height(repeat_tex) / 2);
}

static SceBool Menu_HandleTransportTouch(void) {
	float cx = (RIGHT_PANEL_X + RIGHT_PANEL_R) / 2.0f;
	float prev_x = cx - PLAY_BTN_R - SIDE_BTN_GAP - SIDE_BTN_SIZE;
	float next_x = cx + PLAY_BTN_R + SIDE_BTN_GAP;
	float side_y = TRANSPORT_CY - SIDE_BTN_SIZE / 2;
	float shuffle_x = prev_x - TOGGLE_ICON_GAP - TOGGLE_ICON_SIZE;
	float repeat_x = next_x + SIDE_BTN_SIZE + TOGGLE_ICON_GAP;
	float toggle_y = TRANSPORT_CY - TOGGLE_ICON_SIZE / 2;

	if (Touch_Position(cx - PLAY_BTN_R, TRANSPORT_CY - PLAY_BTN_R, cx + PLAY_BTN_R, TRANSPORT_CY + PLAY_BTN_R)) {
		Audio_Pause();
		return SCE_TRUE;
	}

	if (Touch_Position(prev_x, side_y, prev_x + SIDE_BTN_SIZE, side_y + SIDE_BTN_SIZE)) {
		if (count != 0)
			Music_HandleNext(SCE_FALSE, MUSIC_STATE_NONE);
		return SCE_TRUE;
	}

	if (Touch_Position(next_x, side_y, next_x + SIDE_BTN_SIZE, side_y + SIDE_BTN_SIZE)) {
		if (count != 0)
			Music_HandleNext(SCE_TRUE, MUSIC_STATE_NONE);
		return SCE_TRUE;
	}

	if (Touch_Position(shuffle_x, toggle_y, shuffle_x + TOGGLE_ICON_SIZE, toggle_y + TOGGLE_ICON_SIZE)) {
		state = (state == MUSIC_STATE_SHUFFLE) ? MUSIC_STATE_NONE : MUSIC_STATE_SHUFFLE;
		return SCE_TRUE;
	}

	if (Touch_Position(repeat_x, toggle_y, repeat_x + TOGGLE_ICON_SIZE, toggle_y + TOGGLE_ICON_SIZE)) {
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

		// Cover art panel (see PR notes: vita2d has no texture clip/stencil,
		// so the artwork itself is drawn as a plain rect, not rounded).
		float cover_y = STATUS_H + 26;
		if ((metadata.has_meta) && (metadata.cover_image))
			vita2d_draw_texture_scale(metadata.cover_image, LEFT_PANEL_X, cover_y,
				(float)COVER_SIZE / vita2d_texture_get_width(metadata.cover_image), (float)COVER_SIZE / vita2d_texture_get_height(metadata.cover_image));
		else
			UI_DrawRoundedRect(LEFT_PANEL_X, cover_y, COVER_SIZE, COVER_SIZE, UI_RADIUS_LG, UI_COLOR_SURFACE);

		const char *title = Music_GetDisplayTitle();
		const char *artist = Music_GetDisplayArtist();
		float info_y = cover_y + COVER_SIZE + 20;

		vita2d_font_draw_text(font_ui, LEFT_PANEL_X, UI_TextBaselineY(font_ui, UI_FONT_SIZE_DISPLAY, title, info_y, 26),
			UI_COLOR_TEXT_PRIMARY, UI_FONT_SIZE_DISPLAY, title);
		info_y += 26;

		if (artist[0] != '\0') {
			vita2d_font_draw_text(font_ui, LEFT_PANEL_X, UI_TextBaselineY(font_ui, UI_FONT_SIZE_BODY, artist, info_y, 20),
				UI_COLOR_TEXT_SECONDARY, UI_FONT_SIZE_BODY, artist);
			info_y += 20;
		}

		const char *badge_label; unsigned int badge_color, badge_wash;
		if (UI_GetFormatBadge(FS_GetFileExt(filename), &badge_label, &badge_color, &badge_wash))
			UI_DrawBadge(LEFT_PANEL_X, info_y + 8, UI_FONT_SIZE_BADGE, badge_label, badge_wash, badge_color, badge_color);

		// Seek bar
		SceUInt64 length = Audio_GetLength();
		double ratio = length ? ((double)Audio_GetPosition() / (double)length) : 0.0;
		float seek_w = RIGHT_PANEL_R - RIGHT_PANEL_X;
		UI_DrawPill(RIGHT_PANEL_X, SEEK_Y, seek_w, SEEK_H, UI_COLOR_SURFACE);
		UI_DrawPill(RIGHT_PANEL_X, SEEK_Y, (float)(seek_w * ratio), SEEK_H, UI_COLOR_ACCENT);

		Menu_ConvertSecondsToString(position_time, Audio_GetPositionSeconds());
		vita2d_font_draw_text(font_mono, RIGHT_PANEL_X, UI_TextBaselineY(font_mono, UI_FONT_SIZE_HINT, position_time, SEEK_Y + 10, 20),
			UI_COLOR_TEXT_TERTIARY, UI_FONT_SIZE_HINT, position_time);
		vita2d_font_draw_text(font_mono, RIGHT_PANEL_R - length_time_width, UI_TextBaselineY(font_mono, UI_FONT_SIZE_HINT, length_time, SEEK_Y + 10, 20),
			UI_COLOR_TEXT_TERTIARY, UI_FONT_SIZE_HINT, length_time);

		Menu_DrawTransportControls();
		Menu_DrawUpNext();

		const char *hints[] = { "Menu", "Atras", "Reproducir / Pausa", NULL, "L . R - Anterior / Siguiente" };
		NavRail_DrawHintBar(544 - UI_HINT_BAR_HEIGHT, hints, 5);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_NOW_PLAYING);

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
	SceBool had_track = Audio_HasTrack();

	if (had_track) {
		Audio_Stop();
		Music_FreeCurrentTrack();
		Audio_Term();
		count = 0;
	}

	Menu_GetMusicList();
	Menu_InitMusic(path);

	if (!had_track)
		Utils_LockPower();

	Menu_RunNowPlayingLoop();
}

void Menu_ShowNowPlaying(void) {
	if (!Audio_HasTrack())
		return;

	Menu_RunNowPlayingLoop();
}
