#include <psp2/kernel/processmgr.h>
#include <string.h>

#include "touch.h"
#include "touch_list.h"
#include "ui_theme.h"

// A frame that took longer than this (a dialog, a scan) counts as this long,
// so the list does not jump when the loop comes back.
#define TOUCH_LIST_MAX_DT_MS 100.0f
#define TOUCH_LIST_FRAME_MS  16.67f

void TouchList_Reset(TouchList *list) {
	memset(list, 0, sizeof(*list));
	Gesture_Reset(&list->gesture);
}

void TouchList_Follow(TouchList *list) {
	ListScroll_Stop(&list->scroll);
	list->follow = SCE_TRUE;
}

void TouchList_Layout(TouchList *list, float left, float top, float right, float bottom,
	int rows, float row_h, int selection) {
	list->left = left;
	list->top = top;
	list->right = right;
	list->bottom = bottom;

	ListScroll_SetSize(&list->scroll, rows, row_h, bottom - top);

	if (list->follow) {
		list->follow = SCE_FALSE;
		ListScroll_EnsureVisible(&list->scroll, selection);
	}
}

static SceBool TouchList_Contains(const TouchList *list, float x, float y) {
	return x >= list->left && x < list->right && y >= list->top && y < list->bottom;
}

int TouchList_Update(TouchList *list, SceBool may_own) {
	SceUInt64 now = sceKernelGetProcessTimeWide();
	float dt = (list->last_us != 0) ? (float)(now - list->last_us) / 1000.0f : TOUCH_LIST_FRAME_MS;
	list->last_us = now;

	if (dt > TOUCH_LIST_MAX_DT_MS)
		dt = TOUCH_LIST_MAX_DT_MS;

	float x = (float)Touch_GetX(), y = (float)Touch_GetY();

	if (Touch_CheckPressed())
		list->owned = may_own && TouchList_Contains(list, x, y);

	Gesture_Input in = { list->owned && Touch_CheckHeld(), x, y, dt };
	Gesture_Result res = Gesture_Update(&list->gesture, &in);
	int tapped = -1;

	switch (res.type) {
		case GESTURE_PRESS:
			// Landing on a gliding list stops it, and that touch is spent.
			if (ListScroll_Moving(&list->scroll)) {
				ListScroll_Stop(&list->scroll);
				Gesture_Catch(&list->gesture);
			}
			break;

		case GESTURE_DRAG:
			ListScroll_Drag(&list->scroll, res.dy);
			break;

		case GESTURE_RELEASE:
			ListScroll_Fling(&list->scroll, res.velocity);
			break;

		case GESTURE_TAP:
			// Lifted outside the list: nothing.
			if (TouchList_Contains(list, res.end_x, res.end_y))
				tapped = ListScroll_RowAt(&list->scroll, res.start_y - list->top);
			break;

		case GESTURE_NONE:
			break;
	}

	ListScroll_Step(&list->scroll, dt);

	if (!Touch_CheckHeld())
		list->owned = SCE_FALSE;

	return tapped;
}

int TouchList_PressedRow(const TouchList *list) {
	if (!Gesture_Pressing(&list->gesture))
		return -1;

	return ListScroll_RowAt(&list->scroll, list->gesture.start_y - list->top);
}

int TouchList_FirstRow(const TouchList *list) {
	return ListScroll_FirstRow(&list->scroll);
}

float TouchList_RowY(const TouchList *list, int index) {
	return list->top + ListScroll_RowY(&list->scroll, index);
}

void TouchList_BeginClip(const TouchList *list) {
	UI_PushClip(list->left, list->top, list->right, list->bottom);
}

void TouchList_EndClip(void) {
	UI_PopClip();
}
