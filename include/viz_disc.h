#ifndef _ELEVENMPV_VIZ_DISC_H_
#define _ELEVENMPV_VIZ_DISC_H_

// The vinyl's label: a fan of triangles with the cover art as its texture.
// The rim stays put on screen and the texture coordinates turn, so the cover
// spins inside the circle without moving the polygon. A cover that is not
// square is cropped to its centred square before the circle is cut.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_disc.c).

#define VIZ_DISC_SEGMENTS 96

// Same layout as vita2d_texture_vertex, which viz_draw.c checks.
typedef struct {
	float x, y, z;
	float u, v;
} VizDisc_Vertex;

// Vertices VizDisc_Fan writes for `segments`: the centre, then the rim with
// the first point repeated to close it.
#define VIZ_DISC_FAN_VERTICES(segments) ((segments) + 2)

// Writes a triangle fan of `segments` (at least 3) into `out` for a circle of
// `radius` at (cx, cy), with the texture of tex_w x tex_h turned by `angle`
// radians. Returns the vertices written.
unsigned int VizDisc_Fan(VizDisc_Vertex *out, float cx, float cy, float radius,
	int segments, float angle, float tex_w, float tex_h);

#endif
