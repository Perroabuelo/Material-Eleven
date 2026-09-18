#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/pvf.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "ui_gpu.h"
#include "ui_theme.h"

// Re-sampled every this many frames. Reading it is a syscall, and the numbers
// only move when a texture or a font is created or destroyed, so once every
// couple of seconds is plenty and keeps the overlay off the frame budget.
#define UI_DEBUG_MEM_SAMPLE_FRAMES 120

#define UI_DEBUG_PANEL_X (UI_RAIL_WIDTH + 10)
#define UI_DEBUG_PANEL_Y 10
#define UI_DEBUG_PANEL_W 316
#define UI_DEBUG_PANEL_H 112
#define UI_DEBUG_PANEL_PAD 12
#define UI_DEBUG_LINE_H 20
#define UI_DEBUG_PANEL_BG RGBA8(0x00, 0x00, 0x00, 220)

static UI_DebugMode debug_mode = UI_DEBUG_OFF;
static SceBool combo_was_held = SCE_FALSE;

// Lifetime counters. The pool watermark is the smallest free space seen since
// startup, so a frame that nearly ran out still shows up long after it drew.
static unsigned int pool_low_water = 0xFFFFFFFFu;
static unsigned int pool_exhaustions = 0;
static unsigned int mem_sample_countdown = 0;
static int free_user_kb = 0, free_cdram_kb = 0;
static const char *graphics_mode = "?";
static int cdram_kb_at_init = 0;

void UI_GpuFreeTexture(vita2d_texture **texture) {
	if (!texture || !*texture)
		return;

	vita2d_wait_rendering_done();
	vita2d_free_texture(*texture);
	*texture = NULL;
}

void UI_GpuFreeFont(vita2d_font **font) {
	if (!font || !*font)
		return;

	vita2d_wait_rendering_done();
	vita2d_free_font(*font);
	*font = NULL;
}

void UI_GpuFreePvf(vita2d_pvf **font) {
	if (!font || !*font)
		return;

	vita2d_wait_rendering_done();
	vita2d_free_pvf(*font);
	*font = NULL;
}

void *UI_GpuPoolAlloc(unsigned int size, unsigned int alignment) {
	void *p = vita2d_pool_memalign(size, alignment);

	if (!p)
		pool_exhaustions++;

	return p;
}

void UI_GpuDrawTexture(vita2d_texture *texture, float x, float y) {
	if (!texture)
		return;

	vita2d_draw_texture(texture, x, y);
}

void UI_Debug_SetGraphicsMode(const char *mode, int cdram_kb) {
	graphics_mode = mode ? mode : "?";
	cdram_kb_at_init = cdram_kb;
}

SceBool UI_Debug_IsVisible(void) {
	return debug_mode != UI_DEBUG_OFF;
}

static void UI_Debug_SampleMemory(void) {
	SceKernelFreeMemorySizeInfo info;

	memset(&info, 0, sizeof(info));
	info.size = sizeof(info);

	if (sceKernelGetFreeMemorySize(&info) < 0)
		return;

	free_user_kb = info.size_user / 1024;
	free_cdram_kb = info.size_cdram / 1024;
}


// ---- Glyph probe (task 5.1) ----
//
// vita2d_load_system_pvf takes no size: it calls scePvfSetCharSize with a
// constant baked into the library, so every handle rasterises at one size,
// about 18 px at the resolution vita2d sets. A fallback can therefore be drawn
// at neutral scale for exactly one token, and every other token is a rescale -
// which is the thing this whole change exists to stop doing. The probe shows
// both so the trade can be judged by looking at it.
// The system PVF rasterises at one size and only one: vita2d_load_system_pvf
// takes no size argument and calls scePvfSetCharSize with a constant, which at
// the resolution vita2d sets works out near this. Confirmed on hardware - a
// sample drawn at scale 1.0 is crisp and the same sample at 1.67 is visibly
// soft - so every token's usable scale is its pixel size over this number.
#define UI_PVF_NATIVE_PX 18.0f

#define UI_PROBE_PANEL_X (UI_RAIL_WIDTH + 10)
#define UI_PROBE_PANEL_Y 8
#define UI_PROBE_PANEL_W 800
#define UI_PROBE_PANEL_H 400
#define UI_PROBE_ROW_H   30

static vita2d_pvf *probe_pvf = NULL;
static SceBool probe_attempted = SCE_FALSE;

static int UI_ProbeIsLatin(unsigned int c) { return c < 0x0500; }
static int UI_ProbeIsHangul(unsigned int c) {
	return (c >= 0x1100 && c <= 0x11FF) || (c >= 0x3130 && c <= 0x318F) || (c >= 0xAC00 && c <= 0xD7A3);
}
static int UI_ProbeIsKana(unsigned int c) { return (c >= 0x3040 && c <= 0x30FF) || (c >= 0x31F0 && c <= 0x31FF); }
static int UI_ProbeIsHan(unsigned int c) {
	return (c >= 0x3400 && c <= 0x4DBF) || (c >= 0x4E00 && c <= 0x9FFF) || (c >= 0xF900 && c <= 0xFAFF);
}

