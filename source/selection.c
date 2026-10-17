#include <string.h>

#include "selection.h"

#define SELECTION_MAX ((int)sizeof(((Selection *)0)->bits) * 8)

static int InRange(int full) {
	return full >= 0 && full < SELECTION_MAX;
}

void Selection_Clear(Selection *s) {
	memset(s, 0, sizeof(*s));
}

int Selection_Has(const Selection *s, int full) {
	if (!InRange(full))
		return 0;

	return (s->bits[full / 8] >> (full % 8)) & 1;
}

void Selection_Set(Selection *s, int full, int on) {
	if (!InRange(full) || Selection_Has(s, full) == (on != 0))
		return;

	// The count follows each change, so reading it never walks the bitmap.
	s->bits[full / 8] ^= (uint8_t)(1u << (full % 8));
	s->count += on ? 1 : -1;
}

void Selection_Toggle(Selection *s, int full) {
	Selection_Set(s, full, !Selection_Has(s, full));
}

int Selection_Count(const Selection *s) {
	return s->count;
}

Selection_All Selection_AllState(const Selection *s, const ListFilter *f) {
	int visible = ListFilter_Count(f);
	int marked = 0;

	if (visible == 0)
		return SELECTION_ALL_DISABLED;

	for (int i = 0; i < visible; i++)
		marked += Selection_Has(s, ListFilter_ToFull(f, i));

	if (marked == 0)
		return SELECTION_ALL_NONE;

	return marked == visible ? SELECTION_ALL_ALL : SELECTION_ALL_SOME;
}

void Selection_ToggleAll(Selection *s, const ListFilter *f) {
	Selection_All state = Selection_AllState(s, f);

	if (state == SELECTION_ALL_DISABLED)
		return;

	int on = state != SELECTION_ALL_ALL;

	for (int i = 0; i < ListFilter_Count(f); i++)
		Selection_Set(s, ListFilter_ToFull(f, i), on);
}

int Selection_Next(const Selection *s, int from) {
	if (from < 0)
		from = 0;

	for (int full = from; full < SELECTION_MAX; full++) {
		// A whole empty byte is skipped at once.
		if (full % 8 == 0 && s->bits[full / 8] == 0) {
			full += 7;
			continue;
		}

		if (Selection_Has(s, full))
			return full;
	}

	return -1;
}

int Selection_Step(int pos, int count, KeyRepeat_Step step, int page) {
	if (count <= 0 || step.dir == 0)
		return pos;

	if (pos < 0)
		pos = 0;
	else if (pos > count)
		pos = count;

	if (step.repeat) {
		if (pos == 0)
			return 0;

		int rows = (step.page && page > 1) ? page : 1;
		pos += step.dir * rows;

		if (pos < 1)
			pos = 1;
		else if (pos > count)
			pos = count;

		return pos;
	}

	pos += step.dir;

	if (pos > count)
		pos = 0;
	else if (pos < 0)
		pos = count;

	return pos;
}
