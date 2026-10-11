#include <stddef.h>

#include "list_filter.h"

int ListFilter_Build(ListFilter *f, int count, ListFilter_Match match, void *ctx) {
	if (count < 0)
		count = 0;
	else if (count > LIST_FILTER_MAX)
		count = LIST_FILTER_MAX;

	f->count = 0;

	for (int i = 0; i < count; i++) {
		if (match == NULL || match(i, ctx))
			f->visible[f->count++] = i;
	}

	return f->count;
}

int ListFilter_Count(const ListFilter *f) {
	return f->count;
}

int ListFilter_ToFull(const ListFilter *f, int visible) {
	if (visible < 0 || visible >= f->count)
		return -1;

	return f->visible[visible];
}

int ListFilter_ToVisible(const ListFilter *f, int full) {
	// The visible positions are in ascending full order, so a binary search
	// finds one without walking the list.
	int lo = 0, hi = f->count - 1;

	while (lo <= hi) {
		int mid = lo + (hi - lo) / 2;

		if (f->visible[mid] == full)
			return mid;
		else if (f->visible[mid] < full)
			lo = mid + 1;
		else
			hi = mid - 1;
	}

	return -1;
}
