#ifndef _ELEVENMPV_PATH_INDEX_H_
#define _ELEVENMPV_PATH_INDEX_H_

// Finding an entry of a list by its path without walking the list: the
// positions sorted by path, searched by halves. The Library builds one over
// its index so a playlist of thousands of tracks can tell which of them it
// still has. It knows nothing of tracks; the caller says where each path is.
//
// Pure logic: no vita2d and no SCE headers, so it is tested on the PC
// (tests/test_path_index.c).

// The path of the entry `index` of the list.
typedef const char *(*PathIndex_Get)(int index, void *ctx);

typedef struct {
	int *order; // positions of the list, by path; NULL when empty
	int count;
	PathIndex_Get get;
	void *ctx;
} PathIndex;

// Sorts the `count` entries of the list by path. The list must not change
// while the index is in use. 0, or -1 when memory runs out, and then the index
// is empty and finds nothing.
int PathIndex_Build(PathIndex *idx, int count, PathIndex_Get get, void *ctx);

// The position of an entry with exactly that path, or -1. With repeated paths,
// the first one in the list.
int PathIndex_Find(const PathIndex *idx, const char *path);

// Frees the index and leaves it empty. Takes an empty one too.
void PathIndex_Free(PathIndex *idx);

#endif
