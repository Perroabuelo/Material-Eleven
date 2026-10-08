#include <psp2/audioout.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "audio.h"
#include "common.h"
#include "config.h"
#include "cover.h"
#include "fs.h"
#include "lang.h"
#include "lyrics.h"
#include "lyrics_layout.h"
#include "lyrics_load.h"
#include "menu_audioplayer.h"
#include "menu_displayfiles.h"
#include "menu_library.h"
#include "menu_settings.h"
#include "nav_rail.h"
#include "nav_request.h"
#include "queue.h"
#include "status_bar.h"
#include "touch.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

// La repeticion es una politica sobre el fin de la pista, y por eso vive aqui.
// El barajado es una propiedad del orden y vive en la cola, que es lo que deja
// al mini reproductor respetarlo sin conocer esta pantalla. Antes eran un solo
// int de tres valores, y encender uno apagaba el otro.
static SceBool repeat = SCE_FALSE;
static int length_time_width = 0;
static char *position_time = NULL, *length_time = NULL, *filename = NULL;

// Pantalla donde se eligio la cancion que suena, y adonde lleva volver. La fijan
// solo Menu_PlayAudio y Menu_PlayQueued: entrar por el nav rail o cambiar de
// pista no la cambia.
static UI_Screen playback_origin = UI_SCREEN_FOLDERS;

// The lyrics follow the track's lifecycle: loaded and laid out in
// Menu_InitMusic, freed in Music_FreeCurrentTrack. Defined with the lyrics view.
static void Music_LoadLyrics(const char *path, char *embedded, size_t embedded_len);
static void Music_FreeLyrics(void);

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

static SceBool Menu_InitMusic(const char *path) {
	// Sin esto, todo lo de abajo interroga a un decoder que no abrio.
	if (R_FAILED(Audio_Init(path)))
		return SCE_FALSE;

	// The embedded lyrics belong to this screen from here on.
	char *embedded_lyrics = metadata.lyrics;
	size_t embedded_lyrics_len = metadata.lyrics_len;
	metadata.lyrics = NULL;
	metadata.lyrics_len = 0;

	// A failing ALC mode is no reason to leave the screen without its strings.
	// This used to return here, and since the teardown frees them and nulls
	// them, the next frame would have drawn from null pointers.
	sceAudioOutSetAlcMode(config.alc_mode);

	filename = malloc(128);
	position_time = malloc(35);
	length_time = malloc(35);
	length_time_width = 0;

	// Out of memory: undo Audio_Init too, so the caller's "could not open"
	// path finds nothing loaded, as it would after any other failure.
	if (filename == NULL || position_time == NULL || length_time == NULL) {
		free(filename);
		free(position_time);
		free(length_time);
		filename = position_time = length_time = NULL;
		free(embedded_lyrics);

		UI_GpuFreeTexture(&metadata.cover_image);
		Audio_Term();
		return SCE_FALSE;
	}

	// The name is data, never a format: a file called "100% %s.wav" has to
	// come out as exactly that.
	snprintf(filename, 128, "%s", Utils_Basename(path));

	Menu_ConvertSecondsToString(length_time, Audio_GetLengthSeconds());
	length_time_width = UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, length_time);

	// The fallback rasterises each glyph the first time it is asked for, and a
	// CJK title brings a dozen new ones at once - enough to show as a hitch on
	// the first frame that draws it, which is what testing turned up when moving
	// from a Latin track to a Korean one.
	//
	// Measuring runs the same path drawing does: vita2d_pvf_text_width calls
	// generic_pvf_draw_text, and that is the only function that reaches
	// scePvfGetCharGlyphImage and texture_atlas_insert. So measuring here fills
	// the atlas ahead of time. It costs nothing to repeat for an already-cached
	// glyph, the atlas is keyed by glyph index so one pass covers every size the
	// strings are drawn at, and this runs between frames inside the stall a
	// track change already has - the work lands where the user is already
	// waiting rather than on the first frame they look at.
	UI_TextWidth(UI_FACE_UI, UI_TS_BODY, Music_GetDisplayTitle());
	UI_TextWidth(UI_FACE_UI, UI_TS_BODY, Music_GetDisplayArtist());

	// Laying the lyrics out measures every row, which warms the atlas the same
	// way for the whole text, inside the same stall.
	Music_LoadLyrics(path, embedded_lyrics, embedded_lyrics_len);
	return SCE_TRUE;
}

