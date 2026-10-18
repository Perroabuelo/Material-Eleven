#ifndef _ELEVENMPV_VIZ_SCOPE_H_
#define _ELEVENMPV_VIZ_SCOPE_H_

// Where the oscilloscope starts drawing in its window: the first rising zero
// crossing in the first half, so a periodic wave lands in the same phase every
// frame and stands still instead of sliding sideways.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_scope.c).

#define VIZ_SCOPE_WINDOW 2048   // frames read for the trigger
#define VIZ_SCOPE_SPAN   512    // frames drawn from it

// Index in `samples` (`frames` long) where a span starts: the first i in the
// first half with samples[i - 1] < 0 <= samples[i], or 0 if there is none or
// the span would not fit after it.
unsigned int VizScope_Trigger(const float *samples, unsigned int frames, unsigned int span);

#endif
