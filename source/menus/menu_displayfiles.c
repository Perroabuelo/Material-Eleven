#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "dirbrowse.h"
#include "lang.h"
#include "menu_audioplayer.h"
#include "menu_library.h"
#include "mini_player.h"
#include "menu_settings.h"
#include "nav_rail.h"
#include "nav_request.h"
#include "search_dialog.h"
#include "status_bar.h"
#include "touch.h"
#include "touch_list.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

#define CONTENT_X     (UI_RAIL_WIDTH)
#define TOPBAR_H      64
#define FILTER_W      240
#define FILTER_H      40

static float Menu_FilterBoxX(void) { return 960 - 22 - FILTER_W; }
static float Menu_FilterBoxY(void) { return (TOPBAR_H - FILTER_H) / 2.0f; }

// El desplazamiento de la lista, compartido por la pantalla y por la eleccion
// de carpeta, que dibujan lo mismo.
static TouchList folders_list;

// El listado que se dibujo la ultima vez. Entrar o salir de una carpeta y
// poner o quitar el filtro lo cambian, se haga desde donde se haga: la lista
// vuelve arriba, sin dedo ni deslizamiento, mostrando la seleccion.
static char shown_cwd[sizeof(cwd)] = "";
static char shown_filter[64] = "";

static void Menu_FoldersFollowListing(void) {
	if (strcmp(shown_cwd, cwd) == 0 && strcmp(shown_filter, Dirbrowse_GetFilter()) == 0)
		return;

	snprintf(shown_cwd, sizeof(shown_cwd), "%s", cwd);
	snprintf(shown_filter, sizeof(shown_filter), "%s", Dirbrowse_GetFilter());
	TouchList_Reset(&folders_list);
	TouchList_Follow(&folders_list);
}

// Un boton de direccion detiene el deslizamiento y la vista vuelve a la
// seleccion, aunque el dedo la hubiera dejado fuera. `before` es la seleccion
// antes de leer los botones.
static void Menu_FoldersFollowButtons(int before) {
	if (position != before || (pressed & (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)))
		TouchList_Follow(&folders_list);
}

// El cuerpo de la pantalla de carpetas, sin barra de botones ni riel: lo
// comparten el lazo normal y el fondo del diálogo, que deben mostrar lo mismo.
static void Menu_DrawFoldersContent(void);

// El filtro cambia solo con un término confirmado: descartar, abandonar o no
// poder abrir el diálogo dejan la carpeta como estaba.
static void Menu_PromptFilter(void) {
	char term[64];

	if (SearchDialog_Prompt(Lang_Get(STR_SEARCH_FOLDER), Dirbrowse_GetFilter(), term, sizeof(term),
		Menu_DrawFoldersContent, UI_SCREEN_FOLDERS) != SEARCH_DIALOG_CONFIRMED)
		return;

	if (term[0] != '\0')
		Dirbrowse_SetFilter(term);
	else
		Dirbrowse_ClearFilter();
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

	const char *filter_text = Dirbrowse_HasFilter() ? Dirbrowse_GetFilter() : Lang_Get(STR_SEARCH_FOLDER);
	unsigned int filter_color = Dirbrowse_HasFilter() ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_MUTED;
	UI_DrawText(UI_FACE_UI, UI_TS_LABEL, fx + 14, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, fy, FILTER_H), filter_color, filter_text);
}

static void Menu_DrawFoldersContent(void) {
	Menu_FoldersFollowListing();
	StatusBar_Display();
	Menu_DrawTopBar();
	Dirbrowse_DisplayFiles(&folders_list);
	MiniPlayer_Draw();
}

