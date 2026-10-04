#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/shellutil.h>
#include <psp2/system_param.h>
#include <psp2/sysmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "lang.h"
#include "cover.h"
#include "library.h"
#include "menu_audioplayer.h"
#include "menu_displayfiles.h"
#include "menu_library.h"
#include "menu_settings.h"
#include "nav_request.h"
#include "touch.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"
#include "vitaaudiolib.h"

// The frame's vertex pool. vita2d_init() defaults to 1 MB; the vector chrome
// this skin draws - rounded rects as arc fans, rings, strokes - spends far more
// per frame than the textures it replaced, and the battery and row icons still
// to come spend more again. Sized explicitly here so the margin is a decision
// rather than a default, and watched through the exhaustion counter.
#define UI_VERTEX_POOL_SIZE (2 * 1024 * 1024)

// Multisampling is requested at init and nowhere else: vita2d builds the render
// target with it and cannot change it afterwards. 4x is the console's top mode,
// and the tile architecture resolves it in tile memory, so the expected cost is
// low - but it does enlarge the render target, and that competes with the font
// atlases for the same CDRAM.
//
// The mode cannot be chosen by trying it and checking: disassembling
// libvita2d.a shows vita2d_init_advanced_with_msaa returning a literal 1 on
// both of its exit paths, ignoring every sceGxm error along the way, so its
// return value carries no information about whether the mode was established.
// Degradation therefore has to be decided BEFORE the call, from the memory
// actually free, rather than after it from a result that is always success.
//
// The thresholds are deliberately generous. Nothing has been allocated at this
// point in startup, so a healthy console takes the 4x branch every time; the
// lower branches exist so that a console which cannot afford the mode still
// boots with aliased edges instead of failing to come up.
#define UI_MSAA_4X_MIN_CDRAM_KB (32 * 1024)
#define UI_MSAA_2X_MIN_CDRAM_KB (16 * 1024)

static void UI_InitGraphics(void) {
	SceKernelFreeMemorySizeInfo mem;
	int cdram_kb = 0;

	memset(&mem, 0, sizeof(mem));
	mem.size = sizeof(mem);

	if (sceKernelGetFreeMemorySize(&mem) >= 0)
		cdram_kb = mem.size_cdram / 1024;

	if (cdram_kb >= UI_MSAA_4X_MIN_CDRAM_KB) {
		vita2d_init_advanced_with_msaa(UI_VERTEX_POOL_SIZE, SCE_GXM_MULTISAMPLE_4X);
		UI_Debug_SetGraphicsMode("MSAA 4x", cdram_kb);
	}
	else if (cdram_kb >= UI_MSAA_2X_MIN_CDRAM_KB) {
		vita2d_init_advanced_with_msaa(UI_VERTEX_POOL_SIZE, SCE_GXM_MULTISAMPLE_2X);
		UI_Debug_SetGraphicsMode("MSAA 2x", cdram_kb);
	}
	else {
		vita2d_init_advanced(UI_VERTEX_POOL_SIZE);
		UI_Debug_SetGraphicsMode("sin MSAA", cdram_kb);
	}
}

int main(int argc, char *argv[]) {
	UI_Debug_MarkStackBase();
	UI_InitGraphics();
	UI_Theme_Load();

	sceIoMkdir("ux0:data/ElevenMPV", 0777);
	Config_Load();
	Config_GetLastDirectory();

	// La carpeta de escaneo primero: el indice se descarta si habla de
	// otra, asi que sin ella cargada no se puede decidir.
	Library_LoadRoot();
	Library_Load();
	sceAudioOutSetEffectType(config.eq_mode);

	Utils_InitAppUtil();
	SCE_CTRL_ENTER = Utils_GetEnterButton();
	SCE_CTRL_CANCEL = Utils_GetCancelButton();
	// La misma preferencia que las dos líneas de arriba, dicha ahora también al
	// subsistema de diálogos, que no la hereda por su cuenta.
	Utils_InitCommonDialog();

	// El idioma de la consola solo se puede leer con AppUtil ya iniciado, y hay
	// que resolverlo antes de Menu_DisplayFiles(): nada se dibuja hasta ahi, asi
	// que no queda ningun fotograma en el idioma equivocado. Si no se puede leer,
	// -1 no es espanol y la app sale en ingles.
	int system_lang = -1;
	if (R_FAILED(sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &system_lang)))
		system_lang = -1;
	Lang_SetSystemLanguage(system_lang);
	Lang_Apply(config.language);

	sceAppMgrAcquireBgmPort();

	Touch_Init();

	sceShellUtilInitEvents(0);
	sceSysmoduleLoadModule(SCE_SYSMODULE_MUSIC_EXPORT);
	sceSysmoduleLoadModule(SCE_SYSMODULE_IME);
	Utils_InitPowerTick();

	// Every screen runs until the user picks another one and returns it, and
	// only this loop opens the next. A screen that opened the next one itself
	// would never return, so each jump would leave its frame on the stack for
	// good and a long session would overflow it.
	UI_Screen next = UI_SCREEN_FOLDERS;
	while (next != UI_SCREEN_NONE) {
		// Once per jump and in one place: a touch that changed screen must not
		// count on the new one, and a request nobody took must not leak into it.
		Touch_Reset();
		NavRequest_Take();

		switch (next) {
			case UI_SCREEN_FOLDERS:
				next = Menu_DisplayFiles();
				break;
			case UI_SCREEN_LIBRARY:
				next = Menu_DisplayLibrary();
				break;
			case UI_SCREEN_SETTINGS:
				next = Menu_DisplaySettings();
				break;
			case UI_SCREEN_NOW_PLAYING:
				next = Audio_HasTrack() ? Menu_ShowNowPlaying() : UI_SCREEN_FOLDERS;
				break;
			default:
				next = UI_SCREEN_FOLDERS;
				break;
		}
	}

	// No screen returns UI_SCREEN_NONE yet: START turns the screen off instead of
	// exiting, and the app is closed from the PS button, which ends the
	// process without unwinding. The teardown stays for any future way out,
	// and a track may still be loaded then, since playback survives
	// navigating away from Now Playing.
	if (Audio_HasTrack()) {
		Audio_Stop();
		Audio_Term();
	}

	sceSysmoduleUnloadModule(SCE_SYSMODULE_IME);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_MUSIC_EXPORT);

	Touch_Shutdown();
	sceAppMgrReleaseBgmPort();
	Utils_TermAppUtil();

	Cover_Free();
	Library_Free();
	UI_Debug_Free();
	UI_Theme_Free();
	vita2d_fini();

	sceKernelExitProcess(0);
	return 0;
}
