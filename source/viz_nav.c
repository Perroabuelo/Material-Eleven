#include "viz_nav.h"

void VizNav_Apply(NowPlaying_View *view, Viz_Style *style, VizNav_Key key) {
	switch (key) {
		case VIZ_NAV_UP:
			*view = (*view == NP_VIEW_LYRICS) ? NP_VIEW_NORMAL : NP_VIEW_LYRICS;
			break;

		case VIZ_NAV_DOWN:
			*view = (*view == NP_VIEW_VISUALIZER) ? NP_VIEW_NORMAL : NP_VIEW_VISUALIZER;
			break;

		case VIZ_NAV_LEFT:
			if (*view == NP_VIEW_VISUALIZER)
				*style = (Viz_Style)((*style + VIZ_STYLE_COUNT - 1) % VIZ_STYLE_COUNT);
			break;

		case VIZ_NAV_RIGHT:
			if (*view == NP_VIEW_VISUALIZER)
				*style = (Viz_Style)((*style + 1) % VIZ_STYLE_COUNT);
			break;
	}
}

int VizNav_StyleHasDisc(Viz_Style style) {
	return style == VIZ_STYLE_DISC || style == VIZ_STYLE_PULSE;
}

const char *VizNav_ViewName(NowPlaying_View view) {
	switch (view) {
		case NP_VIEW_LYRICS:
			return "lyrics";
		case NP_VIEW_VISUALIZER:
			return "visualizer";
		default:
			return "normal";
	}
}

const char *VizNav_StyleName(Viz_Style style) {
	switch (style) {
		case VIZ_STYLE_SCOPE:
			return "scope";
		case VIZ_STYLE_VU:
			return "vu";
		case VIZ_STYLE_DISC:
			return "disc";
		case VIZ_STYLE_PULSE:
			return "pulse";
		default:
			return "spectrum";
	}
}