// The same multi-font group a real fallback would ask for, so the probe tests
// the per-codepoint dispatch too and not just whether some font exists.
static const vita2d_system_pvf_config probe_configs[] = {
	{ SCE_PVF_LANGUAGE_LATIN, UI_ProbeIsLatin },
	{ SCE_PVF_LANGUAGE_K,     UI_ProbeIsHangul },
	{ SCE_PVF_LANGUAGE_J,     UI_ProbeIsKana },
	{ SCE_PVF_LANGUAGE_C,     UI_ProbeIsHan },
};
#define UI_PROBE_CONFIG_COUNT ((int)(sizeof(probe_configs) / sizeof(probe_configs[0])))

static const struct {
	const char *label;
	const char *sample;
} probe_samples[] = {
	{ "latin",    "Abc 123" },
	{ "cirilico", "\u0417\u0434\u0440\u0430\u0432" },
	{ "coreano",  "\uba5c\ub85c\ub9dd\uc2a4" },
	{ "japones",  "\u3053\u3093\u306b\u3061\u306f" },
	{ "chino",    "\u4f60\u597d\u4e16\u754c" },
};
#define UI_PROBE_SAMPLE_COUNT ((int)(sizeof(probe_samples) / sizeof(probe_samples[0])))

// Where user content is actually drawn, which is the only place a fallback
// would ever be needed. Chrome is the application's own text and always Latin.
static const struct {
	UI_TextSize ts;
	const char *where;
} probe_tokens[] = {
	{ UI_TS_BADGE,   "artista (mini)" },
	{ UI_TS_LABEL,   "subtitulo, siguientes" },
	{ UI_TS_BODY,    "nombre de archivo, artista" },
	{ UI_TS_TITLE,   "-" },
	{ UI_TS_DISPLAY, "titulo en Now Playing" },
};
#define UI_PROBE_TOKEN_COUNT ((int)(sizeof(probe_tokens) / sizeof(probe_tokens[0])))

// Called from UI_Debug_Update, never from inside a frame.
static void UI_Debug_LoadProbeFont(void) {
	if (probe_attempted)
		return;

	probe_attempted = SCE_TRUE;
	probe_pvf = vita2d_load_system_pvf(UI_PROBE_CONFIG_COUNT, probe_configs);
}

static void UI_Debug_DrawGlyphProbe(void) {
	char line[80];
	float x = UI_PROBE_PANEL_X + 12;
	float y = UI_PROBE_PANEL_Y + 6;
	float col_own = x + 96, col_sys = x + 300;

	UI_DrawRoundedRect(UI_PROBE_PANEL_X, UI_PROBE_PANEL_Y, UI_PROBE_PANEL_W, UI_PROBE_PANEL_H,
		UI_RADIUS_SM, UI_DEBUG_PANEL_BG);

	// ---- What the firmware covers (task 5.1) ----
	snprintf(line, sizeof(line), "COBERTURA  -  sistema %s, escala neutra",
		probe_pvf ? "cargado" : "NO DISPONIBLE");
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_PROBE_ROW_H),
		UI_COLOR_TEXT_MUTED, line);
	y += UI_PROBE_ROW_H;

	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, col_own, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, 20),
		UI_COLOR_TEXT_MUTED, "propia");
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, col_sys, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, 20),
		UI_COLOR_TEXT_MUTED, "sistema");
	y += 22;

	for (int i = 0; i < UI_PROBE_SAMPLE_COUNT; i++) {
		int baseline = UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, UI_PROBE_ROW_H);

		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, baseline, UI_COLOR_TEXT_TERTIARY, probe_samples[i].label);
		UI_DrawText(UI_FACE_UI, UI_TS_BODY, col_own, baseline, UI_COLOR_TEXT_PRIMARY, probe_samples[i].sample);

		if (probe_pvf)
			vita2d_pvf_draw_text(probe_pvf, (int)col_sys, baseline, UI_COLOR_TEXT_PRIMARY,
				1.0f, probe_samples[i].sample);

		y += UI_PROBE_ROW_H;
	}

	// ---- The same sample at every token's required scale ----
	y += 8;
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_PROBE_ROW_H),
		UI_COLOR_TEXT_MUTED, "ESCALA POR TOKEN  -  donde aparece contenido del usuario");
	y += UI_PROBE_ROW_H;

	for (int i = 0; i < UI_PROBE_TOKEN_COUNT; i++) {
		UI_TextSize ts = probe_tokens[i].ts;
		float scale = (float)ui_text_px[ts] / UI_PVF_NATIVE_PX;
		int baseline = UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, UI_PROBE_ROW_H);

		snprintf(line, sizeof(line), "%2u px  x%.2f", ui_text_px[ts], (double)scale);
		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, baseline, UI_COLOR_TEXT_TERTIARY, line);

		if (probe_pvf)
			vita2d_pvf_draw_text(probe_pvf, (int)col_own, baseline, UI_COLOR_TEXT_PRIMARY,
				scale, probe_samples[2].sample); // the Korean sample

		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, col_sys + 60, baseline, UI_COLOR_TEXT_MUTED, probe_tokens[i].where);
		y += UI_PROBE_ROW_H;
	}

	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_PROBE_ROW_H),
		UI_COLOR_TEXT_MUTED, "L + R + SELECT  para cerrar");
}

