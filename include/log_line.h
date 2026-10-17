#ifndef _ELEVENMPV_LOG_LINE_H_
#define _ELEVENMPV_LOG_LINE_H_

#include <stdint.h>

// The lines of the developer log (source/debug_log.c): one event per line,
//
//     <seconds>.<millis> <category> <key>=<value> ...
//
// Pure logic: no vita2d and no SCE headers, so it is tested on a PC
// (tests/test_log_line.c). The console side only supplies the clock, the
// lock and the thread that writes to the card.

// Longest line, newline included.
#define LOG_LINE_MAX 512

// A line being built, on the stack of whoever logs. Keys and categories are
// constants from the code and go in as they are; values are quoted and
// escaped when they need it.
typedef struct {
	char text[LOG_LINE_MAX];
	unsigned int len;       // bytes in text, without the final newline
	unsigned int body;      // where the category starts (after the time)
	uint64_t time_us;       // time given to LogLine_Begin
	int cut;                // a value did not fit: no more fields go in
	int finished;           // LogLine_Finish already added the newline
} LogLine;

// A button mask and the name the log gives it.
typedef struct {
	unsigned int mask;
	const char *name;
} LogButtonName;

// Starts a line: the time since the app started, with exactly three
// decimals, and the category.
void LogLine_Begin(LogLine *line, uint64_t time_us, const char *category);

// Appends key=value. The value goes as it is unless it is empty or has a
// space, a double quote, a backslash or a newline; then it goes between
// double quotes, with \" \\ and \n for those characters.
//
// When the field does not fit, the value is cut at a whole UTF-8 character,
// its quotes are closed, and the line will end in cut=1; later fields are
// dropped.
void LogLine_Str(LogLine *line, const char *key, const char *value);
void LogLine_Int(LogLine *line, const char *key, long long value);

// Appends key=A+B+..., naming the buttons of `mask` in the order of `table`.
// Bits that are not in the table are ignored, and nothing is appended when
// no button of the table is in the mask.
void LogLine_Buttons(LogLine *line, const char *key, unsigned int mask, const LogButtonName *table, int count);

// Ends the line with cut=1 if a value was cut, and the newline. Returns the
// length of the finished line (at most LOG_LINE_MAX). Calling it again
// returns the same length.
unsigned int LogLine_Finish(LogLine *line);

#endif
