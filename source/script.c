#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "script.h"

// The tests swap malloc for one that fails on demand (tests/test_script.c).
#ifdef SCRIPT_TEST_ALLOC
void *Script_TestMalloc(size_t size);
#define script_malloc Script_TestMalloc
#else
#define script_malloc malloc
#endif

// A wait has its category, up to LOG_FIELDS_MAX conditions and a timeout;
// no other command comes close.
#define TOKENS_MAX (LOG_FIELDS_MAX + 3)

typedef struct {
	const char *s;
	unsigned int len;
} Token;

typedef struct {
	Script *script;
	ScriptError *err;
	int line;
	const LogButtonName *buttons;
	int button_count;
} Parser;

static int Fail(Parser *p, const char *fmt, const Token *token) {
	p->err->line = p->line;

	if (token) {
		// A token long enough to fill the reason is shown cut.
		int n = token->len > 48 ? 48 : (int)token->len;
		snprintf(p->err->reason, sizeof(p->err->reason), fmt, n, token->s);
	}
	else {
		snprintf(p->err->reason, sizeof(p->err->reason), "%s", fmt);
	}

	return 0;
}

static int IsBlank(char c) {
	return c == ' ' || c == '\t';
}

static int TokenIs(const Token *t, const char *word) {
	return t->len == strlen(word) && memcmp(t->s, word, t->len) == 0;
}

// Copies `len` bytes and a NUL to the pool. The pool is sized for the worst
// case of the whole file, so running out means a bug here, not a big suite.
static const char *PoolAdd(Parser *p, const char *s, unsigned int len) {
	Script *script = p->script;

	if (script->pool_used + len + 1 > script->pool_size)
		return NULL;

	char *dst = script->pool + script->pool_used;
	memcpy(dst, s, len);
	dst[len] = '\0';
	script->pool_used += len + 1;
	return dst;
}

// Where the line's comment starts: the first '#' outside double quotes.
static unsigned int CommentStart(const char *s, unsigned int len) {
	int quoted = 0;

	for (unsigned int i = 0; i < len; i++) {
		if (quoted && s[i] == '\\')
			i++;
		else if (s[i] == '"')
			quoted = !quoted;
		else if (!quoted && s[i] == '#')
			return i;
	}

	return len;
}

// Splits at blanks outside double quotes. Returns how many tokens, or -1 if
// there are more than `max`.
static int Tokenize(const char *s, unsigned int len, Token *tokens, int max) {
	int count = 0;
	unsigned int i = 0;

	while (i < len) {
		while (i < len && IsBlank(s[i]))
			i++;
		if (i >= len)
			break;

		if (count >= max)
			return -1;

		unsigned int start = i;
		int quoted = 0;
		while (i < len && (quoted || !IsBlank(s[i]))) {
			if (quoted && s[i] == '\\' && i + 1 < len)
				i++;
			else if (s[i] == '"')
				quoted = !quoted;
			i++;
		}

		tokens[count].s = s + start;
		tokens[count].len = i - start;
		count++;
	}

	return count;
}

// Digits only, no sign, at most `max`.
static int ParseUInt(const Token *t, unsigned int len, uint32_t max, uint32_t *out) {
	uint64_t value = 0;

	if (len == 0)
		return 0;

	for (unsigned int i = 0; i < len; i++) {
		if (t->s[i] < '0' || t->s[i] > '9')
			return 0;
		value = value * 10 + (uint64_t)(t->s[i] - '0');
		if (value > max)
			return 0;
	}

	*out = (uint32_t)value;
	return 1;
}