void UI_Debug_Update(void) {
	SceCtrlData pad;
	SceBool held;

	memset(&pad, 0, sizeof(pad));
	sceCtrlPeekBufferPositive(0, &pad, 1);

	// Rising edge of the whole combo, so holding it does not strobe the panel.
	held = ((pad.buttons & UI_DEBUG_TOGGLE_COMBO) == UI_DEBUG_TOGGLE_COMBO);
	if (held && !combo_was_held) {
		debug_mode = (debug_mode + 1) % UI_DEBUG_MODE_COUNT;
		mem_sample_countdown = 0; // refresh the moment it comes up

		// Loading the fallback creates a GPU texture, so it happens here -
		// after vita2d_end_drawing and vita2d_swap_buffers, outside any frame -
		// and never from inside UI_Debug_Draw.
		if (debug_mode == UI_DEBUG_GLYPHS)
			UI_Debug_LoadProbeFont();
	}
	combo_was_held = held;

	// The pool is tracked whether or not the panel is up: the watermark is only
	// worth anything if it covers the frames drawn before anyone looked.
	unsigned int free_space = vita2d_pool_free_space();
	if (free_space < pool_low_water)
		pool_low_water = free_space;

	if (debug_mode != UI_DEBUG_STATS)
		return;

	if (mem_sample_countdown == 0) {
		UI_Debug_SampleMemory();
		mem_sample_countdown = UI_DEBUG_MEM_SAMPLE_FRAMES;
	}
	mem_sample_countdown--;
}

void UI_Debug_Free(void) {
	UI_GpuFreePvf(&probe_pvf);
}

void UI_Debug_Draw(void) {
	char line[64];
	float x = UI_DEBUG_PANEL_X + UI_DEBUG_PANEL_PAD;
	float y = UI_DEBUG_PANEL_Y + 6;

	if (debug_mode == UI_DEBUG_GLYPHS) {
		UI_Debug_DrawGlyphProbe();
		return;
	}

	if (debug_mode != UI_DEBUG_STATS)
		return;

	UI_DrawRoundedRect(UI_DEBUG_PANEL_X, UI_DEBUG_PANEL_Y, UI_DEBUG_PANEL_W, UI_DEBUG_PANEL_H, UI_RADIUS_SM, UI_DEBUG_PANEL_BG);

	snprintf(line, sizeof(line), "LIBRE  user %d KB   cdram %d KB", free_user_kb, free_cdram_kb);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_DEBUG_LINE_H),
		UI_COLOR_TEXT_PRIMARY, line);
	y += UI_DEBUG_LINE_H;

	if (pool_low_water == 0xFFFFFFFFu)
		snprintf(line, sizeof(line), "POOL   marca de agua  -");
	else
		snprintf(line, sizeof(line), "POOL   marca de agua %u B", pool_low_water);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_DEBUG_LINE_H),
		UI_COLOR_TEXT_PRIMARY, line);
	y += UI_DEBUG_LINE_H;

	snprintf(line, sizeof(line), "POOL   agotado %u veces", pool_exhaustions);
	// Red the moment it is not zero: a non-zero count means geometry was
	// dropped on some frame, which is otherwise invisible.
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_DEBUG_LINE_H),
		pool_exhaustions ? UI_COLOR_TRACKER : UI_COLOR_TEXT_PRIMARY, line);
	y += UI_DEBUG_LINE_H;

	snprintf(line, sizeof(line), "GFX    %s   cdram al init %d KB", graphics_mode, cdram_kb_at_init);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_DEBUG_LINE_H),
		UI_COLOR_TEXT_PRIMARY, line);
	y += UI_DEBUG_LINE_H;

	snprintf(line, sizeof(line), "L + R + SELECT  para la sonda de glifos");
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y, UI_DEBUG_LINE_H),
		UI_COLOR_TEXT_MUTED, line);
}
