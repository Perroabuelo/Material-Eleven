#include <psp2/ctrl.h>
#include <psp2/ime_dialog.h>
#include <string.h>

#include "lang.h"
#include "nav_rail.h"
#include "search_dialog.h"
#include "text_encoding.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

// ---- Salidas del lazo del diálogo ----
//
// Mientras gira, el lazo del teclado sustituye al lazo de fotogramas de la
// pantalla que lo abrió: nada más de la aplicación dibuja ni lee la entrada.
// Un diálogo que no salga de SCE_COMMON_DIALOG_STATUS_RUNNING se lleva por
// delante la aplicación entera, que es exactamente como se envió la pantalla
// de carpetas. Tres salidas, de la más precisa a la más tosca.
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

// Lo que el teclado deja escribir, en unidades UTF-16.
#define SEARCH_DIALOG_MAX_UNITS 63

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

// Lo que dibuja el fondo mientras el diálogo gira; solo vale durante
// SearchDialog_Prompt.
static void (*dialog_background)(void);
static UI_Screen dialog_rail_screen;

// Un parámetro rechazado tumba la apertura entera y el buscador no abre nada,
// que fue lo que pasó al pedir un atenuador propio. El fondo transparente sí
// se acepta, pero se pide con red: si alguna consola lo rechazara, se reintenta
// sin él, porque el teclado importa más que el fondo sobre el que se dibuja.
static int SearchDialog_Open(SceImeDialogParam *param) {
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
// origen a estas salidas: cuando el diálogo NO compone, esto es lo único que
// queda en pantalla, y entonces la diferencia entre la pantalla con su leyenda
// de salida y un color plano es la diferencia entre poder salir o no.
//
// Por eso la leyenda de botones se sustituye aquí por la del abandono.
static void SearchDialog_DrawFrame(void) {
	vita2d_clear_screen();
	dialog_background();

	const NavRail_Hint hints[] = {
		{ { HINT_BTN_L, HINT_BTN_R, HINT_BTN_START }, 1, Lang_Get(STR_HINT_CANCEL_SEARCH) },
	};
	NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, 1);

	// El riel se dibuja para que la pantalla siga completa, y su resultado se
	// descarta: detrás del teclado no se acciona nada.
	NavRail_DrawAndHitTest(dialog_rail_screen);
}

// Cierre ordenado: se pide el aborto y se espera de forma acotada a que el
// diálogo deje de estar en ejecución, en vez de terminarlo por debajo.
static void SearchDialog_Abandon(void) {
	sceImeDialogAbort();

	for (int i = 0; i < FILTER_DRAIN_FRAMES && sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING; i++) {
		UI_GpuBeginFrame();
		SearchDialog_DrawFrame();
		vita2d_end_drawing();
		vita2d_common_dialog_update();
		vita2d_swap_buffers();
	}
}

SearchDialog_Result SearchDialog_Prompt(const char *title_utf8, const char *current_utf8,
	char *out, size_t out_cap, void (*draw_background)(void), UI_Screen rail_screen) {
	// The IME takes and returns UTF-16 and the application keeps UTF-8, so
	// every text crosses whole: an accented letter is one code point each way,
	// never a byte cast to a unit or a unit cut down to a byte.
	SceWChar16 title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
	TextEncoding_Utf8ToUtf16(title_utf8, title, SCE_IME_DIALOG_MAX_TITLE_LENGTH);
	SceWChar16 initial[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
	SceWChar16 input[SCE_IME_DIALOG_MAX_TEXT_LENGTH];

	TextEncoding_Utf8ToUtf16(current_utf8, initial, SEARCH_DIALOG_MAX_UNITS + 1);
	memcpy(input, initial, sizeof(initial));

	SceImeDialogParam param;
	sceImeDialogParamInit(&param);
	param.dialogMode = SCE_IME_DIALOG_DIALOG_MODE_WITH_CANCEL;
	param.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_WITH_CLEAR;
	param.title = title;
	param.maxTextLength = SEARCH_DIALOG_MAX_UNITS;
	param.initialText = initial;
	param.inputTextBuffer = input;
	if (SearchDialog_Open(&param) < 0)
		return SEARCH_DIALOG_FAILED;

	dialog_background = draw_background;
	dialog_rail_screen = rail_screen;

	SceBool abandoned = SCE_FALSE, ever_composed = SCE_FALSE;
	int compose_fails = 0, combo_frames = 0, frames = 0;

	// Con el teclado delante START no apaga la pantalla: L + R + START es el
	// abandono, y la pantalla de fondo no recibe ninguna otra orden.
	Utils_SetScreenOffEnabled(SCE_FALSE);

	while (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) {
		UI_GpuBeginFrame();
		SearchDialog_DrawFrame();
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

	SearchDialog_Result outcome = SEARCH_DIALOG_CANCELLED;

	// Al abandonar, el resultado no se mira: quien llamó deja todo como estaba.
	if (abandoned) {
		SearchDialog_Abandon();
		outcome = SEARCH_DIALOG_ABANDONED;
	}
	else if (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_FINISHED) {
		SceImeDialogResult result;
		memset(&result, 0, sizeof(result));
		sceImeDialogGetResult(&result);

		if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
			TextEncoding_Utf16ToUtf8(input, out, out_cap);
			outcome = SEARCH_DIALOG_CONFIRMED;
		}
	}

	sceImeDialogTerm();

	return outcome;
}
