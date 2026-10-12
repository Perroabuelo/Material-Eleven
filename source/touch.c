/*
		
		Taken from VitaComix by Arkanite.
		A manga/comic book reader for the Playstation Vita
		
		File: touch.c
		
*/
#include <psp2/touch.h>
#include <string.h>

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
	
	if (touch.reportNum > 0) {
		touchState->held = 1;
		touchState->posX = (lerp(touch.report[0].x, 1919, 960));
		touchState->posY = (lerp(touch.report[0].y, 1087, 544));
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
}
