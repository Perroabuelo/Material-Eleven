#include "screen_off.h"

void ScreenOff_Init(ScreenOff_State *s, unsigned int start, unsigned int ltrigger, unsigned int rtrigger) {
	s->start = start;
	s->ltrigger = ltrigger;
	s->rtrigger = rtrigger;
	s->enabled = 1;
}

unsigned int ScreenOff_Filter(ScreenOff_State *s, unsigned int pressed, unsigned int held, ScreenOff_Action *action) {
	*action = SCREEN_OFF_KEEP;

	if (!s->enabled)
		return pressed;

	if (pressed & s->start) {
		unsigned int both_triggers = s->ltrigger | s->rtrigger;

		if ((held & both_triggers) == both_triggers)
			return pressed;

		*action = SCREEN_OFF_TURN_OFF;
		return pressed & ~s->start;
	}

	return pressed;
}
