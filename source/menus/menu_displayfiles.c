#include <psp2/ime_dialog.h>
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
#include "status_bar.h"
#include "touch.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

#define CONTENT_X     (UI_RAIL_WIDTH)
#define TOPBAR_H      64
#define FILTER_W      240
#define FILTER_H      40

static float Menu_FilterBoxX(void) { return 960 - 22 - FILTER_W; }
static float Menu_FilterBoxY(void) { return (TOPBAR_H - FILTER_H) / 2.0f; }

// ---- Salidas del lazo del diálogo ----
//
// Mientras gira, el lazo del teclado sustituye al lazo de fotogramas de
// Menu_DisplayFiles: nada más de la aplicación dibuja ni lee la entrada. Un
// diálogo que no salga de SCE_COMMON_DIALOG_STATUS_RUNNING se lleva por
// delante la aplicación entera, que es exactamente como se envió esta
// pantalla. Tres salidas, de la más precisa a la más tosca.
//
// 1. El código de error de la composición. vita2d_common_dialog_update hace
//    una llamada de cola a sceCommonDialogUpdate y devuelve su error sin
//    tocarlo, así que este es el defecto en sí y no una inferencia sobre él:
//    componer con una escena abierta falla en todos y cada uno de los
//    fotogramas. Se piden varios seguidos para no abandonar por un tropiezo.
#define FILTER_COMPOSE_FAILS 8
// 2. El abandono que pide el usuario. L y R juntos no aparecen al escribir, y
//    START los aísla de cualquier pulsación suelta; el teclado del sistema no
//    usa ninguno de los tres. El mantenido evita el disparo accidental.
#define FILTER_ABORT_COMBO  (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER | SCE_CTRL_START)
#define FILTER_ABORT_FRAMES 90
// 3. La última red, por si ni la composición informa ni el pad llega. Holgada
//    a propósito: cortarle la escritura a alguien lento convertiría un
//    bloqueo en una pérdida de texto silenciosa.
#define FILTER_MAX_FRAMES 7200
// Abortar es una petición, no un cierre: hay que dejar que el diálogo salga de
// RUNNING antes de terminarlo, y esa espera también va acotada.
#define FILTER_DRAIN_FRAMES 60

// El cuerpo de la pantalla de carpetas, sin barra de botones ni riel: lo
// comparten el lazo normal y el fondo del diálogo, que deben mostrar lo mismo.
static void Menu_DrawFoldersContent(void);

// El teclado del sistema tapa la pantalla que hay debajo, y no se consiguió
// que dejara de hacerlo. Lo comprobado en consola, por si alguien lo reintenta:
//
//   bgColor {0,0,0,0}    aceptado, y aun así no se ve nada debajo
//   dimmerColor a 160    rechazado, SCE_COMMON_DIALOG_ERROR_INVALID_DIMMER_COLOR
//   dimmerColor NULL     aceptado, y es el que sigue tapando
//
// O sea que lo que cubre es el atenuador propio del diálogo, y los valores que
// acepta para uno ajeno quedaron sin averiguar. El fondo transparente se queda
// puesto porque es correcto y no cuesta nada, no porque se note.
//
// Es un puntero que el diálogo conserva, así que el almacenamiento vive fuera
// de la función que abre el teclado y no en su marco de pila.
static SceCommonDialogColor filter_bg_color = { 0, 0, 0, 0 };

// Un parámetro rechazado tumba la apertura entera y el buscador no abre nada,
// que fue lo que pasó al pedir un atenuador propio. El fondo transparente sí
// se acepta, pero se pide con red: si alguna consola lo rechazara, se reintenta
// sin él, porque el teclado importa más que el fondo sobre el que se dibuja.
static int Menu_OpenFilterDialog(SceImeDialogParam *param) {
	param->commonParam.bgColor = &filter_bg_color;

	int ret = sceImeDialogInit(param);
	if (ret >= 0)
		return ret;

	param->commonParam.bgColor = NULL;

	return sceImeDialogInit(param);
}

// El fotograma que se dibuja mientras el teclado está delante: la misma
// pantalla que el usuario tenía, no un color plano.
//
// Con el teclado compuesto no se ve nada de esto, porque el diálogo lo cubre
// (ver arriba). Se sigue dibujando por el caso contrario, que es el que dio
// origen a este change: cuando el diálogo NO compone, esto es lo único que
// queda en pantalla, y entonces la diferencia entre la carpeta con su leyenda
// de salida y un color plano es la diferencia entre poder salir o no.
//
// Por eso la leyenda de botones se sustituye aquí por la del abandono.
static void Menu_DrawFilterFrame(void) {
	vita2d_clear_screen();
	Menu_DrawFoldersContent();

	const NavRail_Hint hints[] = {
		{ { HINT_BTN_L, HINT_BTN_R, HINT_BTN_START }, 1, Lang_Get(STR_HINT_CANCEL_SEARCH) },
	};
	NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, 1);

	// El riel se dibuja para que la pantalla siga completa, y su resultado se
	// descarta: detrás del teclado no se acciona nada.
	NavRail_DrawAndHitTest(UI_SCREEN_FOLDERS);
}

// Cierre ordenado: se pide el aborto y se espera de forma acotada a que el
// diálogo deje de estar en ejecución, en vez de terminarlo por debajo.
static void Menu_AbandonFilterDialog(void) {
	sceImeDialogAbort();

	for (int i = 0; i < FILTER_DRAIN_FRAMES && sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING; i++) {
		vita2d_start_drawing();
		Menu_DrawFilterFrame();
		vita2d_end_drawing();
		vita2d_common_dialog_update();
		vita2d_swap_buffers();
	}
}

