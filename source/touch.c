/*
		
		Taken from VitaComix by Arkanite.
		A manga/comic book reader for the Playstation Vita
		
		File: touch.c
		
*/
#include <psp2/touch.h>
#include <string.h>

#include "debug_log.h"
#include "suite.h"
#include "touch.h"

#define lerp(value, from_max, to_max) ((((value * 10) * (to_max * 10)) / (from_max * 10)) / 10)

static SceTouchData touch;

typedef struct {
	int posX;
	int posY;
	int held;
	int pressed;
	int pressedPrev;
	int released;
	int releasedPrev;
} touchStateData;

// Static so the touch layer neither holds nor leaves any allocation.
static touchStateData touchStateStorage;
static touchStateData *const touchState = &touchStateStorage;

// Whether the log saw the finger come down. Kept apart from touchState: a
// screen jump resets that one with the finger still on the panel, which must
// not log a second "down", and its first frame reports a release that never
// had a press. The position is kept apart too: when the finger leaves on
// the frame right after a jump, touchState only holds the zeros of the reset.
static int log_touching = 0;
static int log_x = 0, log_y = 0;

// Whether the user really had a finger on the panel last frame, while a
// suite touches for them: a new touch stops the suite. Set at start, like
// the buttons in utils.c.
static int old_physical = 1;

static void Touch_Log(const char *event) {
	LogLine line;

	DebugLog_Begin(&line, "touch");
	LogLine_Str(&line, "event", event);
	LogLine_Int(&line, "x", log_x);
	LogLine_Int(&line, "y", log_y);
	DebugLog_Commit(&line);
}

void Touch_Reset(void) {
	memset(touchState, 0, sizeof(*touchState));
}

int Touch_Init(void) {
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
	Touch_Reset();
	
	return 1;
}

// Nothing to release since the state is static; kept so main.c stays
// symmetrical with Touch_Init.
void Touch_Shutdown(void) {
}

int Touch_GetX(void) {
	return touchState->posX;
}

int Touch_GetY(void) {
	return touchState->posY;
}

int Touch_CheckPressed(void) {
	return touchState->pressed;
}

int Touch_CheckReleased(void) {
	return touchState->released;
}

int Touch_CheckHeld(void) {
	return touchState->held;
}
	
void Touch_Update(void) {
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
	
	int down = touch.reportNum > 0;
	int x = 0, y = 0;
	if (down) {
		x = lerp(touch.report[0].x, 1919, 960);
		y = lerp(touch.report[0].y, 1087, 544);
	}

	// The scenario's finger is already in screen pixels.
	int physical = down;
	if (Suite_Running()) {
		if (physical && !old_physical)
			Suite_Abort();
		else
			down = Suite_Touch(&x, &y);
	}
	old_physical = physical;

	if (down) {
		touchState->held = 1;
		touchState->posX = x;
		touchState->posY = y;
		touchState->released = 0;
		touchState->releasedPrev = 0;
		if (touchState->pressedPrev == 0) {
			touchState->pressedPrev = 1;
			touchState->pressed = 1;
		}
		else 
			touchState->pressed = 0;
	}
	else {
		touchState->held = 0;
		touchState->pressed = 0;
		touchState->pressedPrev = 0;
		if (touchState->releasedPrev == 0) {
			touchState->releasedPrev = 1;
			touchState->released = 1;
		}
		else 
			touchState->released = 0;
	}

	if (DebugLog_On()) {
		if (touchState->held) {
			log_x = touchState->posX;
			log_y = touchState->posY;
		}

		if (touchState->pressed && !log_touching) {
			log_touching = 1;
			Touch_Log("down");
		}
		else if (touchState->released && log_touching) {
			// log_x/log_y hold the last position the finger had, since the
			// panel no longer reports one.
			log_touching = 0;
			Touch_Log("up");
		}
	}
}
