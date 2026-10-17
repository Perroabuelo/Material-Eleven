#ifndef _ELEVENMPV_SCRIPT_H_
#define _ELEVENMPV_SCRIPT_H_

#include <stdint.h>

#include "log_line.h"

// The scenario suites of the dev/console-scripts spec: suite.txt is parsed
// here into steps, which the console side (source/suite.c) runs one frame at
// a time.
//
// Pure logic: no vita2d and no SCE headers, so it is tested on a PC
// (tests/test_script.c). The button masks come from the caller, in the same
// table the log names them with.

// Largest suite.txt that is read.
#define SCRIPT_TEXT_MAX  (64 * 1024)
// Most steps in a suite, each `scenario` line included.
#define SCRIPT_STEPS_MAX 1024
// Longest duration a step may ask for.
#define SCRIPT_DURATION_MAX_MS (3600 * 1000)
// Most times a press may repeat.
#define SCRIPT_REPEAT_MAX 1000

// Screen pixels, the same the touch lines of the log use.
#define SCRIPT_SCREEN_W 960
#define SCRIPT_SCREEN_H 544

// A wait without timeout= gives up after this long.
#define SCRIPT_WAIT_DEFAULT_MS 5000

typedef enum {
	SCRIPT_SCENARIO = 0,
	SCRIPT_PRESS,
	SCRIPT_HOLD,
	SCRIPT_TAP,
	SCRIPT_SWIPE,
	SCRIPT_WAIT,
	SCRIPT_SLEEP,
	SCRIPT_SEED
} ScriptCmd;

// One line of suite.txt. Every pointer points into the Script that holds
// the step, NUL-terminated.
typedef struct {
	ScriptCmd cmd;
	int line;               // in suite.txt, from 1
	const char *text;       // the line without its comment or the blanks around it
	const char *arg;        // scenario: its name; wait: the category
	const char *cond;       // wait: cond_count "key\0value\0" pairs in a row
	int cond_count;
	unsigned int buttons;   // press, hold
	int repeat;             // press: the xN, 1 without it
	uint32_t duration_ms;   // hold, swipe, sleep; wait: its timeout
	int16_t x1, y1, x2, y2; // tap: x1, y1; swipe: from x1, y1 to x2, y2
	uint32_t seed;
} ScriptStep;

// A parsed suite, in one block: the steps, then the texts they point to.
typedef struct {
	int step_count;
	int scenario_count;
	ScriptStep steps[SCRIPT_STEPS_MAX];
	unsigned int pool_size;
	unsigned int pool_used;
	char pool[];
} Script;

// Where and why a suite could not be parsed. `line` is 0 when the fault is
// in the whole file rather than in one line: too big, no scenarios, or no
// memory.
typedef struct {
	int line;
	char reason[160];
} ScriptError;

// Parses the `len` bytes of `text` (never reading past them), with the
// button names of `buttons`. Returns the suite, to free with Script_Free, or
// NULL with the first error in `err`.
Script *Script_Parse(const char *text, unsigned int len, const LogButtonName *buttons, int button_count, ScriptError *err);

void Script_Free(Script *script);

// The `scenario` step that starts scenario `index` (from 0), and how many
// steps follow it up to the next one. NULL when there is no such scenario.
const ScriptStep *Script_Scenario(const Script *script, int index, int *step_count);

// 1 when `line` is of the wait's category and has every one of its fields
// with exactly that value. Other fields of the line do not matter.
int Script_WaitMatches(const ScriptStep *wait, const LogParsed *line);

// How long a tap keeps the finger down: what a real tap lasts, short of a
// long press.
#define SCRIPT_TAP_MS 100

// Most waits between two actions that see the lines since the first of them
// (see ScriptRun_Observe). Later ones only see lines from when they start.
#define SCRIPT_GROUP_MAX 32
// How many categories remember their last line, for the reason of a wait
// that times out.
#define SCRIPT_LAST_SLOTS 4

typedef enum {
	SCRIPT_RUNNING = 0,
	SCRIPT_PASS,
	SCRIPT_FAIL
} ScriptResult;

// The last state line of a screen, without its time: what the console side
// keeps to drop repeated states (DebugLog_LastState). NULL when there is
// none for `category`, or it is not a screen's state.
typedef const char *(*ScriptRun_LastState)(const char *category, unsigned int *len, void *ctx);

// What one frame gets from the scenario.
typedef struct {
	unsigned int buttons;           // held this frame
	int touching;                   // a finger on the front panel this frame
	int x, y;                       // where, in screen pixels
	const ScriptStep *started;      // the step that started this frame, or NULL
	ScriptResult result;            // not RUNNING: the scenario ended this frame
	// With SCRIPT_FAIL:
	const char *reason;
	int line;                       // the failing step's line in suite.txt
	const char *last;               // the last line of the wait's category, without its time, or NULL
} ScriptFrame;

// One scenario being run. Lives wherever the caller puts it; no memory of
// its own.
typedef struct {
	const ScriptStep *steps;        // the scenario's steps, after its `scenario` line
	int count;
	int cur;                        // the current step, -1 before the first
	int done;                       // the current step has finished
	int phase;
	uint64_t start_us;              // when the current step started
	ScriptResult result;
	int fail_line;
	const char *reason;
	// The waits that see the lines since the last action, and which of them
	// a line already met.
	int group[SCRIPT_GROUP_MAX];
	unsigned char met[SCRIPT_GROUP_MAX];
	int group_count;
	int current_met;                // a line met the current wait, when it is not in the group
	struct {
		char category[32];
		char text[LOG_LINE_MAX];    // without the time; empty: none seen yet
		unsigned int age;
	} last[SCRIPT_LAST_SLOTS];
	unsigned int clock;             // ages the slots
	const char *fail_last;
	ScriptRun_LastState last_state;
	void *ctx;
} ScriptRun;

// Gets scenario `index` of `script` ready to run from its first step. The
// lines observed from now on count for the waits before its first action.
// `last_state` may be NULL.
void ScriptRun_Init(ScriptRun *run, const Script *script, int index, ScriptRun_LastState last_state, void *ctx);

// Moves the scenario on by one frame at `now_us` (any clock in microseconds
// that only goes forward) and says what the frame gets:
//
//   press  the buttons for one frame and none for the next, per repeat;
//   hold   the buttons until the duration is over, then one frame without;
//   tap    the finger for SCRIPT_TAP_MS, then one frame without;
//   swipe  the finger along the line for the duration, then at the end
//          point once more, then one frame without;
//   sleep  nothing until the duration is over;
//   seed   nothing; the caller applies the seed of `started`;
//   wait   nothing until a line meets it or it times out.
//
// At most one step starts per frame. Once the result is not RUNNING, every
// later call returns that result again with nothing held.
ScriptFrame ScriptRun_Tick(ScriptRun *run, uint64_t now_us);

// Hands the run a line as the log wrote it (time included, newline or
// not). It meets the waits of the current group: the ones after the last
// action up to the next, or after the start of the scenario if there was
// no action yet.
void ScriptRun_Observe(ScriptRun *run, const char *text, unsigned int len);

#endif
