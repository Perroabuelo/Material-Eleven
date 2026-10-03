#include <math.h>
#include <stddef.h>

#include "accent.h"

// Cover-art sampling: roughly a 48x48 grid of samples, whatever the cover's
// real size. Runs once per track load, not per frame.
#define UI_COVER_SAMPLE_GRID 48
// A pixel needs this much saturation, and a lightness away from both extremes,
// before its hue counts as a vote.
#define UI_HUE_MIN_SAT 0.18f
#define UI_HUE_MIN_LUM 0.10f
#define UI_HUE_MAX_LUM 0.92f

// The fixed accent #FF9166 sits at S 1.00 / L 0.70. This band brackets it, so a
// derived accent lands in the same contrast range against UI_COLOR_BG (L 0.08)
// and stays apart from UI_COLOR_TEXT_SECONDARY (S 0.20 / L 0.70) by saturation.
#define UI_ACCENT_MIN_SAT 0.60f
#define UI_ACCENT_MAX_SAT 0.95f
#define UI_ACCENT_MIN_LUM 0.58f
#define UI_ACCENT_MAX_LUM 0.76f
// The HSL band alone is not enough: lightness is a poor stand-in for perceived
// luminance, so a violet at L 0.60 reads far darker than an orange at the same
// L. These drive a second pass that lifts the lightness until the accent clears
// a real contrast floor over the background.
#define UI_ACCENT_MIN_CONTRAST 4.5f
#define UI_ACCENT_LUM_CEILING 0.88f
#define UI_ACCENT_LUM_STEP 0.02f

static void UI_RgbToHsl(float r, float g, float b, float *out_h, float *out_s, float *out_l) {
	float max = r > g ? (r > b ? r : b) : (g > b ? g : b);
	float min = r < g ? (r < b ? r : b) : (g < b ? g : b);
	float span = max - min;

	*out_l = (max + min) / 2.0f;

	if (span < 0.0001f) {
		*out_h = 0.0f;
		*out_s = 0.0f;
		return;
	}

	*out_s = (*out_l > 0.5f) ? (span / (2.0f - max - min)) : (span / (max + min));

	float hue;
	if (max == r)
		hue = (g - b) / span + (g < b ? 6.0f : 0.0f);
	else if (max == g)
		hue = (b - r) / span + 2.0f;
	else
		hue = (r - g) / span + 4.0f;

	*out_h = hue / 6.0f;
}

static float UI_HueToChannel(float p, float q, float t) {
	if (t < 0.0f)
		t += 1.0f;
	if (t > 1.0f)
		t -= 1.0f;

	if (t < 1.0f / 6.0f)
		return p + (q - p) * 6.0f * t;
	if (t < 1.0f / 2.0f)
		return q;
	if (t < 2.0f / 3.0f)
		return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;

	return p;
}

static unsigned int UI_HslToRgba(float h, float s, float l) {
	float r, g, b;

	if (s < 0.0001f)
		r = g = b = l;
	else {
		float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
		float p = 2.0f * l - q;
		r = UI_HueToChannel(p, q, h + 1.0f / 3.0f);
		g = UI_HueToChannel(p, q, h);
		b = UI_HueToChannel(p, q, h - 1.0f / 3.0f);
	}

	return ACCENT_RGBA8((int)(r * 255.0f + 0.5f), (int)(g * 255.0f + 0.5f), (int)(b * 255.0f + 0.5f), 255);
}

// Byte 0 of a pixel is red in all three layouts; the 3 and 4 byte ones
// continue with green and blue, the 1 byte one is a single luminance channel.
static int UI_CoverReadPixel(const unsigned char *px, unsigned int bytes_per_pixel,
	float *out_r, float *out_g, float *out_b) {
	if (bytes_per_pixel == 4 && px[3] < 128)
		return 0; // transparent, carries no color

	*out_r = px[0] / 255.0f;
	*out_g = (bytes_per_pixel >= 3) ? (px[1] / 255.0f) : *out_r;
	*out_b = (bytes_per_pixel >= 3) ? (px[2] / 255.0f) : *out_r;

	return 1;
}

static void UI_HueHistogramAdd(Accent_HueHistogram *hist, float r, float g, float b) {
	float hue, sat, lum;
	int bucket;

	hist->sampled++;
	UI_RgbToHsl(r, g, b, &hue, &sat, &lum);

	// Flat black, flat white and gray carry no hue to vote with.
	if (sat < UI_HUE_MIN_SAT || lum < UI_HUE_MIN_LUM || lum > UI_HUE_MAX_LUM)
		return;

	bucket = (int)(hue * UI_HUE_BUCKETS);
	if (bucket < 0)
		bucket = 0;
	if (bucket >= UI_HUE_BUCKETS)
		bucket = UI_HUE_BUCKETS - 1;

	// Weighted by saturation, so a vivid minority outvotes a washed-out majority.
	hist->weight[bucket] += sat;
	hist->sum_r[bucket] += r;
	hist->sum_g[bucket] += g;
	hist->sum_b[bucket] += b;
	hist->count[bucket]++;
	hist->chromatic++;
}