static int ParseDuration(Parser *p, const Token *t, uint32_t *ms) {
	unsigned int len = t->len;
	uint32_t scale;

	if (len > 2 && t->s[len - 2] == 'm' && t->s[len - 1] == 's') {
		len -= 2;
		scale = 1;
	}
	else if (len > 1 && t->s[len - 1] == 's') {
		len -= 1;
		scale = 1000;
	}
	else {
		return Fail(p, "invalid duration \"%.*s\" (use ms or s)", t);
	}

	uint32_t value;
	if (!ParseUInt(t, len, SCRIPT_DURATION_MAX_MS / scale, &value) || value == 0)
		return Fail(p, "invalid duration \"%.*s\"", t);

	*ms = value * scale;
	return 1;
}

static int ParseCoord(Parser *p, const Token *t, int limit, int16_t *out) {
	uint32_t value;

	if (!ParseUInt(t, t->len, 0xFFFF, &value))
		return Fail(p, "invalid coordinate \"%.*s\"", t);
	if (value >= (uint32_t)limit)
		return Fail(p, limit == SCRIPT_SCREEN_W ? "x out of range (0-959): %.*s" : "y out of range (0-543): %.*s", t);

	*out = (int16_t)value;
	return 1;
}

// "A+B+..." with the names of the table.
static int ParseButtons(Parser *p, const Token *t, unsigned int *mask) {
	unsigned int i = 0;

	*mask = 0;

	while (1) {
		Token name = { t->s + i, 0 };
		while (i < t->len && t->s[i] != '+') {
			i++;
			name.len++;
		}

		if (name.len == 0)
			return Fail(p, "invalid buttons \"%.*s\"", t);

		int found = 0;
		for (int b = 0; b < p->button_count; b++) {
			if (TokenIs(&name, p->buttons[b].name)) {
				*mask |= p->buttons[b].mask;
				found = 1;
				break;
			}
		}

		if (!found)
			return Fail(p, "unknown button \"%.*s\"", &name);

		if (i >= t->len)
			return 1;
		i++;
	}
}

static int Arguments(Parser *p, const Token *tokens, int count, int least, int most) {
	if (count - 1 < least)
		return Fail(p, "missing arguments for %.*s", &tokens[0]);
	if (count - 1 > most)
		return Fail(p, "too many arguments for %.*s", &tokens[0]);
	return 1;
}

// "wait <category> <key>=<value> ... [timeout=<duration>]": the conditions are
// joined into one log line body and read with the log's own parser, so a
// value is quoted and escaped exactly as the log writes it.
static int ParseWait(Parser *p, ScriptStep *step, const Token *tokens, int count) {
	char body[LOG_LINE_MAX];
	unsigned int len = 0;
	int timeout = 0;

	const Token *category = &tokens[1];
	if (count >= 2 && (memchr(category->s, '=', category->len) || memchr(category->s, '"', category->len)))
		return Fail(p, "missing category before \"%.*s\"", category);

	if (count < 3)
		return Fail(p, "missing arguments for %.*s", &tokens[0]);

	step->duration_ms = SCRIPT_WAIT_DEFAULT_MS;

	if (category->len >= sizeof(body))
		return Fail(p, "condition too long", NULL);
	memcpy(body, category->s, category->len);
	len = category->len;

	for (int i = 2; i < count; i++) {
		const Token *t = &tokens[i];

		if (t->len > 8 && memcmp(t->s, "timeout=", 8) == 0) {
			if (timeout)
				return Fail(p, "timeout given twice", NULL);
			Token value = { t->s + 8, t->len - 8 };
			if (!ParseDuration(p, &value, &step->duration_ms))
				return 0;
			timeout = 1;
			continue;
		}

		if (!memchr(t->s, '=', t->len))
			return Fail(p, "invalid condition \"%.*s\" (use key=value)", t);

		if (len + 1 + t->len >= sizeof(body))
			return Fail(p, "condition too long", NULL);
		body[len++] = ' ';
		memcpy(body + len, t->s, t->len);
		len += t->len;
	}

	LogParsed parsed;
	if (!LogLine_ParseBody(body, len, &parsed))
		return Fail(p, "invalid condition", NULL);
	if (parsed.count == 0)
		return Fail(p, "missing condition for wait", NULL);

	step->arg = PoolAdd(p, parsed.category, strlen(parsed.category));
	if (!step->arg)
		return Fail(p, "out of room", NULL);

	for (int i = 0; i < parsed.count; i++) {
		const char *key = PoolAdd(p, parsed.fields[i].key, strlen(parsed.fields[i].key));
		const char *value = PoolAdd(p, parsed.fields[i].value, strlen(parsed.fields[i].value));
		if (!key || !value)
			return Fail(p, "out of room", NULL);
		if (i == 0)
			step->cond = key;
	}
	step->cond_count = parsed.count;
	return 1;
}

