#ifndef _ELEVENMPV_FADE_H_
#define _ELEVENMPV_FADE_H_

#include <stdint.h>

// The gain the audio thread multiplies each buffer by: the short ramp of a
// pause or a resume, times the master gain the sleep timer lowers.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_fade.c). Only the audio thread touches a ramp (Audio_Decode).

typedef struct {
	float gain;          // 0..1, the gain of the last frame applied
	float target;        // 0 or 1
	float step;          // how much gain moves per frame, signed
	unsigned int left;   // frames until gain lands on target
} Fade_Ramp;

// A ramp resting at `gain`, with nothing to do.
void Fade_Reset(Fade_Ramp *r, float gain);

// Moves the ramp toward `target` from the gain it has now, at the slope that
// covers the whole way between 0 and 1 in `frames` frames: a ramp turned
// around halfway gets back in half the frames, with no jump. With `frames` 0
// the gain jumps to `target`.
void Fade_Start(Fade_Ramp *r, float target, unsigned int frames);

// Multiplies each of the `frames` frames of `channels` interleaved samples by
// gain x master. The gain moves one step before each frame, so after
// Fade_Start(r, t, n) from the far end the n-th frame is the first at `t`. The
// master goes linearly from `master_from` to `master_to` along the buffer,
// ending exactly at `master_to`, so a caller that passes the last buffer's
// `master_to` as the next one's `master_from` gets no jump between them.
// Rounds to nearest and saturates to int16. With gain and master at 1 the
// buffer is left as it is.
void Fade_Apply(Fade_Ramp *r, int16_t *buf, unsigned int frames, unsigned int channels,
	float master_from, float master_to);

// 1 once the gain is on the target.
int Fade_Done(const Fade_Ramp *r);

#endif
