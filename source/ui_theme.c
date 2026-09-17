#include <math.h>
#include <string.h>

#include "common.h"
#include "ui_theme.h"

// vita2d submits its own 2D primitives at this depth; the vector helpers below
// match it so they interleave with vita2d_draw_rectangle in submission order.
#define UI_VEC_Z 0.5f
// Segments around a full ring - 24 keeps the gear/knob edges smooth at the
// sizes this app draws them (radius <= 12).
#define UI_RING_SEGMENTS 24
// Segments per 90-degree corner arc, scaled with the radius. The radii in use
// run from 2 (the seek bar pill) to 38 (the play button).
#define UI_ARC_MIN_SEGMENTS 6
#define UI_ARC_MAX_SEGMENTS 16

// Cover-art sampling: roughly a 48x48 grid of samples, whatever the cover's
// real size. Runs once per track load, not per frame.
#define UI_COVER_SAMPLE_GRID 48
#define UI_HUE_BUCKETS 24
// A pixel needs this much saturation, and a lightness away from both extremes,
// before its hue counts as a vote.
#define UI_HUE_MIN_SAT 0.18f
#define UI_HUE_MIN_LUM 0.10f
#define UI_HUE_MAX_LUM 0.92f
// Below this share of sampled pixels, the cover has no usable hue at all and
// the caller keeps the fixed accent.
#define UI_HUE_MIN_SHARE 0.06f

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

vita2d_font *font_ui = NULL;
vita2d_font *font_mono = NULL;

unsigned int ui_color_accent = UI_ACCENT_FIXED;
unsigned int ui_color_accent_wash = RGBA8(0xFF, 0x91, 0x66, UI_ACCENT_WASH_ALPHA);

void UI_Theme_Load(void) {
	font_ui = vita2d_load_font_file("app0:Manrope.ttf");
	font_mono = vita2d_load_font_file("app0:IBMPlexMono-Medium.ttf");
}

void UI_Theme_Free(void) {
	vita2d_free_font(font_mono);
	vita2d_free_font(font_ui);
}

static void UI_SetVertex(vita2d_color_vertex *v, float x, float y, unsigned int color) {
	v->x = x;
	v->y = y;
	v->z = UI_VEC_Z;
	v->color = color;
}

// vita2d_draw_array hands the pointer straight to sceGxmSetVertexStream without
// copying it, and sceGxmDraw only queues the draw - the GPU reads the vertices
// later, when the frame is flushed. So they must live in GPU-visible memory that
// outlives this call, never on the stack. vita2d's frame pool is exactly that,
// and vita2d_start_drawing resets it each frame. It returns NULL when full.
static vita2d_color_vertex *UI_VertexBuffer(unsigned int count) {
	return (vita2d_color_vertex *)vita2d_pool_memalign(count * sizeof(vita2d_color_vertex), sizeof(vita2d_color_vertex));
}

void UI_DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, unsigned int color) {
	vita2d_color_vertex *v = UI_VertexBuffer(3);

	if (!v)
		return;

	UI_SetVertex(&v[0], x0, y0, color);
	UI_SetVertex(&v[1], x1, y1, color);
	UI_SetVertex(&v[2], x2, y2, color);

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLES, v, 3);
}

void UI_DrawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, unsigned int color) {
	vita2d_color_vertex *v = UI_VertexBuffer(4);

	if (!v)
		return;

	UI_SetVertex(&v[0], x0, y0, color);
	UI_SetVertex(&v[1], x1, y1, color);
	UI_SetVertex(&v[2], x2, y2, color);
	UI_SetVertex(&v[3], x3, y3, color);

	// A fan over 4 perimeter-ordered vertices is (0,1,2) + (0,2,3).
	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_FAN, v, 4);
}

