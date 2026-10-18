#ifndef _ELEVENMPV_VIZ_DRAW_H_
#define _ELEVENMPV_VIZ_DRAW_H_

#include "viz_nav.h"

// The visualizer's frame: reads from the ring (viz_tap.h) the window that is
// being heard, analyses it (viz_fft, viz_bands, viz_meter, viz_scope) and
// draws the style in the area given, in the track's accent color. All its
// state is static and fixed in size: it allocates nothing.

// What the console still holds after the two buffers the audio thread keeps,
// in frames. Tuned on the console with pulsos.wav (design.md, D2).
#define VIZ_HW_LATENCY_FRAMES 960

// Analyses the frame that is being heard and draws `style` in the area at
// (x, y) of w x h. `dt` is the time since the last frame, in seconds;
// `playing` is false while paused, for the vinyl.
void VizDraw_Frame(Viz_Style style, float x, float y, float w, float h, float dt, int playing);

#endif
