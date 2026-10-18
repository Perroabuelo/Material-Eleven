#ifndef _ELEVENMPV_VIZ_METER_H_
#define _ELEVENMPV_VIZ_METER_H_

#include <stdint.h>

// What a bar or a VU needle shows of a level that changes every frame: it
// rises at once and falls exponentially, with a peak mark that holds the
// recent maximum for a moment and then drops. Every rate is in seconds, so 30
// and 60 fps fall alike. Also the per-channel RMS level the VU meters read.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_meter.c).

#define VIZ_METER_FALL      0.25f   // time constant of the fall, in seconds
#define VIZ_METER_PEAK_HOLD 0.8f    // seconds the peak mark stays put
#define VIZ_METER_PEAK_FALL 1.5f    // then it drops this much per second

#define VIZ_VU_FLOOR_DB     -48.0f
#define VIZ_VU_WINDOW_MS    50

typedef struct {
	float level;    // in [0, 1]
	float peak;     // in [level, 1]
	float hold;     // seconds left before the peak starts to drop
} VizMeter;

void VizMeter_Reset(VizMeter *meter);

// Moves the meter towards `target` (in [0, 1]) over `dt` seconds.
void VizMeter_Update(VizMeter *meter, float target, float dt);

// RMS of channel `channel` (0 left, 1 right) of `frames` stereo frames, in
// dB, clamped to [VIZ_VU_FLOOR_DB, 0].
float VizMeter_RmsDb(const int16_t *stereo, unsigned int frames, int channel);

// A VU level in dB as a share of the scale: 0 at the floor, 1 at 0 dBFS.
float VizMeter_VuNorm(float db);

#endif
