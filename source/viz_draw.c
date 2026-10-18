#include <math.h>
#include "audio.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "vitaaudiolib.h"
#include "viz_bands.h"
#include "viz_disc.h"
#include "viz_draw.h"
#include "viz_fft.h"
#include "viz_meter.h"
#include "viz_scope.h"
#include "viz_spin.h"
#include "viz_tap.h"

#define VIZ_WINDOW       VIZ_SCOPE_WINDOW   // frames read per frame
#define VIZ_BAR_GAP      3.0f
#define VIZ_PEAK_H       2.0f
#define VIZ_SCOPE_THICK  2.0f
#define VIZ_VU_H         28.0f
#define VIZ_VU_GAP       36.0f
#define VIZ_VU_LABEL_W   28.0f

// The vinyl, as shares of its radius.
#define VIZ_LABEL_SHARE  0.42f
#define VIZ_HOLE_SHARE   0.05f
#define VIZ_GROOVES      6
#define VIZ_FEATHER      1.5f     // px of the soft edge over each rim
#define VIZ_DISC_COLOR   RGBA8(0x0C, 0x0B, 0x10, 255)
#define VIZ_GROOVE_COLOR RGBA8(0xFF, 0xFF, 0xFF, 22)

// Radial spectrum: the bands mirrored down both sides.
#define VIZ_RADIAL_BARS  (VIZ_BANDS * 2)
#define VIZ_RADIAL_GAP   6.0f

// Pulse: how much the record grows with the bass, and its halo.
#define VIZ_PULSE_GROW   0.06f
#define VIZ_HALO_RINGS   3

_Static_assert(sizeof(VizDisc_Vertex) == sizeof(vita2d_texture_vertex), "VizDisc_Vertex must match vita2d_texture_vertex");

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
static VizSpin spin;

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

static vita2d_color_vertex *VizDraw_Vertices(unsigned int count) {
	return UI_GpuPoolAlloc(count * sizeof(vita2d_color_vertex), sizeof(vita2d_color_vertex));
}

static void VizDraw_FillCircle(float cx, float cy, float r, unsigned int color) {
	vita2d_color_vertex *v = VizDraw_Vertices(VIZ_DISC_SEGMENTS + 2);

	if (!v)
		return;

	v[0] = (vita2d_color_vertex){ cx, cy, 0.5f, color };
	for (int i = 0; i <= VIZ_DISC_SEGMENTS; i++) {
		float a = 2.0f * UI_PI * (i % VIZ_DISC_SEGMENTS) / VIZ_DISC_SEGMENTS;
		v[i + 1] = (vita2d_color_vertex){ cx + r * cosf(a), cy + r * sinf(a), 0.5f, color };
	}

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_FAN, v, VIZ_DISC_SEGMENTS + 2);
}

// A ring from `inner` to `outer` whose color goes from `in` to `out`: the soft
// rims and the halo. With the alpha falling outwards, an edge drawn over the
// rim of a shape of the same color reads smooth without relying on MSAA.
static void VizDraw_GradientRing(float cx, float cy, float inner, float outer, unsigned int in, unsigned int out) {
	vita2d_color_vertex *v = VizDraw_Vertices((VIZ_DISC_SEGMENTS + 1) * 2);

	if (!v)
		return;

	if (inner < 0.0f)
		inner = 0.0f;

	for (int i = 0; i <= VIZ_DISC_SEGMENTS; i++) {
		float a = 2.0f * UI_PI * (i % VIZ_DISC_SEGMENTS) / VIZ_DISC_SEGMENTS;
		float c = cosf(a), s = sinf(a);

		v[i * 2] = (vita2d_color_vertex){ cx + c * outer, cy + s * outer, 0.5f, out };
		v[i * 2 + 1] = (vita2d_color_vertex){ cx + c * inner, cy + s * inner, 0.5f, in };
	}

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, v, (VIZ_DISC_SEGMENTS + 1) * 2);
}

