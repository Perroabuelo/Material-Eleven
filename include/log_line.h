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

// A ring of bytes over memory the caller owns, holding whole lines until the
// writer thread takes them. Not thread-safe: the console side holds its lock
// around every call.
typedef struct {
	char *data;
	unsigned int size;
	unsigned int head;      // where the oldest unread byte is
	unsigned int used;      // unread bytes
	unsigned int pending;   // lines dropped since the last "dropped" line
	unsigned int accepted;  // lines taken this session
	unsigned int dropped;   // lines dropped this session
} LogRing;

// `size` may be 0, or too small for any line: then every push is dropped and
// counted, and nothing is written to `mem`.
void LogRing_Init(LogRing *ring, void *mem, unsigned int size);

// Takes a whole line (finishing it if needed), or drops it whole and counts
// it when it does not fit. After a drop, the next push first tries to add
// "<time> log event=dropped count=N", with the time of the new line; only if
// that fits is the count cleared and the new line tried. Returns 1 when the
// line was taken.
int LogRing_Push(LogRing *ring, LogLine *line);

// Moves up to `max` of the oldest bytes to `dst` and frees them. Returns how
// many it moved. Reads do not stop at line ends: the bytes come out in the
// order they went in, so writing them one block after another rebuilds the
// lines.
unsigned int LogRing_Read(LogRing *ring, char *dst, unsigned int max);

// The last state line written by one screen, so that the same state is not
// written frame after frame. Lines are compared without their time.
typedef struct {
	char body[LOG_LINE_MAX];
	unsigned int len;  // 0: nothing written yet, the next line always goes
} LogState;

// 1 when `line` differs from the last one remembered by `state`, which then
// remembers this one; 0 when it is the same.
int LogState_Changed(LogState *state, LogLine *line);

// Forgets the last line, so the next one is written whatever it says.
void LogState_Reset(LogState *state);

#endif
