#ifndef _ELEVENMPV_UI_THEME_H_
#define _ELEVENMPV_UI_THEME_H_

#include <psp2/types.h>
#include <vita2d.h>

// ---- Text ----
//
// Nothing outside ui_theme.c names a font handle. A draw site names a face and
// a size token and the theme resolves the handle, because the handle is the
// contract: vita2d_font's glyph atlas is keyed by glyph index alone, with no
// size in the key, so a glyph shared between two sizes is rasterised once at
// whichever size drew it first and rescaled for the other. Keeping handles
// behind this API is what makes "one handle per drawn size" enforceable rather
// than a convention - see openspec/changes/close-ui-contract.

typedef enum {
	UI_FACE_UI = 0, // Manrope - body text, titles, labels
	UI_FACE_MONO,   // IBM Plex Mono - durations, format badges, hints
	UI_FACE_COUNT
} UI_Face;

typedef enum {
	UI_TS_BADGE = 0,
	UI_TS_LABEL_SMALL,
	UI_TS_HINT,
	UI_TS_BODY,
	UI_TS_TITLE,
	UI_TS_TITLE_LARGE,
	UI_TS_DISPLAY,
	UI_TS_COUNT
} UI_TextSize;

// All four tolerate a face whose font failed to load: they draw nothing and
// measure zero rather than dereferencing a null handle.
void UI_DrawText(UI_Face face, UI_TextSize ts, float x, float baseline_y, unsigned int color, const char *text);
int UI_TextWidth(UI_Face face, UI_TextSize ts, const char *text);
int UI_TextHeight(UI_Face face, UI_TextSize ts, const char *text);
void UI_TextDimensions(UI_Face face, UI_TextSize ts, const char *text, int *out_w, int *out_h);

// The interface accent and its low-alpha wash. Unlike the rest of the palette
// these are runtime values, shared by every screen and by the nav rail: they
// follow the current track's cover art via UI_Theme_SetAccentFromCoverArt, and
// fall back to UI_ACCENT_FIXED when there is no cover or no track.
extern unsigned int ui_color_accent;
extern unsigned int ui_color_accent_wash;

// Palette (Material3-inspired dark skin, see openspec/changes/add-ui-skin).
#define UI_COLOR_BG              RGBA8(0x12, 0x0F, 0x17, 255)
#define UI_COLOR_BG_ELEVATED     RGBA8(0x17, 0x14, 0x1F, 255)
#define UI_COLOR_SURFACE         RGBA8(0x1C, 0x18, 0x26, 255)
#define UI_COLOR_SURFACE_2       RGBA8(0x24, 0x1F, 0x30, 255)
// The accent is the one palette entry derived at runtime, from the current
// track's cover art - see ui_color_accent below. These two are its fallback
// value and the alpha its wash is built at.
#define UI_ACCENT_FIXED          RGBA8(0xFF, 0x91, 0x66, 255)
#define UI_ACCENT_WASH_ALPHA     36
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

// Nominal pixel size behind each UI_TextSize token, indexed by the enum.
// Draw sites name the token, never the number.
extern const unsigned int ui_text_px[UI_TS_COUNT];

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
// Pill with a 1px border, sized to fit `label` drawn in the mono face at `ts`.
void UI_DrawBadge(float x, float y, UI_TextSize ts, const char *label, unsigned int bg, unsigned int fg, unsigned int border);
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
int UI_TextBaselineY(UI_Face face, UI_TextSize ts, const char *text, float box_top, float box_h);

// ---- Dynamic accent derived from cover art ----

// Dominant chromatic color of a decoded cover texture, from a subsampled hue
// histogram. Returns SCE_FALSE (leaving `out_color` untouched) when the texture
// cannot be sampled or carries no usable hue, such as a grayscale cover.
SceBool UI_CoverDominantColor(const vita2d_texture *cover, unsigned int *out_color);
// Clamps a color's saturation and lightness into the band that stays legible
// over UI_COLOR_BG and apart from UI_COLOR_TEXT_SECONDARY.
unsigned int UI_MakeAccentLegible(unsigned int color);

// Points ui_color_accent / ui_color_accent_wash at the color derived from
// `cover`. A NULL cover, or one with no usable hue, restores UI_ACCENT_FIXED.
// Called once per track load, never per frame.
void UI_Theme_SetAccentFromCoverArt(const vita2d_texture *cover);
// Restores the fixed accent, for when no track is loaded.
void UI_Theme_ResetAccent(void);

// Format badge styling (color + label) for a recognized audio extension.
// Returns SCE_FALSE (and leaves outputs untouched) for an unrecognized extension.
SceBool UI_GetFormatBadge(const char *ext, const char **out_label, unsigned int *out_color, unsigned int *out_wash);

#endif
