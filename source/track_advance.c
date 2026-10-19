#include <stddef.h>

#include "track_advance.h"

int TrackAdvance_Run(const TrackAdvance_Ops *ops, int count, int forward, int replay) {
	if (count <= 0)
		return 0;

	const char *next = replay ? ops->current(ops->ctx) : ops->advance(ops->ctx, forward);

	// A track that does not open cannot keep the screen: it is skipped and the
	// next one of the plan is tried, in the same direction. At most one full
	// lap of the queue, which is what bounds this when none opens.
	for (int tries = 0; tries < count; tries++) {
		if (next != NULL && ops->open(ops->ctx, next))
			return 1;

		next = ops->advance(ops->ctx, forward);
	}

	return 0;
}

int TrackAdvance_PrevRestarts(uint32_t position_ms) {
	return position_ms > TRACK_ADVANCE_RESTART_MS;
}
