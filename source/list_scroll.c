#include <math.h>

#include "list_scroll.h"

// Inertia keeps FRICTION of its speed every 60 Hz frame (16.67 ms), stops
// below STOP_SPEED and does not start below MIN_FLING. Tuned on the console.
#define FRICTION   0.95f
#define FRAME_MS   16.67f
#define STOP_SPEED 0.02f
#define MIN_FLING  0.3f

float ListScroll_MaxOffset(const ListScroll *scroll) {
	float max = scroll->content_h - scroll->viewport_h;
	return max > 0.0f ? max : 0.0f;
}

// Returns 1 if the offset had to be clamped.
static int ListScroll_Clamp(ListScroll *scroll) {
	float max = ListScroll_MaxOffset(scroll);

	if (scroll->offset < 0.0f) {
		scroll->offset = 0.0f;
		return 1;
	}

	if (scroll->offset > max) {
		scroll->offset = max;
		return 1;
	}

	return 0;
}

void ListScroll_SetContent(ListScroll *scroll, float content_h, float viewport_h) {
	scroll->content_h = content_h > 0.0f ? content_h : 0.0f;
	scroll->viewport_h = viewport_h > 0.0f ? viewport_h : 0.0f;

	if (ListScroll_Clamp(scroll))
		scroll->velocity = 0.0f;
}

void ListScroll_SetSize(ListScroll *scroll, int rows, float row_h, float viewport_h) {
	if (rows < 0)
		rows = 0;

	scroll->row_h = row_h > 0.0f ? row_h : 0.0f;
	ListScroll_SetContent(scroll, rows * scroll->row_h, viewport_h);
}

void ListScroll_Drag(ListScroll *scroll, float dy) {
	scroll->velocity = 0.0f;
	scroll->offset -= dy;
	ListScroll_Clamp(scroll);
}

void ListScroll_Fling(ListScroll *scroll, float finger_velocity) {
	float v = -finger_velocity;

	if (fabsf(v) < MIN_FLING || ListScroll_MaxOffset(scroll) <= 0.0f)
		v = 0.0f;

	scroll->velocity = v;
}

void ListScroll_Step(ListScroll *scroll, float dt_ms) {
	if (scroll->velocity == 0.0f || dt_ms <= 0.0f)
		return;

	scroll->offset += scroll->velocity * dt_ms;
	if (ListScroll_Clamp(scroll)) {
		scroll->velocity = 0.0f;
		return;
	}

	scroll->velocity *= powf(FRICTION, dt_ms / FRAME_MS);
	if (fabsf(scroll->velocity) < STOP_SPEED)
		scroll->velocity = 0.0f;
}

void ListScroll_Stop(ListScroll *scroll) {
	scroll->velocity = 0.0f;
}

int ListScroll_Moving(const ListScroll *scroll) {
	return scroll->velocity != 0.0f;
}

void ListScroll_ShowRange(ListScroll *scroll, float top, float bottom) {
	scroll->velocity = 0.0f;

	if (top < scroll->offset)
		scroll->offset = top;
	else if (bottom > scroll->offset + scroll->viewport_h)
		scroll->offset = bottom - scroll->viewport_h;

	ListScroll_Clamp(scroll);
}

void ListScroll_EnsureVisible(ListScroll *scroll, int index) {
	if (index < 0 || scroll->row_h <= 0.0f || index * scroll->row_h >= scroll->content_h) {
		scroll->velocity = 0.0f;
		return;
	}

	ListScroll_ShowRange(scroll, index * scroll->row_h, (index + 1) * scroll->row_h);
}

int ListScroll_RowAt(const ListScroll *scroll, float y) {
	if (y < 0.0f || y >= scroll->viewport_h || scroll->row_h <= 0.0f)
		return -1;

	float content_y = y + scroll->offset;
	if (content_y >= scroll->content_h)
		return -1;

	return (int)(content_y / scroll->row_h);
}

int ListScroll_FirstRow(const ListScroll *scroll) {
	if (scroll->row_h <= 0.0f)
		return 0;

	return (int)(scroll->offset / scroll->row_h);
}

float ListScroll_RowY(const ListScroll *scroll, int index) {
	return index * scroll->row_h - scroll->offset;
}