// One line with a command, its comment and blanks already gone.
static int ParseLine(Parser *p, const char *s, unsigned int len) {
	Script *script = p->script;
	Token tokens[TOKENS_MAX];

	int count = Tokenize(s, len, tokens, TOKENS_MAX);
	if (count < 0)
		return Fail(p, "too many arguments", NULL);
	// Blank lines never get here; this keeps the compiler sure of it.
	if (count == 0)
		return 1;

	const Token *cmd = &tokens[0];
	ScriptStep step;
	memset(&step, 0, sizeof(step));
	step.line = p->line;

	if (TokenIs(cmd, "scenario")) {
		if (count < 2)
			return Fail(p, "missing scenario name", NULL);
		step.cmd = SCRIPT_SCENARIO;
		// The name is the rest of the line, spaces and all.
		unsigned int start = (unsigned int)(tokens[1].s - s);
		step.arg = PoolAdd(p, tokens[1].s, len - start);
		if (!step.arg)
			return Fail(p, "out of room", NULL);
	}
	else if (TokenIs(cmd, "press")) {
		step.cmd = SCRIPT_PRESS;
		if (!Arguments(p, tokens, count, 1, 2) || !ParseButtons(p, &tokens[1], &step.buttons))
			return 0;
		step.repeat = 1;
		if (count == 3) {
			const Token *t = &tokens[2];
			Token digits = { t->s + 1, t->len - 1 };
			uint32_t n;
			if (t->s[0] != 'x' || !ParseUInt(&digits, digits.len, SCRIPT_REPEAT_MAX, &n) || n == 0)
				return Fail(p, "invalid repeat \"%.*s\" (use x1 to x1000)", t);
			step.repeat = (int)n;
		}
	}
	else if (TokenIs(cmd, "hold")) {
		step.cmd = SCRIPT_HOLD;
		if (!Arguments(p, tokens, count, 2, 2)
			|| !ParseButtons(p, &tokens[1], &step.buttons)
			|| !ParseDuration(p, &tokens[2], &step.duration_ms))
			return 0;
	}
	else if (TokenIs(cmd, "tap")) {
		step.cmd = SCRIPT_TAP;
		if (!Arguments(p, tokens, count, 2, 2)
			|| !ParseCoord(p, &tokens[1], SCRIPT_SCREEN_W, &step.x1)
			|| !ParseCoord(p, &tokens[2], SCRIPT_SCREEN_H, &step.y1))
			return 0;
	}
	else if (TokenIs(cmd, "swipe")) {
		step.cmd = SCRIPT_SWIPE;
		if (!Arguments(p, tokens, count, 5, 5)
			|| !ParseCoord(p, &tokens[1], SCRIPT_SCREEN_W, &step.x1)
			|| !ParseCoord(p, &tokens[2], SCRIPT_SCREEN_H, &step.y1)
			|| !ParseCoord(p, &tokens[3], SCRIPT_SCREEN_W, &step.x2)
			|| !ParseCoord(p, &tokens[4], SCRIPT_SCREEN_H, &step.y2)
			|| !ParseDuration(p, &tokens[5], &step.duration_ms))
			return 0;
	}
	else if (TokenIs(cmd, "wait")) {
		step.cmd = SCRIPT_WAIT;
		if (!ParseWait(p, &step, tokens, count))
			return 0;
	}
	else if (TokenIs(cmd, "sleep")) {
		step.cmd = SCRIPT_SLEEP;
		if (!Arguments(p, tokens, count, 1, 1) || !ParseDuration(p, &tokens[1], &step.duration_ms))
			return 0;
	}
	else if (TokenIs(cmd, "seed")) {
		step.cmd = SCRIPT_SEED;
		if (!Arguments(p, tokens, count, 1, 1))
			return 0;
		if (!ParseUInt(&tokens[1], tokens[1].len, 0xFFFFFFFFu, &step.seed))
			return Fail(p, "invalid seed \"%.*s\"", &tokens[1]);
	}
	else {
		return Fail(p, "unknown command \"%.*s\"", cmd);
	}

	if (step.cmd != SCRIPT_SCENARIO && script->scenario_count == 0)
		return Fail(p, "step before the first scenario", NULL);

	if (script->step_count >= SCRIPT_STEPS_MAX)
		return Fail(p, "more than 1024 steps", NULL);

	step.text = PoolAdd(p, s, len);
	if (!step.text)
		return Fail(p, "out of room", NULL);

	if (step.cmd == SCRIPT_SCENARIO)
		script->scenario_count++;
	script->steps[script->step_count++] = step;
	return 1;
}