static void Music_FreeCurrentTrack(void) {
	// Tearing a track down stalls the render loop for a long time - Audio_Term
	// waits for the audio thread and the next track decodes its cover art - so the frame still
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

	Music_FreeLyrics();
	UI_GpuFreeTexture(&metadata.cover_image);
}

// Puesta cuando ninguna pista de la cola llego a abrirse. La lee el lazo
// de Now Playing para salir, en vez de quedarse dibujando sin track.
static SceBool track_failed = SCE_FALSE;

// El generador se siembra una vez y despues se le deja avanzar. Sembrarlo antes
// de cada tirada, como se hacia, lo ata al reloj: time() tiene resolucion de un
// segundo, asi que dos sorteos dentro del mismo segundo daban el mismo numero, y
// sorteos en segundos seguidos daban la primera salida de semillas consecutivas.
static void Music_SeedOnce(void) {
	static SceBool seeded = SCE_FALSE;

	if (seeded)
		return;

	time_t t;
	srand((unsigned)time(&t));
	seeded = SCE_TRUE;
}

// Solo avanza y abre: que pista toca lo decide la cola, venga el avance del
// transporte, de los gatillos, del mini reproductor o del fin de la pista.
// `replay` reabre la que suena en vez de moverse, que es lo que pide la
// repeticion.
static void Music_HandleNext(SceBool forward, SceBool replay) {
	int count = Queue_Count();
	const char *next = NULL;

	Audio_Stop();
	Music_FreeCurrentTrack();
	Audio_Term();

	// Every route to a new track passes here, and no frame is drawn until it
	// returns, so the next frame drawn is the first one after the change.
	UI_Debug_ArmCapture();

	if (replay)
		Queue_PeekAhead(0, &next, NULL, NULL);
	else
		next = Queue_Advance(forward);

	// Una pista que no abre no puede quedarse con la pantalla: se salta y se
	// prueba la siguiente del plan en la misma direccion. Como mucho una vuelta
	// entera a la cola, que es lo que acota esto cuando no abre ninguna.
	for (int tries = 0; tries < count; tries++) {
		if (next != NULL && Menu_InitMusic(next))
			return;

		next = Queue_Advance(forward);
	}

	track_failed = SCE_TRUE;
}

// Se baraja al encenderlo, y ahi es donde hace falta el generador ya sembrado.
// La pista en curso no se corta: la cola la deja al frente del plan.
static void Music_ToggleShuffle(void) {
	Music_SeedOnce();
	Queue_SetShuffle(!Queue_IsShuffled());
}

const char *Music_GetDisplayTitle(void) {
	if ((metadata.has_meta) && (metadata.title[0] != '\0'))
		return metadata.title;

	// Nunca NULL: el desmontaje libera filename y lo anula, y esta pantalla
	// ya se cayo una vez dibujando desde ahi.
	return (filename != NULL) ? filename : "";
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
	if (Audio_HasTrack() && Queue_Count() != 0)
		Music_HandleNext(SCE_FALSE, SCE_FALSE);
}

void Music_Next(void) {
	if (Audio_HasTrack() && Queue_Count() != 0)
		Music_HandleNext(SCE_TRUE, SCE_FALSE);
}

// ---- Now Playing rendering ----

