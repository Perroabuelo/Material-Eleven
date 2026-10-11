#include <stddef.h>

#include "seek_bar.h"

void SeekBar_Reset(SeekBar_State *state) {
	state->grabbed = 0;
	state->fraction = 0.0f;
}

float SeekBar_FractionAt(const SeekBar_Geometry *geo, float x) {
	if (!(geo->w > 0.0f))
		return 0.0f;

	float f = (x - geo->x) / geo->w;

	if (!(f > 0.0f))
		return 0.0f;

	return (f > 1.0f) ? 1.0f : f;
}

static int SeekBar_Hit(const SeekBar_Geometry *geo, float x, float y) {
	return geo->w > 0.0f && x >= geo->x && x <= geo->x + geo->w && y >= geo->hit_top && y <= geo->hit_bottom;
}

SeekBar_Result SeekBar_Update(SeekBar_State *state, const SeekBar_Geometry *geo,
	const SeekBar_Input *in, double *out_fraction) {
	if (!state->grabbed) {
		if (!in->pressed || !SeekBar_Hit(geo, in->x, in->y))
			return SEEKBAR_IDLE;

		state->grabbed = 1;
		state->fraction = SeekBar_FractionAt(geo, in->x);
		return SEEKBAR_DRAGGING;
	}

	// Once grabbed only x counts: the finger may wander over the cover or the
	// transport and keep dragging. The release frame keeps the last held x.
	if (in->held && !in->released) {
		state->fraction = SeekBar_FractionAt(geo, in->x);
		return SEEKBAR_DRAGGING;
	}

	state->grabbed = 0;

	if (out_fraction != NULL)
		*out_fraction = state->fraction;

	return SEEKBAR_COMMIT;
}

float SeekBar_DisplayFraction(const SeekBar_State *state, float playback_fraction) {
	return state->grabbed ? state->fraction : playback_fraction;
}
