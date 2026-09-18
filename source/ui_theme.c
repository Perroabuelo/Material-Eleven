#include <math.h>
#include <string.h>

#include <psp2/pvf.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "common.h"
#include "touch.h"
#include "ui_gpu.h"
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

unsigned int ui_color_accent = UI_ACCENT_FIXED;
unsigned int ui_color_accent_wash = RGBA8(0xFF, 0x91, 0x66, UI_ACCENT_WASH_ALPHA);

// ---- Text ----

// Device pixels per token. Each is its unit value times the 1.379 px per
// density-independent unit this panel works out to; see UI_TextSize.
const unsigned int ui_text_px[UI_TS_COUNT] = {
	[UI_TS_BADGE] = 15,   // 10.9 units - the language's minimum label size
	[UI_TS_LABEL] = 17,   // 12.3
	[UI_TS_BODY] = 19,    // 13.8
	[UI_TS_TITLE] = 22,   // 16.0
	[UI_TS_DISPLAY] = 30, // 21.8
};

static const char *const ui_face_file[UI_FACE_COUNT] = {
	[UI_FACE_UI] = "app0:Manrope.ttf",
	[UI_FACE_MONO] = "app0:IBMPlexMono-Medium.ttf",
};

// One handle per face and size actually drawn, because vita2d_font's glyph
// atlas is keyed by glyph index with no size in the key: on a hit it divides
// the requested size by the size the glyph was cached at and feeds that to
// vita2d_draw_texture_tint_part_scale. A glyph shared between two sizes is
// therefore rasterised once, at whichever size drew it first, and bilinearly
// rescaled everywhere else - which is why sharpness used to depend on the order
// the user navigated in. A handle per size gives each one its own atlas.
//
// Each handle costs one 512x512 single-channel sheet, 256 KB. Only the pairs
// marked here are loaded; ADDING A DRAW SITE AT A NEW FACE/SIZE PAIR MEANS
// ADDING IT TO THIS TABLE, or that text draws nothing at all.
static const SceBool ui_face_uses[UI_FACE_COUNT][UI_TS_COUNT] = {
	[UI_FACE_UI] = {
		[UI_TS_LABEL] = SCE_TRUE,
		[UI_TS_BODY] = SCE_TRUE,
		[UI_TS_TITLE] = SCE_TRUE,
		[UI_TS_DISPLAY] = SCE_TRUE,
	},
	[UI_FACE_MONO] = {
		[UI_TS_BADGE] = SCE_TRUE,
		[UI_TS_LABEL] = SCE_TRUE,
	},
};

static vita2d_font *ui_font[UI_FACE_COUNT][UI_TS_COUNT];

// Ascender-to-descender extent of the face itself at each size, measured once
// from a reference string rather than from whatever string a draw site happens
// to pass. See UI_TextBaselineY.
static int ui_face_extent[UI_FACE_COUNT][UI_TS_COUNT];

// Carries a cap-height letter, an x-height letter and two descenders, so it
// spans the full em box of any Latin face at that size.
#define UI_METRICS_REFERENCE "Agjy"

static vita2d_font *UI_FontFor(UI_Face face, UI_TextSize ts) {
	if (face < 0 || face >= UI_FACE_COUNT || ts < 0 || ts >= UI_TS_COUNT)
		return NULL;

	return ui_font[face][ts];
}

// ---- Non-Latin fallback ----
//
// See ui_theme.h for why this exists. Two engines, one string: runs our own
// face covers are drawn with it, runs it does not are drawn with the console's
// fonts, and the position advances by whatever each engine reports.

// One handle, not one per size. design.md expected the per-size rule to apply
// to this engine too, but vita2d_load_system_pvf calls scePvfSetCharSize with a
// constant, so every handle it returns would be the same 18 px atlas. Several
// would cost memory and buy nothing.
static vita2d_pvf *ui_fallback = NULL;

// The console reports Latin, Korean, Japanese and Chinese, each picked by its
// own predicate, so dispatch is per codepoint rather than per block. That
// matters because our own Cyrillic coverage is partial, not absent.
static int UI_FallbackGroupLatin(unsigned int c) { return c < 0x0500; }
static int UI_FallbackGroupHangul(unsigned int c) {
	return (c >= 0x1100 && c <= 0x11FF) || (c >= 0x3130 && c <= 0x318F) || (c >= 0xAC00 && c <= 0xD7A3);
}
static int UI_FallbackGroupKana(unsigned int c) { return (c >= 0x3040 && c <= 0x30FF) || (c >= 0x31F0 && c <= 0x31FF); }
static int UI_FallbackGroupHan(unsigned int c) {
	return (c >= 0x3400 && c <= 0x4DBF) || (c >= 0x4E00 && c <= 0x9FFF) || (c >= 0xF900 && c <= 0xFAFF);
}

