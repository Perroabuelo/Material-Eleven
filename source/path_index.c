#include <stdlib.h>
#include <string.h>

#include "path_index.h"

// The tests swap malloc for one that fails on demand (tests/test_path_index.c).
#ifdef PATH_INDEX_TEST_ALLOC
void *PathIndex_TestMalloc(size_t size);
#define path_index_malloc PathIndex_TestMalloc
#else
#define path_index_malloc malloc
#endif

// qsort takes no context, and nothing here runs on two threads at once.
static const PathIndex *sorting = NULL;

static int PathIndex_Compare(const void *a, const void *b) {
	int ia = *(const int *)a;
	int ib = *(const int *)b;
	int c = strcmp(sorting->get(ia, sorting->ctx), sorting->get(ib, sorting->ctx));

	// Equal paths keep the list order, so Find gives the first one.
	if (c != 0)
		return c;

	return (ia > ib) - (ia < ib);
}

int PathIndex_Build(PathIndex *idx, int count, PathIndex_Get get, void *ctx) {
	memset(idx, 0, sizeof(*idx));

	if (count <= 0)
		return 0;

	int *order = path_index_malloc((size_t)count * sizeof(int));
	if (order == NULL)
		return -1;

	for (int i = 0; i < count; i++)
		order[i] = i;

	idx->order = order;
	idx->count = count;
	idx->get = get;
	idx->ctx = ctx;

	sorting = idx;
	qsort(order, (size_t)count, sizeof(int), PathIndex_Compare);
	sorting = NULL;
	return 0;
}

int PathIndex_Find(const PathIndex *idx, const char *path) {
	int lo = 0;
	int hi = idx->count;

	// The first position whose path is not below `path`.
	while (lo < hi) {
		int mid = lo + (hi - lo) / 2;

		if (strcmp(idx->get(idx->order[mid], idx->ctx), path) < 0)
			lo = mid + 1;
		else
			hi = mid;
	}

	if (lo < idx->count && strcmp(idx->get(idx->order[lo], idx->ctx), path) == 0)
		return idx->order[lo];

	return -1;
}

void PathIndex_Free(PathIndex *idx) {
	free(idx->order);
	memset(idx, 0, sizeof(*idx));
}
