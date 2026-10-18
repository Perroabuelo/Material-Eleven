#include "viz_scope.h"

unsigned int VizScope_Trigger(const float *samples, unsigned int frames, unsigned int span) {
	if (frames < span)
		return 0;

	unsigned int last = frames / 2;
	if (last > frames - span)
		last = frames - span;

	for (unsigned int i = 1; i <= last; i++)
		if (samples[i - 1] < 0.0f && samples[i] >= 0.0f)
			return i;

	return 0;
}
