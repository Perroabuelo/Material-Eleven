#include <psp2/power.h>
#include <psp2/rtc.h>
#include <stdio.h>

#include "common.h"
#include "textures.h"
#include "ui_gpu.h"
#include "ui_theme.h"

// The band the clock and battery sit in, shared with the screens that draw a
// hairline under it.
#define STATUS_BAR_H 32

static int percent_width = 0;

static char *StatusBar_GetCurrentTime(void) {
	static char buffer[27];

	SceDateTime time;
	sceRtcGetCurrentClockLocalTime(&time);
	int hours = sceRtcGetHour(&time);
	int am_or_pm = 0;

	if (hours < 12)
		am_or_pm = 1;
	if (hours == 0)
		hours = 12;
	else if (hours > 12)
		hours = hours - 12;

	if ((hours >= 1) && (hours < 10))
		snprintf(buffer, 27, "%2i:%02i %s", hours, sceRtcGetMinute(&time), am_or_pm ? "AM" : "PM");
	else
		snprintf(buffer, 27, "%2i:%02i %s", hours, sceRtcGetMinute(&time), am_or_pm ? "AM" : "PM");

	return buffer;
}

static void StatusBar_GetBatteryStatus(int x, int y) {
	int percent = 0;
	SceBool state = SCE_FALSE;
	char buf[13];

	if (R_FAILED(state = scePowerIsBatteryCharging()))
		state = SCE_FALSE;

	if (R_SUCCEEDED(percent = scePowerGetBatteryLifePercent())) {
		if (percent < 20)
			UI_GpuDrawTexture(battery_low, x, (STATUS_BAR_H - 24) / 2);
		else if ((percent >= 20) && (percent < 30)) {
			if (state)
				UI_GpuDrawTexture(battery_20_charging, x, (STATUS_BAR_H - 24) / 2);
			else
				UI_GpuDrawTexture(battery_20, x, (STATUS_BAR_H - 24) / 2);
		}
		else if ((percent >= 30) && (percent < 50)) {
			if (state)
				UI_GpuDrawTexture(battery_50_charging, x, (STATUS_BAR_H - 24) / 2);
			else
				UI_GpuDrawTexture(battery_50, x, (STATUS_BAR_H - 24) / 2);
		}
		else if ((percent >= 50) && (percent < 60)) {
			if (state)
				UI_GpuDrawTexture(battery_50_charging, x, (STATUS_BAR_H - 24) / 2);
			else
				UI_GpuDrawTexture(battery_50, x, (STATUS_BAR_H - 24) / 2);
		}
		else if ((percent >= 60) && (percent < 80)) {
			if (state)
				UI_GpuDrawTexture(battery_60_charging, x, (STATUS_BAR_H - 24) / 2);
			else
				UI_GpuDrawTexture(battery_60, x, (STATUS_BAR_H - 24) / 2);
		}
		else if ((percent >= 80) && (percent < 90)) {
			if (state)
				UI_GpuDrawTexture(battery_80_charging, x, (STATUS_BAR_H - 24) / 2);
			else
				UI_GpuDrawTexture(battery_80, x, (STATUS_BAR_H - 24) / 2);
		}
		else if ((percent >= 90) && (percent < 100)) {
			if (state)
				UI_GpuDrawTexture(battery_90_charging, x, (STATUS_BAR_H - 24) / 2);
			else
				UI_GpuDrawTexture(battery_90, x, (STATUS_BAR_H - 24) / 2);
		}
		else if (percent == 100) {
			if (state)
				UI_GpuDrawTexture(battery_full_charging, x, (STATUS_BAR_H - 24) / 2);
			else
				UI_GpuDrawTexture(battery_full, x, (STATUS_BAR_H - 24) / 2);
		}

		snprintf(buf, 13, "%d%%", percent);
		percent_width = UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, buf);
		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, (x - percent_width - 5), y, UI_COLOR_TEXT_TERTIARY, buf);
	}
	else {
		snprintf(buf, 13, "%d%%", percent);
		percent_width = UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, buf);
		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, (x - percent_width - 5), y, UI_COLOR_TEXT_TERTIARY, buf);
		UI_GpuDrawTexture(battery_unknown, x, (STATUS_BAR_H - 24) / 2);
	}
}

void StatusBar_Display(void) {
	const char *now = StatusBar_GetCurrentTime();
	int width = UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, now);
	int baseline = UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, 0, STATUS_BAR_H);

	StatusBar_GetBatteryStatus((950 - width) - (32 + 10), baseline);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, 950 - width, baseline, UI_COLOR_TEXT_TERTIARY, now);
}
