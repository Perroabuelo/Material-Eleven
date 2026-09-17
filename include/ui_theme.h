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

// Radius families (drawn as vector arcs, see openspec/changes/vectorize-ui-controls).
#define UI_RADIUS_SM 10
#define UI_RADIUS_LG 24

#define UI_PI 3.14159265358979f

// Shared layout constants.
#define UI_RAIL_WIDTH      76
#define UI_HINT_BAR_HEIGHT 32

// Font sizes (Manrope / IBM Plex Mono, matching the mockups' scale).
#define UI_FONT_SIZE_HINT        13
#define UI_FONT_SIZE_BADGE       11
#define UI_FONT_SIZE_LABEL_SMALL 12
#define UI_FONT_SIZE_BODY        14
#define UI_FONT_SIZE_TITLE       16
#define UI_FONT_SIZE_TITLE_LARGE 19
#define UI_FONT_SIZE_DISPLAY     22

// Screen identifiers for the nav rail / cross-screen state.
typedef enum {
	UI_SCREEN_NOW_PLAYING = 0,
	UI_SCREEN_FOLDERS = 1,
	UI_SCREEN_SETTINGS = 2,
	UI_SCREEN_NONE = -1
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

// ---- Vector primitives (vita2d_draw_array; no texture involved) ----
// Every UI control and chrome shape is composed from these, so icons stay
// crisp at their drawn size instead of being a rescaled PNG.

// Filled triangle.
void UI_DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, unsigned int color);
// Filled convex quad; vertices given in order around the perimeter.
void UI_DrawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, unsigned int color);
// Straight `thickness`-px stroke with round caps (the mockups' strokeLinecap: round).
void UI_DrawStroke(float x0, float y0, float x1, float y1, float thickness, unsigned int color);
// Annulus `thickness` px wide whose outer edge sits at `radius`.
void UI_DrawRing(float cx, float cy, float radius, float thickness, unsigned int color);

// ---- Transport glyphs (shared by Now Playing and the Folders mini-player) ----
// `size` is the glyph's nominal box, centered on (cx, cy).

// Play: one right-pointing triangle.
void UI_DrawPlayGlyph(float cx, float cy, float size, unsigned int color);
// Pause: two bars.
void UI_DrawPauseGlyph(float cx, float cy, float size, unsigned int color);
// Skip to next / previous track: two triangles plus the end bar.
void UI_DrawSkipGlyph(float cx, float cy, float size, SceBool forward, unsigned int color);

// Baseline Y so `text` sits vertically centered within [box_top, box_top+box_h).
int UI_TextBaselineY(vita2d_font *f, unsigned int size, const char *text, float box_top, float box_h);

// Format badge styling (color + label) for a recognized audio extension.
// Returns SCE_FALSE (and leaves outputs untouched) for an unrecognized extension.
SceBool UI_GetFormatBadge(const char *ext, const char **out_label, unsigned int *out_color, unsigned int *out_wash);

#endif
