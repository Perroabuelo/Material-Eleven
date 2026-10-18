#ifndef _ELEVENMPV_VIZ_NAV_H_
#define _ELEVENMPV_VIZ_NAV_H_

// Which view Now Playing shows (normal, lyrics or visualizer) and which style
// the visualizer draws, and how the D-pad moves between them. Up and Down
// each toggle their own view, jumping straight from the other one; Left and
// Right walk the styles, wrapping around, and only in the visualizer.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_nav.c).

typedef enum {
	NP_VIEW_NORMAL,
	NP_VIEW_LYRICS,
	NP_VIEW_VISUALIZER
} NowPlaying_View;

typedef enum {
	VIZ_STYLE_SPECTRUM,
	VIZ_STYLE_SCOPE,
	VIZ_STYLE_VU,
	VIZ_STYLE_DISC,     // the vinyl with a radial spectrum
	VIZ_STYLE_PULSE,    // the vinyl that pulses with the bass
	VIZ_STYLE_COUNT
} Viz_Style;

typedef enum {
	VIZ_NAV_UP,
	VIZ_NAV_DOWN,
	VIZ_NAV_LEFT,       // also a swipe to the right: the previous style
	VIZ_NAV_RIGHT       // also a swipe to the left: the next style
} VizNav_Key;

// Applies `key` to the view and the style.
void VizNav_Apply(NowPlaying_View *view, Viz_Style *style, VizNav_Key key);

// True for the styles that draw the vinyl, where the header leaves out the
// small cover art.
int VizNav_StyleHasDisc(Viz_Style style);

// Names for the nowplaying line of the debug log.
const char *VizNav_ViewName(NowPlaying_View view);
const char *VizNav_StyleName(Viz_Style style);

#endif