Script *Script_Parse(const char *text, unsigned int len, const LogButtonName *buttons, int button_count, ScriptError *err) {
	Parser p = { NULL, err, 0, buttons, button_count };

	err->line = 0;
	err->reason[0] = '\0';

	if (len > SCRIPT_TEXT_MAX) {
		Fail(&p, "suite.txt is larger than 64 KB", NULL);
		return NULL;
	}

	// Each step keeps its line and, at most, a name or condition no longer
	// than the line, each with its NUL.
	unsigned int pool_size = 2 * len + 2 * SCRIPT_STEPS_MAX;
	Script *script = script_malloc(sizeof(Script) + pool_size);
	if (!script) {
		Fail(&p, "out of memory", NULL);
		return NULL;
	}

	script->step_count = 0;
	script->scenario_count = 0;
	script->pool_size = pool_size;
	script->pool_used = 0;
	p.script = script;

	unsigned int start = 0;
	while (start < len) {
		unsigned int end = start;
		while (end < len && text[end] != '\n')
			end++;

		p.line++;

		const char *s = text + start;
		unsigned int n = end - start;
		start = end + 1;

		if (memchr(s, '\0', n)) {
			Fail(&p, "NUL byte in the line", NULL);
			free(script);
			return NULL;
		}

		// A file saved on Windows ends its lines in \r\n.
		if (n > 0 && s[n - 1] == '\r')
			n--;

		n = CommentStart(s, n);
		while (n > 0 && IsBlank(s[n - 1]))
			n--;
		while (n > 0 && IsBlank(s[0])) {
			s++;
			n--;
		}

		if (n == 0)
			continue;

		if (!ParseLine(&p, s, n)) {
			free(script);
			return NULL;
		}
	}

	if (script->scenario_count == 0) {
		p.line = 0;
		Fail(&p, "no scenarios", NULL);
		free(script);
		return NULL;
	}

	return script;
}

void Script_Free(Script *script) {
	free(script);
}

const ScriptStep *Script_Scenario(const Script *script, int index, int *step_count) {
	int found = -1;

	for (int i = 0; i < script->step_count; i++) {
		if (script->steps[i].cmd != SCRIPT_SCENARIO || ++found != index)
			continue;

		int n = 0;
		while (i + 1 + n < script->step_count && script->steps[i + 1 + n].cmd != SCRIPT_SCENARIO)
			n++;

		if (step_count)
			*step_count = n;
		return &script->steps[i];
	}

	if (step_count)
		*step_count = 0;
	return NULL;
}

