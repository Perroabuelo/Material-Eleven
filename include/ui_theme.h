#ifndef _ELEVENMPV_UI_THEME_H_
#define _ELEVENMPV_UI_THEME_H_

#include <psp2/types.h>
#include <vita2d.h>

// Manrope - body text, titles, labels.
extern vita2d_font *font_ui;
// IBM Plex Mono - numeric / technical strings (durations, format badges, hints).
extern vita2d_font *font_mono;

// Palette (Material3-inspired dark skin, see openspec/changes/add-ui-skin).
#define UI_COLOR_BG              RGBA8(0x12, 0x0F, 0x17, 255)
#define UI_COLOR_BG_ELEVATED     RGBA8(0x17, 0x14, 0x1F, 255)
#define UI_COLOR_SURFACE         RGBA8(0x1C, 0x18, 0x26, 255)
#define UI_COLOR_SURFACE_2       RGBA8(0x24, 0x1F, 0x30, 255)
#define UI_COLOR_ACCENT          RGBA8(0xFF, 0x91, 0x66, 255)
#define UI_COLOR_ACCENT_WASH     RGBA8(0xFF, 0x91, 0x66, 36)
#define UI_COLOR_TEXT_PRIMARY    RGBA8(0xF4, 0xEF, 0xEA, 255)
#define UI_COLOR_TEXT_SECONDARY  RGBA8(0xB0, 0xA8, 0xC0, 255)
#define UI_COLOR_TEXT_TERTIARY   RGBA8(0x75, 0x6C, 0x89, 255)
#define UI_COLOR_TEXT_MUTED      RGBA8(0x6B, 0x64, 0x78, 255)
#define UI_COLOR_HAIRLINE        RGBA8(0xFF, 0xFF, 0xFF, 15)
#define UI_COLOR_LOSSLESS        RGBA8(0x7F, 0xE0, 0xC1, 255)
#define UI_COLOR_LOSSLESS_WASH   RGBA8(0x7F, 0xE0, 0xC1, 41)
#define UI_COLOR_LOSSY           RGBA8(0xB7, 0x9C, 0xE8, 255)
#define UI_COLOR_LOSSY_WASH      RGBA8(0xB7, 0x9C, 0xE8, 41)
#define UI_COLOR_TRACKER         RGBA8(0xF0, 0xC3, 0x4D, 255)
#define UI_COLOR_TRACKER_WASH    RGBA8(0xF0, 0xC3, 0x4D, 41)

// Radius families (see design.md: 9-sliced corner textures, two baked sizes).
#define UI_RADIUS_SM 10
#define UI_RADIUS_LG 24

// Shared layout constants.
#define UI_RAIL_WIDTH      76
#define UI_HINT_BAR_HEIGHT 32

// Screen identifiers for the nav rail / cross-screen state.
typedef enum {
	UI_SCREEN_NOW_PLAYING,
	UI_SCREEN_FOLDERS,
	UI_SCREEN_SETTINGS
} UI_Screen;

void UI_Theme_Load(void);
void UI_Theme_Free(void);

// Draws a filled rect with `radius`-px rounded corners via the 9-sliced
// corner textures (falls back to a plain rect when radius <= 0).
void UI_DrawRoundedRect(float x, float y, float w, float h, int radius, unsigned int color);
// Fully-rounded rect (radius = h/2) - pills, toggle tracks, search fields.
void UI_DrawPill(float x, float y, float w, float h, unsigned int color);
// Pill with a 1px border, sized to fit `label` drawn in font_mono.
void UI_DrawBadge(float x, float y, unsigned int size, const char *label, unsigned int bg, unsigned int fg, unsigned int border);
// Accent-wash rounded-rect highlight behind an active list row.
void UI_DrawRowHighlight(float x, float y, float w, float h);

// Baseline Y so `text` sits vertically centered within [box_top, box_top+box_h).
int UI_TextBaselineY(vita2d_font *f, unsigned int size, const char *text, float box_top, float box_h);

// Format badge styling (color + label) for a recognized audio extension.
// Returns SCE_FALSE (and leaves outputs untouched) for an unrecognized extension.
SceBool UI_GetFormatBadge(const char *ext, const char **out_label, unsigned int *out_color, unsigned int *out_wash);

#endif
