#include <string.h>

#include "common.h"
#include "textures.h"
#include "ui_theme.h"

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

static void UI_DrawCornerAtlasQuad(vita2d_texture *atlas, int native_radius, float dst_x, float dst_y,
	float tex_x, float tex_y, float radius, unsigned int color) {
	float scale = (float)radius / (float)native_radius;
	vita2d_draw_texture_tint_part_scale(atlas, dst_x, dst_y, tex_x, tex_y, native_radius, native_radius, scale, scale, color);
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

	vita2d_texture *atlas = (radius <= UI_RADIUS_SM) ? ui_corner_sm : ui_corner_lg;
	int native_radius = (radius <= UI_RADIUS_SM) ? UI_RADIUS_SM : UI_RADIUS_LG;

	// Vertical band covering the full height minus the left/right corner columns.
	vita2d_draw_rectangle(x + radius, y, w - 2 * radius, h, color);
	// Left / right middle strips, between the top and bottom corners.
	if (h - 2 * radius > 0) {
		vita2d_draw_rectangle(x, y + radius, radius, h - 2 * radius, color);
		vita2d_draw_rectangle(x + w - radius, y + radius, radius, h - 2 * radius, color);
	}

	UI_DrawCornerAtlasQuad(atlas, native_radius, x, y, 0, 0, radius, color); // top-left
	UI_DrawCornerAtlasQuad(atlas, native_radius, x + w - radius, y, native_radius, 0, radius, color); // top-right
	UI_DrawCornerAtlasQuad(atlas, native_radius, x, y + h - radius, 0, native_radius, radius, color); // bottom-left
	UI_DrawCornerAtlasQuad(atlas, native_radius, x + w - radius, y + h - radius, native_radius, native_radius, radius, color); // bottom-right
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
