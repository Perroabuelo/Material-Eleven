#ifndef _ELEVENMPV_NAV_REQUEST_H_
#define _ELEVENMPV_NAV_REQUEST_H_

#include "ui_screen.h"

// A deferred request to change screen, for code that decides to leave the
// current screen from several calls below the screen's own loop - playing a
// track starts in Dirbrowse_OpenFile or Menu_LibraryPlaySelected, not in the
// loop that has to return the next screen to main().
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_nav_request.c).

// Asks to go to `screen` once the current screen gets the chance. A later
// request replaces an earlier one that nobody took yet.
void NavRequest_Set(UI_Screen screen);

// Returns the pending request and clears it, or UI_SCREEN_NONE when there is
// none.
UI_Screen NavRequest_Take(void);

#endif
