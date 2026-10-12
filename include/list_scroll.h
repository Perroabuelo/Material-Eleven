#ifndef _ELEVENMPV_LIST_SCROLL_H_
#define _ELEVENMPV_LIST_SCROLL_H_

// The scroll position of a list of same-height rows, in pixels: dragged by a
// finger, gliding after a flick and moved by the buttons so the selected row
// shows. It never leaves [0, ListScroll_MaxOffset], so the list cannot show
// empty space past its first or last row.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_list_scroll.c). The screen feeds it the gestures from
// gesture.c and draws rows from ListScroll_FirstRow at ListScroll_RowY.

typedef struct {
	float offset;       // pixels of content above the top of the area
	float velocity;     // inertia, in px/ms of offset; 0 when still
	float content_h;    // rows * row_h
	float viewport_h;   // height of the area the rows are drawn in
	float row_h;
} ListScroll;

// Sets the list's size and keeps the offset in range. Called every frame
// before drawing; a list that fits stays at 0.
void ListScroll_SetSize(ListScroll *scroll, int rows, float row_h, float viewport_h);

float ListScroll_MaxOffset(const ListScroll *scroll);

// Follows a finger that moved `dy` pixels (+ is down) this frame. Stops any
// inertia.
void ListScroll_Drag(ListScroll *scroll, float dy);

// The finger was let go at `finger_velocity` px/ms (+ is down). Too slow a
// release leaves the list still.
void ListScroll_Fling(ListScroll *scroll, float finger_velocity);

// Advances the inertia by one frame that took `dt_ms`. Slows down by time,
// not by frames, and stops at either end.
void ListScroll_Step(ListScroll *scroll, float dt_ms);

void ListScroll_Stop(ListScroll *scroll);
int ListScroll_Moving(const ListScroll *scroll);

// Moves the view the least needed for row `index` to be fully inside the
// area. A half-shown row is aligned to the edge it comes in by. Stops any
// inertia. Out-of-range indexes are ignored.
void ListScroll_EnsureVisible(ListScroll *scroll, int index);

// Index of the row under `y`, measured from the top of the area, or -1 when
// `y` is outside the area or past the last row.
int ListScroll_RowAt(const ListScroll *scroll, float y);

// First row that shows, at least in part, and where row `index` is drawn
// relative to the top of the area.
int ListScroll_FirstRow(const ListScroll *scroll);
float ListScroll_RowY(const ListScroll *scroll, int index);

#endif