#define CONTENT_X       (UI_RAIL_WIDTH)
#define STATUS_H        32
#define LEFT_PANEL_X     (CONTENT_X + 32)
#define LEFT_PANEL_W     372
#define NP_COVER_SIZE    258
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
#define UPNEXT_ART       34
// Covers read from disk per frame for Up Next: two rows, the second one a frame later.
#define UPNEXT_COVER_BUDGET 1

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

	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, 24), UI_COLOR_TEXT_MUTED, Lang_Get(STR_UP_NEXT));
	y += 30;

	// Una lectura de disco por fotograma: la segunda caratula llega un fotograma
	// despues, y cambiar de pista no espera a ninguna.
	int budget = UPNEXT_COVER_BUDGET;

	// Lo que dice el plan, sin casos especiales: el recorrido envuelve, asi que
	// siempre hay algo que va a sonar despues, y una cola de una sola pista
	// anuncia esa misma pista porque es literalmente lo que sonara.
	for (int i = 1; i <= 2 && i <= Queue_Count(); i++) {
		const char *path = NULL, *queued = NULL, *album = NULL;

		if (!Queue_PeekAhead(i, &path, &queued, &album))
			break;

		// El nombre que trajo el productor, para que esta lista y la vista de la
		// que salio digan lo mismo. Una cola de carpeta no trae ninguno, y ahi el
		// respaldo sigue siendo el nombre de archivo, como siempre.
		const char *name = (queued != NULL) ? queued : Utils_Basename(path);

		// Solo una cola de biblioteca trae album, y con la misma clave que usa la
		// biblioteca: el album, o la ruta si esta vacio. Una de carpeta se queda
		// con el marcador.
		vita2d_texture *art = (album != NULL) ? Cover_Get((album[0] != '\0') ? album : NULL, path, &budget) : NULL;

		if (art != NULL)
			vita2d_draw_texture_scale(art, x, y + 7, UPNEXT_ART / (float)COVER_SIZE, UPNEXT_ART / (float)COVER_SIZE);
		else
			UI_DrawRoundedRect(x, y + 7, UPNEXT_ART, UPNEXT_ART, 9, UI_COLOR_SURFACE_2);
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
		UI_DrawPlayGlyph(cx, TRANSPORT_CY, PLAY_GLYPH_SIZE, ui_color_on_accent);
	else
		UI_DrawPauseGlyph(cx, TRANSPORT_CY, PLAY_GLYPH_SIZE, ui_color_on_accent);

	float shuffle_x = prev_x - TOGGLE_ICON_GAP - TOGGLE_ICON_SIZE;
	Menu_DrawShuffleGlyph(shuffle_x + TOGGLE_ICON_SIZE / 2.0f, TRANSPORT_CY, STATE_GLYPH_SIZE,
		Queue_IsShuffled() ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_TERTIARY);

	float repeat_x = next_x + SIDE_BTN_SIZE + TOGGLE_ICON_GAP;
	Menu_DrawRepeatGlyph(repeat_x + TOGGLE_ICON_SIZE / 2.0f, TRANSPORT_CY, STATE_GLYPH_SIZE,
		repeat ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_TERTIARY);
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
		if (Queue_Count() != 0)
			Music_HandleNext(SCE_FALSE, SCE_FALSE);
		return SCE_TRUE;
	}

	if (UI_TouchTarget(next_x, side_y, SIDE_BTN_SIZE, SIDE_BTN_SIZE)) {
		if (Queue_Count() != 0)
			Music_HandleNext(SCE_TRUE, SCE_FALSE);
		return SCE_TRUE;
	}

	if (UI_TouchTarget(shuffle_x, toggle_y, TOGGLE_ICON_SIZE, TOGGLE_ICON_SIZE)) {
		Music_ToggleShuffle();
		return SCE_TRUE;
	}

	if (UI_TouchTarget(repeat_x, toggle_y, TOGGLE_ICON_SIZE, TOGGLE_ICON_SIZE)) {
		repeat = !repeat;
		return SCE_TRUE;
	}

	return SCE_FALSE;
}

// ---- Lyrics view ----
//
// Inside the content column, below the status bar: a compact header, the
// lyrics, and a seek bar over the hint bar. The lyrics are laid out once per
// track (Music_LoadLyrics), so a frame neither reads, allocates nor measures
// lyric text.

