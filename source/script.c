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
