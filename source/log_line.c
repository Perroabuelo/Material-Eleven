#include <stdio.h>
#include <string.h>

#include "log_line.h"

#define CUT_FIELD " cut=1"

// Room for the fields: the newline and a possible " cut=1" are always kept
// free, so a cut can be marked whatever came before it.
#define FIELDS_END (LOG_LINE_MAX - 1 - (sizeof(CUT_FIELD) - 1))

static void Append(LogLine *line, const char *s, unsigned int n) {
	memcpy(line->text + line->len, s, n);
	line->len += n;
}

void LogLine_Begin(LogLine *line, uint64_t time_us, const char *category) {
	unsigned long long ms = time_us / 1000;
	int n = snprintf(line->text, FIELDS_END + 1, "%llu.%03llu ", ms / 1000, ms % 1000);

	line->len = (n > 0 && n <= (int)FIELDS_END) ? (unsigned int)n : 0;
	line->body = line->len;
	line->time_us = time_us;
	line->cut = 0;
	line->finished = 0;

	unsigned int cat_len = strlen(category);
	if (line->len + cat_len > FIELDS_END)
		cat_len = FIELDS_END - line->len;

	Append(line, category, cat_len);
}

static int NeedsQuotes(const char *value) {
	if (value[0] == '\0')
		return 1;

	for (const char *p = value; *p; p++) {
		if (*p == ' ' || *p == '"' || *p == '\\' || *p == '\n')
			return 1;
	}

	return 0;
}

// Bytes in the UTF-8 sequence that starts with `c`. A stray continuation
// byte or an invalid lead counts as one, so bad input still moves forward.
static unsigned int SequenceLength(unsigned char c) {
	if (c >= 0xC0 && c <= 0xDF)
		return 2;
	if (c >= 0xE0 && c <= 0xEF)
		return 3;
	if (c >= 0xF0 && c <= 0xF7)
		return 4;
	return 1;
}

void LogLine_Str(LogLine *line, const char *key, const char *value) {
	if (line->cut || line->finished)
		return;

	if (value == NULL)
		value = "";

	int quoted = NeedsQuotes(value);
	unsigned int key_len = strlen(key);

	// " key=" plus both quotes: without that much room the field cannot even
	// start.
	if (line->len + 1 + key_len + 1 + (quoted ? 2 : 0) > FIELDS_END) {
		line->cut = 1;
		return;
	}

	Append(line, " ", 1);
	Append(line, key, key_len);
	Append(line, "=", 1);
	if (quoted)
		Append(line, "\"", 1);

	unsigned int limit = FIELDS_END - (quoted ? 1 : 0);
	const unsigned char *p = (const unsigned char *)value;

	while (*p) {
		if (*p == '"' || *p == '\\' || *p == '\n') {
			if (line->len + 2 > limit) {
				line->cut = 1;
				break;
			}

			char esc[2] = { '\\', (*p == '\n') ? 'n' : (char)*p };
			Append(line, esc, 2);
			p++;
			continue;
		}

		unsigned int n = SequenceLength(*p);
		// A sequence cut short by the end of the string goes as far as it
		// goes.
		for (unsigned int i = 1; i < n; i++) {
			if (p[i] == '\0') {
				n = i;
				break;
			}
		}

		if (line->len + n > limit) {
			line->cut = 1;
			break;
		}

		Append(line, (const char *)p, n);
		p += n;
	}

	if (quoted)
		Append(line, "\"", 1);
}

void LogLine_Int(LogLine *line, const char *key, long long value) {
	char buf[24];
	snprintf(buf, sizeof(buf), "%lld", value);
	LogLine_Str(line, key, buf);
}

void LogLine_Buttons(LogLine *line, const char *key, unsigned int mask, const LogButtonName *table, int count) {
	// Twelve names of up to eight letters and their '+' fit with room to
	// spare; a longer table is cut like any other value.
	char buf[LOG_LINE_MAX];
	unsigned int len = 0;

	for (int i = 0; i < count; i++) {
		if (!(mask & table[i].mask))
			continue;

		unsigned int n = strlen(table[i].name);
		if (len + (len ? 1 : 0) + n + 1 > sizeof(buf))
			break;

		if (len)
			buf[len++] = '+';
		memcpy(buf + len, table[i].name, n);
		len += n;
	}

	if (len == 0)
		return;

	buf[len] = '\0';
	LogLine_Str(line, key, buf);
}

unsigned int LogLine_Finish(LogLine *line) {
	if (!line->finished) {
		if (line->cut)
			Append(line, CUT_FIELD, sizeof(CUT_FIELD) - 1);
		Append(line, "\n", 1);
		line->finished = 1;
	}

	return line->len;
}

void LogRing_Init(LogRing *ring, void *mem, unsigned int size) {
	ring->data = mem;
	ring->size = mem ? size : 0;
	ring->head = 0;
	ring->used = 0;
	ring->pending = 0;
	ring->accepted = 0;
	ring->dropped = 0;
}

// Copies `len` bytes in after the unread ones, if they fit.
static int RingWrite(LogRing *ring, const char *src, unsigned int len) {
	if (len > ring->size - ring->used)
		return 0;

	unsigned int tail = (ring->head + ring->used) % ring->size;
	unsigned int first = ring->size - tail;
	if (first > len)
		first = len;

	memcpy(ring->data + tail, src, first);
	memcpy(ring->data, src + first, len - first);
	ring->used += len;
	return 1;
}

int LogRing_Push(LogRing *ring, LogLine *line) {
	unsigned int len = LogLine_Finish(line);

	if (ring->size == 0) {
		ring->pending++;
		ring->dropped++;
		return 0;
	}

	if (ring->pending > 0) {
		LogLine note;
		LogLine_Begin(&note, line->time_us, "log");
		LogLine_Str(&note, "event", "dropped");
		LogLine_Int(&note, "count", ring->pending);
		unsigned int note_len = LogLine_Finish(&note);

		if (!RingWrite(ring, note.text, note_len)) {
			ring->pending++;
			ring->dropped++;
			return 0;
		}

		ring->pending = 0;
	}

	if (!RingWrite(ring, line->text, len)) {
		ring->pending++;
		ring->dropped++;
		return 0;
	}

	ring->accepted++;
	return 1;
}

unsigned int LogRing_Read(LogRing *ring, char *dst, unsigned int max) {
	unsigned int len = ring->used < max ? ring->used : max;
	if (len == 0)
		return 0;

	unsigned int first = ring->size - ring->head;
	if (first > len)
		first = len;

	memcpy(dst, ring->data + ring->head, first);
	memcpy(dst + first, ring->data, len - first);
	ring->head = (ring->head + len) % ring->size;
	ring->used -= len;
	return len;
}

int LogState_Changed(LogState *state, LogLine *line) {
	unsigned int end = LogLine_Finish(line);
	unsigned int len = end - line->body;

	if (state->len == len && memcmp(state->body, line->text + line->body, len) == 0)
		return 0;

	memcpy(state->body, line->text + line->body, len);
	state->len = len;
	return 1;
}

void LogState_Reset(LogState *state) {
	state->len = 0;
}
