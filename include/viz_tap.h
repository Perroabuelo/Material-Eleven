#ifndef _ELEVENMPV_VIZ_TAP_H_
#define _ELEVENMPV_VIZ_TAP_H_

#include <stdint.h>

// The visualizer's copy of what the audio thread hands to the console: a ring
// of stereo int16 frames with one writer (the audio thread, in Audio_Decode)
// and one reader (the UI). No locks: the writer announces how far it is about
// to write, copies, and then publishes how far it wrote; the reader copies a
// window and throws it away if the writer may have reached it meanwhile.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_tap.c).

// Ring size in stereo frames: 32 KB, about 170 ms at 48 kHz, well past the
// longest window the visualizer reads plus the delay it reads with.
#define VIZ_TAP_FRAMES 8192

typedef struct {
	int16_t samples[VIZ_TAP_FRAMES * 2];
	volatile uint32_t begun;     // frames the writer has started to write
	volatile uint32_t written;   // frames fully written since the reset (a
	                             // track would need a day to wrap it)
	volatile uint32_t rate;      // sample rate of the frames written
} VizTap;

// The ring the audio thread and the UI share.
VizTap *VizTap_Shared(void);

// Forgets everything written. Only while no writer runs: Audio_Init calls it
// before starting the new track's audio thread.
void VizTap_Reset(VizTap *tap);

// Appends `frames` frames of `channels` interleaved samples (1 or 2) played at
// `rate`. A mono frame goes to both channels. Writes longer than the ring keep
// only their last VIZ_TAP_FRAMES frames.
void VizTap_Write(VizTap *tap, const int16_t *samples, unsigned int frames,
	unsigned int channels, unsigned int rate);

// Copies to `out` (frames * 2 samples) the `frames` stereo frames that end
// `delay` frames before the last one written, with silence for any part from
// before the reset. Returns 0, with `out` left undefined, when the writer may
// have overwritten the window during the copy: the caller keeps its previous
// frame. On success, *out_rate (if not NULL) is the rate published with the
// frames, 0 if nothing was written yet. `frames` must not exceed
// VIZ_TAP_FRAMES.
int VizTap_Read(const VizTap *tap, unsigned int delay, int16_t *out,
	unsigned int frames, unsigned int *out_rate);

#endif
