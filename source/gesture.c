#include <string.h>

#include "gesture.h"

void Gesture_Reset(Gesture_State *state) {
	memset(state, 0, sizeof(*state));
}

void Gesture_Catch(Gesture_State *state) {
	if (state->down)
		state->caught = 1;
}

int Gesture_Pressing(const Gesture_State *state) {
	return state->down && !state->dragging && !state->caught;
}

static void Gesture_AddSample(Gesture_State *state, float dy, float dt_ms) {
	state->sample_dy[state->sample_next] = dy;
	state->sample_dt[state->sample_next] = dt_ms;
	state->sample_next = (state->sample_next + 1) % GESTURE_SPEED_SAMPLES;
	if (state->sample_count < GESTURE_SPEED_SAMPLES)
		state->sample_count++;
}

static float Gesture_Speed(const Gesture_State *state) {
	float dy = 0.0f, dt = 0.0f;

	for (int i = 0; i < state->sample_count; i++) {
		dy += state->sample_dy[i];
		dt += state->sample_dt[i];
	}

	return dt > 0.0f ? dy / dt : 0.0f;
}

Gesture_Result Gesture_Update(Gesture_State *state, const Gesture_Input *in) {
	Gesture_Result res;
	memset(&res, 0, sizeof(res));
	res.type = GESTURE_NONE;

	if (!in->down) {
		if (!state->down)
			return res;

		res.start_x = state->start_x;
		res.start_y = state->start_y;
		res.end_x = state->last_x;
		res.end_y = state->last_y;

		if (state->dragging) {
			res.type = GESTURE_RELEASE;
			res.velocity = Gesture_Speed(state);
		}
		else if (state->caught)
			res.type = GESTURE_RELEASE;
		else
			res.type = GESTURE_TAP;

		Gesture_Reset(state);
		return res;
	}

	if (!state->down) {
		Gesture_Reset(state);
		state->down = 1;
		state->start_x = state->last_x = in->x;
		state->start_y = state->last_y = in->y;

		res.type = GESTURE_PRESS;
		res.start_x = in->x;
		res.start_y = in->y;
		return res;
	}

	res.start_x = state->start_x;
	res.start_y = state->start_y;

	// What the finger moved this frame, for the speed. The catch-up of the
	// first drag frame below is not speed.
	float frame_dy = in->y - state->last_y;

	if (!state->dragging) {
		float dx = in->x - state->start_x;
		float dy = in->y - state->start_y;

		if (dx * dx + dy * dy <= GESTURE_DRAG_THRESHOLD * GESTURE_DRAG_THRESHOLD) {
			// Still a tap: the list stays put, so the movement up to here
			// is reported with the first drag frame instead.
			state->last_x = in->x;
			state->last_y = in->y;

			if (in->hold_ms > 0.0f && !state->caught) {
				state->held_ms += in->dt_ms;

				if (state->held_ms >= in->hold_ms) {
					state->caught = 1;
					res.type = GESTURE_HOLD;
				}
			}

			return res;
		}

		state->dragging = 1;
		// The first drag frame carries everything moved since landing, so
		// the list catches up with the finger.
		state->last_y = state->start_y;
	}

	res.type = GESTURE_DRAG;
	res.dy = in->y - state->last_y;
	Gesture_AddSample(state, frame_dy, in->dt_ms);

	state->last_x = in->x;
	state->last_y = in->y;
	return res;
}