void UI_DrawStroke(float x0, float y0, float x1, float y1, float thickness, unsigned int color) {
	float dx = x1 - x0, dy = y1 - y0;
	float len = sqrtf(dx * dx + dy * dy);
	float r = thickness / 2.0f;

	if (len < 0.001f) {
		vita2d_draw_fill_circle(x0, y0, r, color);
		return;
	}

	// Offset perpendicular to the segment by half the thickness on each side.
	float nx = (-dy / len) * r, ny = (dx / len) * r;
	UI_DrawQuad(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, color);

	vita2d_draw_fill_circle(x0, y0, r, color);
	vita2d_draw_fill_circle(x1, y1, r, color);
}

void UI_DrawRing(float cx, float cy, float radius, float thickness, unsigned int color) {
	vita2d_color_vertex *v = UI_VertexBuffer((UI_RING_SEGMENTS + 1) * 2);
	float inner = radius - thickness;

	if (!v)
		return;

	if (inner < 0.0f)
		inner = 0.0f;

	for (int i = 0; i <= UI_RING_SEGMENTS; i++) {
		float a = (2.0f * UI_PI * (float)i) / (float)UI_RING_SEGMENTS;
		float c = cosf(a), s = sinf(a);

		UI_SetVertex(&v[i * 2], cx + c * radius, cy + s * radius, color);
		UI_SetVertex(&v[i * 2 + 1], cx + c * inner, cy + s * inner, color);
	}

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, v, (UI_RING_SEGMENTS + 1) * 2);
}

// Nudged right by a fraction of its width so it reads optically centered.
void UI_DrawPlayGlyph(float cx, float cy, float size, unsigned int color) {
	float h = size, w = size * 0.87f;
	float left = cx - w * 0.38f;

	UI_DrawTriangle(left, cy - h / 2.0f, left + w, cy, left, cy + h / 2.0f, color);
}

void UI_DrawPauseGlyph(float cx, float cy, float size, unsigned int color) {
	float bw = size * 0.26f, gap = size * 0.18f, h = size * 0.92f;

	vita2d_draw_rectangle(cx - gap / 2.0f - bw, cy - h / 2.0f, bw, h, color);
	vita2d_draw_rectangle(cx + gap / 2.0f, cy - h / 2.0f, bw, h, color);
}

void UI_DrawSkipGlyph(float cx, float cy, float size, SceBool forward, unsigned int color) {
	float h = size * 0.68f, tw = size * 0.30f, bw = size * 0.11f;
	float left = cx - (tw * 2.0f + bw) / 2.0f;
	float top = cy - h / 2.0f, bottom = cy + h / 2.0f;

	if (forward) {
		UI_DrawTriangle(left, top, left + tw, cy, left, bottom, color);
		UI_DrawTriangle(left + tw, top, left + tw * 2.0f, cy, left + tw, bottom, color);
		vita2d_draw_rectangle(left + tw * 2.0f, top, bw, h, color);
	}
	else {
		vita2d_draw_rectangle(left, top, bw, h, color);
		UI_DrawTriangle(left + bw + tw, top, left + bw, cy, left + bw + tw, bottom, color);
		UI_DrawTriangle(left + bw + tw * 2.0f, top, left + bw + tw, cy, left + bw + tw * 2.0f, bottom, color);
	}
}

// Quarter disc filling one `radius` x `radius` corner box, as a triangle fan
// from the corner's center. The fan's first and last radii land exactly on the
// box edges, so the arc butts against the straight bands without overlapping
// them - which matters because rounded rects are also drawn in semi-transparent
// colors (the accent wash, the hairline), where an overlap would show a seam.
static void UI_DrawCornerArc(float cx, float cy, float radius, float start_angle, unsigned int color) {
	int segments = (int)(radius * 0.7f);
	vita2d_color_vertex *v;

	if (segments < UI_ARC_MIN_SEGMENTS)
		segments = UI_ARC_MIN_SEGMENTS;
	if (segments > UI_ARC_MAX_SEGMENTS)
		segments = UI_ARC_MAX_SEGMENTS;

	v = UI_VertexBuffer(segments + 2);
	if (!v)
		return;

	UI_SetVertex(&v[0], cx, cy, color);

	for (int i = 0; i <= segments; i++) {
		float a = start_angle + (UI_PI / 2.0f) * ((float)i / (float)segments);
		UI_SetVertex(&v[i + 1], cx + cosf(a) * radius, cy + sinf(a) * radius, color);
	}

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_FAN, v, segments + 2);
}