static void Menu_HandleControls(void) {
	int visible_count = Dirbrowse_GetVisibleCount();

	if (visible_count > 0) {
		int before = position;
		Utils_ListStep(&position, visible_count, FILES_PER_PAGE);

		if (pressed & SCE_CTRL_LEFT)
			position = 0;
		else if (pressed & SCE_CTRL_RIGHT)
			position = visible_count - 1;

		Menu_FoldersFollowButtons(before);

		if (pressed & SCE_CTRL_ENTER)
			Dirbrowse_OpenFile();
	}

	// El filtro es un estado superpuesto a la carpeta, así que cancelar deshace
	// lo más reciente primero y solo después sube. Es además la única salida
	// por botón físico cuando un filtro sin coincidencias deja la lista vacía
	// en la raíz, donde no hay carpeta superior a la que volver y por tanto no
	// había ninguna: el filtro solo podía quitarse por toque.
	if (pressed & SCE_CTRL_CANCEL) {
		if (Dirbrowse_HasFilter())
			Dirbrowse_ClearFilter();
		else if (strcmp(cwd, root_path) != 0) {
			Dirbrowse_Navigate(SCE_TRUE);
			Dirbrowse_PopulateFiles(SCE_TRUE);
		}
	}
}

// Modo seleccion de carpeta. No es un selector nuevo: es esta misma pantalla,
// su misma navegacion y su mismo dibujado, con la accion de confirmar cambiada
// y sin ninguna ruta hacia la reproduccion. Escribir un selector propio habria
// duplicado navegacion, dibujado y entrada que aqui ya estaban resueltos.
SceBool Menu_PickFolder(char *out, int cap) {
	char saved_cwd[512];
	char saved_filter[64];
	int saved_position = position;
	SceBool picked = SCE_FALSE;

	snprintf(saved_cwd, sizeof(saved_cwd), "%s", cwd);
	snprintf(saved_filter, sizeof(saved_filter), "%s", Dirbrowse_GetFilter());

	Dirbrowse_ClearFilter();
	Dirbrowse_PopulateFiles(SCE_TRUE);
	Touch_Reset();
	TouchList_Reset(&folders_list);
	TouchList_Follow(&folders_list);

	while (SCE_TRUE) {
		UI_GpuBeginFrame();
		vita2d_clear_screen();

		Menu_DrawFoldersContent();

		const char *back_hint = Lang_Get((strcmp(cwd, root_path) != 0) ? STR_PARENT_FOLDER : STR_HINT_CANCEL);
		const NavRail_Hint hints[] = {
			{ { HINT_BTN_CONFIRM }, 0, Lang_Get(STR_HINT_ENTER) },
			{ { HINT_BTN_CANCEL }, 0, back_hint },
			{ { HINT_BTN_TRIANGLE }, 0, Lang_Get(STR_HINT_CHOOSE_FOLDER) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};
		NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, 4);

		// El rail se dibuja pero no navega: salir de aqui a media eleccion dejaria
		// el navegador movido de donde estaba.
		NavRail_DrawAndHitTest(UI_SCREEN_LIBRARY);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		int visible_count = Dirbrowse_GetVisibleCount();
		int tapped_row = TouchList_Update(&folders_list, SCE_TRUE, NULL);

		if (visible_count > 0) {
			int before = position;
			Utils_ListStep(&position, visible_count, FILES_PER_PAGE);
			Menu_FoldersFollowButtons(before);
		}

		// Tocar una entrada es seleccionarla y confirmar.
		SceBool tapped = tapped_row >= 0 && tapped_row < visible_count;

		if (tapped)
			position = tapped_row;

		if (tapped || (pressed & SCE_CTRL_ENTER)) {
			File *file = Dirbrowse_GetFileIndex(position);

			// Sobre una carpeta, confirmar entra; sobre cualquier otra cosa elige
			// la carpeta actual. Lo que no hace nunca es reproducir.
			if (file != NULL && file->is_dir) {
				if (R_SUCCEEDED(Dirbrowse_Navigate(SCE_FALSE)))
					Dirbrowse_PopulateFiles(SCE_TRUE);
			}
			else {
				picked = SCE_TRUE;
				break;
			}
		}

		if (pressed & SCE_CTRL_TRIANGLE) {
			picked = SCE_TRUE;
			break;
		}

		if (pressed & SCE_CTRL_CANCEL) {
			if (strcmp(cwd, root_path) != 0) {
				Dirbrowse_Navigate(SCE_TRUE);
				Dirbrowse_PopulateFiles(SCE_TRUE);
			}
			else
				break;
		}
	}

	if (picked)
		snprintf(out, cap, "%s", cwd);

	// El navegador vuelve exactamente a donde estaba, se haya elegido o no:
	// library/index pide que escanear no altere donde esta parado.
	snprintf(cwd, sizeof(cwd), "%s", saved_cwd);
	Dirbrowse_PopulateFiles(SCE_TRUE);
	Dirbrowse_SetFilter(saved_filter);
	position = saved_position;
	Touch_Reset();
	TouchList_Reset(&folders_list);
	TouchList_Follow(&folders_list);

	return picked;
}

