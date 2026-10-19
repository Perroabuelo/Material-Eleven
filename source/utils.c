#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/system_param.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "debug_log.h"
#include "key_repeat.h"
#include "menu_audioplayer.h"
#include "screen_off.h"
#include "suite.h"

static SceCtrlData pad, old_pad;
// START apaga la pantalla desde cualquier pantalla, porque todas leen el pad
// por aqui. La regla vive en screen_off.c, que se prueba en PC.
static ScreenOff_State screen_off = { SCE_CTRL_START, SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER, 1 };
// Holding Up or Down repeats the step, but only for lists, through
// Utils_ListStep: `pressed` stays an edge for everything else. The rule lives
// in key_repeat.c, which is tested on a PC.
static KeyRepeat_State key_repeat = { SCE_CTRL_UP, SCE_CTRL_DOWN, 0, 0, 0 };
static KeyRepeat_Step list_step;
// The buttons the user really held last frame, while a suite feeds the app
// its own: a new one stops the suite. All set at start, so one held since
// before the app opened does not count until it is pressed again.
static unsigned int old_physical = ~0u;

// Whether the app turned the screen off and the console has not turned it
// back on, so the visualizer can skip its work. The power callback clears it:
// on the console, PS turning the screen on arrives as APP_RESUMING, and the
// frame loop keeps its pace with the screen off, so there is no stall to
// notice it by instead. Without the callback the mark is never set.
static SceUID power_cb = -1;
static SceBool display_off = SCE_FALSE;

static int Utils_PowerCallback(int notifyId, int notifyCount, int notifyArg, void *common) {
	if (notifyArg & (SCE_POWER_CB_APP_RESUMING | SCE_POWER_CB_APP_RESUME))
		display_off = SCE_FALSE;

	return 0;
}

void Utils_SetMax(int *set, int value, int max) {
	if (*set > max)
		*set = value;
}

void Utils_SetMin(int *set, int value, int min) {
	if (*set < min)
		*set = value;
}

int Utils_ReadControls(void) {
	// Every screen, the search dialog and the library scans pass here once per
	// frame, outside drawing: the one place where a finished track moves the
	// queue on, whichever screen is open.
	Music_Update();
	Music_SleepTick();

	memset(&pad, 0, sizeof(SceCtrlData));
	sceCtrlPeekBufferPositive(0, &pad, 1);

	// Only the buttons the log names: PS sets a bit of its own, and closing
	// the app in the middle of a suite has to leave it interrupted, not
	// aborted.
	unsigned int physical = pad.buttons & DebugLog_NamedButtons();
	if (Suite_Running()) {
		if (physical & ~old_physical)
			Suite_Abort();
		else
			pad.buttons = Suite_Tick();
	}
	else {
		Suite_Ending(physical & ~old_physical);
	}
	old_physical = physical;

	pressed = pad.buttons & ~old_pad.buttons;

	// What the user pressed, before the screen-off rule decides what the app
	// gets to see.
	if (DebugLog_On())
		DebugLog_Input(pressed, old_pad.buttons & ~pad.buttons);

	// Runs the power callback on this thread, which created it.
	if (power_cb >= 0)
		sceKernelCheckCallback();

	ScreenOff_Action action = SCREEN_OFF_KEEP;
	pressed = ScreenOff_Filter(&screen_off, pressed, pad.buttons, &action);
	if (action == SCREEN_OFF_TURN_OFF) {
		scePowerRequestDisplayOff();
		if (power_cb >= 0)
			display_off = SCE_TRUE;
	}

	list_step = KeyRepeat_Update(&key_repeat, pad.buttons & ~SCE_CTRL_START, pressed, sceKernelGetProcessTimeWide());

	old_pad = pad;
	return 0;
}

void Utils_ListStep(int *sel, int count, int page) {
	*sel = KeyRepeat_Move(*sel, count, list_step, page);
}

KeyRepeat_Step Utils_GetListStep(void) {
	return list_step;
}

void Utils_InitPowerCallback(void) {
	power_cb = sceKernelCreateCallback("power_cb", 0, Utils_PowerCallback, NULL);

	if (power_cb >= 0 && scePowerRegisterCallback(power_cb) < 0) {
		sceKernelDeleteCallback(power_cb);
		power_cb = -1;
	}
}

