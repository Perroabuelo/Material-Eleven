#include <stddef.h>

#include "library_row.h"

int LibraryRow_GetLayout(int version, LibraryRow_Layout *out) {
	if (out == NULL)
		return 0;

	switch (version) {
		case 2:
			*out = (LibraryRow_Layout){
				.size = 0, .mtime = 1, .tagged = -1, .ext = 3, .track = -1, .disc = -1,
				.title = 4, .artist = 5, .album = 6, .path = 7, .fields = 8
			};
			return 1;

		case 3:
			*out = (LibraryRow_Layout){
				.size = 0, .mtime = 1, .tagged = 2, .ext = 3, .track = 4, .disc = 5,
				.title = 6, .artist = 7, .album = 8, .path = 9, .fields = 10
			};
			return 1;

		default:
			return 0;
	}
}