void UI_DrawRoundedRect(float x, float y, float w, float h, int radius, unsigned int color) {
	if (radius <= 0 || w <= 0 || h <= 0) {
		if (w > 0 && h > 0)
			vita2d_draw_rectangle(x, y, w, h, color);
		return;
	}

	float max_radius = (w < h ? w : h) / 2.0f;
	if (radius > max_radius)
		radius = (int)max_radius;

	if (radius <= 0) {
		vita2d_draw_rectangle(x, y, w, h, color);
		return;
	}

	float r = (float)radius;

	// Vertical band covering the full height minus the left/right corner columns.
	vita2d_draw_rectangle(x + r, y, w - 2 * r, h, color);
	// Left / right middle strips, between the top and bottom corners.
	if (h - 2 * r > 0) {
		vita2d_draw_rectangle(x, y + r, r, h - 2 * r, color);
		vita2d_draw_rectangle(x + w - r, y + r, r, h - 2 * r, color);
	}

	// Screen space has y growing downwards, so each arc sweeps +90 degrees
	// clockwise from the start angle given here.
	UI_DrawCornerArc(x + r, y + r, r, UI_PI, color); // top-left
	UI_DrawCornerArc(x + w - r, y + r, r, UI_PI * 1.5f, color); // top-right
	UI_DrawCornerArc(x + w - r, y + h - r, r, 0.0f, color); // bottom-right
	UI_DrawCornerArc(x + r, y + h - r, r, UI_PI * 0.5f, color); // bottom-left
}

void UI_DrawPill(float x, float y, float w, float h, unsigned int color) {
	UI_DrawRoundedRect(x, y, w, h, (int)(h / 2.0f), color);
}

void UI_DrawBadge(float x, float y, unsigned int size, const char *label, unsigned int bg, unsigned int fg, unsigned int border) {
	int text_w = vita2d_font_text_width(font_mono, size, label);
	int text_h = vita2d_font_text_height(font_mono, size, label);
	float pad_x = 8.0f, pad_y = 4.0f;
	float w = text_w + pad_x * 2;
	float h = text_h + pad_y * 2;

	if (border) {
		UI_DrawPill(x, y, w, h, border);
		UI_DrawPill(x + 1, y + 1, w - 2, h - 2, bg);
	}
	else
		UI_DrawPill(x, y, w, h, bg);

	vita2d_font_draw_text(font_mono, x + pad_x, UI_TextBaselineY(font_mono, size, label, y, h), fg, size, label);
}

void UI_DrawRowHighlight(float x, float y, float w, float h) {
	UI_DrawRoundedRect(x, y, w, h, UI_RADIUS_SM + 2, ui_color_accent_wash);
}

int UI_TextBaselineY(vita2d_font *f, unsigned int size, const char *text, float box_top, float box_h) {
	int height = vita2d_font_text_height(f, size, text);
	return (int)(box_top + ((box_h - height) / 2) + height);
}

// ---- Dynamic accent derived from cover art ----

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

	return RGBA8((int)(r * 255.0f + 0.5f), (int)(g * 255.0f + 0.5f), (int)(b * 255.0f + 0.5f), 255);
}

// vita2d decodes cover art straight into a GPU texture, always in linear layout
// (the library only ever calls sceGxmTextureInitLinear), so the pixels are
// readable from the CPU. The two loaders produce different formats:
// vita2d_load_PNG_buffer goes through vita2d_create_empty_texture (4 bytes per
// pixel, U8U8U8U8_ABGR) while vita2d_load_JPEG_buffer asks for U8U8U8_BGR
// (3 bytes), or U8_R (1 byte) for a grayscale JPEG. In each of those the
// swizzle's least significant channel is red, so byte 0 of a pixel is always R.
static SceBool UI_CoverBytesPerPixel(SceGxmTextureFormat format, unsigned int *out_bytes) {
	switch (format) {
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR: *out_bytes = 4; return SCE_TRUE;
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR: *out_bytes = 3; return SCE_TRUE;
		case SCE_GXM_TEXTURE_FORMAT_U8_R: *out_bytes = 1; return SCE_TRUE;
		default: return SCE_FALSE;
	}
}

