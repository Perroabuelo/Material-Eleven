#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
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
#define UI_DEBUG_PANEL_H 92
#define UI_DEBUG_PANEL_PAD 12
#define UI_DEBUG_LINE_H 20
#define UI_DEBUG_PANEL_BG RGBA8(0x00, 0x00, 0x00, 220)

static SceBool debug_visible = SCE_FALSE;
static SceBool combo_was_held = SCE_FALSE;

// Lifetime counters. The pool watermark is the smallest free space seen since
// startup, so a frame that nearly ran out still shows up long after it drew.
static unsigned int pool_low_water = 0xFFFFFFFFu;
static unsigned int pool_exhaustions = 0;
static unsigned int mem_sample_countdown = 0;
static int free_user_kb = 0, free_cdram_kb = 0;

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

SceBool UI_Debug_IsVisible(void) {
	return debug_visible;
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

void UI_Debug_Update(void) {
	SceCtrlData pad;
	SceBool held;

	memset(&pad, 0, sizeof(pad));
	sceCtrlPeekBufferPositive(0, &pad, 1);

	// Rising edge of the whole combo, so holding it does not strobe the panel.
	held = ((pad.buttons & UI_DEBUG_TOGGLE_COMBO) == UI_DEBUG_TOGGLE_COMBO);
	if (held && !combo_was_held) {
		debug_visible = !debug_visible;
		mem_sample_countdown = 0; // refresh the moment it comes up
	}
	combo_was_held = held;

	// The pool is tracked whether or not the panel is up: the watermark is only
	// worth anything if it covers the frames drawn before anyone looked.
	unsigned int free_space = vita2d_pool_free_space();
	if (free_space < pool_low_water)
		pool_low_water = free_space;

	if (!debug_visible)
		return;

	if (mem_sample_countdown == 0) {
		UI_Debug_SampleMemory();
		mem_sample_countdown = UI_DEBUG_MEM_SAMPLE_FRAMES;
	}
	mem_sample_countdown--;
}

void UI_Debug_Draw(void) {
	char line[64];
	float x = UI_DEBUG_PANEL_X + UI_DEBUG_PANEL_PAD;
	float y = UI_DEBUG_PANEL_Y + 6;

	if (!debug_visible)
		return;

	UI_DrawRoundedRect(UI_DEBUG_PANEL_X, UI_DEBUG_PANEL_Y, UI_DEBUG_PANEL_W, UI_DEBUG_PANEL_H, UI_RADIUS_SM, UI_DEBUG_PANEL_BG);

	snprintf(line, sizeof(line), "LIBRE  user %d KB   cdram %d KB", free_user_kb, free_cdram_kb);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, line, y, UI_DEBUG_LINE_H),
		UI_COLOR_TEXT_PRIMARY, line);
	y += UI_DEBUG_LINE_H;

	if (pool_low_water == 0xFFFFFFFFu)
		snprintf(line, sizeof(line), "POOL   marca de agua  -");
	else
		snprintf(line, sizeof(line), "POOL   marca de agua %u B", pool_low_water);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, line, y, UI_DEBUG_LINE_H),
		UI_COLOR_TEXT_PRIMARY, line);
	y += UI_DEBUG_LINE_H;

	snprintf(line, sizeof(line), "POOL   agotado %u veces", pool_exhaustions);
	// Red the moment it is not zero: a non-zero count means geometry was
	// dropped on some frame, which is otherwise invisible.
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, line, y, UI_DEBUG_LINE_H),
		pool_exhaustions ? UI_COLOR_TRACKER : UI_COLOR_TEXT_PRIMARY, line);
	y += UI_DEBUG_LINE_H;

	snprintf(line, sizeof(line), "L + R + SELECT  para ocultar");
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, line, y, UI_DEBUG_LINE_H),
		UI_COLOR_TEXT_MUTED, line);
}
