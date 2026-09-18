#ifndef _ELEVENMPV_UI_GPU_H_
#define _ELEVENMPV_UI_GPU_H_

#include <psp2/types.h>
#include <vita2d.h>

// GPU resource lifetime and observability. Everything here exists because the
// same class of bug landed twice during vectorize-ui-controls (ac8f53d,
// f3d908e): the CPU released or reused memory the GPU was still reading. The
// lesson was not that the synchronisation was missing but that it sat in the
// wrong place, so these are single points of passage rather than conventions.

// ---- Single destruction point ----
//
// Every texture and every font in the application is released through one of
// these two and through nothing else; no other translation unit calls
// vita2d_free_texture or vita2d_free_font. Both wait for pending rendering
// first and free after, unconditionally - the wait is never nested inside a
// branch, which is exactly the shape that produced f3d908e, where it only ran
// when the outgoing track happened to carry cover art.
//
// Both take the owning pointer and NULL it, so a released handle cannot reach
// a later draw call.
void UI_GpuFreeTexture(vita2d_texture **texture);
void UI_GpuFreeFont(vita2d_font **font);

// ---- Per-frame vertex pool ----
//
// The only route geometry takes to vita2d_draw_array. vita2d hands the pointer
// straight to sceGxmSetVertexStream without copying, and the GPU reads it when
// the frame is flushed, so vertices must live in GPU-visible memory that
// outlives the call - never on the stack, which is what ac8f53d got wrong.
// Returns NULL when the frame's pool is exhausted, and records that, so a shape
// that does not fit shows up as a counter in the overlay instead of silently
// vanishing.
void *UI_GpuPoolAlloc(unsigned int size, unsigned int alignment);

// Draws `texture` at (x, y), or nothing at all when it is NULL. A resource that
// failed to load leaves a gap instead of taking the frame down with it.
void UI_GpuDrawTexture(vita2d_texture *texture, float x, float y);

// ---- Debug overlay ----
//
// Free memory, the frame pool's low-water mark and the exhaustion counter.
// Toggled at runtime by holding L + R + SELECT; off at startup, so it costs
// nothing in normal use.
#define UI_DEBUG_TOGGLE_COMBO (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER | SCE_CTRL_SELECT)

// Once per frame, before drawing: samples the pad and flips the overlay.
void UI_Debug_Update(void);
// Once per frame, last of all, so the panel sits on top of the screen below it.
void UI_Debug_Draw(void);
SceBool UI_Debug_IsVisible(void);

#endif
