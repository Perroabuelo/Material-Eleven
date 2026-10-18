#include <stddef.h>

#include "sleep_timer.h"

#define MINUTE_US 60000000ULL

uint32_t SleepTimer_OptionMinutes(Sleep_Option o) {
	switch (o) {
		case SLEEP_15: return 15;
		case SLEEP_30: return 30;
		case SLEEP_45: return 45;
		case SLEEP_60: return 60;
		case SLEEP_90: return 90;
		default:       return 0;
	}
}

void SleepTimer_Set(Sleep_Timer *t, Sleep_Option o, uint64_t now_us) {
	if (o < SLEEP_OFF || o >= SLEEP_OPTION_COUNT)
		o = SLEEP_OFF;

	uint32_t minutes = SleepTimer_OptionMinutes(o);

	t->option = o;
	t->deadline_us = (minutes > 0) ? now_us + minutes * MINUTE_US : 0;
}

Sleep_Tick SleepTimer_Tick(Sleep_Timer *t, uint64_t now_us, float *gain) {
	*gain = 1.0f;

	if (t->deadline_us == 0)
		return SLEEP_TICK_NONE;

	// A clock that jumps past the deadline (a frame that took long, the app
	// coming back from the background) still expires it once.
	if (now_us >= t->deadline_us) {
		t->option = SLEEP_OFF;
		t->deadline_us = 0;
		*gain = 0.0f;
		return SLEEP_TICK_EXPIRED;
	}

	uint64_t left = t->deadline_us - now_us;
	if (left < SLEEP_FADE_US)
		*gain = (float)left / (float)SLEEP_FADE_US;

	return SLEEP_TICK_NONE;
}

int SleepTimer_TrackEnded(Sleep_Timer *t) {
	if (t->option != SLEEP_END_OF_TRACK)
		return 0;

	t->option = SLEEP_OFF;
	t->deadline_us = 0;
	return 1;
}

uint32_t SleepTimer_MinutesLeft(const Sleep_Timer *t, uint64_t now_us) {
	if (t->deadline_us == 0 || now_us >= t->deadline_us)
		return 0;

	uint64_t left = t->deadline_us - now_us;
	return (uint32_t)((left + MINUTE_US - 1) / MINUTE_US);
}

const char *SleepTimer_Name(Sleep_Option o) {
	switch (o) {
		case SLEEP_OFF:          return "off";
		case SLEEP_15:           return "15";
		case SLEEP_30:           return "30";
		case SLEEP_45:           return "45";
		case SLEEP_60:           return "60";
		case SLEEP_90:           return "90";
		case SLEEP_END_OF_TRACK: return "track";
		default:                 return "off";
	}
}
