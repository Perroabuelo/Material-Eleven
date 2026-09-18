#include <psp2/appmgr.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/shellutil.h>
#include <psp2/sysmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_displayfiles.h"
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
	UI_InitGraphics();
	UI_Theme_Load();

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
	sceSysmoduleLoadModule(SCE_SYSMODULE_IME);
	Utils_InitPowerTick();

	Menu_DisplayFiles();

	// Playback now survives navigating away from Now Playing, so a track
	// may still be loaded here if the user exits (START) while it plays
	// in the background; tear it down before the process ends.
	if (Audio_HasTrack()) {
		Audio_Stop();
		Audio_Term();
	}

	sceSysmoduleUnloadModule(SCE_SYSMODULE_IME);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_MUSIC_EXPORT);

	Touch_Shutdown();
	sceAppMgrReleaseBgmPort();
	Utils_TermAppUtil();

	UI_Debug_Free();
	UI_Theme_Free();
	vita2d_fini();

	sceKernelExitProcess(0);
	return 0;
}