// The record at (cx, cy) with radius `r`, turned to the spin's angle: the dark
// disc with its grooves, the cover (or a plain label in the accent) and the
// hole.
static void VizDraw_Vinyl(float cx, float cy, float r) {
	float label = r * VIZ_LABEL_SHARE;

	VizDraw_FillCircle(cx, cy, r, VIZ_DISC_COLOR);
	VizDraw_GradientRing(cx, cy, r, r + VIZ_FEATHER, VIZ_DISC_COLOR, VizDraw_Alpha(VIZ_DISC_COLOR, 0));

	for (int g = 0; g < VIZ_GROOVES; g++) {
		float gr = label + (r - label) * (g + 1) / (VIZ_GROOVES + 1);
		UI_DrawRingN(cx, cy, gr, 1.0f, VIZ_DISC_SEGMENTS, VIZ_GROOVE_COLOR);
	}

	vita2d_texture *cover = (metadata.has_meta) ? metadata.cover_image : NULL;
	VizDisc_Vertex *fan = cover ? UI_GpuPoolAlloc(VIZ_DISC_FAN_VERTICES(VIZ_DISC_SEGMENTS) * sizeof(VizDisc_Vertex), sizeof(float)) : NULL;

	if (fan) {
		unsigned int n = VizDisc_Fan(fan, cx, cy, label, VIZ_DISC_SEGMENTS, spin.angle,
			vita2d_texture_get_width(cover), vita2d_texture_get_height(cover));
		vita2d_draw_array_textured(cover, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, (const vita2d_texture_vertex *)fan, n, 0xFFFFFFFF);
	}
	else {
		VizDraw_FillCircle(cx, cy, label, ui_color_accent);

		// A plain label shows nothing turning, so it carries a mark.
		float mr = label * 0.7f;
		vita2d_draw_fill_circle(cx + mr * cosf(spin.angle), cy + mr * sinf(spin.angle), label * 0.08f,
			VizDraw_Alpha(VIZ_DISC_COLOR, 120));
	}

	// The label's rim fades into the disc.
	VizDraw_GradientRing(cx, cy, label - VIZ_FEATHER, label, VizDraw_Alpha(VIZ_DISC_COLOR, 0), VIZ_DISC_COLOR);

	VizDraw_FillCircle(cx, cy, r * VIZ_HOLE_SHARE, UI_COLOR_BG);
}

// The bands as bars standing out from a circle of radius `r`, lows at the top
// and highs at the bottom on both sides, in one draw call. They stay put while
// the record turns.
static void VizDraw_RadialBars(float cx, float cy, float r, float length) {
	vita2d_color_vertex *v = VizDraw_Vertices(VIZ_RADIAL_BARS * 6);

	if (!v)
		return;

	float half_w = UI_PI * r / VIZ_RADIAL_BARS * 0.6f;

	for (int b = 0; b < VIZ_RADIAL_BARS; b++) {
		int band = b % VIZ_BANDS;
		float side = (b < VIZ_BANDS) ? 1.0f : -1.0f;
		// From the top (-pi/2) down to the bottom, clockwise on the right.
		float a = -UI_PI / 2.0f + side * UI_PI * (band + 0.5f) / VIZ_BANDS;
		float c = cosf(a), s = sinf(a);
		float len = 2.0f + bands[band].level * length;

		float x0 = cx + c * r, y0 = cy + s * r;
		float x1 = cx + c * (r + len), y1 = cy + s * (r + len);
		float nx = -s * half_w, ny = c * half_w;

		vita2d_color_vertex *q = &v[b * 6];
		q[0] = (vita2d_color_vertex){ x0 + nx, y0 + ny, 0.5f, ui_color_accent };
		q[1] = (vita2d_color_vertex){ x1 + nx, y1 + ny, 0.5f, ui_color_accent };
		q[2] = (vita2d_color_vertex){ x1 - nx, y1 - ny, 0.5f, ui_color_accent };
		q[3] = q[0];
		q[4] = q[2];
		q[5] = (vita2d_color_vertex){ x0 - nx, y0 - ny, 0.5f, ui_color_accent };
	}

	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLES, v, VIZ_RADIAL_BARS * 6);
}

static void VizDraw_Disc(float x, float y, float w, float h) {
	float half = (w < h ? w : h) / 2.0f;
	float r = half * 0.58f;
	float cx = x + w / 2.0f, cy = y + h / 2.0f;

	VizDraw_RadialBars(cx, cy, r + VIZ_RADIAL_GAP, half - r - VIZ_RADIAL_GAP - 2.0f);
	VizDraw_Vinyl(cx, cy, r);
}

static void VizDraw_Pulse(float x, float y, float w, float h) {
	float half = (w < h ? w : h) / 2.0f;
	float rest = half * 0.74f;
	float r = rest * (1.0f + VIZ_PULSE_GROW * bass.level);
	float cx = x + w / 2.0f, cy = y + h / 2.0f;
	float level = (vu[0].level + vu[1].level) / 2.0f;

	// The halo behind the record: rings that reach further and glow brighter
	// with the level, dim in silence.
	for (int k = VIZ_HALO_RINGS; k >= 1; k--) {
		float reach = (half - r) * level * k / VIZ_HALO_RINGS;
		unsigned int alpha = (unsigned int)(20.0f + 90.0f * level) / k;
		VizDraw_GradientRing(cx, cy, r, r + 2.0f + reach, VizDraw_Alpha(ui_color_accent, alpha), VizDraw_Alpha(ui_color_accent, 0));
	}

	VizDraw_Vinyl(cx, cy, r);
}

void VizDraw_Frame(Viz_Style style, float x, float y, float w, float h, float dt, int playing) {
	VizDraw_Analyze(dt);
	VizSpin_Update(&spin, playing, dt);

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
		case VIZ_STYLE_DISC:
			VizDraw_Disc(x, y, w, h);
			break;
		case VIZ_STYLE_PULSE:
			VizDraw_Pulse(x, y, w, h);
			break;
		default:
			break;
	}
}
