#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/shellutil.h>
#include <psp2/system_param.h>
#include <string.h>

#include "common.h"
#include "screen_off.h"

static SceCtrlData pad, old_pad;
// START apaga la pantalla desde cualquier pantalla, porque todas leen el pad
// por aqui. La regla vive en screen_off.c, que se prueba en PC.
static ScreenOff_State screen_off = { SCE_CTRL_START, SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER, 1, 0 };
static int lock_power = 0;

void Utils_SetMax(int *set, int value, int max) {
	if (*set > max)
		*set = value;
}

void Utils_SetMin(int *set, int value, int min) {
	if (*set < min)
		*set = value;
}

int Utils_ReadControls(void) {
	memset(&pad, 0, sizeof(SceCtrlData));
	sceCtrlPeekBufferPositive(0, &pad, 1);

	pressed = pad.buttons & ~old_pad.buttons;

	int request_off = 0;
	pressed = ScreenOff_Filter(&screen_off, pressed, pad.buttons, &request_off);
	if (request_off)
		scePowerRequestDisplayOff();

	// old_pad guarda el pad real, no el flanco filtrado: un boton que se
	// mantiene despues de encender la pantalla no vuelve a contar como pulsado.
	old_pad = pad;
	return 0;
}

void Utils_SetScreenOffEnabled(SceBool enabled) {
	screen_off.enabled = enabled ? 1 : 0;
	screen_off.armed = 0;
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

static int power_tick_thread(SceSize args, void *argp) {
	while (1) {
		if (lock_power > 0)
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

void Utils_LockPower(void) {
	if (!lock_power)
		sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

	lock_power++;
}

void Utils_UnlockPower(void) {
	if (lock_power)
		sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

	lock_power--;
	if (lock_power < 0)
		lock_power = 0;
}
