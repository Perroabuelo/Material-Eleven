#ifndef _ELEVENMPV_SELECTION_H_
#define _ELEVENMPV_SELECTION_H_

#include <stdint.h>

#include "key_repeat.h"
#include "list_filter.h"

// The rows marked in the Library's selection mode, as positions of the full
// list (see list_filter.h). A search only changes which of them are visible,
// so the hidden marks still count and the actions still reach them. Walking
// the marks goes in full-list order, which is the order things are added in.
//
// Pure logic: no vita2d and no SCE headers, so it is tested on the PC
// (tests/test_selection.c). A fixed bitmap, so nothing here can fail.

typedef struct {
	uint8_t bits[LIST_FILTER_MAX / 8];
	int count;
} Selection;

// What the "All" checkbox shows for the visible rows.
typedef enum {
	SELECTION_ALL_DISABLED, // no visible rows: it cannot be used
	SELECTION_ALL_NONE,     // no visible row is marked
	SELECTION_ALL_SOME,
	SELECTION_ALL_ALL
} Selection_All;

// Unmarks everything.
void Selection_Clear(Selection *s);

// Marks `full` if it was not, unmarks it if it was. Out of range does nothing.
void Selection_Toggle(Selection *s, int full);

// Marks or unmarks `full`. Out of range does nothing.
void Selection_Set(Selection *s, int full, int on);

// Whether `full` is marked; 0 out of range.
int Selection_Has(const Selection *s, int full);

// How many are marked, hidden ones included.
int Selection_Count(const Selection *s);

// The checkbox state over the rows `f` leaves visible.
Selection_All Selection_AllState(const Selection *s, const ListFilter *f);

// What activating the checkbox does: when every visible row is marked it
// unmarks them, otherwise it marks them all. Rows `f` hides keep their marks.
// With no visible rows it does nothing.
void Selection_ToggleAll(Selection *s, const ListFilter *f);

// The first marked position at or after `from`, or -1 when there is none.
int Selection_Next(const Selection *s, int from);

// Up and Down in selection mode, over `count` + 1 positions: 0 is the "All"
// checkbox and 1..count are the visible rows. The rule of KeyRepeat_Move, with
// the checkbox in the loop:
//
// - A new press moves one position and wraps around through the checkbox:
//   up from row 1 reaches it, and up from it reaches the last row.
// - A repeated step moves one row, or `page` rows if it is a page step, and
//   stops at row 1 or at the last row without entering the checkbox. One
//   that starts on the checkbox stays there.
// - With `count` <= 0 the checkbox is disabled and `pos` does not change.
int Selection_Step(int pos, int count, KeyRepeat_Step step, int page);

#endif
