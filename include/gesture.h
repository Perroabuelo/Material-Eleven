#ifndef _ELEVENMPV_GESTURE_H_
#define _ELEVENMPV_GESTURE_H_

// Tells a tap from a drag on one finger. Fed one frame at a time with whether
// the finger is down, where it is and how long the frame took, it reports the
// landing (to highlight what is under it), each frame of a drag, a tap when
// the finger lifts without having moved past the drag threshold, a hold when
// it stays still long enough (only for screens that ask for it), and the
// speed at which a drag was let go (to start inertia).
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_gesture.c). The screen builds the input from touch.c and only
// feeds it the touches that started on its list.

// How far, in screen pixels, the finger may move from where it landed and
// still count as a tap.
#define GESTURE_DRAG_THRESHOLD 16.0f

// Frames of movement averaged for the release speed, so one noisy sample
// cannot start a fling.
#define GESTURE_SPEED_SAMPLES 4

// How long, in ms, a finger stays still before it counts as a hold, for the
// screens that ask for holds (Gesture_Input.hold_ms).
#define GESTURE_HOLD_MS 500

typedef enum {
	GESTURE_NONE,       // no finger, or a finger holding still before the threshold
	GESTURE_PRESS,      // the finger just landed at (start_x, start_y)
	GESTURE_DRAG,       // the finger is dragging: it moved `dy` this frame
	GESTURE_TAP,        // lifted without dragging; landed at start, lifted at end
	GESTURE_HOLD,       // still at (start_x, start_y) for hold_ms; once per touch
	GESTURE_RELEASE     // lifted after a drag (or a caught or held tap) at `velocity`
} Gesture_Event;

typedef struct {
	int down;           // finger on the panel this frame
	float x, y;         // its position (ignored when up)
	float dt_ms;        // time since the previous frame
	float hold_ms;      // how long still makes a hold; 0 never reports one
} Gesture_Input;

typedef struct {
	Gesture_Event type;
	float dy;                   // DRAG: vertical movement this frame, + is down
	float start_x, start_y;     // where the finger landed
	float end_x, end_y;         // TAP/RELEASE: where it was last seen
	float velocity;             // RELEASE: px/ms over the last frames, + is down
} Gesture_Result;

typedef struct {
	int down;
	int dragging;
	int caught;
	float held_ms;      // time down without dragging, for the hold
	float start_x, start_y;
	float last_x, last_y;
	float sample_dy[GESTURE_SPEED_SAMPLES];
	float sample_dt[GESTURE_SPEED_SAMPLES];
	int sample_next;
	int sample_count;
} Gesture_State;

// Forgets the finger without reporting anything. Used when the screen or the
// view changes under it.
void Gesture_Reset(Gesture_State *state);

// Marks the current touch as the one that stopped a moving list: lifting it
// reports GESTURE_RELEASE at zero speed instead of GESTURE_TAP, and it never
// becomes a hold.
void Gesture_Catch(Gesture_State *state);

// True while a finger is down and has not started dragging or been caught:
// what is under it should look pressed.
int Gesture_Pressing(const Gesture_State *state);

// Advances the recogniser by one frame. A hold marks the touch as caught, so
// lifting it reports GESTURE_RELEASE and not GESTURE_TAP; dragging after it
// still drags.
Gesture_Result Gesture_Update(Gesture_State *state, const Gesture_Input *in);

#endif