static const vita2d_system_pvf_config ui_fallback_configs[] = {
	{ SCE_PVF_LANGUAGE_LATIN, UI_FallbackGroupLatin },
	{ SCE_PVF_LANGUAGE_K,     UI_FallbackGroupHangul },
	{ SCE_PVF_LANGUAGE_J,     UI_FallbackGroupKana },
	{ SCE_PVF_LANGUAGE_C,     UI_FallbackGroupHan },
};
#define UI_FALLBACK_CONFIG_COUNT ((int)(sizeof(ui_fallback_configs) / sizeof(ui_fallback_configs[0])))

// Coverage is asked of FreeType directly rather than assumed from a range. A
// face opened here is never drawn with - vita2d owns the handles that draw -
// it exists only to answer FT_Get_Char_Index.
static FT_Library ui_ft = NULL;
static FT_Face ui_ft_face[UI_FACE_COUNT];

// Distinct uncovered codepoints drawn so far, as a bitmap over the BMP: 8 KB
// for an exact count, against a hash table that would need tuning. The atlas
// holds roughly six hundred full-width glyphs, so the threshold sits below that
// with room for the sheet's packing overhead.
#define UI_FALLBACK_SEEN_BITS 0x10000
#define UI_FALLBACK_RENEW_AT  480

static unsigned char ui_fallback_seen[UI_FALLBACK_SEEN_BITS / 8];
static int ui_fallback_seen_count = 0;
static int ui_fallback_renewals = 0;
static SceBool ui_fallback_needs_renew = SCE_FALSE;

static void UI_FallbackLoad(void) {
	ui_fallback = vita2d_load_system_pvf(UI_FALLBACK_CONFIG_COUNT, ui_fallback_configs);
	memset(ui_fallback_seen, 0, sizeof(ui_fallback_seen));
	ui_fallback_seen_count = 0;
	ui_fallback_needs_renew = SCE_FALSE;
}

void UI_Theme_RenewFallbackIfNeeded(void) {
	if (!ui_fallback_needs_renew)
		return;

	// Through the single destruction point, which waits for pending rendering
	// before it frees. Callers guarantee this runs between frames.
	UI_GpuFreePvf(&ui_fallback);
	UI_FallbackLoad();
	ui_fallback_renewals++;
}

static void UI_FallbackNoteCodepoint(unsigned int cp) {
	unsigned int idx, bit;

	if (cp >= UI_FALLBACK_SEEN_BITS)
		return;

	idx = cp >> 3;
	bit = 1u << (cp & 7);

	if (ui_fallback_seen[idx] & bit)
		return;

	ui_fallback_seen[idx] |= bit;
	if (++ui_fallback_seen_count >= UI_FALLBACK_RENEW_AT)
		ui_fallback_needs_renew = SCE_TRUE;
}

// Minimal UTF-8 walk. Returns the next position, or NULL at the end. A
// malformed byte is reported as U+FFFD and consumed, so a tag truncated
// mid-sequence cannot spin the caller.
static const char *UI_Utf8Next(const char *s, unsigned int *out_cp) {
	unsigned char c = (unsigned char)*s;
	unsigned int cp;
	int extra;

	if (!c)
		return NULL;

	if (c < 0x80) { cp = c; extra = 0; }
	else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
	else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
	else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
	else { *out_cp = 0xFFFD; return s + 1; }

	for (int i = 1; i <= extra; i++) {
		unsigned char cc = (unsigned char)s[i];

		if ((cc & 0xC0) != 0x80) { *out_cp = 0xFFFD; return s + i; }
		cp = (cp << 6) | (cc & 0x3F);
	}

	*out_cp = cp;
	return s + extra + 1;
}

static SceBool UI_FaceCovers(UI_Face face, unsigned int cp) {
	if (face < 0 || face >= UI_FACE_COUNT || !ui_ft_face[face])
		return SCE_TRUE; // cannot ask: let our own face try, and say so in the overlay

	return FT_Get_Char_Index(ui_ft_face[face], cp) != 0 ? SCE_TRUE : SCE_FALSE;
}

SceBool UI_Theme_FallbackStatus(int *out_seen, int *out_renewals, SceBool *out_can_query) {
	SceBool can_query = SCE_FALSE;

	for (int f = 0; f < UI_FACE_COUNT; f++) {
		if (ui_ft_face[f])
			can_query = SCE_TRUE;
	}

	if (out_seen)
		*out_seen = ui_fallback_seen_count;
	if (out_renewals)
		*out_renewals = ui_fallback_renewals;
	if (out_can_query)
		*out_can_query = can_query;

	return (ui_fallback != NULL && can_query) ? SCE_TRUE : SCE_FALSE;
}

