#ifndef _ELEVENMPV_UI_SCREEN_H_
#define _ELEVENMPV_UI_SCREEN_H_

// Screen identifiers for the nav rail / cross-screen state.
//
// Pure: no vita2d and no SCE headers, so the screen dispatch logic that names
// a screen can be tested on a PC (tests/test_nav_request.c). ui_theme.h
// includes it, so code that already sees ui_theme.h needs nothing new.
typedef enum {
	UI_SCREEN_NOW_PLAYING = 0,
	UI_SCREEN_FOLDERS = 1,
	UI_SCREEN_SETTINGS = 2,
	// Añadida al final y no intercalada: el orden del rail lo decide
	// NavRail_GetButtons, asi que renumerar no compraria nada.
	UI_SCREEN_LIBRARY = 3,
	UI_SCREEN_NONE = -1
} UI_Screen;

#endif
