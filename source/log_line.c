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