typedef struct {
	const unsigned char *pixels;
	unsigned int w, h, stride, bytes_per_pixel, step_x, step_y;
} UI_CoverSampler;

typedef struct {
	float weight[UI_HUE_BUCKETS];
	float sum_r[UI_HUE_BUCKETS], sum_g[UI_HUE_BUCKETS], sum_b[UI_HUE_BUCKETS];
	int count[UI_HUE_BUCKETS];
	int sampled, chromatic;
} UI_HueHistogram;

static SceBool UI_CoverSamplerInit(const vita2d_texture *cover, UI_CoverSampler *s) {
	if (!cover)
		return SCE_FALSE;

	if (!UI_CoverBytesPerPixel(vita2d_texture_get_format(cover), &s->bytes_per_pixel))
		return SCE_FALSE;

	s->pixels = (const unsigned char *)vita2d_texture_get_datap(cover);
	s->w = vita2d_texture_get_width(cover);
	s->h = vita2d_texture_get_height(cover);
	s->stride = vita2d_texture_get_stride(cover);

	if (!s->pixels || s->w == 0 || s->h == 0 || s->stride == 0)
		return SCE_FALSE;

	s->step_x = (s->w + UI_COVER_SAMPLE_GRID - 1) / UI_COVER_SAMPLE_GRID;
	s->step_y = (s->h + UI_COVER_SAMPLE_GRID - 1) / UI_COVER_SAMPLE_GRID;
	if (s->step_x == 0)
		s->step_x = 1;
	if (s->step_y == 0)
		s->step_y = 1;

	return SCE_TRUE;
}

// Byte 0 of a pixel is red in all three formats above; the 3 and 4 byte ones
// continue with green and blue, the 1 byte one is a single luminance channel.
static SceBool UI_CoverReadPixel(const unsigned char *px, unsigned int bytes_per_pixel,
	float *out_r, float *out_g, float *out_b) {
	if (bytes_per_pixel == 4 && px[3] < 128)
		return SCE_FALSE; // transparent, carries no color

	*out_r = px[0] / 255.0f;
	*out_g = (bytes_per_pixel >= 3) ? (px[1] / 255.0f) : *out_r;
	*out_b = (bytes_per_pixel >= 3) ? (px[2] / 255.0f) : *out_r;

	return SCE_TRUE;
}

