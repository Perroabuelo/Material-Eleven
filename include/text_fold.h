#ifndef _ELEVENMPV_TEXT_FOLD_H_
#define _ELEVENMPV_TEXT_FOLD_H_

#include <stddef.h>

// The one comparison rule for every search in the application (Folders and
// Library): case, accents and diaereses do not count, and ñ is n, so "cancion"
// finds "CANCIÓN". Pure logic: no vita2d and no SCE headers, so it is tested on
// the PC (tests/test_text_fold.c).

// The longest text TextFold_Contains folds, NUL included; the rest of a longer
// haystack is not searched. Matches LIBRARY_PATH_MAX, the longest name or tag
// field the application keeps.
#define TEXT_FOLD_MAX 256

// Writes the comparable form of the UTF-8 string `utf8` into `out`, at most
// `cap` bytes counting the NUL, always NUL-terminated when `cap` > 0, and
// returns its length. ASCII goes to lower case, and Latin-1 Supplement and
// Latin Extended-A (U+00C0 to U+017F) to their base letters without marks in
// lower case (Á to a, Ñ to n, ß to ss, Æ to ae). Everything else, invalid
// bytes included, is copied as is. A character that does not fit whole ends
// the output.
size_t TextFold_Fold(const char *utf8, char *out, size_t cap);

// Whether `haystack_utf8`, once folded, contains `folded_needle`, which must
// already be folded with TextFold_Fold: callers fold a search term once, not
// once per entry. An empty needle is contained in everything.
int TextFold_Contains(const char *haystack_utf8, const char *folded_needle);

#endif
