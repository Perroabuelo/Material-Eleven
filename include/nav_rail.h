#ifndef _ELEVENMPV_NAV_RAIL_H_
#define _ELEVENMPV_NAV_RAIL_H_

#include "ui_theme.h"

// Draws the persistent nav rail for `active` and returns the destination
// the user just tapped this frame, or UI_SCREEN_NONE. Touch_Update() must
// already have been called this frame by the caller. Physical-button
// shortcuts (SELECT/START/cancel) are handled separately by each screen.
UI_Screen NavRail_DrawAndHitTest(UI_Screen active);

// Draws the bottom physical-button legend spanning the content column
// (to the right of the rail). `segments` holds `count` left-aligned
// labels drawn in font_mono; a NULL entry is skipped.
void NavRail_DrawHintBar(float y, const char **segments, int count);

// A physical button as the hint bar shows it. CONFIRM and CANCEL are resolved
// when drawn, from the console's own assignment (SCE_CTRL_ENTER/CANCEL), so the
// bar cannot show a different symbol from the one the screen listens to.
typedef enum {
	HINT_BTN_NONE = 0,
	HINT_BTN_CONFIRM,
	HINT_BTN_CANCEL,
	HINT_BTN_TRIANGLE,
	HINT_BTN_SQUARE,
	HINT_BTN_L,
	HINT_BTN_R,
	HINT_BTN_SELECT,
	HINT_BTN_START
} NavRail_HintButton;

#define NAV_RAIL_HINT_MAX_BUTTONS 3

// One legend entry: its buttons, then its label. Unused button slots are
// HINT_BTN_NONE. `combo` draws a "+" between the buttons (pressed together);
// without it they sit side by side (either one). A NULL label skips the entry.
typedef struct {
	NavRail_HintButton buttons[NAV_RAIL_HINT_MAX_BUTTONS];
	int combo;
	const char *label;
} NavRail_Hint;

// Draws the bottom legend from `count` entries, left to right in the order
// given. Symbol buttons are vector glyphs in their PlayStation color; L, R,
// SELECT and START are neutral chips with their name. An entry that does not
// fit in the bar's width is not drawn, and neither is any entry after it, so
// callers list entries from most to least important.
void NavRail_DrawHints(float y, const NavRail_Hint *hints, int count);

#endif
