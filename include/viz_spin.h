#ifndef _ELEVENMPV_VIZ_SPIN_H_
#define _ELEVENMPV_VIZ_SPIN_H_

// How the vinyl turns: at 33 1/3 rpm while the music plays, slowing to a stop
// in a couple of seconds on pause and picking up speed again on resume, never
// jumping. The angle stays within [0, 2*pi) so it keeps its precision however
// long the record spins.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_spin.c).

#define VIZ_SPIN_RPM   (100.0f / 3.0f)
#define VIZ_SPIN_TAU   0.5f     // time constant of the speed change, in seconds
#define VIZ_SPIN_SNAP  0.05f    // rad/s from the target where it lands on it

typedef struct {
	float speed;    // rad/s
	float angle;    // rad, in [0, 2*pi)
} VizSpin;

void VizSpin_Reset(VizSpin *spin);

// Advances the record `dt` seconds, playing or paused.
void VizSpin_Update(VizSpin *spin, int playing, float dt);

// The speed at 33 1/3 rpm, in rad/s.
float VizSpin_TargetSpeed(void);

#endif
