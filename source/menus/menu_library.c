#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "fs.h"
#include "library.h"
#include "menu_audioplayer.h"
#include "menu_displayfiles.h"
#include "menu_library.h"
#include "menu_settings.h"
#include "nav_rail.h"
#include "queue.h"
#include "status_bar.h"
#include "touch.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

#define CONTENT_X   (UI_RAIL_WIDTH)
#define TOPBAR_H    64
#define ROW_H       64
#define ROWS_PER_PAGE 5
#define LIST_TOP    (TOPBAR_H + 8)

// Cuantos fotogramas se queda un aviso delante. A 60 fps son unos cuatro
// segundos: suficiente para leerlo, y se va solo sin pedir confirmacion.
#define NOTICE_FRAMES 240

// Cada cuantos fotogramas se vuelve a preguntar si la carpeta sigue ahi.
// A 60 fps, una vez por segundo.
#define ROOT_CHECK_PERIOD 60

static int selection = 0;
static SceBool root_missing = SCE_FALSE;
static int root_check_frames = 0;
static char notice[160] = "";
static int notice_frames = 0;

static void Menu_LibraryNotice(const char *text) {
	snprintf(notice, sizeof(notice), "%s", text);
	notice_frames = NOTICE_FRAMES;
}

// ---------------------------------------------------------------------------
// estado de la pantalla

typedef enum {
	LIBRARY_STATE_NO_ROOT = 0,  // no ha elegido carpeta nunca
	LIBRARY_STATE_UNBUILT,      // hay carpeta, pero no hay indice
	LIBRARY_STATE_EMPTY,        // escaneo hecho, carpeta sin musica
	LIBRARY_STATE_READY         // hay pistas
} Menu_LibraryState;

static Menu_LibraryState Menu_LibraryGetState(void) {
	if (!Library_HasRoot())
		return LIBRARY_STATE_NO_ROOT;

	if (!Library_IsBuilt())
		return LIBRARY_STATE_UNBUILT;

	if (Library_Count() == 0)
		return LIBRARY_STATE_EMPTY;

	return LIBRARY_STATE_READY;
}

// ---------------------------------------------------------------------------
// dibujado

static void Menu_DrawLibraryTopBar(void) {
	vita2d_draw_rectangle(CONTENT_X, 0, 960 - CONTENT_X, TOPBAR_H, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(CONTENT_X, TOPBAR_H - 1, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);

	float x = CONTENT_X + 22;

	UI_DrawText(UI_FACE_UI, UI_TS_TITLE, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_TITLE, 0, TOPBAR_H - 18),
		UI_COLOR_TEXT_PRIMARY, "Biblioteca");

	if (Library_HasRoot()) {
		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, x,
			UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, TOPBAR_H - 26, 20), 960 - x - 22,
			UI_COLOR_TEXT_TERTIARY, Library_GetRoot());
	}
}

// Un estado vacio que dice cual de los cuatro es y que hacer, en vez de una
// lista vacia sin explicacion.
static void Menu_DrawLibraryPlaceholder(const char *line, const char *action) {
	float x = CONTENT_X + 22, y = 180.0f;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, 26),
		960 - x - 22, UI_COLOR_TEXT_SECONDARY, line);

	if (action != NULL) {
		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, y + 34, 24),
			UI_COLOR_TEXT_TERTIARY, action);
	}
}

static void Menu_DrawLibraryList(void) {
	int count = Library_Count();
	int first = selection - (selection % ROWS_PER_PAGE);
	float y = LIST_TOP;

	for (int i = first; i < first + ROWS_PER_PAGE && i < count; i++) {
		const Library_Track *track = Library_GetTrack(i);

		if (track == NULL)
			break;

		if (i == selection)
			UI_DrawRowHighlight(CONTENT_X, y, 960 - CONTENT_X, ROW_H);

		float x = CONTENT_X + 22;

		// Sin tags todavia - eso es la fase 2 -, asi que el titulo que se ve es
		// el respaldo por nombre de archivo, que es lo que el spec pide cuando
		// no hay metadatos.
		const char *title = (track->title[0] != '\0') ? track->title : Utils_Basename(track->path);

		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y + 6, 24),
			960 - x - 100, UI_COLOR_TEXT_PRIMARY, title);

		const char *artist = (track->artist[0] != '\0') ? track->artist : track->path;

		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y + 34, 20),
			960 - x - 100, UI_COLOR_TEXT_TERTIARY, artist);

		y += ROW_H;
	}

	char counter[96];

	if (Library_IsTruncated())
		snprintf(counter, sizeof(counter), "%d de %d  -  biblioteca truncada en el maximo", selection + 1, count);
	else
		snprintf(counter, sizeof(counter), "%d de %d", selection + 1, count);

	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, CONTENT_X + 22,
		UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, 544 - UI_HINT_BAR_HEIGHT - 30, 22),
		UI_COLOR_TEXT_MUTED, counter);
}

static void Menu_DrawLibraryNotice(void) {
	if (notice_frames <= 0)
		return;

	float h = 44.0f, y = 544 - UI_HINT_BAR_HEIGHT - h - 8;

	UI_DrawRoundedRect(CONTENT_X + 16, y, 960 - CONTENT_X - 32, h, 12, UI_COLOR_SURFACE_2);
	UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, CONTENT_X + 32, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y, h),
		960 - CONTENT_X - 64, UI_COLOR_TEXT_PRIMARY, notice);
}

// ---------------------------------------------------------------------------
// acciones

