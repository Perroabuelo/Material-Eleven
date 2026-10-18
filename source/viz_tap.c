#include <string.h>

#include "viz_tap.h"

static VizTap shared_tap;

VizTap *VizTap_Shared(void) {
	return &shared_tap;
}

void VizTap_Reset(VizTap *tap) {
	tap->begun = 0;
	tap->written = 0;
	tap->rate = 0;
	__sync_synchronize();
}

void VizTap_Write(VizTap *tap, const int16_t *samples, unsigned int frames,
	unsigned int channels, unsigned int rate) {
	if (frames == 0 || (channels != 1 && channels != 2))
		return;

	if (frames > VIZ_TAP_FRAMES) {
		samples += (frames - VIZ_TAP_FRAMES) * channels;
		frames = VIZ_TAP_FRAMES;
	}

	uint32_t start = tap->written;
	uint32_t end = start + frames;

	// Announced before the copy, so a reader can tell its window was
	// being overwritten.
	tap->begun = end;
	__sync_synchronize();

	for (unsigned int i = 0; i < frames; i++) {
		unsigned int pos = (start + i) % VIZ_TAP_FRAMES;
		int16_t l = samples[i * channels];
		int16_t r = channels == 2 ? samples[i * channels + 1] : l;

		tap->samples[pos * 2] = l;
		tap->samples[pos * 2 + 1] = r;
	}

	tap->rate = rate;
	__sync_synchronize();
	tap->written = end;
}

int VizTap_Read(const VizTap *tap, unsigned int delay, int16_t *out,
	unsigned int frames, unsigned int *out_rate) {
	if (frames > VIZ_TAP_FRAMES)
		return 0;

	uint32_t written = tap->written;
	__sync_synchronize();
	unsigned int rate = tap->rate;

	// Positions are counted from the reset, so anything before 0 is the
	// silence before the track started.
	int64_t end = (int64_t)written - delay;
	int64_t start = end - frames;

	for (unsigned int i = 0; i < frames; i++) {
		int64_t pos = start + i;

		if (pos < 0) {
			out[i * 2] = 0;
			out[i * 2 + 1] = 0;
			continue;
		}

		unsigned int at = (uint32_t)pos % VIZ_TAP_FRAMES;
		out[i * 2] = tap->samples[at * 2];
		out[i * 2 + 1] = tap->samples[at * 2 + 1];
	}

	__sync_synchronize();
	uint32_t begun = tap->begun;

	// The writer reuses a position VIZ_TAP_FRAMES frames later, so the
	// window is intact only if nothing written past it reached that far.
	int64_t first = start > 0 ? start : 0;
	if ((int64_t)begun - first > VIZ_TAP_FRAMES)
		return 0;

	if (out_rate != NULL)
		*out_rate = rate;

	return 1;
}
