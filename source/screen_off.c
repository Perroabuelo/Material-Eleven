#include "screen_off.h"

void ScreenOff_Init(ScreenOff_State *s, unsigned int start, unsigned int ltrigger, unsigned int rtrigger) {
	s->start = start;
	s->ltrigger = ltrigger;
	s->rtrigger = rtrigger;
	s->enabled = 1;
	s->armed = 0;
}

unsigned int ScreenOff_Filter(ScreenOff_State *s, unsigned int pressed, unsigned int held, ScreenOff_Action *action) {
	*action = SCREEN_OFF_KEEP;

	if (!s->enabled)
		return pressed;

	// La pulsacion que enciende la pantalla no es una orden para lo que hay
	// debajo: el usuario todavia no ve que tiene seleccionado.
	if (s->armed) {
		if (pressed != 0) {
			s->armed = 0;
			*action = SCREEN_OFF_TURN_ON;
			return 0;
		}
		return pressed;
	}

	if (pressed & s->start) {
		unsigned int both_triggers = s->ltrigger | s->rtrigger;

		if ((held & both_triggers) == both_triggers)
			return pressed;

		*action = SCREEN_OFF_TURN_OFF;
		s->armed = 1;
		return pressed & ~s->start;
	}

	return pressed;
}