int Script_WaitMatches(const ScriptStep *wait, const LogParsed *line) {
	if (wait->cmd != SCRIPT_WAIT || strcmp(line->category, wait->arg) != 0)
		return 0;

	const char *key = wait->cond;
	for (int i = 0; i < wait->cond_count; i++) {
		const char *value = key + strlen(key) + 1;
		const char *got = LogParsed_Get(line, key);

		if (!got || strcmp(got, value) != 0)
			return 0;

		key = value + strlen(value) + 1;
	}

	return 1;
}

static int IsAction(ScriptCmd cmd) {
	return cmd == SCRIPT_PRESS || cmd == SCRIPT_HOLD || cmd == SCRIPT_TAP || cmd == SCRIPT_SWIPE;
}

// The slot that remembers the last line of `category`. With `create`, takes
// the oldest one if none does yet. -1 for a category too long to keep, or
// one without a slot when not creating.
static int Slot(ScriptRun *run, const char *category, int create) {
	unsigned int len = strlen(category);
	int oldest = 0;

	if (len >= sizeof(run->last[0].category))
		return -1;

	for (int i = 0; i < SCRIPT_LAST_SLOTS; i++) {
		if (run->last[i].age && strcmp(run->last[i].category, category) == 0) {
			run->last[i].age = ++run->clock;
			return i;
		}
		if (run->last[i].age < run->last[oldest].age)
			oldest = i;
	}

	if (!create)
		return -1;

	memcpy(run->last[oldest].category, category, len + 1);
	run->last[oldest].text[0] = '\0';
	run->last[oldest].age = ++run->clock;
	return oldest;
}

// The waits from `from` up to the next action see every line from now on.
static void BuildGroup(ScriptRun *run, int from) {
	run->group_count = 0;

	for (int i = from; i < run->count && !IsAction(run->steps[i].cmd); i++) {
		if (run->steps[i].cmd != SCRIPT_WAIT)
			continue;

		Slot(run, run->steps[i].arg, 1);

		if (run->group_count < SCRIPT_GROUP_MAX) {
			run->group[run->group_count] = i;
			run->met[run->group_count] = 0;
			run->group_count++;
		}
	}
}

static int GroupIndex(const ScriptRun *run, int step) {
	for (int g = 0; g < run->group_count; g++) {
		if (run->group[g] == step)
			return g;
	}

	return -1;
}

// The screen's state as it stands, for a wait that asks for what is already
// on screen.
static int StateMeets(ScriptRun *run, const ScriptStep *wait) {
	unsigned int len = 0;
	LogParsed parsed;

	if (!run->last_state)
		return 0;

	const char *body = run->last_state(wait->arg, &len, run->ctx);
	return body && LogLine_ParseBody(body, len, &parsed) && Script_WaitMatches(wait, &parsed);
}

static void StartStep(ScriptRun *run, uint64_t now_us) {
	const ScriptStep *step = &run->steps[run->cur];

	run->start_us = now_us;
	run->phase = 0;
	run->done = 0;
	run->current_met = 0;

	if (IsAction(step->cmd)) {
		BuildGroup(run, run->cur + 1);
	}
	else if (step->cmd == SCRIPT_WAIT) {
		Slot(run, step->arg, 1);
		int g = GroupIndex(run, run->cur);
		if ((g >= 0 && run->met[g]) || StateMeets(run, step))
			run->done = 1;
	}
	else if (step->cmd == SCRIPT_SEED) {
		run->done = 1;
	}
}

static void TimeOut(ScriptRun *run, const ScriptStep *step) {
	run->result = SCRIPT_FAIL;
	run->reason = "wait timeout";
	run->fail_line = step->line;
	run->fail_last = NULL;

	int slot = Slot(run, step->arg, 1);
	if (slot < 0)
		return;

	// Nothing of the category seen while running: the screen's state, if it
	// is one, is still the last line of it.
	if (run->last[slot].text[0] == '\0' && run->last_state) {
		unsigned int len = 0;
		const char *body = run->last_state(step->arg, &len, run->ctx);

		if (body) {
			while (len > 0 && body[len - 1] == '\n')
				len--;
			if (len >= sizeof(run->last[slot].text))
				len = sizeof(run->last[slot].text) - 1;
			memcpy(run->last[slot].text, body, len);
			run->last[slot].text[len] = '\0';
		}
	}

	if (run->last[slot].text[0] != '\0')
		run->fail_last = run->last[slot].text;
}