#define LYRICS_MARGIN_X   48
#define LYRICS_X          (CONTENT_X + LYRICS_MARGIN_X)
#define LYRICS_W          (960 - CONTENT_X - 2 * LYRICS_MARGIN_X)
#define LYRICS_HEADER_Y   (STATUS_H + 8)
#define LYRICS_HEADER_H   56
#define LYRICS_ART        40
#define LYRICS_BAR_H      40
#define LYRICS_TOP        (LYRICS_HEADER_Y + LYRICS_HEADER_H)
#define LYRICS_BOTTOM     (544 - UI_HINT_BAR_HEIGHT - LYRICS_BAR_H)
#define LYRICS_SEEK_Y     (LYRICS_BOTTOM + 10)
#define LYRICS_ROW_H      34
#define LYRICS_LINE_GAP   8
// An unsynced text starts this far below the top of the lyrics area.
#define LYRICS_TOP_PAD    16
// After a drag, synced lyrics stay where the user left them this long.
#define LYRICS_RETURN_US  3000000
// Share of the remaining distance the view covers each frame.
#define LYRICS_EASE       0.2f

// Lyrics_Line.flags bit this screen sets once per track.
#define LYRICS_FLAG_FALLBACK LYRICS_LINE_CALLER_FLAGS

static Lyrics lyrics = { 0 };
static LyricsLayout lyrics_layout = { 0 };

// Which view is open. Kept in memory only, so it survives track changes and
// trips through other screens, and the app always starts on the normal view.
static SceBool lyrics_view = SCE_FALSE;

// Content y shown at the vertical middle of the lyrics area.
static float lyrics_scroll = 0.0f;
static SceBool lyrics_dragging = SCE_FALSE;
// The user moved the lyrics: synced ones wait LYRICS_RETURN_US after the
// release before following the song again.
static SceBool lyrics_moved = SCE_FALSE;
static int lyrics_drag_y = 0;
static SceUInt64 lyrics_release_time = 0;

// Lines that need the fallback are drawn at its largest sharp size, current
// or not, so they only differ by color (ui/typography: nothing is rescaled).
static UI_TextSize Menu_LyricsLineSize(uint32_t line, SceBool current) {
	if (lyrics.lines[line].flags & LYRICS_FLAG_FALLBACK)
		return UI_TS_FALLBACK_MAX;

	return current ? UI_TS_TITLE : UI_TS_BODY;
}

// The layout measures each line at its largest size, so highlighting a line
// never changes how many rows it takes.
static float Menu_LyricsMeasure(const char *text, size_t len, uint32_t line, void *ctx) {
	char row[LYRICS_LAYOUT_MAX_ROW_BYTES + 1];

	memcpy(row, text, len);
	row[len] = '\0';
	return UI_TextWidth(UI_FACE_UI, Menu_LyricsLineSize(line, SCE_TRUE), row);
}

static float Menu_LyricsRowY(uint32_t row) {
	return row * LYRICS_ROW_H + lyrics_layout.rows[row].line * LYRICS_LINE_GAP;
}

// Synced lyrics stop with the first and the last row at the middle of the
// area; unsynced ones with the text against the top or the bottom.
static void Menu_LyricsScrollRange(float *lo, float *hi) {
	float half = (LYRICS_BOTTOM - LYRICS_TOP) / 2.0f;
	float last = (lyrics_layout.count > 0) ? Menu_LyricsRowY(lyrics_layout.count - 1) : 0.0f;

	if (lyrics.synced) {
		*lo = LYRICS_ROW_H / 2.0f;
		*hi = last + LYRICS_ROW_H / 2.0f;
	}
	else {
		*lo = half - LYRICS_TOP_PAD;
		*hi = last + LYRICS_ROW_H - (half - LYRICS_TOP_PAD);
		if (*hi < *lo)
			*hi = *lo;
	}
}

// Where the view rests: the first row of the line playing at the middle
// (the first line before its time comes), or the top of an unsynced text.
static float Menu_LyricsTarget(int line) {
	float lo, hi;
	Menu_LyricsScrollRange(&lo, &hi);

	if (!lyrics.synced || lyrics_layout.count == 0)
		return lo;

	uint32_t row = (line < 0) ? 0 : LyricsLayout_FirstRow(&lyrics_layout, line);

	if (row >= lyrics_layout.count)
		row = 0;

	return Menu_LyricsRowY(row) + LYRICS_ROW_H / 2.0f;
}

