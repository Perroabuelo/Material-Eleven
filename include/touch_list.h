#ifndef _ELEVENMPV_TOUCH_LIST_H_
#define _ELEVENMPV_TOUCH_LIST_H_

#include <psp2/types.h>

#include "gesture.h"
#include "list_scroll.h"

// A scrolling list on screen: its area, its scroll position and the finger
// working it. Glues touch.c to gesture.c and list_scroll.c, which hold the
// logic and are tested on a PC, so every list scrolls, glides and takes taps
// the same way.
//
// Each frame, the screen calls TouchList_Layout while drawing (it sets the
// size and applies a pending TouchList_Follow), draws its rows between
// TouchList_BeginClip and TouchList_EndClip from TouchList_FirstRow at
// TouchList_RowY, and after Touch_Update calls TouchList_Update once.
//
// A touch belongs to whatever it landed on. The list owns only a touch that
// lands inside its area while the screen allows it, and ignores everything
// else until the finger lifts; so a drag that started on the list never
// reaches the rail, the mini player or the top bar, which act on landing.

typedef struct {
	ListScroll scroll;
	Gesture_State gesture;
	float left, top, right, bottom;
	SceBool owned;
	SceBool follow;
	SceUInt64 last_us;
} TouchList;

// Drops the finger and the inertia and goes back to the top. For a new list:
// another view, another folder, another screen.
void TouchList_Reset(TouchList *list);

// The selection was moved by a button or the list changed under it: stop
// gliding and show the selected row on the next TouchList_Layout.
void TouchList_Follow(TouchList *list);

void TouchList_Layout(TouchList *list, float left, float top, float right, float bottom,
	int rows, float row_h, int selection);

// Feeds this frame's touch. `may_own` is SCE_FALSE while something else has
// the screen (an open menu). Returns the row tapped this frame, or -1.
int TouchList_Update(TouchList *list, SceBool may_own);

// The row under a finger that is resting on it, or -1. Drawn pressed.
int TouchList_PressedRow(const TouchList *list);

int TouchList_FirstRow(const TouchList *list);
float TouchList_RowY(const TouchList *list, int index);

void TouchList_BeginClip(const TouchList *list);
void TouchList_EndClip(void);

#endif
