#include "key_repeat.h"

void KeyRepeat_Init(KeyRepeat_State *s, unsigned int up, unsigned int down) {
	s->up = up;
	s->down = down;
	s->dir = 0;
	s->start_us = 0;
	s->next_us = 0;
}

KeyRepeat_Step KeyRepeat_Update(KeyRepeat_State *s, unsigned int held, unsigned int pressed, uint64_t now_us) {
	KeyRepeat_Step step = { 0, 0, 0 };

	// The edge works as it always did: Up wins over Down.
	if (pressed & s->up)
		step.dir = -1;
	else if (pressed & s->down)
		step.dir = 1;

	int up_held = (held & s->up) != 0;
	int down_held = (held & s->down) != 0;
	int held_dir = (up_held == down_held) ? 0 : (up_held ? -1 : 1);

	if (held_dir == 0) {
		s->dir = 0;
		return step;
	}

	if (step.dir == held_dir || held_dir != s->dir) {
		s->dir = held_dir;
		s->start_us = now_us;
		s->next_us = now_us + KEY_REPEAT_DELAY_US;
		return step;
	}

	if (step.dir == 0 && now_us >= s->next_us) {
		step.dir = held_dir;
		step.repeat = 1;
		step.page = (now_us - s->start_us) >= KEY_REPEAT_PAGE_US;
		s->next_us = now_us + KEY_REPEAT_INTERVAL_US;
	}

	return step;
}

int KeyRepeat_Move(int sel, int count, KeyRepeat_Step step, int page) {
	if (count <= 0)
		return 0;

	if (step.repeat) {
		int rows = (step.page && page > 1) ? page : 1;
		sel += step.dir * rows;

		if (sel < 0)
			sel = 0;
		else if (sel > count - 1)
			sel = count - 1;

		return sel;
	}

	sel += step.dir;

	if (sel > count - 1)
		sel = 0;
	else if (sel < 0)
		sel = count - 1;

	return sel;
}