static void Menu_LyricsSnap(void) {
	lyrics_dragging = SCE_FALSE;
	lyrics_moved = SCE_FALSE;
	lyrics_scroll = Menu_LyricsTarget(Lyrics_LineAt(&lyrics, Audio_GetPositionMs()));
}

static void Music_LoadLyrics(const char *path, char *embedded, size_t embedded_len) {
	LyricsLoad_ForTrack(path, embedded, embedded_len, &lyrics);

	for (uint32_t i = 0; i < lyrics.count; i++) {
		if (UI_TextNeedsFallback(UI_FACE_UI, lyrics.text + lyrics.lines[i].off))
			lyrics.lines[i].flags |= LYRICS_FLAG_FALLBACK;
	}

	// Without memory for the rows there is nothing to draw: no lyrics, then.
	if (LyricsLayout_Build(&lyrics, LYRICS_W, Menu_LyricsMeasure, NULL, &lyrics_layout) < 0)
		Lyrics_Free(&lyrics);

	Menu_LyricsSnap();
}

static void Music_FreeLyrics(void) {
	LyricsLayout_Free(&lyrics_layout);
	Lyrics_Free(&lyrics);
	lyrics_scroll = 0.0f;
	lyrics_dragging = SCE_FALSE;
	lyrics_moved = SCE_FALSE;
}

// Synced lyrics ease towards the line playing, unless the user is holding
// them or let go less than LYRICS_RETURN_US ago. Unsynced ones never move
// on their own.
static void Menu_LyricsFollow(int current) {
	if (!lyrics.synced || lyrics_dragging)
		return;

	if (lyrics_moved) {
		if (sceKernelGetProcessTimeWide() - lyrics_release_time < LYRICS_RETURN_US)
			return;
		lyrics_moved = SCE_FALSE;
	}

	float target = Menu_LyricsTarget(current);
	float d = target - lyrics_scroll;

	lyrics_scroll = (d > -0.5f && d < 0.5f) ? target : lyrics_scroll + d * LYRICS_EASE;
}

static void Menu_LyricsHandleDrag(void) {
	if (lyrics_layout.count == 0) {
		lyrics_dragging = SCE_FALSE;
		return;
	}

	if (Touch_CheckPressed()) {
		// Only a touch that starts on the lyrics drags them; the rail and the
		// seek bar keep theirs.
		lyrics_dragging = Touch_GetX() >= CONTENT_X && Touch_GetY() >= LYRICS_TOP && Touch_GetY() < LYRICS_SEEK_Y - 12;
		lyrics_drag_y = Touch_GetY();
		return;
	}

	if (!lyrics_dragging)
		return;

	if (Touch_CheckHeld()) {
		int dy = Touch_GetY() - lyrics_drag_y;

		if (dy != 0) {
			float lo, hi;
			Menu_LyricsScrollRange(&lo, &hi);

			lyrics_scroll -= dy;
			if (lyrics_scroll < lo)
				lyrics_scroll = lo;
			if (lyrics_scroll > hi)
				lyrics_scroll = hi;

			lyrics_drag_y = Touch_GetY();
			lyrics_moved = SCE_TRUE;
		}
	}
	else {
		lyrics_dragging = SCE_FALSE;
		lyrics_release_time = sceKernelGetProcessTimeWide();
	}
}

// The bar spans the view, but Audio_Seek still takes a pixel on the old
// 450 px bar, so the touch is scaled to that.
static void Menu_LyricsHandleSeek(void) {
	if (!Touch_Position(LYRICS_X, LYRICS_SEEK_Y - 12, LYRICS_X + LYRICS_W, LYRICS_SEEK_Y + 12))
		return;

	SceBool was_paused = Audio_IsPaused();
	if (!was_paused)
		Audio_Pause();

	Audio_Seek((SceUInt64)((Touch_GetX() - LYRICS_X) * 450 / LYRICS_W));

	if (was_paused != Audio_IsPaused())
		Audio_Pause();
}