void Utils_TermPowerCallback(void) {
	if (power_cb < 0)
		return;

	scePowerUnregisterCallback(power_cb);
	sceKernelDeleteCallback(power_cb);
	power_cb = -1;
	display_off = SCE_FALSE;
}

SceBool Utils_DisplayOff(void) {
	return display_off;
}

void Utils_SetScreenOffEnabled(SceBool enabled) {
	screen_off.enabled = enabled ? 1 : 0;
}

SceUInt32 Utils_HeldButtons(void) {
	return pad.buttons;
}

int Utils_InitAppUtil(void) {
	SceAppUtilInitParam init;
	SceAppUtilBootParam boot;
	memset(&init, 0, sizeof(SceAppUtilInitParam));
	memset(&boot, 0, sizeof(SceAppUtilBootParam));
	
	int ret = 0;
	
	if (R_FAILED(ret = sceAppUtilInit(&init, &boot)))
		return ret;

	if (R_FAILED(ret = sceAppUtilMusicMount()))
		return ret;
	
	return 0;
}

int Utils_TermAppUtil(void) {
	int ret = 0;

	if (R_FAILED(ret = sceAppUtilMusicUmount()))
	
	if (R_FAILED(ret = sceAppUtilShutdown()))
		return ret;
	
	return 0;
}

// Los diálogos del sistema no heredan las preferencias de la consola: hay que
// dárselas una vez, antes de abrir el primero. Sin esto el teclado se dibuja
// en el idioma y con la asignación de botones por defecto del diálogo, no con
// los que el usuario eligió y que el resto de la aplicación ya respeta.
void Utils_InitCommonDialog(void) {
	SceCommonDialogConfigParam param;
	int language = 0, enter_button = 0;

	sceCommonDialogConfigParamInit(&param);

	if (R_SUCCEEDED(sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &language)))
		param.language = (SceSystemParamLang)language;

	if (R_SUCCEEDED(sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button)))
		param.enterButtonAssign = (SceSystemParamEnterButtonAssign)enter_button;

	sceCommonDialogSetConfigParam(&param);
}

int Utils_GetEnterButton(void) {
	int button = 0;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button);
	
	if (button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
		return SCE_CTRL_CIRCLE;
	else
		return SCE_CTRL_CROSS;

	return 0;
}

int Utils_GetCancelButton(void) {
	int button = 0;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button);
	
	if (button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
		return SCE_CTRL_CROSS;
	else
		return SCE_CTRL_CIRCLE;

	return 0;
}

int Utils_Alphasort(const void *p1, const void *p2) {
	SceIoDirent *entryA = (SceIoDirent *) p1;
	SceIoDirent *entryB = (SceIoDirent *) p2;
	
	if ((SCE_S_ISDIR(entryA->d_stat.st_mode)) && !(SCE_S_ISDIR(entryB->d_stat.st_mode)))
		return -1;
	else if (!(SCE_S_ISDIR(entryA->d_stat.st_mode)) && (SCE_S_ISDIR(entryB->d_stat.st_mode)))
		return 1;
		
	return strcasecmp(entryA->d_name, entryB->d_name);
}

char *Utils_Basename(const char *filename) {
	char *p = strrchr (filename, '/');
	return p ? p + 1 : (char *) filename;
}

// Mientras suena un track la consola no se suspende sola, con la pantalla
// encendida o apagada; en pausa o sin nada cargado, si. Va separado a proposito
// del bloqueo del boton PS: tenerlos juntos hizo que quitar el bloqueo se
// llevara tambien esta senal. Las dos lecturas son flags simples, y una lectura
// desfasada dura a lo sumo un ciclo, muy por debajo del temporizador mas corto.
static int power_tick_thread(SceSize args, void *argp) {
	while (1) {
		if (Audio_HasTrack() && !Audio_IsPaused())
			sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);

		sceKernelDelayThread(10 * 1000 * 1000);
	}
	return 0;
}

void Utils_InitPowerTick(void) {
	SceUID thid = 0;
	if (R_SUCCEEDED(thid = sceKernelCreateThread("power_tick_thread", power_tick_thread, 0x10000100, 0x40000, 0, 0, NULL)))
		sceKernelStartThread(thid, 0, NULL);
}