SceBool UI_TextNeedsFallback(UI_Face face, const char *text) {
	unsigned int cp;

	if (!text)
		return SCE_FALSE;

	while ((text = UI_Utf8Next(text, &cp)) != NULL) {
		if (!UI_FaceCovers(face, cp))
			return SCE_TRUE;
	}

	return SCE_FALSE;
}

// Walks `text`, splitting it wherever the engine has to change, and either
// draws the runs or just measures them. Drawing and measuring share this one
// walker so a clipped or right-aligned string cannot be measured by one rule
// and drawn by another.
static float UI_TextRuns(UI_Face face, UI_TextSize ts, float x, float baseline_y,
	unsigned int color, const char *text, SceBool draw) {
	char run[128];
	const char *p = text;
	float pen = x;
	float scale;
	vita2d_font *own;

	if (!text)
		return 0.0f;

	own = UI_FontFor(face, ts);
	scale = (float)ui_text_px[ts] / UI_PVF_NATIVE_PX;

	while (*p) {
		const char *next;
		unsigned int cp;
		SceBool run_covered;
		int len = 0;

		next = UI_Utf8Next(p, &cp);
		if (!next)
			break;

		run_covered = UI_FaceCovers(face, cp);

		// Gather as much as fits in the buffer while the engine stays the same.
		while (next && (size_t)(len + (next - p)) < sizeof(run) - 1) {
			memcpy(run + len, p, (size_t)(next - p));
			len += (int)(next - p);

			if (!run_covered)
				UI_FallbackNoteCodepoint(cp);

			p = next;
			if (!*p)
				break;

			next = UI_Utf8Next(p, &cp);
			if (!next || UI_FaceCovers(face, cp) != run_covered)
				break;
		}
		run[len] = '\0';

		if (run_covered) {
			if (own) {
				if (draw)
					vita2d_font_draw_text(own, (int)pen, (int)baseline_y, color, ui_text_px[ts], run);
				pen += (float)vita2d_font_text_width(own, ui_text_px[ts], run);
			}
		}
		else if (ui_fallback) {
			if (draw)
				vita2d_pvf_draw_text(ui_fallback, (int)pen, (int)baseline_y, color, scale, run);
			pen += (float)vita2d_pvf_text_width(ui_fallback, scale, run);
		}
	}

	return pen - x;
}

void UI_Theme_Load(void) {
	// One FreeType face per typeface, for coverage queries only.
	if (FT_Init_FreeType(&ui_ft) == 0) {
		for (int f = 0; f < UI_FACE_COUNT; f++) {
			if (FT_New_Face(ui_ft, ui_face_file[f], 0, &ui_ft_face[f]) != 0)
				ui_ft_face[f] = NULL;
		}
	}

	UI_FallbackLoad();

	for (int f = 0; f < UI_FACE_COUNT; f++) {
		for (int t = 0; t < UI_TS_COUNT; t++) {
			if (!ui_face_uses[f][t])
				continue;

			ui_font[f][t] = vita2d_load_font_file(ui_face_file[f]);

			if (ui_font[f][t])
				ui_face_extent[f][t] = vita2d_font_text_height(ui_font[f][t], ui_text_px[t], UI_METRICS_REFERENCE);
		}
	}
}

void UI_Theme_Free(void) {
	UI_GpuFreePvf(&ui_fallback);

	for (int f = UI_FACE_COUNT - 1; f >= 0; f--) {
		for (int t = UI_TS_COUNT - 1; t >= 0; t--)
			UI_GpuFreeFont(&ui_font[f][t]);
	}

	for (int f = 0; f < UI_FACE_COUNT; f++) {
		if (ui_ft_face[f])
			FT_Done_Face(ui_ft_face[f]);
	}
	if (ui_ft)
		FT_Done_FreeType(ui_ft);
}

void UI_DrawText(UI_Face face, UI_TextSize ts, float x, float baseline_y, unsigned int color, const char *text) {
	UI_TextRuns(face, ts, x, baseline_y, color, text, SCE_TRUE);
}

int UI_TextWidth(UI_Face face, UI_TextSize ts, const char *text) {
	return (int)UI_TextRuns(face, ts, 0.0f, 0.0f, 0, text, SCE_FALSE);
}

int UI_TextHeight(UI_Face face, UI_TextSize ts, const char *text) {
	vita2d_font *f = UI_FontFor(face, ts);

	if (!f || !text)
		return 0;

	return vita2d_font_text_height(f, ui_text_px[ts], text);
}

