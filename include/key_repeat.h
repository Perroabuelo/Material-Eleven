#ifndef _ELEVENMPV_KEY_REPEAT_H_
#define _ELEVENMPV_KEY_REPEAT_H_

#include <stdint.h>

// Holding Up or Down in a list keeps stepping the selection: one step on the
// press, a pause, then one step per interval, and after a long hold one page
// per step. Only lists consume these steps; `pressed` stays an edge for every
// other use.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_key_repeat.c). Utils_ReadControls (source/utils.c) feeds it the
// held buttons, the edge and the clock once per frame.

#define KEY_REPEAT_DELAY_US    (400 * 1000)   // press -> first repeated step
#define KEY_REPEAT_INTERVAL_US (66 * 1000)    // between repeated steps (~15/s)
#define KEY_REPEAT_PAGE_US     (2000 * 1000)  // press -> steps become pages

typedef struct {
	// Masks of Up and Down on the console pad.
	unsigned int up, down;
	// Direction being held (-1 up, +1 down), 0 when idle.
	int dir;
	uint64_t start_us;  // when the current hold began
	uint64_t next_us;   // when the next repeated step is due
} KeyRepeat_State;

typedef struct {
	int dir;     // -1 up, +1 down, 0 no step this frame
	int repeat;  // 0 for a new press, 1 for a step made by holding
	int page;    // a repeated step that moves a whole page
} KeyRepeat_Step;

void KeyRepeat_Init(KeyRepeat_State *s, unsigned int up, unsigned int down);

// Advances by one frame. `held` are the buttons down this frame, `pressed`
// the edge, `now_us` the current time. Returns at most one step per frame:
// a slow frame does not make up for missed steps, the next one is scheduled
// from now.
//
// - A new press of Up or Down steps at once and restarts the delay.
// - Up and Down held together do not repeat.
// - Switching from one to the other restarts the delay.
KeyRepeat_Step KeyRepeat_Update(KeyRepeat_State *s, unsigned int held, unsigned int pressed, uint64_t now_us);

// The selection after applying `step` to `sel` in a list of `count` rows.
//
// - A new press moves one row and wraps around at either end.
// - A repeated step moves one row, or `page` rows if it is a page step, and
//   stops at the first or last row.
// - No step leaves `sel` alone, except that a selection past an end wraps as
//   a press would, the way the lists always corrected it.
// - `count` <= 0 gives 0.
int KeyRepeat_Move(int sel, int count, KeyRepeat_Step step, int page);

#endif
