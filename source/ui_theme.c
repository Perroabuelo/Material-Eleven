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

vita2d_font *font_ui = NULL;
vita2d_font *font_mono = NULL;

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

void UI_DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, unsigned int color) {
	vita2d_color_vertex v[3];

	UI_SetVertex(&v[0], x0, y0, color);
	UI_SetVertex(&v[1], x1, y1, color);
	UI_SetVertex(&v[2], x2, y2, color);

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLES, v, 3);
}

void UI_DrawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, unsigned int color) {
	vita2d_color_vertex v[4];

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
	vita2d_color_vertex v[(UI_RING_SEGMENTS + 1) * 2];
	float inner = radius - thickness;

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
	vita2d_color_vertex v[UI_ARC_MAX_SEGMENTS + 2];
	int segments = (int)(radius * 0.7f);

	if (segments < UI_ARC_MIN_SEGMENTS)
		segments = UI_ARC_MIN_SEGMENTS;
	if (segments > UI_ARC_MAX_SEGMENTS)
		segments = UI_ARC_MAX_SEGMENTS;

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
	UI_DrawRoundedRect(x, y, w, h, UI_RADIUS_SM + 2, UI_COLOR_ACCENT_WASH);
}

int UI_TextBaselineY(vita2d_font *f, unsigned int size, const char *text, float box_top, float box_h) {
	int height = vita2d_font_text_height(f, size, text);
	return (int)(box_top + ((box_h - height) / 2) + height);
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