// The frame of the current step, which may finish it.
static void Step(ScriptRun *run, uint64_t now_us, ScriptFrame *frame) {
	const ScriptStep *step = &run->steps[run->cur];
	uint64_t elapsed = now_us - run->start_us;
	uint64_t duration = (uint64_t)step->duration_ms * 1000;

	if (run->done)
		return;

	switch (step->cmd) {
		case SCRIPT_PRESS:
			if (run->phase >= 2 * step->repeat) {
				run->done = 1;
				break;
			}
			if (run->phase % 2 == 0)
				frame->buttons = step->buttons;
			run->phase++;
			break;

		case SCRIPT_HOLD:
		case SCRIPT_TAP:
			if (step->cmd == SCRIPT_TAP)
				duration = (uint64_t)SCRIPT_TAP_MS * 1000;
			if (run->phase == 0 && elapsed < duration) {
				frame->buttons = step->buttons;
				frame->touching = step->cmd == SCRIPT_TAP;
				frame->x = step->x1;
				frame->y = step->y1;
			}
			else if (run->phase == 0) {
				run->phase = 1;
			}
			else {
				run->done = 1;
			}
			break;

		case SCRIPT_SWIPE:
			if (run->phase == 0) {
				if (elapsed >= duration) {
					elapsed = duration;
					run->phase = 1;
				}
				frame->touching = 1;
				frame->x = step->x1 + (int)((int64_t)(step->x2 - step->x1) * (int64_t)elapsed / (int64_t)duration);
				frame->y = step->y1 + (int)((int64_t)(step->y2 - step->y1) * (int64_t)elapsed / (int64_t)duration);
			}
			else if (run->phase == 1) {
				run->phase = 2;
			}
			else {
				run->done = 1;
			}
			break;

		case SCRIPT_SLEEP:
			if (elapsed >= duration)
				run->done = 1;
			break;

		case SCRIPT_WAIT: {
			int g = GroupIndex(run, run->cur);
			if ((g >= 0 && run->met[g]) || run->current_met)
				run->done = 1;
			else if (elapsed >= duration)
				TimeOut(run, step);
			break;
		}

		default:
			run->done = 1;
			break;
	}
}

void ScriptRun_Init(ScriptRun *run, const Script *script, int index, ScriptRun_LastState last_state, void *ctx) {
	int count = 0;
	const ScriptStep *first = Script_Scenario(script, index, &count);

	memset(run, 0, sizeof(*run));
	run->steps = first ? first + 1 : NULL;
	run->count = count;
	run->cur = -1;
	run->done = 1;
	run->result = SCRIPT_RUNNING;
	run->last_state = last_state;
	run->ctx = ctx;

	// Every category a wait of the scenario asks for remembers its lines
	// from the start, so a timeout can show one from before its group.
	for (int i = 0; i < run->count; i++) {
		if (run->steps[i].cmd == SCRIPT_WAIT)
			Slot(run, run->steps[i].arg, 1);
	}

	BuildGroup(run, 0);
}

