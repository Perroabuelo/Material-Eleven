#include <psp2/types.h>

#include "textures.h"
#include "ui_gpu.h"

extern SceUChar8 _binary_res_battery_20_png_start;
extern SceUChar8 _binary_res_battery_30_png_start;
extern SceUChar8 _binary_res_battery_50_png_start;
extern SceUChar8 _binary_res_battery_60_png_start;
extern SceUChar8 _binary_res_battery_80_png_start;
extern SceUChar8 _binary_res_battery_90_png_start;
extern SceUChar8 _binary_res_battery_full_png_start;

extern SceUChar8 _binary_res_battery_20_charging_png_start;
extern SceUChar8 _binary_res_battery_30_charging_png_start;
extern SceUChar8 _binary_res_battery_50_charging_png_start;
extern SceUChar8 _binary_res_battery_60_charging_png_start;
extern SceUChar8 _binary_res_battery_80_charging_png_start;
extern SceUChar8 _binary_res_battery_90_charging_png_start;
extern SceUChar8 _binary_res_battery_full_charging_png_start;

extern SceUChar8 _binary_res_battery_low_png_start;
extern SceUChar8 _binary_res_battery_unknown_png_start;

extern SceUChar8 _binary_res_icon_audio_png_start;
extern SceUChar8 _binary_res_icon_file_png_start;
extern SceUChar8 _binary_res_icon_folder_png_start;
extern SceUChar8 _binary_res_icon_back_png_start;

extern SceUChar8 _binary_res_default_artwork_png_start;
extern SceUChar8 _binary_res_default_artwork_blur_png_start;

extern SceUChar8 _binary_res_radio_button_checked_png_start;
extern SceUChar8 _binary_res_radio_button_unchecked_png_start;

// A PNG that fails to decode comes back NULL, and setting filters on it used to
// dereference it right here. Every draw site guards the handle instead, so a
// resource that does not load leaves a gap rather than taking the app down.
static vita2d_texture *Texture_LoadImageBilinear(SceUChar8 *buffer) {
	vita2d_texture *texture = vita2d_load_PNG_buffer(buffer);

	if (!texture)
		return NULL;

	vita2d_texture_set_filters(texture, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);
	return texture;
}

void Textures_Load(void) {
	battery_20 = Texture_LoadImageBilinear(&_binary_res_battery_20_png_start);
	battery_30 = Texture_LoadImageBilinear(&_binary_res_battery_30_png_start);
	battery_50 = Texture_LoadImageBilinear(&_binary_res_battery_50_png_start);
	battery_60 = Texture_LoadImageBilinear(&_binary_res_battery_60_png_start);
	battery_80 = Texture_LoadImageBilinear(&_binary_res_battery_80_png_start);
	battery_90 = Texture_LoadImageBilinear(&_binary_res_battery_90_png_start);
	battery_full = Texture_LoadImageBilinear(&_binary_res_battery_full_png_start);

	battery_20_charging = Texture_LoadImageBilinear(&_binary_res_battery_20_charging_png_start);
	battery_30_charging = Texture_LoadImageBilinear(&_binary_res_battery_30_charging_png_start);
	battery_50_charging = Texture_LoadImageBilinear(&_binary_res_battery_50_charging_png_start);
	battery_60_charging = Texture_LoadImageBilinear(&_binary_res_battery_60_charging_png_start);
	battery_80_charging = Texture_LoadImageBilinear(&_binary_res_battery_80_charging_png_start);
	battery_90_charging = Texture_LoadImageBilinear(&_binary_res_battery_90_charging_png_start);
	battery_full_charging = Texture_LoadImageBilinear(&_binary_res_battery_full_charging_png_start);

	battery_low = Texture_LoadImageBilinear(&_binary_res_battery_low_png_start);
	battery_unknown = Texture_LoadImageBilinear(&_binary_res_battery_unknown_png_start);

	icon_audio = Texture_LoadImageBilinear(&_binary_res_icon_audio_png_start);
	icon_file = Texture_LoadImageBilinear(&_binary_res_icon_file_png_start);
	icon_dir = Texture_LoadImageBilinear(&_binary_res_icon_folder_png_start);
	icon_back = Texture_LoadImageBilinear(&_binary_res_icon_back_png_start);

	default_artwork = Texture_LoadImageBilinear(&_binary_res_default_artwork_png_start);
	default_artwork_blur = Texture_LoadImageBilinear(&_binary_res_default_artwork_blur_png_start);

	radio_on = Texture_LoadImageBilinear(&_binary_res_radio_button_checked_png_start);
	radio_off = Texture_LoadImageBilinear(&_binary_res_radio_button_unchecked_png_start);
}

void Textures_Free(void) {
	UI_GpuFreeTexture(&radio_off);
	UI_GpuFreeTexture(&radio_on);

	UI_GpuFreeTexture(&default_artwork_blur);
	UI_GpuFreeTexture(&default_artwork);

	UI_GpuFreeTexture(&icon_back);
	UI_GpuFreeTexture(&icon_dir);
	UI_GpuFreeTexture(&icon_file);
	UI_GpuFreeTexture(&icon_audio);

	UI_GpuFreeTexture(&battery_unknown);
	UI_GpuFreeTexture(&battery_low);

	UI_GpuFreeTexture(&battery_full_charging);
	UI_GpuFreeTexture(&battery_90_charging);
	UI_GpuFreeTexture(&battery_80_charging);
	UI_GpuFreeTexture(&battery_60_charging);
	UI_GpuFreeTexture(&battery_50_charging);
	UI_GpuFreeTexture(&battery_30_charging);
	UI_GpuFreeTexture(&battery_20_charging);

	UI_GpuFreeTexture(&battery_full);
	UI_GpuFreeTexture(&battery_90);
	UI_GpuFreeTexture(&battery_80);
	UI_GpuFreeTexture(&battery_60);
	UI_GpuFreeTexture(&battery_50);
	UI_GpuFreeTexture(&battery_30);
	UI_GpuFreeTexture(&battery_20);
}
