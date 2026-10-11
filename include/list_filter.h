#ifndef _ELEVENMPV_LIST_FILTER_H_
#define _ELEVENMPV_LIST_FILTER_H_

// A filtered list as a layer of indices over a full list that is already
// built: which full positions are visible, in their full order. The Library
// works in visible positions and translates to full ones only to reach an
// entry. It knows nothing of tracks; the caller says what matches.
//
// Pure logic: no vita2d and no SCE headers, so it is tested on the PC
// (tests/test_list_filter.c). No dynamic memory, so building cannot fail.

// The longest list it holds. Matches LIBRARY_MAX_TRACKS, the longest view the
// Library builds, tracks or names; menu_library.c checks that they agree.
#define LIST_FILTER_MAX 4000

typedef struct {
	int visible[LIST_FILTER_MAX];
	int count;
} ListFilter;

// Whether the full entry `index` stays visible.
typedef int (*ListFilter_Match)(int index, void *ctx);

// Keeps, in order, the entries of a full list of `count` (clamped to
// 0..LIST_FILTER_MAX) for which `match` is true, or all of them when `match`
// is NULL. Returns how many are visible.
int ListFilter_Build(ListFilter *f, int count, ListFilter_Match match, void *ctx);

int ListFilter_Count(const ListFilter *f);

// The full position of the visible position `visible`, or -1 out of range.
int ListFilter_ToFull(const ListFilter *f, int visible);

// The visible position of the full position `full`, or -1 when it is filtered
// out or out of range.
int ListFilter_ToVisible(const ListFilter *f, int full);

#endif
