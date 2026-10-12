#ifndef _ELEVENMPV_SEARCH_DIALOG_H_
#define _ELEVENMPV_SEARCH_DIALOG_H_

#include <stddef.h>

#include "ui_screen.h"

// The system text-entry dialog used by every search box in the application
// (Folders and Library), with its emergency exits. One implementation, so a
// fix to the exits reaches every screen that searches.

typedef enum {
	SEARCH_DIALOG_CONFIRMED = 0, // `out` holds the term, possibly ""
	SEARCH_DIALOG_CANCELLED,     // the user dismissed it; `out` is untouched
	SEARCH_DIALOG_ABANDONED,     // an exit fired (L+R+START, no compose, timeout)
	SEARCH_DIALOG_FAILED         // the dialog could not be opened
} SearchDialog_Result;

// Opens the dialog with `title_utf8` as its title and `current_utf8` already
// typed, and runs it until it closes. Only on SEARCH_DIALOG_CONFIRMED does
// `out` (of `out_cap` bytes) receive the text, NUL-terminated.
//
// `draw_background` draws the calling screen's body, without legend or rail:
// it is all that shows when the dialog does not compose. The module adds the
// legend of the abandon combo and the rail of `rail_screen`, whose taps are
// discarded.
SearchDialog_Result SearchDialog_Prompt(const char *title_utf8, const char *current_utf8,
	char *out, size_t out_cap, void (*draw_background)(void), UI_Screen rail_screen);

#endif
