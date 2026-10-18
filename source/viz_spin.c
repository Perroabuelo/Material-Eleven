#include <math.h>

#include "viz_spin.h"

#define TWO_PI_F 6.28318530718f

void VizSpin_Reset(VizSpin *spin) {
	spin->speed = 0.0f;
	spin->angle = 0.0f;
}

float VizSpin_TargetSpeed(void) {
	return VIZ_SPIN_RPM * TWO_PI_F / 60.0f;
}

void VizSpin_Update(VizSpin *spin, int playing, float dt) {
	if (dt <= 0.0f)
		return;

	float target = playing ? VizSpin_TargetSpeed() : 0.0f;

	spin->speed = target + (spin->speed - target) * expf(-dt / VIZ_SPIN_TAU);

	// The exponential never arrives; this close, the eye cannot tell.
	if (fabsf(spin->speed - target) < VIZ_SPIN_SNAP)
		spin->speed = target;

	spin->angle = fmodf(spin->angle + spin->speed * dt, TWO_PI_F);
	if (spin->angle < 0.0f)
		spin->angle += TWO_PI_F;
}
