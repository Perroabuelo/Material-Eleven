#include <math.h>
#include <string.h>

#include "ui_gpu.h"
#include "ui_theme.h"
#include "vitaaudiolib.h"
#include "viz_bands.h"
#include "viz_draw.h"
#include "viz_fft.h"
#include "viz_meter.h"
#include "viz_scope.h"
#include "viz_tap.h"

#define VIZ_WINDOW       VIZ_SCOPE_WINDOW   // frames read per frame
#define VIZ_BAR_GAP      3.0f
#define VIZ_PEAK_H       2.0f
#define VIZ_SCOPE_THICK  2.0f
#define VIZ_VU_H         28.0f
#define VIZ_VU_GAP       36.0f
#define VIZ_VU_LABEL_W   28.0f

// The window being heard, kept when a read has to be thrown away.
static int16_t window[VIZ_WINDOW * 2];
static float mono[VIZ_WINDOW];
static float magnitudes[VIZ_FFT_BINS];
static float band_db[VIZ_BANDS];

static VizBands_Layout layout = { 0 };
static VizMeter bands[VIZ_BANDS];
static VizMeter vu[2];
static VizBass bass_gain;
static VizMeter bass;

// The accent at another opacity.
static unsigned int VizDraw_Alpha(unsigned int color, unsigned int alpha) {
	return (color & 0x00FFFFFF) | (alpha << 24);
}

static void VizDraw_Analyze(float dt) {
	unsigned int rate = 0;
	unsigned int delay = 2 * vitaAudioGetGrain() + VIZ_HW_LATENCY_FRAMES;

	if (VizTap_Read(VizTap_Shared(), delay, window, VIZ_WINDOW, &rate) && rate != 0 && rate != layout.rate)
		VizBands_SetRate(&layout, rate);
	if (layout.rate == 0)
		VizBands_SetRate(&layout, 48000);

	VizFFT_MonoFromStereo(window, VIZ_WINDOW, mono);

	// The spectrum of the newest part of the window.
	VizFFT_Magnitudes(mono + VIZ_WINDOW - VIZ_FFT_SIZE, magnitudes);
	VizBands_Levels(&layout, magnitudes, band_db);

	for (int i = 0; i < VIZ_BANDS; i++)
		VizMeter_Update(&bands[i], VizBands_Norm(band_db[i]), dt);

	unsigned int vu_frames = layout.rate * VIZ_VU_WINDOW_MS / 1000;
	if (vu_frames > VIZ_WINDOW)
		vu_frames = VIZ_WINDOW;

	const int16_t *recent = window + (VIZ_WINDOW - vu_frames) * 2;
	for (int ch = 0; ch < 2; ch++)
		VizMeter_Update(&vu[ch], VizMeter_VuNorm(VizMeter_RmsDb(recent, vu_frames, ch)), dt);

	VizMeter_Update(&bass, VizBass_Update(&bass_gain, &layout, magnitudes, dt), dt);
}

static void VizDraw_Spectrum(float x, float y, float w, float h) {
	float bar_w = (w - VIZ_BAR_GAP * (VIZ_BANDS - 1)) / VIZ_BANDS;
	float bottom = y + h;
	unsigned int peak_color = VizDraw_Alpha(ui_color_accent, 160);

	for (int i = 0; i < VIZ_BANDS; i++) {
		float bx = x + i * (bar_w + VIZ_BAR_GAP);
		float bar_h = bands[i].level * h;

		// A sliver at rest, so the bars read as bars even in silence.
		if (bar_h < 2.0f)
			bar_h = 2.0f;

		vita2d_draw_rectangle(bx, bottom - bar_h, bar_w, bar_h, ui_color_accent);

		if (bands[i].peak > bands[i].level + 0.01f) {
			float py = bottom - bands[i].peak * h;
			vita2d_draw_rectangle(bx, py, bar_w, VIZ_PEAK_H, peak_color);
		}
	}
}

static void VizDraw_Scope(float x, float y, float w, float h) {
	unsigned int start = VizScope_Trigger(mono, VIZ_WINDOW, VIZ_SCOPE_SPAN);
	vita2d_color_vertex *v = UI_GpuPoolAlloc(VIZ_SCOPE_SPAN * 2 * sizeof(vita2d_color_vertex), sizeof(vita2d_color_vertex));

	if (!v)
		return;

	float mid = y + h / 2.0f;
	float half = h / 2.0f - VIZ_SCOPE_THICK;
	float step = w / (VIZ_SCOPE_SPAN - 1);
	float r = VIZ_SCOPE_THICK / 2.0f;

	for (int i = 0; i < VIZ_SCOPE_SPAN; i++) {
		float s = mono[start + i];

		if (s > 1.0f)
			s = 1.0f;
		else if (s < -1.0f)
			s = -1.0f;

		float px = x + i * step, py = mid - s * half;

		v[i * 2] = (vita2d_color_vertex){ px, py - r, 0.5f, ui_color_accent };
		v[i * 2 + 1] = (vita2d_color_vertex){ px, py + r, 0.5f, ui_color_accent };
	}

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, v, VIZ_SCOPE_SPAN * 2);
}

static void VizDraw_Vu(float x, float y, float w, float h) {
	static const char *labels[2] = { "L", "R" };
	float top = y + (h - (VIZ_VU_H * 2 + VIZ_VU_GAP)) / 2.0f;
	float track_x = x + VIZ_VU_LABEL_W;
	float track_w = w - VIZ_VU_LABEL_W;

	for (int ch = 0; ch < 2; ch++) {
		float ty = top + ch * (VIZ_VU_H + VIZ_VU_GAP);

		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, UI_CapCenteredBaselineY(UI_FACE_MONO, UI_TS_LABEL, ty, VIZ_VU_H),
			UI_COLOR_TEXT_SECONDARY, labels[ch]);

		vita2d_draw_rectangle(track_x, ty, track_w, VIZ_VU_H, UI_COLOR_SURFACE);
		vita2d_draw_rectangle(track_x, ty, track_w * vu[ch].level, VIZ_VU_H, ui_color_accent);

		if (vu[ch].peak > 0.0f) {
			float px = track_x + track_w * vu[ch].peak - 3.0f;
			if (px < track_x)
				px = track_x;
			vita2d_draw_rectangle(px, ty - 3.0f, 3.0f, VIZ_VU_H + 6.0f, UI_COLOR_TEXT_PRIMARY);
		}
	}

	// Ticks every 6 dB between the meters, so the scale reads as decibels.
	float tick_y = top + VIZ_VU_H + VIZ_VU_GAP / 2.0f - 4.0f;
	for (float db = VIZ_VU_FLOOR_DB; db <= 0.0f; db += 6.0f) {
		float tx = track_x + track_w * VizMeter_VuNorm(db);
		vita2d_draw_rectangle(tx - 0.5f, tick_y, 1.0f, 8.0f, UI_COLOR_TEXT_MUTED);
	}
}

void VizDraw_Frame(Viz_Style style, float x, float y, float w, float h, float dt, int playing) {
	(void)playing;

	VizDraw_Analyze(dt);

	switch (style) {
		case VIZ_STYLE_SPECTRUM:
			VizDraw_Spectrum(x, y, w, h);
			break;
		case VIZ_STYLE_SCOPE:
			VizDraw_Scope(x, y, w, h);
			break;
		case VIZ_STYLE_VU:
			VizDraw_Vu(x, y, w, h);
			break;
		default:
			break;
	}
}