static void Menu_DrawLyricsHeader(void) {
	float y = LYRICS_HEADER_Y + (LYRICS_HEADER_H - LYRICS_ART) / 2.0f;

	if ((metadata.has_meta) && (metadata.cover_image))
		vita2d_draw_texture_scale(metadata.cover_image, LYRICS_X, y,
			(float)LYRICS_ART / vita2d_texture_get_width(metadata.cover_image), (float)LYRICS_ART / vita2d_texture_get_height(metadata.cover_image));
	else
		UI_DrawRoundedRect(LYRICS_X, y, LYRICS_ART, LYRICS_ART, 9, UI_COLOR_SURFACE);

	float text_x = LYRICS_X + LYRICS_ART + 14;
	float right = LYRICS_X + LYRICS_W;

	const char *badge_label; unsigned int badge_color, badge_wash;
	if (UI_GetFormatBadge(FS_GetFileExt(filename), &badge_label, &badge_color, &badge_wash)) {
		float badge_w = UI_BadgeWidth(UI_TS_BADGE, badge_label);
		right -= badge_w;
		UI_DrawBadge(right, LYRICS_HEADER_Y + LYRICS_HEADER_H / 2.0f - 11, UI_TS_BADGE, badge_label, badge_wash, badge_color);
		right -= 16;
	}

	const char *artist = Music_GetDisplayArtist();
	float title_top = (artist[0] != '\0') ? y - 2 : y + 8;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, text_x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, title_top, 24),
		right - text_x, UI_COLOR_TEXT_PRIMARY, Music_GetDisplayTitle());

	if (artist[0] != '\0')
		UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, text_x, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, title_top + 22, 20),
			right - text_x, UI_COLOR_TEXT_SECONDARY, artist);
}

static void Menu_DrawLyricsRows(void) {
	int current = Lyrics_LineAt(&lyrics, Audio_GetPositionMs());
	Menu_LyricsFollow(current);

	float mid = (LYRICS_TOP + LYRICS_BOTTOM) / 2.0f;
	char row[LYRICS_LAYOUT_MAX_ROW_BYTES + 1];

	vita2d_set_clip_rectangle(CONTENT_X, LYRICS_TOP, 960, LYRICS_BOTTOM);
	vita2d_enable_clipping();

	for (uint32_t r = 0; r < lyrics_layout.count; r++) {
		float top = mid + Menu_LyricsRowY(r) - lyrics_scroll - LYRICS_ROW_H / 2.0f;

		if (top + LYRICS_ROW_H < LYRICS_TOP)
			continue;
		if (top > LYRICS_BOTTOM)
			break;

		const LyricsLayout_Row *rw = &lyrics_layout.rows[r];

		if (rw->len == 0)
			continue;

		SceBool is_current = lyrics.synced && (int)rw->line == current;
		UI_TextSize ts = Menu_LyricsLineSize(rw->line, is_current);
		unsigned int color;

		if (is_current)
			color = UI_COLOR_TEXT_PRIMARY;
		else if (lyrics.lines[rw->line].flags & LYRICS_LINE_HEADER)
			color = UI_COLOR_TEXT_MUTED;
		else
			color = lyrics.synced ? UI_COLOR_TEXT_TERTIARY : UI_COLOR_TEXT_SECONDARY;

		memcpy(row, lyrics.text + rw->off, rw->len);
		row[rw->len] = '\0';
		UI_DrawText(UI_FACE_UI, ts, LYRICS_X, UI_TextBaselineY(UI_FACE_UI, ts, top, LYRICS_ROW_H), color, row);
	}

	vita2d_disable_clipping();
}

