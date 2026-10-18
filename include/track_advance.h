#ifndef _ELEVENMPV_TRACK_ADVANCE_H_
#define _ELEVENMPV_TRACK_ADVANCE_H_

// What to open when the queue moves: the next track of the plan, or the one
// playing again when it repeats, skipping tracks that do not open.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_track_advance.c). The queue and the decoders come in as the
// caller's functions; Music_HandleNext (menus/menu_audioplayer.c) passes
// adapters over Menu_InitMusic and the real queue.

#include <stdint.h>

// Past this much of the current track, going back restarts it instead of
// moving to the previous one.
#define TRACK_ADVANCE_RESTART_MS 3000

typedef struct {
	// 1 if the track at `path` opened and is now the one loaded.
	int (*open)(void *ctx, const char *path);
	// Moves one place through the plan and returns the path there, or NULL.
	const char *(*advance)(void *ctx, int forward);
	// The track playing, or NULL.
	const char *(*current)(void *ctx);
	void *ctx;
} TrackAdvance_Ops;

// Starts at the current track when `replay` is set and at the next one in the
// `forward` direction otherwise, then tries `open` at most `count` times,
// moving in that same direction after each failure. A NULL path is never
// passed to `open`. With `count` 0 nothing is tried.
//
// Returns 1 if a track opened, 0 if none did.
int TrackAdvance_Run(const TrackAdvance_Ops *ops, int count, int forward, int replay);

// 1 if going back from `position_ms` into the current track should restart it
// (more than TRACK_ADVANCE_RESTART_MS played), 0 if it should open the
// previous one.
int TrackAdvance_PrevRestarts(uint32_t position_ms);

#endif
