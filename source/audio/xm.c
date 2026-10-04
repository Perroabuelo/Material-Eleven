#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "xmp.h"

// libs/lib/libxmp-lite.a was built against an older newlib that exposed the
// ctype table through __ctype_ptr__ instead of the current _ctype_ symbol.
extern const char _ctype_[];
const char *__ctype_ptr__ = _ctype_;

static xmp_context xmp;
static struct xmp_frame_info frame_info;
static struct xmp_module_info module_info;
static SceUInt64 samples_read = 0, total_samples = 0;

int XM_Init(const char *path) {
	char xmp_path[512];

	// xmp_load_module asks for char * rather than const char *. This used to
	// be a strdup that was never freed, one per track played.
	snprintf(xmp_path, sizeof(xmp_path), "%s", path);

	xmp = xmp_create_context();
	if (xmp == NULL)
		return -1;

	if (xmp_load_module(xmp, xmp_path) < 0) {
		xmp_free_context(xmp);
		xmp = NULL;
		return -1;
	}

	xmp_start_player(xmp, 44100, 0);
	xmp_get_frame_info(xmp, &frame_info);
	total_samples = (frame_info.total_time * 44.1);

	xmp_get_module_info(xmp, &module_info);
	if (module_info.mod->name[0] != '\0') {
        metadata.has_meta = SCE_TRUE;
        strcpy(metadata.title, module_info.mod->name);
    }

	return 0;
}

SceUInt32 XM_GetSampleRate(void) {
	return 44100;
}

SceUInt8 XM_GetChannels(void) {
	return 2;
}

void XM_Decode(void *buf, unsigned int length, void *userdata) {
	xmp_play_buffer(xmp, buf, (int)length * (sizeof(SceInt16) * 2), 0);
	samples_read += length;

	if (samples_read >= total_samples)
		playing = SCE_FALSE;
}

SceUInt64 XM_GetPosition(void) {
	return samples_read;
}

SceUInt64 XM_GetLength(void) {
	return total_samples;
}

SceUInt64 XM_Seek(SceUInt64 index) {
	int seek_sample = (total_samples * (index / 450.0));
	
	if (xmp_seek_time(xmp, seek_sample/44.1) >= 0) {
		samples_read = seek_sample;
		return samples_read;
	}

	return -1;
}

void XM_Term(void) {
	samples_read = 0;
	xmp_end_player(xmp);
	xmp_release_module(xmp);
	xmp_free_context(xmp);
}
