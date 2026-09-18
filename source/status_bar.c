#include <psp2/power.h>
#include <psp2/rtc.h>
#include <stdio.h>

#include "common.h"
#include "status_bar.h"
#include "ui_theme.h"

// The band the clock and battery sit in, shared with the screens that draw a
// hairline under it.
#define STATUS_BAR_H 32

// Battery, drawn as geometry. The shell is a rounded outline with a terminal
// nub; the charge is a rounded bar inside it whose length is the level itself,
// so every percentage has its own width. The fifteen PNGs this replaces could
// only show seven steps, and one of those - the 30% art - was unreachable,
// because the ladder that picked them sent 30..49 to the 50% bucket.
#define BATT_W        30
#define BATT_H        15
#define BATT_RADIUS    4
#define BATT_BORDER  1.5f
#define BATT_INSET   3.0f
#define BATT_NUB_W     3
#define BATT_NUB_H     6
// Below this the bar turns amber, which is all the old battery_low art conveyed.
#define BATT_LOW_PERCENT 20

static void StatusBar_DrawChargingBolt(float cx, float cy, unsigned int color) {
	float w = 3.5f, h = 8.0f;

	// Two triangles meeting at the waist, the usual lightning silhouette.
	UI_DrawTriangle(cx + w * 0.2f, cy - h / 2.0f, cx - w, cy + h * 0.1f, cx + w * 0.1f, cy + h * 0.1f, color);
	UI_DrawTriangle(cx - w * 0.2f, cy + h / 2.0f, cx + w, cy - h * 0.1f, cx - w * 0.1f, cy - h * 0.1f, color);
}

// `percent` below zero means the level could not be read: the shell is drawn
// empty rather than showing a charge the console never reported.
static void StatusBar_DrawBattery(float x, float y, int percent, SceBool charging) {
	unsigned int shell = UI_COLOR_TEXT_TERTIARY;
	unsigned int fill = (percent >= 0 && percent < BATT_LOW_PERCENT) ? UI_COLOR_TRACKER : UI_COLOR_TEXT_SECONDARY;
	float track_w = BATT_W - BATT_INSET * 2.0f;

	// Outline as a filled rounded rect with the background punched back out of
	// it, since the vector helpers draw fills rather than strokes.
	UI_DrawRoundedRect(x, y, BATT_W, BATT_H, BATT_RADIUS, shell);
	UI_DrawRoundedRect(x + BATT_BORDER, y + BATT_BORDER, BATT_W - BATT_BORDER * 2.0f, BATT_H - BATT_BORDER * 2.0f,
		BATT_RADIUS - 1, UI_COLOR_BG);

	vita2d_draw_rectangle(x + BATT_W, y + (BATT_H - BATT_NUB_H) / 2.0f, BATT_NUB_W, BATT_NUB_H, shell);

	if (percent > 0) {
		float w = track_w * ((float)percent / 100.0f);

		// Keep a sliver visible at 1% rather than rounding it away to nothing.
		if (w < 2.0f)
			w = 2.0f;

		UI_DrawRoundedRect(x + BATT_INSET, y + BATT_INSET, w, BATT_H - BATT_INSET * 2.0f, 2, fill);
	}

	if (charging)
		StatusBar_DrawChargingBolt(x + BATT_W / 2.0f, y + BATT_H / 2.0f, UI_COLOR_TEXT_PRIMARY);
}

static const char *StatusBar_GetCurrentTime(void) {
	static char buffer[27];
	SceDateTime time;
	int hours, am_or_pm;

	sceRtcGetCurrentClockLocalTime(&time);
	hours = sceRtcGetHour(&time);
	am_or_pm = (hours < 12);

	if (hours == 0)
		hours = 12;
	else if (hours > 12)
		hours = hours - 12;

	snprintf(buffer, sizeof(buffer), "%2i:%02i %s", hours, sceRtcGetMinute(&time), am_or_pm ? "AM" : "PM");

	return buffer;
}

// Draws the level to the right of `x` and its percentage to the left of it.
static void StatusBar_DrawBatteryStatus(float x, int baseline) {
	int percent = scePowerGetBatteryLifePercent();
	SceBool charging = (scePowerIsBatteryCharging() > 0) ? SCE_TRUE : SCE_FALSE;
	char buf[13];
	int width;

	if (percent < 0) {
		// Unknown level: the shell alone, and no number to go with it.
		StatusBar_DrawBattery(x, (STATUS_BAR_H - BATT_H) / 2.0f, -1, charging);
		return;
	}

	snprintf(buf, sizeof(buf), "%d%%", percent);
	width = UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, buf);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, x - width - 8, baseline, UI_COLOR_TEXT_TERTIARY, buf);

	StatusBar_DrawBattery(x, (STATUS_BAR_H - BATT_H) / 2.0f, percent, charging);
}

void StatusBar_Display(void) {
	// One call, one string, one measurement: the clock used to be built twice
	// per frame, once to measure and once to draw, so the width and the text
	// came from two separate reads of the clock.
	const char *now = StatusBar_GetCurrentTime();
	int width = UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, now);
	int baseline = UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, 0, STATUS_BAR_H);

	StatusBar_DrawBatteryStatus((950 - width) - (BATT_W + BATT_NUB_W + 14), baseline);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, 950 - width, baseline, UI_COLOR_TEXT_TERTIARY, now);
}