ScriptFrame ScriptRun_Tick(ScriptRun *run, uint64_t now_us) {
	ScriptFrame frame;
	int started = 0;

	memset(&frame, 0, sizeof(frame));

	while (run->result == SCRIPT_RUNNING) {
		if (run->done) {
			// One step starts per frame, so each gets its own step line
			// before anything it causes.
			if (started)
				break;

			if (++run->cur >= run->count) {
				run->cur = run->count;
				run->result = SCRIPT_PASS;
				break;
			}

			StartStep(run, now_us);
			frame.started = &run->steps[run->cur];
			started = 1;
		}

		Step(run, now_us, &frame);

		// A step that ended without giving this frame anything hands it to
		// the next one.
		if (!run->done || started)
			break;
	}

	if (run->result != SCRIPT_RUNNING) {
		frame.buttons = 0;
		frame.touching = 0;
		frame.result = run->result;
	}

	if (run->result == SCRIPT_FAIL) {
		frame.reason = run->reason;
		frame.line = run->fail_line;
		frame.last = run->fail_last;
	}

	return frame;
}

void ScriptRun_Observe(ScriptRun *run, const char *text, unsigned int len) {
	LogParsed parsed;

	if (run->result != SCRIPT_RUNNING || !LogLine_Parse(text, len, &parsed))
		return;

	int slot = Slot(run, parsed.category, 0);
	if (slot >= 0) {
		// The line from its category on, without the newline: a line that
		// parsed has its time up to the first space.
		const char *body = (const char *)memchr(text, ' ', len) + 1;
		unsigned int n = len - (unsigned int)(body - text);
		while (n > 0 && body[n - 1] == '\n')
			n--;
		memcpy(run->last[slot].text, body, n);
		run->last[slot].text[n] = '\0';
	}

	for (int g = 0; g < run->group_count; g++) {
		if (!run->met[g] && Script_WaitMatches(&run->steps[run->group[g]], &parsed))
			run->met[g] = 1;
	}

	if (run->cur >= 0 && run->cur < run->count && GroupIndex(run, run->cur) < 0
		&& Script_WaitMatches(&run->steps[run->cur], &parsed))
		run->current_met = 1;
}

static const char *const state_keys[] = { "next", "current", "relaunch", "pass", "fail" };
#define STATE_KEYS ((int)(sizeof(state_keys) / sizeof(state_keys[0])))

unsigned int SuiteState_Format(const SuiteState *state, char *buf, unsigned int size) {
	int n = snprintf(buf, size, "next=%d\ncurrent=%d\nrelaunch=%d\npass=%d\nfail=%d\nend\n",
		state->next, state->current, state->relaunch, state->pass, state->fail);

	return (n > 0 && (unsigned int)n < size) ? (unsigned int)n : 0;
}

int SuiteState_Parse(const char *text, unsigned int len, SuiteState *state) {
	int values[STATE_KEYS];
	unsigned int pos = 0;

	if (text == NULL || len > SUITE_STATE_MAX)
		return 0;

	for (int k = 0; k < STATE_KEYS; k++) {
		unsigned int key_len = strlen(state_keys[k]);

		if (pos + key_len + 1 > len || memcmp(text + pos, state_keys[k], key_len) != 0 || text[pos + key_len] != '=')
			return 0;
		pos += key_len + 1;

		// Only `current` can be -1.
		int negative = 0;
		if (k == 1 && pos < len && text[pos] == '-') {
			negative = 1;
			pos++;
		}

		int value = 0, digits = 0;
		while (pos < len && text[pos] >= '0' && text[pos] <= '9') {
			value = value * 10 + (text[pos] - '0');
			// No count in a suite gets near this.
			if (++digits > 5)
				return 0;
			pos++;
		}

		if (digits == 0 || pos >= len || text[pos] != '\n')
			return 0;
		pos++;

		if (negative) {
			if (value != 1)
				return 0;
			value = -1;
		}

		values[k] = value;
	}

	if (len - pos != 4 || memcmp(text + pos, "end\n", 4) != 0)
		return 0;

	if (values[2] > 1 || values[0] > SCRIPT_STEPS_MAX || values[1] > SCRIPT_STEPS_MAX)
		return 0;

	state->next = values[0];
	state->current = values[1];
	state->relaunch = values[2];
	state->pass = values[3];
	state->fail = values[4];
	return 1;
}
