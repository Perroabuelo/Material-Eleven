#ifndef _ELEVENMPV_DEBUG_LOG_H_
#define _ELEVENMPV_DEBUG_LOG_H_

#include "log_line.h"
#include "ui_screen.h"

// The developer log: off unless ux0:data/ElevenMPV/debug exists when the app
// starts, then one line per event in ux0:data/ElevenMPV/debug.log (the line
// format is in log_line.h, the events in the dev/debug-log spec).
//
// Every logging point is wrapped the same way, so that with the log off
// nothing is formatted and the cost is reading one flag:
//
//     if (DebugLog_On()) {
//         LogLine line;
//         DebugLog_Begin(&line, "track");
//         LogLine_Str(&line, "event", "open");
//         DebugLog_Commit(&line);
//     }
//
// Commit only copies the line into a ring under a lock; a low priority
// thread writes the ring to the card about every quarter of a second. Safe to
// call from any thread once DebugLog_Init has run.

#define DEBUG_LOG_DIR    "ux0:data/ElevenMPV"
#define DEBUG_LOG_MARKER DEBUG_LOG_DIR "/debug"
#define DEBUG_LOG_FILE   DEBUG_LOG_DIR "/debug.log"
#define DEBUG_LOG_PREV   DEBUG_LOG_DIR "/debug.prev.log"

// One per screen with a state line: each remembers its last line so that an
// unchanged state is not written again.
typedef enum {
	DEBUG_STATE_FOLDERS = 0,
	DEBUG_STATE_LIBRARY,
	DEBUG_STATE_SETTINGS,
	DEBUG_STATE_NOWPLAYING,
	DEBUG_STATE_COUNT
} DebugLog_StateChannel;

typedef enum {
	DEBUG_LOG_OFF = 0,     // no marker
	DEBUG_LOG_ACTIVE,
	DEBUG_LOG_FAILED       // marker found, but the log could not start
} DebugLog_Mode;

typedef struct {
	DebugLog_Mode mode;
	const char *reason;      // why it failed, for the overlay; NULL otherwise
	unsigned int lines;      // lines taken this session
	unsigned int dropped;    // lines dropped this session
} DebugLog_Status;

extern int debug_log_on;

static inline int DebugLog_On(void) {
	return debug_log_on;
}

// Looks for the marker and, if it is there, moves debug.log to
// debug.prev.log, reserves the ring, and starts the writer thread. Any step
// that fails undoes the ones before it and leaves the log off, with the
// reason in DebugLog_GetStatus. Call once, after the data folder exists.
//
// With `relaunch` (a suite relaunched the app between two scenarios), the
// log is not moved: the new process goes on at the end of debug.log.
void DebugLog_Init(int relaunch);

// Stops the writer, writes what is left and frees everything. Nothing to do
// when the log never started.
void DebugLog_Shutdown(void);

// Writes every line taken so far to the card before returning, for a
// process about to end or be replaced. The log goes on as before.
void DebugLog_Sync(void);

// Starts a line with the time since the app started.
void DebugLog_Begin(LogLine *line, const char *category);

// Hands a line to the writer; dropped and counted if the ring is full.
void DebugLog_Commit(LogLine *line);

// Like DebugLog_Commit, but only if the line differs from the last one this
// channel wrote.
void DebugLog_CommitState(LogLine *line, DebugLog_StateChannel channel);

// Logs "screen to=<name> heap_kb=<n>" and forgets every channel's last
// state, so the new screen's first state line is always written.
void DebugLog_Screen(UI_Screen to);

// Appends key=A+B+... with the log's names for the buttons in `mask`
// (CROSS, CIRCLE, ..., SELECT). Nothing when none of them is in the mask.
void DebugLog_Buttons(LogLine *line, const char *key, unsigned int mask);

// Logs "input down=... up=..." for the buttons pressed and released this
// frame. Only the buttons the log names count: a frame where nothing else
// changed writes nothing.
void DebugLog_Input(unsigned int down, unsigned int up);

// The last state line of the screen named `category` (folders, library,
// settings, nowplaying), from the category on, without the time and with
// its newline, and its length in `len`. NULL for any other category, when
// that screen has written nothing since the last screen line, or with the
// log off. Main thread only.
const char *DebugLog_LastState(const char *category, unsigned int *len);

// The names the log gives the buttons, in its order.
const LogButtonName *DebugLog_ButtonNames(int *count);

// The mask of every button the log names.
unsigned int DebugLog_NamedButtons(void);

DebugLog_Status DebugLog_GetStatus(void);

#endif
