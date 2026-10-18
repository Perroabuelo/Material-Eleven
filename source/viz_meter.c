#include <math.h>

#include "viz_meter.h"

void VizMeter_Reset(VizMeter *meter) {
	meter->level = 0.0f;
	meter->peak = 0.0f;
	meter->hold = 0.0f;
}

void VizMeter_Update(VizMeter *meter, float target, float dt) {
	if (target < 0.0f)
		target = 0.0f;
	else if (target > 1.0f)
		target = 1.0f;
	if (dt < 0.0f)
		dt = 0.0f;

	if (target >= meter->level)
		meter->level = target;
	else
		meter->level = target + (meter->level - target) * expf(-dt / VIZ_METER_FALL);

	if (meter->level >= meter->peak) {
		meter->peak = meter->level;
		meter->hold = VIZ_METER_PEAK_HOLD;
		return;
	}

	// The part of dt past the hold is time spent falling.
	float fall = dt - meter->hold;
	meter->hold = fall > 0.0f ? 0.0f : -fall;

	if (fall > 0.0f) {
		meter->peak -= VIZ_METER_PEAK_FALL * fall;
		if (meter->peak < meter->level)
			meter->peak = meter->level;
	}
}

float VizMeter_RmsDb(const int16_t *stereo, unsigned int frames, int channel) {
	if (frames == 0)
		return VIZ_VU_FLOOR_DB;

	double sum = 0.0;
	for (unsigned int i = 0; i < frames; i++) {
		double s = stereo[i * 2 + channel] / 32768.0;
		sum += s * s;
	}

	double rms = sqrt(sum / frames);
	const double floor_linear = pow(10.0, VIZ_VU_FLOOR_DB / 20.0);

	if (rms <= floor_linear)
		return VIZ_VU_FLOOR_DB;

	float db = (float)(20.0 * log10(rms));
	return db > 0.0f ? 0.0f : db;
}

float VizMeter_VuNorm(float db) {
	float n = 1.0f - db / VIZ_VU_FLOOR_DB;
	return n < 0.0f ? 0.0f : (n > 1.0f ? 1.0f : n);
}
