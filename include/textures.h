#ifndef _ELEVENMPV_TEXTURES_H_
#define _ELEVENMPV_TEXTURES_H_

#include <vita2d.h>

// UI controls and chrome are drawn as vector geometry (see ui_theme.h), so the
// only textures left are real content and indicators: file-row icons, battery
// state and cover art.
vita2d_texture *icon_dir, *icon_file, *icon_audio, *battery_20, *battery_20_charging, *battery_30, *battery_30_charging, *battery_50, *battery_50_charging, \
	*battery_60, *battery_60_charging, *battery_80, *battery_80_charging, *battery_90, *battery_90_charging, *battery_full, *battery_full_charging, \
	*battery_low, *battery_unknown, *default_artwork, *default_artwork_blur, *icon_back, *radio_on, *radio_off;

void Textures_Load(void);
void Textures_Free(void);

#endif