void UI_TextDimensions(UI_Face face, UI_TextSize ts, const char *text, int *out_w, int *out_h) {
	vita2d_font *f = UI_FontFor(face, ts);

	if (out_w)
		*out_w = 0;
	if (out_h)
		*out_h = 0;

	if (!f || !text)
		return;

	vita2d_font_text_dimensions(f, ui_text_px[ts], text, out_w, out_h);
}

// Centred on the face's own extent at this size, not on the extent of the
// string being drawn. Measuring the string moved the baseline with its content:
// a row whose name had no descender sat lower than the row under it, and the
// elapsed time hopped vertically as its digits changed.
int UI_TextBaselineY(UI_Face face, UI_TextSize ts, float box_top, float box_h) {
	int extent;

	if (face < 0 || face >= UI_FACE_COUNT || ts < 0 || ts >= UI_TS_COUNT)
		return (int)box_top;

	extent = ui_face_extent[face][ts];

	return (int)(box_top + ((box_h - extent) / 2) + extent);
}

// Drawn after the clipped run, outside the clip, so it is always fully visible.
#define UI_ELLIPSIS "..."

void UI_DrawTextClipped(UI_Face face, UI_TextSize ts, float x, float baseline_y, float max_w,
	unsigned int color, const char *text) {
	int extent, ellipsis_w;
	float run_w;

	if (!text || max_w <= 0.0f)
		return;

	if (UI_TextWidth(face, ts, text) <= (int)max_w) {
		UI_DrawText(face, ts, x, baseline_y, color, text);
		return;
	}

	extent = ui_face_extent[face][ts];
	ellipsis_w = UI_TextWidth(face, ts, UI_ELLIPSIS);
	run_w = max_w - (float)ellipsis_w;

	// Too narrow to show anything plus the indicator: the indicator alone says
	// more than a single cut-off letter would.
	if (run_w <= 0.0f) {
		UI_DrawText(face, ts, x, baseline_y, color, UI_ELLIPSIS);
		return;
	}

	// The band is the face's extent either side of the baseline, which covers
	// ascenders and descenders at this size with room to spare.
	vita2d_set_clip_rectangle((int)x, (int)(baseline_y - extent), (int)(x + run_w), (int)(baseline_y + extent));
	vita2d_enable_clipping();
	UI_DrawText(face, ts, x, baseline_y, color, text);
	vita2d_disable_clipping();

	UI_DrawText(face, ts, x + run_w, baseline_y, color, UI_ELLIPSIS);
}

SceBool UI_TouchTarget(float x, float y, float w, float h) {
	float grow_x = (w < UI_TOUCH_MIN) ? ((UI_TOUCH_MIN - w) / 2.0f) : 0.0f;
	float grow_y = (h < UI_TOUCH_MIN) ? ((UI_TOUCH_MIN - h) / 2.0f) : 0.0f;

	return Touch_Position(x - grow_x, y - grow_y, x + w + grow_x, y + h + grow_y) ? SCE_TRUE : SCE_FALSE;
}

static void UI_SetVertex(vita2d_color_vertex *v, float x, float y, unsigned int color) {
	v->x = x;
	v->y = y;
	v->z = UI_VEC_Z;
	v->color = color;
}

// Every vector shape below gets its vertices here and nowhere else; see
// UI_GpuPoolAlloc in ui_gpu.h for why the frame pool is the only valid source
// and how exhaustion is made observable. Returns NULL when the pool is full.
static vita2d_color_vertex *UI_VertexBuffer(unsigned int count) {
	return (vita2d_color_vertex *)UI_GpuPoolAlloc(count * sizeof(vita2d_color_vertex), sizeof(vita2d_color_vertex));
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

void UI_DrawBadge(float x, float y, UI_TextSize ts, const char *label, unsigned int bg, unsigned int fg, unsigned int border) {
	int text_w = UI_TextWidth(UI_FACE_MONO, ts, label);
	int text_h = UI_TextHeight(UI_FACE_MONO, ts, label);
	float pad_x = 8.0f, pad_y = 4.0f;
	float w = text_w + pad_x * 2;
	float h = text_h + pad_y * 2;

	if (border) {
		UI_DrawPill(x, y, w, h, border);
		UI_DrawPill(x + 1, y + 1, w - 2, h - 2, bg);
	}
	else
		UI_DrawPill(x, y, w, h, bg);

	UI_DrawText(UI_FACE_MONO, ts, x + pad_x, UI_TextBaselineY(UI_FACE_MONO, ts, y, h), fg, label);
}

void UI_DrawRowHighlight(float x, float y, float w, float h) {
	UI_DrawRoundedRect(x, y, w, h, UI_RADIUS_SM + 2, ui_color_accent_wash);
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