static void Menu_LibraryPickRoot(void) {
	char picked[LIBRARY_PATH_MAX];

	if (!Menu_PickFolder(picked, sizeof(picked)))
		return;

	Library_SetRoot(picked);
	selection = 0;
	Library_RunScan();

	if (Library_IsTruncated())
		Menu_LibraryNotice("La coleccion supera el maximo: la biblioteca quedo truncada.");
}

static void Menu_LibraryRescan(void) {
	if (!Library_HasRoot())
		return;

	if (!Library_RootAvailable()) {
		Menu_LibraryNotice("Esa carpeta ya no esta disponible. Elegi otra con Cuadrado.");
		return;
	}

	selection = 0;
	Library_RunScan();

	if (Library_IsTruncated())
		Menu_LibraryNotice("La coleccion supera el maximo: la biblioteca quedo truncada.");
}

// El productor de biblioteca: vuelca la vista vigente en la cola, en el orden
// en que se ve, y reproduce desde ahi. Siguiente y anterior recorren esa cola y
// no la carpeta en la que este el archivo.
static void Menu_LibraryPlaySelected(void) {
	const Library_Track *track = Library_GetTrack(selection);

	if (track == NULL)
		return;

	// El indice es una cache de lo que habia en disco, y el disco pudo cambiar
	// desde el ultimo escaneo. Comprobarlo aqui es barato; validar el indice
	// entero al arrancar seria un escaneo.
	if (!FS_FileExists(track->path)) {
		Menu_LibraryNotice("Ese archivo ya no esta. Reescanea para poner la biblioteca al dia.");
		return;
	}

	Queue_Clear();

	for (int i = 0; i < Library_Count(); i++) {
		const Library_Track *t = Library_GetTrack(i);

		if (t == NULL || !Queue_Add(t->path))
			break;
	}

	Queue_SetPosition(selection);
	Menu_PlayQueued(track->path);
}

// ---------------------------------------------------------------------------

static void Menu_HandleLibraryControls(Menu_LibraryState state) {
	int count = Library_Count();

	if (state == LIBRARY_STATE_READY && count > 0) {
		if (pressed & SCE_CTRL_UP)
			selection--;
		else if (pressed & SCE_CTRL_DOWN)
			selection++;

		Utils_SetMax(&selection, 0, count - 1);
		Utils_SetMin(&selection, count - 1, 0);

		if (pressed & SCE_CTRL_LEFT)
			selection = 0;
		else if (pressed & SCE_CTRL_RIGHT)
			selection = count - 1;

		if (pressed & SCE_CTRL_ENTER)
			Menu_LibraryPlaySelected();
	}
	else if (pressed & SCE_CTRL_ENTER) {
		// Sin nada que reproducir, confirmar hace lo unico que procede en cada
		// uno de los otros tres estados.
		if (state == LIBRARY_STATE_NO_ROOT)
			Menu_LibraryPickRoot();
		else
			Menu_LibraryRescan();
	}

	if (pressed & SCE_CTRL_TRIANGLE)
		Menu_LibraryRescan();
	else if (pressed & SCE_CTRL_SQUARE)
		Menu_LibraryPickRoot();
}

void Menu_DisplayLibrary(void) {
	vita2d_set_clear_color(UI_COLOR_BG);

	while (SCE_TRUE) {
		Menu_LibraryState state = Menu_LibraryGetState();

		vita2d_start_drawing();
		vita2d_clear_screen();

		StatusBar_Display();
		Menu_DrawLibraryTopBar();

		// La comprobacion es una llamada al sistema de archivos, asi que se
		// hace cada tantos fotogramas y no en todos.
		if ((root_check_frames--) <= 0) {
			root_check_frames = ROOT_CHECK_PERIOD;
			root_missing = Library_HasRoot() && !Library_RootAvailable();
		}

		if (root_missing) {
			UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, CONTENT_X + 22,
				UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, TOPBAR_H + 10, 24), 960 - CONTENT_X - 44,
				UI_COLOR_TEXT_SECONDARY, "La carpeta de la biblioteca no esta disponible. Cuadrado - elegir otra.");
		}

		switch (state) {
			case LIBRARY_STATE_NO_ROOT:
				Menu_DrawLibraryPlaceholder("Todavia no elegiste una carpeta para la biblioteca.",
					"Cuadrado - Elegir carpeta");
				break;
			case LIBRARY_STATE_UNBUILT:
				Menu_DrawLibraryPlaceholder("Esta carpeta todavia no se escaneo.",
					"Triangulo - Escanear");
				break;
			case LIBRARY_STATE_EMPTY:
				Menu_DrawLibraryPlaceholder("En esa carpeta no habia ninguna pista reproducible.",
					"Cuadrado - Elegir otra carpeta");
				break;
			case LIBRARY_STATE_READY:
				Menu_DrawLibraryList();
				break;
		}

		Menu_DrawLibraryNotice();

		const char *play_hint = (state == LIBRARY_STATE_READY) ? "Reproducir" : "Empezar";
		const char *hints[] = { play_hint, NULL, "Triangulo - Reescanear", "Cuadrado - Carpeta", "SELECT - Ajustes" };
		NavRail_DrawHintBar(544 - UI_HINT_BAR_HEIGHT, hints, 5);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_LIBRARY);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		if (notice_frames > 0)
			notice_frames--;

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
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack()) {
			Touch_Reset();
			Menu_ShowNowPlaying();
			return;
		}

		Menu_HandleLibraryControls(state);

		if (pressed & SCE_CTRL_SELECT) {
			Menu_DisplaySettings();
			return;
		}

		if (pressed & SCE_CTRL_START)
			break;
	}
}
