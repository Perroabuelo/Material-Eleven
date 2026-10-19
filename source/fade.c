#include "fade.h"

void Fade_Reset(Fade_Ramp *r, float gain) {
	r->gain = gain;
	r->target = gain;
	r->step = 0.0f;
	r->left = 0;
}

void Fade_Start(Fade_Ramp *r, float target, unsigned int frames) {
	float distance = target - r->gain;
	if (distance < 0.0f)
		distance = -distance;

	r->target = target;

	// Counted in whole frames, so the ramp lands on the target exactly
	// instead of wherever the float steps add up to.
	unsigned int left = (unsigned int)(distance * (float)frames + 0.5f);
	if (left == 0) {
		r->gain = target;
		r->step = 0.0f;
		r->left = 0;
		return;
	}

	r->step = (target - r->gain) / (float)left;
	r->left = left;
}

static inline int16_t scale(int16_t sample, float gain) {
	float v = (float)sample * gain;
	v += (v >= 0.0f) ? 0.5f : -0.5f;

	if (v >= 32767.0f)
		return 32767;
	if (v <= -32768.0f)
		return -32768;

	return (int16_t)v;
}

void Fade_Apply(Fade_Ramp *r, int16_t *buf, unsigned int frames, unsigned int channels,
	float master_from, float master_to) {
	// The normal case while playing: nothing to multiply.
	if (r->left == 0 && r->gain == 1.0f && master_from == 1.0f && master_to == 1.0f)
		return;

	float master_step = (frames > 0) ? (master_to - master_from) / (float)frames : 0.0f;

	for (unsigned int i = 0; i < frames; i++) {
		if (r->left > 0) {
			r->left--;
			r->gain = (r->left == 0) ? r->target : r->gain + r->step;
		}

		float master = (i + 1 == frames) ? master_to : master_from + master_step * (float)(i + 1);
		float g = r->gain * master;

		for (unsigned int c = 0; c < channels; c++)
			buf[i * channels + c] = scale(buf[i * channels + c], g);
	}
}

int Fade_Done(const Fade_Ramp *r) {
	return r->left == 0;
}
