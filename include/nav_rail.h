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

#endif
