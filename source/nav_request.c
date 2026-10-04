#include "nav_request.h"

static UI_Screen pending = UI_SCREEN_NONE;

void NavRequest_Set(UI_Screen screen) {
	pending = screen;
}

UI_Screen NavRequest_Take(void) {
	UI_Screen screen = pending;

	pending = UI_SCREEN_NONE;
	return screen;
}
