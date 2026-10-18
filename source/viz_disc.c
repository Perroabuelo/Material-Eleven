#include <math.h>

#include "viz_disc.h"

#define TWO_PI_F 6.28318530718f

unsigned int VizDisc_Fan(VizDisc_Vertex *out, float cx, float cy, float radius,
	int segments, float angle, float tex_w, float tex_h) {
	if (segments < 3)
		segments = 3;

	// Half the side of the centred square, as a share of each axis.
	float su = 0.5f, sv = 0.5f;
	if (tex_w > tex_h && tex_w > 0.0f)
		su = 0.5f * tex_h / tex_w;
	else if (tex_h > tex_w && tex_h > 0.0f)
		sv = 0.5f * tex_w / tex_h;

	out[0] = (VizDisc_Vertex){ cx, cy, 0.5f, 0.5f, 0.5f };

	for (int i = 0; i <= segments; i++) {
		// The last point is the first again, computed the same way so the
		// fan closes without a seam.
		float theta = TWO_PI_F * (i % segments) / segments;
		float tex = theta + angle;

		out[i + 1] = (VizDisc_Vertex){
			cx + radius * cosf(theta),
			cy + radius * sinf(theta),
			0.5f,
			0.5f + su * cosf(tex),
			0.5f + sv * sinf(tex)
		};
	}

	return VIZ_DISC_FAN_VERTICES(segments);
}