static void UI_HueHistogramAdd(UI_HueHistogram *hist, float r, float g, float b) {
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

static SceBool UI_HueHistogramPeak(const UI_HueHistogram *hist, unsigned int *out_color) {
	int best = 0;
	float inv;

	if (hist->sampled == 0 || (float)hist->chromatic < (float)hist->sampled * UI_HUE_MIN_SHARE)
		return SCE_FALSE;

	for (int i = 1; i < UI_HUE_BUCKETS; i++) {
		if (hist->weight[i] > hist->weight[best])
			best = i;
	}

	if (hist->count[best] == 0)
		return SCE_FALSE;

	inv = 1.0f / (float)hist->count[best];
	*out_color = RGBA8((int)(hist->sum_r[best] * inv * 255.0f + 0.5f), (int)(hist->sum_g[best] * inv * 255.0f + 0.5f),
		(int)(hist->sum_b[best] * inv * 255.0f + 0.5f), 255);

	return SCE_TRUE;
}

SceBool UI_CoverDominantColor(const vita2d_texture *cover, unsigned int *out_color) {
	UI_HueHistogram hist;
	UI_CoverSampler s;

	if (!out_color || !UI_CoverSamplerInit(cover, &s))
		return SCE_FALSE;

	memset(&hist, 0, sizeof(hist));

	for (unsigned int y = 0; y < s.h; y += s.step_y) {
		const unsigned char *row = s.pixels + (size_t)y * s.stride;

		for (unsigned int x = 0; x < s.w; x += s.step_x) {
			float r, g, b;

			if (UI_CoverReadPixel(row + (size_t)x * s.bytes_per_pixel, s.bytes_per_pixel, &r, &g, &b))
				UI_HueHistogramAdd(&hist, r, g, b);
		}
	}

	return UI_HueHistogramPeak(&hist, out_color);
}

static float UI_SrgbToLinear(float c) {
	return (c <= 0.03928f) ? (c / 12.92f) : powf((c + 0.055f) / 1.055f, 2.4f);
}

static float UI_RelativeLuminance(unsigned int color) {
	return 0.2126f * UI_SrgbToLinear((float)(color & 0xFF) / 255.0f)
		+ 0.7152f * UI_SrgbToLinear((float)((color >> 8) & 0xFF) / 255.0f)
		+ 0.0722f * UI_SrgbToLinear((float)((color >> 16) & 0xFF) / 255.0f);
}

static float UI_ContrastOverBg(unsigned int color) {
	float lum = UI_RelativeLuminance(color);
	float bg = UI_RelativeLuminance(UI_COLOR_BG);

	return (lum > bg) ? ((lum + 0.05f) / (bg + 0.05f)) : ((bg + 0.05f) / (lum + 0.05f));
}

unsigned int UI_MakeAccentLegible(unsigned int color) {
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
	while (lum < UI_ACCENT_LUM_CEILING && UI_ContrastOverBg(accent) < UI_ACCENT_MIN_CONTRAST) {
		lum += UI_ACCENT_LUM_STEP;
		accent = UI_HslToRgba(hue, sat, lum);
	}

	return accent;
}

static void UI_Theme_ApplyAccent(unsigned int accent) {
	ui_color_accent = accent;
	ui_color_accent_wash = RGBA8(accent & 0xFF, (accent >> 8) & 0xFF, (accent >> 16) & 0xFF, UI_ACCENT_WASH_ALPHA);
}

void UI_Theme_ResetAccent(void) {
	UI_Theme_ApplyAccent(UI_ACCENT_FIXED);
}

void UI_Theme_SetAccentFromCoverArt(const vita2d_texture *cover) {
	unsigned int dominant = 0;

	if (cover && UI_CoverDominantColor(cover, &dominant))
		UI_Theme_ApplyAccent(UI_MakeAccentLegible(dominant));
	else
		UI_Theme_ResetAccent();
}

SceBool UI_GetFormatBadge(const char *ext, const char **out_label, unsigned int *out_color, unsigned int *out_wash) {
	if (!strncasecmp(ext, "flac", 4)) { *out_label = "FLAC"; *out_color = UI_COLOR_LOSSLESS; *out_wash = UI_COLOR_LOSSLESS_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "wav", 4)) { *out_label = "WAV"; *out_color = UI_COLOR_LOSSLESS; *out_wash = UI_COLOR_LOSSLESS_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "mp3", 4)) { *out_label = "MP3"; *out_color = UI_COLOR_LOSSY; *out_wash = UI_COLOR_LOSSY_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "ogg", 4)) { *out_label = "OGG"; *out_color = UI_COLOR_LOSSY; *out_wash = UI_COLOR_LOSSY_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "opus", 4)) { *out_label = "OPUS"; *out_color = UI_COLOR_LOSSY; *out_wash = UI_COLOR_LOSSY_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "it", 4)) { *out_label = "IT"; *out_color = UI_COLOR_TRACKER; *out_wash = UI_COLOR_TRACKER_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "mod", 4)) { *out_label = "MOD"; *out_color = UI_COLOR_TRACKER; *out_wash = UI_COLOR_TRACKER_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "s3m", 4)) { *out_label = "S3M"; *out_color = UI_COLOR_TRACKER; *out_wash = UI_COLOR_TRACKER_WASH; return SCE_TRUE; }
	if (!strncasecmp(ext, "xm", 4)) { *out_label = "XM"; *out_color = UI_COLOR_TRACKER; *out_wash = UI_COLOR_TRACKER_WASH; return SCE_TRUE; }
	return SCE_FALSE;
}