static void Menu_DrawLyricsView(void) {
	Menu_DrawLyricsHeader();

	if (lyrics_layout.count > 0)
		Menu_DrawLyricsRows();
	else {
		const char *none = Lang_Get(STR_NO_LYRICS);
		float x = LYRICS_X + (LYRICS_W - UI_TextWidth(UI_FACE_UI, UI_TS_BODY, none)) / 2.0f;
		UI_DrawText(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, (LYRICS_TOP + LYRICS_BOTTOM) / 2.0f - 15, 30),
			UI_COLOR_TEXT_TERTIARY, none);
	}

	SceUInt64 length = Audio_GetLength();
	double ratio = length ? ((double)Audio_GetPosition() / (double)length) : 0.0;
	UI_DrawPill(LYRICS_X, LYRICS_SEEK_Y, LYRICS_W, SEEK_H, UI_COLOR_SURFACE);
	UI_DrawPill(LYRICS_X, LYRICS_SEEK_Y, (float)(LYRICS_W * ratio), SEEK_H, ui_color_accent);

	Menu_ConvertSecondsToString(position_time, Audio_GetPositionSeconds());
	float time_y = UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, LYRICS_SEEK_Y + 8, 22);
	UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, LYRICS_X, time_y, UI_COLOR_TEXT_TERTIARY, position_time);
	UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, LYRICS_X + LYRICS_W - length_time_width, time_y, UI_COLOR_TEXT_TERTIARY, length_time);
}

static void Menu_DrawPlayerView(void) {
	// Cover art panel. vita2d does offer rectangular clipping, and the
	// text overflow policy uses it; what it has no equivalent for is a
	// stencil or an arbitrary-shape clip, so the artwork is still drawn as
	// a plain rect rather than one with rounded corners.
	float cover_y = STATUS_H + 22;
	if ((metadata.has_meta) && (metadata.cover_image))
		vita2d_draw_texture_scale(metadata.cover_image, LEFT_PANEL_X, cover_y,
			(float)NP_COVER_SIZE / vita2d_texture_get_width(metadata.cover_image), (float)NP_COVER_SIZE / vita2d_texture_get_height(metadata.cover_image));
	else
		UI_DrawRoundedRect(LEFT_PANEL_X, cover_y, NP_COVER_SIZE, NP_COVER_SIZE, UI_RADIUS_LG, UI_COLOR_SURFACE);

	const char *title = Music_GetDisplayTitle();
	const char *artist = Music_GetDisplayArtist();
	float info_y = cover_y + NP_COVER_SIZE + 20;

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
		UI_DrawBadge(LEFT_PANEL_X, info_y + 8, UI_TS_BADGE, badge_label, badge_wash, badge_color);

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
}

