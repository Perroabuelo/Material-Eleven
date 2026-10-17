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

#endif