UI_Screen Menu_DisplayFiles(void) {
	Dirbrowse_PopulateFiles(SCE_FALSE);
	vita2d_set_clear_color(UI_COLOR_BG);

	// Al volver, la entrada elegida se ve, y no sigue ningun dedo ni
	// deslizamiento de la vez anterior.
	TouchList_Reset(&folders_list);
	TouchList_Follow(&folders_list);

	while (SCE_TRUE) {
		UI_GpuBeginFrame();
		vita2d_clear_screen();

		Menu_DrawFoldersContent();

		// La leyenda anuncia lo que el botón hace ahora mismo, no lo que suele
		// hacer: con un filtro puesto cancelar lo quita, y en la raíz sin
		// filtro no hay nada que cancelar. Abrir tampoco se anuncia cuando el
		// filtro no dejó ninguna entrada sobre la que actuar.
		const char *open_hint = Dirbrowse_GetVisibleCount() > 0 ? Lang_Get(STR_HINT_OPEN_PLAY) : NULL;
		const char *back_hint = Dirbrowse_HasFilter() ? Lang_Get(STR_HINT_CLEAR_FILTER)
			: ((strcmp(cwd, root_path) != 0) ? Lang_Get(STR_PARENT_FOLDER) : NULL);

		const NavRail_Hint hints[] = {
			{ { HINT_BTN_CONFIRM }, 0, open_hint },
			{ { HINT_BTN_CANCEL }, 0, back_hint },
			{ { HINT_BTN_TRIANGLE }, 0, Lang_Get(STR_HINT_SEARCH) },
			{ { HINT_BTN_SELECT }, 0, Lang_Get(STR_HINT_SETTINGS) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};
		NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, 5);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_FOLDERS);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		// La lista toma solo los toques que empiezan en ella: el riel, la
		// barra superior y el mini reproductor quedan fuera de su area.
		int tapped_row = TouchList_Update(&folders_list, SCE_TRUE, NULL);

		if (tapped == UI_SCREEN_SETTINGS || tapped == UI_SCREEN_LIBRARY)
			return tapped;
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack())
			return tapped;

		if (MiniPlayer_HandleTouch())
			continue;

		// Triángulo y la caja abren el mismo filtro.
		if ((pressed & SCE_CTRL_TRIANGLE) || Touch_Position(Menu_FilterBoxX(), Menu_FilterBoxY(), Menu_FilterBoxX() + FILTER_W, Menu_FilterBoxY() + FILTER_H)) {
			Menu_PromptFilter();
			continue;
		}

		// Tocar una entrada es seleccionarla y confirmar: entra en la carpeta
		// (tambien en ..) o abre el archivo.
		if (tapped_row >= 0 && tapped_row < Dirbrowse_GetVisibleCount()) {
			position = tapped_row;
			Dirbrowse_OpenFile();
		}
		else
			Menu_HandleControls();

		// Abrir un archivo de audio no entra a Reproduciendo: lo pide.
		UI_Screen requested = NavRequest_Take();
		if (requested != UI_SCREEN_NONE)
			return requested;

		if (pressed & SCE_CTRL_SELECT)
			return UI_SCREEN_SETTINGS;
	}
}
