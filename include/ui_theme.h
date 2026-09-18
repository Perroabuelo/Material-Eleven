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

// Five sizes, derived from the panel rather than copied from the mockups. The
// console is 960x544 across 5 inches, about 220.7 ppp, so one density-
// independent unit of the design language this skin imitates is 1.379 px. The
// mockups were reviewed in a desktop browser, where that canvas spans some
// 25 cm instead of 11, and their pixel values were taken into the code as-is:
// the old scale of 11-22 px came out at 8 to 16 units, with its two smallest
// steps below the language's own minimum label size. These five are that scale
// corrected and consolidated - seven steps of hierarchy is more than 960x544
// can express anyway.
typedef enum {
	UI_TS_BADGE = 0, // 15 px - badges, clock, battery, small mono
	UI_TS_LABEL,     // 17 px - labels, button hints, secondary rows
	UI_TS_BODY,      // 19 px - list entries, settings items
	UI_TS_TITLE,     // 22 px - screen and section headers
	UI_TS_DISPLAY,   // 30 px - the now-playing track title
	UI_TS_COUNT
} UI_TextSize;

// All four tolerate a face whose font failed to load: they draw nothing and
// measure zero rather than dereferencing a null handle.
void UI_DrawText(UI_Face face, UI_TextSize ts, float x, float baseline_y, unsigned int color, const char *text);
int UI_TextWidth(UI_Face face, UI_TextSize ts, const char *text);
int UI_TextHeight(UI_Face face, UI_TextSize ts, const char *text);
void UI_TextDimensions(UI_Face face, UI_TextSize ts, const char *text, int *out_w, int *out_h);

// Draws `text` from `x`, never spilling past `x + max_w`. Text that does not
// fit is cut off at a rectangular clip and followed by a continuation
// indicator, so a long file name stops short of the badge beside it instead of
// running through it. Clipping is enabled and disabled inside this one call:
// nothing can leave it switched on by returning early, which would make
// whatever drew next disappear.
void UI_DrawTextClipped(UI_Face face, UI_TextSize ts, float x, float baseline_y, float max_w,
	unsigned int color, const char *text);

// ---- Non-Latin coverage ----
//
// Manrope and IBM Plex Mono carry no Hangul, kana or CJK at all, and only part
// of Cyrillic - 40.6% and 65.6% of the block. An uncovered codepoint resolves
// to glyph index 0, and since the atlas is keyed by glyph index every one of
// them collapses onto the same .notdef and draws as the same repeated shape.
// So the text calls above split a string into runs of consecutive codepoints
// one engine can draw, and hand the rest to the console's own fonts. Call sites
// need do nothing; drawing and measuring go through the same walker so they
// cannot disagree.
//
// The fallback rasterises at one fixed size - vita2d_load_system_pvf hardcodes
// scePvfSetCharSize - so it is only sharp near that size. Measured on hardware:
// fine at the badge, label and body tokens, soft from the title token up.
#define UI_PVF_NATIVE_PX 18.0f

// True when some codepoint in `text` is outside what `face` covers, so the
// caller can pick a token the fallback can draw sharply. Now Playing uses it to
// drop a track title from the display token to body when the title is not
// Latin: a smaller sharp title beats a large soft one.
SceBool UI_TextNeedsFallback(UI_Face face, const char *text);

// The largest token whose fallback scale still reads well.
#define UI_TS_FALLBACK_MAX UI_TS_BODY

// The fallback atlas has no eviction, so a long session over CJK content would
// fill its sheet and start dropping glyphs silently. Once enough distinct
// uncovered codepoints have been drawn, the handle is marked for renewal; this
// performs it. It destroys and recreates a GPU resource, which is the exact
// operation behind the second crash this change was written after, so it must
// be called between frames and never during one. Folder changes are the chosen
// moment: there is already a pause there for disk reads.
void UI_Theme_RenewFallbackIfNeeded(void);

// State of the fallback, for the debug overlay. `out_seen` is how many distinct
// uncovered codepoints have been drawn, `out_renewals` how many times the
// handle has been replaced. Returns false when the fallback is not usable at
// all - either the console gave no font, or FreeType could not open our own
// faces to ask what they cover, in which case nothing is ever routed to it.
SceBool UI_Theme_FallbackStatus(int *out_seen, int *out_renewals, SceBool *out_can_query);

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
#define UI_HINT_BAR_HEIGHT 40

// Minimum touch target: 48 density-independent units, which at 1.379 px per
// unit is 66 px, or 7.6 mm on the panel. The old 44 and 40 px targets were
// 5.1 and 4.6 mm - the same mockup-pixel mistake the type scale had. The drawn
// control keeps its own size; only the area that answers a touch grows, which
// is what UI_TouchTarget is for.
#define UI_TOUCH_MIN 66

// True when the current touch falls inside [x, x+w) x [y, y+h) after that box
// has been grown about its own centre to at least UI_TOUCH_MIN on each axis.
// A box already that big is hit-tested unchanged.
SceBool UI_TouchTarget(float x, float y, float w, float h);

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

// Baseline Y that centers the face's own ascender-to-descender extent within
// [box_top, box_top+box_h). It takes no string on purpose: a baseline derived
// from the string being drawn shifts with its content, so a row without
// descenders sat lower than its neighbour and the elapsed time hopped as its
// digits changed.
int UI_TextBaselineY(UI_Face face, UI_TextSize ts, float box_top, float box_h);

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