// The IME takes its title in UTF-16 and the string table is UTF-8. Every text
// in the table sits in the BMP, so a code point is always one UTF-16 unit; a
// four-byte or malformed sequence is skipped rather than guessed at.
static void Menu_Utf8ToUtf16(const char *src, SceWChar16 *dst, int cap) {
	const unsigned char *p = (const unsigned char *)src;
	int n = 0;

	while (*p && n < cap - 1) {
		if (p[0] < 0x80) {
			dst[n++] = p[0];
			p += 1;
		}
		else if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
			dst[n++] = (SceWChar16)(((p[0] & 0x1F) << 6) | (p[1] & 0x3F));
			p += 2;
		}
		else if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
			dst[n++] = (SceWChar16)(((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F));
			p += 3;
		}
		else
			p++;
	}

	dst[n] = 0;
}

static void Menu_PromptFilter(void) {
	SceWChar16 title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
	Menu_Utf8ToUtf16(Lang_Get(STR_SEARCH_FOLDER), title, SCE_IME_DIALOG_MAX_TITLE_LENGTH);
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
	if (Menu_OpenFilterDialog(&param) < 0)
		return;

	SceBool abandoned = SCE_FALSE, ever_composed = SCE_FALSE;
	int compose_fails = 0, combo_frames = 0, frames = 0;

	// Con el teclado delante START no apaga la pantalla: L + R + START es el
	// abandono, y la pantalla de fondo no recibe ninguna otra orden.
	Utils_SetScreenOffEnabled(SCE_FALSE);

	while (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) {
		vita2d_start_drawing();
		Menu_DrawFilterFrame();
		vita2d_end_drawing();
		// Detrás del cierre de la escena y delante de la presentación: la
		// composición escribe en el buffer de pantalla, no en la escena, así
		// que pedirla con una escena abierta es inválido y falla en silencio.
		// Ese era el defecto, y esta línea es la corrección.
		int composed = vita2d_common_dialog_update();
		vita2d_swap_buffers();

		// El pad se lee aquí y solo para el abandono: la pantalla de fondo no
		// procesa entrada mientras el teclado está delante, porque el usuario
		// no puede ver qué está pulsando.
		Utils_ReadControls();

		// Solo cuenta el fallo de un diálogo que no ha llegado a componerse ni
		// una vez. Abandonar uno que ya se veía sería peor que no hacer nada:
		// no se le puede cerrar sin componerlo, y un diálogo del sistema vivo
		// se queda con la entrada, así que la red dejaría la aplicación sin
		// navegación en lugar de salvarla. Comprobado en consola sobre el build
		// con el defecto: sale del lazo, y la navegación no vuelve.
		if (composed >= 0)
			ever_composed = SCE_TRUE;

		compose_fails = (composed < 0 && !ever_composed) ? compose_fails + 1 : 0;
		combo_frames = ((Utils_HeldButtons() & FILTER_ABORT_COMBO) == FILTER_ABORT_COMBO) ? combo_frames + 1 : 0;
		frames++;

		if ((compose_fails >= FILTER_COMPOSE_FAILS) || (combo_frames >= FILTER_ABORT_FRAMES) || (frames >= FILTER_MAX_FRAMES)) {
			abandoned = SCE_TRUE;
			break;
		}
	}

	Utils_SetScreenOffEnabled(SCE_TRUE);

	// Al abandonar, el resultado no se mira: el filtro queda como estaba.
	if (abandoned)
		Menu_AbandonFilterDialog();
	else if (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_FINISHED) {
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

	const char *filter_text = Dirbrowse_HasFilter() ? Dirbrowse_GetFilter() : Lang_Get(STR_SEARCH_FOLDER);
	unsigned int filter_color = Dirbrowse_HasFilter() ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_MUTED;
	UI_DrawText(UI_FACE_UI, UI_TS_LABEL, fx + 14, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, fy, FILTER_H), filter_color, filter_text);
}

static void Menu_DrawFoldersContent(void) {
	StatusBar_Display();
	Menu_DrawTopBar();
	Dirbrowse_DisplayFiles();
	MiniPlayer_Draw();
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

	while (SCE_TRUE) {
		vita2d_start_drawing();
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

		if (visible_count > 0) {
			if (pressed & SCE_CTRL_UP)
				position--;
			else if (pressed & SCE_CTRL_DOWN)
				position++;

			Utils_SetMax(&position, 0, visible_count - 1);
			Utils_SetMin(&position, visible_count - 1, 0);
		}

		if (pressed & SCE_CTRL_ENTER) {
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

	return picked;
}

void Menu_DisplayFiles(void) {
	Dirbrowse_PopulateFiles(SCE_FALSE);
	vita2d_set_clear_color(UI_COLOR_BG);

	while (SCE_TRUE) {
		vita2d_start_drawing();
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
			{ { HINT_BTN_SELECT }, 0, Lang_Get(STR_HINT_SETTINGS) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};
		NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, 4);

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
		else if (tapped == UI_SCREEN_LIBRARY) {
			Menu_DisplayLibrary();
			return;
		}
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack()) {
			Menu_ShowNowPlaying();
			return;
		}

		if (MiniPlayer_HandleTouch())
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
	}
}
