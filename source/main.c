#include <psp2/appmgr.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/shellutil.h>
#include <psp2/sysmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_displayfiles.h"
#include "textures.h"
#include "touch.h"
#include "ui_theme.h"
#include "utils.h"
#include "vitaaudiolib.h"

int main(int argc, char *argv[]) {
	vita2d_init();
	font = vita2d_load_font_file("app0:Roboto-Regular.ttf");
	UI_Theme_Load();
	Textures_Load();

	sceIoMkdir("ux0:data/ElevenMPV", 0777);
	Config_Load();
	Config_GetLastDirectory();
	sceAudioOutSetEffectType(config.eq_mode);

	Utils_InitAppUtil();
	SCE_CTRL_ENTER = Utils_GetEnterButton();
	SCE_CTRL_CANCEL = Utils_GetCancelButton();

	sceAppMgrAcquireBgmPort();

	Touch_Init();

	sceShellUtilInitEvents(0);
	sceSysmoduleLoadModule(SCE_SYSMODULE_MUSIC_EXPORT);
	Utils_InitPowerTick();

	Menu_DisplayFiles();

	sceSysmoduleUnloadModule(SCE_SYSMODULE_MUSIC_EXPORT);

	Touch_Shutdown();
	sceAppMgrReleaseBgmPort();
	Utils_TermAppUtil();

	Textures_Free();
	UI_Theme_Free();
	vita2d_free_font(font);
	vita2d_fini();

	sceKernelExitProcess(0);
	return 0;
}