int Accent_SamplePixels(const unsigned char *pixels, unsigned int w, unsigned int h, unsigned int stride,
	unsigned int bytes_per_pixel, Accent_HueHistogram *hist) {
	unsigned int step_x, step_y;

	if (!pixels || !hist || w == 0 || h == 0 || stride == 0)
		return 0;
	if (bytes_per_pixel != 1 && bytes_per_pixel != 3 && bytes_per_pixel != 4)
		return 0;

	step_x = (w + UI_COVER_SAMPLE_GRID - 1) / UI_COVER_SAMPLE_GRID;
	step_y = (h + UI_COVER_SAMPLE_GRID - 1) / UI_COVER_SAMPLE_GRID;
	if (step_x == 0)
		step_x = 1;
	if (step_y == 0)
		step_y = 1;

	for (unsigned int y = 0; y < h; y += step_y) {
		const unsigned char *row = pixels + (size_t)y * stride;

		for (unsigned int x = 0; x < w; x += step_x) {
			float r, g, b;

			if (UI_CoverReadPixel(row + (size_t)x * bytes_per_pixel, bytes_per_pixel, &r, &g, &b))
				UI_HueHistogramAdd(hist, r, g, b);
		}
	}

	return 1;
}

int Accent_HistogramPeak(const Accent_HueHistogram *hist, float min_share, unsigned int *out_color) {
	int best = 0;
	float inv;

	if (hist->sampled == 0 || (float)hist->chromatic < (float)hist->sampled * min_share)
		return 0;

	for (int i = 1; i < UI_HUE_BUCKETS; i++) {
		if (hist->weight[i] > hist->weight[best])
			best = i;
	}

	if (hist->count[best] == 0)
		return 0;

	inv = 1.0f / (float)hist->count[best];
	*out_color = ACCENT_RGBA8((int)(hist->sum_r[best] * inv * 255.0f + 0.5f), (int)(hist->sum_g[best] * inv * 255.0f + 0.5f),
		(int)(hist->sum_b[best] * inv * 255.0f + 0.5f), 255);

	return 1;
}

Accent_Cover Accent_ClassifyCover(const unsigned char *pixels, unsigned int w, unsigned int h, unsigned int stride,
	unsigned int bytes_per_pixel, unsigned int *out_color) {
	Accent_HueHistogram hist = {0};

	if (!out_color || !Accent_SamplePixels(pixels, w, h, stride, bytes_per_pixel, &hist))
		return ACCENT_COVER_NONE;

	return Accent_HistogramPeak(&hist, UI_HUE_MIN_SHARE, out_color) ? ACCENT_COVER_CHROMATIC : ACCENT_COVER_ACHROMATIC;
}

static float UI_SrgbToLinear(float c) {
	return (c <= 0.03928f) ? (c / 12.92f) : powf((c + 0.055f) / 1.055f, 2.4f);
}

static float UI_RelativeLuminance(unsigned int color) {
	return 0.2126f * UI_SrgbToLinear((float)(color & 0xFF) / 255.0f)
		+ 0.7152f * UI_SrgbToLinear((float)((color >> 8) & 0xFF) / 255.0f)
		+ 0.0722f * UI_SrgbToLinear((float)((color >> 16) & 0xFF) / 255.0f);
}

static float UI_ContrastOver(unsigned int color, unsigned int bg) {
	float lum = UI_RelativeLuminance(color);
	float bg_lum = UI_RelativeLuminance(bg);

	return (lum > bg_lum) ? ((lum + 0.05f) / (bg_lum + 0.05f)) : ((bg_lum + 0.05f) / (lum + 0.05f));
}

unsigned int Accent_MakeLegible(unsigned int color, unsigned int bg) {
	float r = (float)(color & 0xFF) / 255.0f;
	float g = (float)((color >> 8) & 0xFF) / 255.0f;
	float b = (float)((color >> 16) & 0xFF) / 255.0f;
	float hue, sat, lum;
	unsigned int accent;

	UI_RgbToHsl(r, g, b, &hue, &sat, &lum);

	if (sat < UI_ACCENT_MIN_SAT)
		sat = UI_ACCENT_MIN_SAT;
	if (sat > UI_ACCENT_MAX_SAT)
		sat = UI_ACCENT_MAX_SAT;
	if (lum < UI_ACCENT_MIN_LUM)
		lum = UI_ACCENT_MIN_LUM;
	if (lum > UI_ACCENT_MAX_LUM)
		lum = UI_ACCENT_MAX_LUM;

	accent = UI_HslToRgba(hue, sat, lum);

	// Second pass on perceived luminance, not HSL lightness: a blue or violet
	// sits well below an orange of the same lightness, so lift it until the
	// accent actually clears the contrast floor over the background.
	while (lum < UI_ACCENT_LUM_CEILING && UI_ContrastOver(accent, bg) < UI_ACCENT_MIN_CONTRAST) {
		lum += UI_ACCENT_LUM_STEP;
		accent = UI_HslToRgba(hue, sat, lum);
	}

	return accent;
}
