#ifndef _ELEVENMPV_SLEEP_TIMER_H_
#define _ELEVENMPV_SLEEP_TIMER_H_

#include <stdint.h>

// The sleep timer: stops the music after a chosen time, fading it out over
// the last seconds, or when the current track ends.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_sleep_timer.c). The clock comes in as microseconds from the
// caller; Music_SleepTick (menus/menu_audioplayer.c) passes
// sceKernelGetProcessTimeWide() once per frame.

// The options, in the order the menu lists them.
typedef enum {
	SLEEP_OFF,
	SLEEP_15,
	SLEEP_30,
	SLEEP_45,
	SLEEP_60,
	SLEEP_90,
	SLEEP_END_OF_TRACK,
	SLEEP_OPTION_COUNT
} Sleep_Option;

// How long the music fades out before a timed option expires.
#define SLEEP_FADE_US 10000000ULL

typedef struct {
	Sleep_Option option;
	uint64_t deadline_us;   // when a timed option expires, 0 otherwise
} Sleep_Timer;

typedef enum {
	SLEEP_TICK_NONE,
	SLEEP_TICK_EXPIRED
} Sleep_Tick;

// Starts `o` at `now_us`, replacing whatever was set. SLEEP_OFF cancels.
void SleepTimer_Set(Sleep_Timer *t, Sleep_Option o, uint64_t now_us);

// Advances the timer to `now_us`. *gain is 1, or within the last
// SLEEP_FADE_US of a timed option the share of that time still left (down to
// 0). Returns SLEEP_TICK_EXPIRED once, on the first tick at or past the
// deadline, and leaves the timer off with *gain at 0.
Sleep_Tick SleepTimer_Tick(Sleep_Timer *t, uint64_t now_us, float *gain);

// For a track that ended on its own: 1 if the timer was on SLEEP_END_OF_TRACK,
// which turns it off, and 0 (with nothing changed) otherwise.
int SleepTimer_TrackEnded(Sleep_Timer *t);

// Minutes left of a timed option, rounded up; 0 for the other options.
uint32_t SleepTimer_MinutesLeft(const Sleep_Timer *t, uint64_t now_us);

// The minutes a timed option counts, 0 for the others.
uint32_t SleepTimer_OptionMinutes(Sleep_Option o);

// The option's name in the log: "off", "15", "30", "45", "60", "90" or
// "track".
const char *SleepTimer_Name(Sleep_Option o);

#endif
