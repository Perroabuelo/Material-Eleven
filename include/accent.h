#ifndef _ELEVENMPV_ACCENT_H_
#define _ELEVENMPV_ACCENT_H_

// Color math behind the dynamic accent: the hue histogram that finds a cover's
// dominant color, and the clamp that keeps the result legible.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_accent.c). Colors are RGBA8 packed the way vita2d packs them, red
// in the low byte. Reading the cover texture itself stays in ui_theme.c.

#define ACCENT_RGBA8(r, g, b, a) ((((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | (((g) & 0xFF) << 8) | (((r) & 0xFF) << 0))

#define UI_HUE_BUCKETS 24
// Below this share of sampled pixels, the cover has no usable hue at all and
// counts as ACCENT_COVER_ACHROMATIC. 1 % still catches a small detail of color,
// such as a red title over a gray photo (about 4 %), while a black and white
// cover stays at 0.5 % or less. The share only decides whether there is a hue,
// never which hue wins, so a cover above the old 6 % keeps the same peak.
#define UI_HUE_MIN_SHARE 0.01f

typedef struct {
	float weight[UI_HUE_BUCKETS];
	float sum_r[UI_HUE_BUCKETS], sum_g[UI_HUE_BUCKETS], sum_b[UI_HUE_BUCKETS];
	int count[UI_HUE_BUCKETS];
	int sampled, chromatic;
} Accent_HueHistogram;

// Feeds a subsampled grid of an image into `hist`, which must start zeroed.
// `bytes_per_pixel` is 4 (R, G, B, A), 3 (R, G, B) or 1 (luminance only), and
// `stride` is in bytes. Returns 0 when the image cannot be sampled at all.
int Accent_SamplePixels(const unsigned char *pixels, unsigned int w, unsigned int h, unsigned int stride,
	unsigned int bytes_per_pixel, Accent_HueHistogram *hist);

// Mean color of the winning hue bucket. Returns 0, leaving `out_color`
// untouched, when fewer than `min_share` of the sampled pixels carry a hue.
int Accent_HistogramPeak(const Accent_HueHistogram *hist, float min_share, unsigned int *out_color);

// What a cover gives the accent. "No cover" and "a cover without color" are
// kept apart: the first keeps the fixed accent, the second gets the neutral one.
typedef enum {
	ACCENT_COVER_NONE = 0,   // no image, or one that could not be read
	ACCENT_COVER_ACHROMATIC, // read, but under UI_HUE_MIN_SHARE of it has a hue
	ACCENT_COVER_CHROMATIC   // read, and `out_color` holds its dominant color
} Accent_Cover;

// Classifies an image with the two calls above and UI_HUE_MIN_SHARE. Only
// ACCENT_COVER_CHROMATIC writes `out_color`.
Accent_Cover Accent_ClassifyCover(const unsigned char *pixels, unsigned int w, unsigned int h, unsigned int stride,
	unsigned int bytes_per_pixel, unsigned int *out_color);

// Color for a glyph drawn over a fill of the accent `kind` produced: `dark`
// over the light neutral accent, `light` over every other one, which keeps
// the look the colored and fixed accents always had.
unsigned int Accent_OnAccentColor(Accent_Cover kind, unsigned int dark, unsigned int light);

// Clamps a color's saturation and lightness into the band that stays legible
// over `bg` and apart from UI_COLOR_TEXT_SECONDARY.
unsigned int Accent_MakeLegible(unsigned int color, unsigned int bg);

#endif