static UI_Screen Menu_RunNowPlayingLoop(void) {
	track_failed = SCE_FALSE;

	while (SCE_TRUE) {
		// Antes de dibujar: sin track, las cadenas de esta pantalla estan
		// liberadas y puestas a NULL.
		// Ninguna pista de la cola abrio: se vuelve adonde se eligio, igual
		// que con el boton de volver.
		if (track_failed) {
			track_failed = SCE_FALSE;
			return playback_origin;
		}

		UI_GpuBeginFrame();
		vita2d_clear_screen();

		vita2d_draw_rectangle(CONTENT_X, STATUS_H - 1, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);
		StatusBar_Display();

		if (lyrics_view)
			Menu_DrawLyricsView();
		else
			Menu_DrawPlayerView();

		// Confirmar pausa y reanuda, y la leyenda dice cual de las dos hara ahora.
		// Letras va antes de Triangulo: la barra corta desde el final.
		const NavRail_Hint hints[] = {
			{ { HINT_BTN_CONFIRM }, 0, Lang_Get(Audio_IsPaused() ? STR_HINT_PLAY : STR_HINT_PAUSE) },
			{ { HINT_BTN_CANCEL }, 0, Lang_Get(STR_HINT_BACK) },
			{ { HINT_BTN_L, HINT_BTN_R }, 0, Lang_Get(STR_HINT_PREV_NEXT) },
			{ { HINT_BTN_UP }, 0, Lang_Get(STR_HINT_LYRICS) },
			{ { HINT_BTN_TRIANGLE }, 0, Lang_Get(STR_HINT_SHUFFLE) },
			{ { HINT_BTN_SQUARE }, 0, Lang_Get(STR_HINT_REPEAT) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};
		NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, sizeof(hints) / sizeof(hints[0]));

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_NOW_PLAYING);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		// La pista termino sola. La repeticion la reabre; si no, se avanza por
		// el plan vigente, barajado o no, igual que en un salto manual.
		if (!playing && Queue_Count() != 0)
			Music_HandleNext(SCE_TRUE, repeat);

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		if (tapped == UI_SCREEN_FOLDERS || tapped == UI_SCREEN_LIBRARY || tapped == UI_SCREEN_SETTINGS)
			return tapped;

		if (pressed & SCE_CTRL_ENTER)
			Audio_Pause();

		if (pressed & SCE_CTRL_UP) {
			lyrics_view = !lyrics_view;

			// Synced lyrics open on the line playing, not scrolling there from
			// wherever they were left.
			if (lyrics_view && !lyrics_moved)
				Menu_LyricsSnap();
		}

		// The lyrics view has no on-screen transport: touches there scroll the
		// lyrics or seek with its own bar.
		if (lyrics_view) {
			Menu_LyricsHandleDrag();
			Menu_LyricsHandleSeek();
		}
		else if (!Menu_HandleTransportTouch() && Touch_CheckHeld() && Touch_Position(RIGHT_PANEL_X, SEEK_Y - 12, RIGHT_PANEL_R, SEEK_Y + 12)) {
			SceBool was_paused = Audio_IsPaused();
			if (!was_paused)
				Audio_Pause();

			Audio_Seek((SceUInt64)(Touch_GetX() - RIGHT_PANEL_X));

			if (was_paused != Audio_IsPaused())
				Audio_Pause();
		}

		if (pressed & SCE_CTRL_TRIANGLE)
			Music_ToggleShuffle();
		else if (pressed & SCE_CTRL_SQUARE)
			repeat = !repeat;

		if (pressed & SCE_CTRL_LTRIGGER) {
			if (Queue_Count() != 0)
				Music_HandleNext(SCE_FALSE, SCE_FALSE);
		}
		else if (pressed & SCE_CTRL_RTRIGGER) {
			if (Queue_Count() != 0)
				Music_HandleNext(SCE_TRUE, SCE_FALSE);
		}

		if (pressed & SCE_CTRL_CANCEL)
			return playback_origin;
	}
}

static void Menu_StopCurrentTrack(void) {
	if (Audio_HasTrack()) {
		Audio_Stop();
		Music_FreeCurrentTrack();
		Audio_Term();
	}
}

SceBool Menu_PlayAudio(char *path) {
	Menu_StopCurrentTrack();

	// La cola es la carpeta, que es lo que este camino siempre quiso decir.
	Queue_Clear();
	Queue_FillFromFolder(cwd);

	if (!Menu_InitMusic(path))
		return SCE_FALSE;

	playback_origin = UI_SCREEN_FOLDERS;

	// La cola arranca en la pista que se toco, y con el barajado encendido la
	// baraja detras de ella. Si esa ruta no entro en la cola - una carpeta por
	// encima del techo - la pista suena igual y la cola sigue desde su principio.
	Queue_SeekToPath(path);

	NavRequest_Set(UI_SCREEN_NOW_PLAYING);
	return SCE_TRUE;
}

SceBool Menu_PlayQueued(const char *path) {
	Menu_StopCurrentTrack();

	// Sin tocar la cola: la trae hecha quien llama, y rehacerla desde cwd
	// es justamente lo que dejo de ser obligatorio. Lo que si se hace aqui es
	// plantar la cola en la pista elegida, como en el camino de carpeta.
	if (!Menu_InitMusic(path))
		return SCE_FALSE;

	playback_origin = UI_SCREEN_LIBRARY;
	Queue_SeekToPath(path);

	NavRequest_Set(UI_SCREEN_NOW_PLAYING);
	return SCE_TRUE;
}

UI_Screen Menu_ShowNowPlaying(void) {
	if (!Audio_HasTrack())
		return UI_SCREEN_FOLDERS;

	return Menu_RunNowPlayingLoop();
}
