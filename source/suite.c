#include <psp2/appmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/kernel/processmgr.h>

#include "debug_log.h"
#include "menu_audioplayer.h"
#include "script.h"
#include "suite.h"

#define CONFIG_FILE  "ux0:data/ElevenMPV/config.cfg"
#define LASTDIR_FILE "ux0:data/ElevenMPV/lastdir.txt"

#define STATE_FILE     SUITE_DIR "/state"
#define STATE_TMP_FILE SUITE_DIR "/state.tmp"

// How long the summary stays up before the app closes.
#define SUMMARY_US (3 * 1000 * 1000)

// What was there before the suite, and what is put back after each
// scenario. A "no" marker means the file did not exist: putting it back is
// removing it.
typedef struct {
	const char *path;
	const char *copy;
	const char *missing;
} Saved;

static const Saved saved[] = {
	{ CONFIG_FILE,  SUITE_DIR "/config.cfg",  SUITE_DIR "/noconfig"  },
	{ LASTDIR_FILE, SUITE_DIR "/lastdir.txt", SUITE_DIR "/nolastdir" },
};
#define SAVED_COUNT ((int)(sizeof(saved) / sizeof(saved[0])))

int suite_running = 0;

static Script *script = NULL;
static ScriptRun run;
static SuiteState state;
// What the scenario gives the frame being read.
static ScriptFrame frame;
// The scenario running, for the banner.
static char banner[96];
// After the last scenario: the summary is on screen until then, and the app
// closes.
static int ending = 0;
static SceUInt64 ending_at = 0;

static int Exists(const char *path) {
	SceIoStat stat;
	return sceIoGetstat(path, &stat) >= 0;
}

// Reads up to `max` bytes. Returns how many, or -1 if the file cannot be
// opened.
static int ReadAll(const char *path, char *buf, unsigned int max) {
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return -1;

	unsigned int len = 0;
	while (len < max) {
		int n = sceIoRead(fd, buf + len, max - len);
		if (n <= 0)
			break;
		len += (unsigned int)n;
	}

	sceIoClose(fd);
	return (int)len;
}

static int WriteAll(const char *path, const char *buf, unsigned int len) {
	SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return 0;

	unsigned int done = 0;
	while (done < len) {
		int n = sceIoWrite(fd, buf + done, len - done);
		if (n <= 0)
			break;
		done += (unsigned int)n;
	}

	sceIoClose(fd);
	return done == len;
}

static int CopyFile(const char *from, const char *to) {
	static char block[4 * 1024];

	SceUID in = sceIoOpen(from, SCE_O_RDONLY, 0);
	if (in < 0)
		return 0;

	SceUID out = sceIoOpen(to, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (out < 0) {
		sceIoClose(in);
		return 0;
	}

	int ok = 1, n;
	while ((n = sceIoRead(in, block, sizeof(block))) > 0) {
		if (sceIoWrite(out, block, n) != n) {
			ok = 0;
			break;
		}
	}
	if (n < 0)
		ok = 0;

	sceIoClose(out);
	sceIoClose(in);
	return ok;
}

// The state goes to a temporary file first, so a process closed halfway
// through writing it still leaves one whole copy: the old one, or the new
// one under its temporary name.
static int ReadState(SuiteState *out) {
	char buf[SUITE_STATE_MAX];

	int len = ReadAll(STATE_FILE, buf, sizeof(buf));
	if (len >= 0 && SuiteState_Parse(buf, (unsigned int)len, out))
		return 1;

	len = ReadAll(STATE_TMP_FILE, buf, sizeof(buf));
	return len >= 0 && SuiteState_Parse(buf, (unsigned int)len, out);
}

static int WriteState(const SuiteState *in) {
	char buf[SUITE_STATE_MAX];

	unsigned int len = SuiteState_Format(in, buf, sizeof(buf));
	if (len == 0 || !WriteAll(STATE_TMP_FILE, buf, len))
		return 0;

	sceIoRemove(STATE_FILE);
	return sceIoRename(STATE_TMP_FILE, STATE_FILE) >= 0;
}

static int Backup(void) {
	for (int i = 0; i < SAVED_COUNT; i++) {
		if (Exists(saved[i].path)) {
			if (!CopyFile(saved[i].path, saved[i].copy))
				return 0;
		}
		else if (!WriteAll(saved[i].missing, "", 0)) {
			return 0;
		}
	}

	return 1;
}

static void Restore(void) {
	for (int i = 0; i < SAVED_COUNT; i++) {
		if (Exists(saved[i].missing))
			sceIoRemove(saved[i].path);
		else if (Exists(saved[i].copy))
			CopyFile(saved[i].copy, saved[i].path);
	}
}

static void RemoveDir(void) {
	for (int i = 0; i < SAVED_COUNT; i++) {
		sceIoRemove(saved[i].copy);
		sceIoRemove(saved[i].missing);
	}

	sceIoRemove(STATE_FILE);
	sceIoRemove(STATE_TMP_FILE);
	sceIoRmdir(SUITE_DIR);
}

// The suite is over, whichever way: the card is left as before it, and
// suite.txt as suite.done.txt so it does not run again.
static void Finish(void) {
	if (Exists(SUITE_DIR)) {
		Restore();
		RemoveDir();
	}

	if (Exists(SUITE_FILE)) {
		sceIoRemove(SUITE_DONE_FILE);
		sceIoRename(SUITE_FILE, SUITE_DONE_FILE);
	}

	suite_running = 0;
	memset(&frame, 0, sizeof(frame));
	Script_Free(script);
	script = NULL;
}

static void Log(LogLine *line, const char *event) {
	DebugLog_Begin(line, "script");
	LogLine_Str(line, "event", event);
}

static void LogError(const ScriptError *err) {
	LogLine line;
	Log(&line, "error");
	LogLine_Int(&line, "line", err->line);
	LogLine_Str(&line, "reason", err->reason);
	DebugLog_Commit(&line);
}

static void LogAbort(const char *reason) {
	LogLine line;
	Log(&line, "abort");
	LogLine_Str(&line, "reason", reason);
	DebugLog_Commit(&line);
}

static void LogEnd(int interrupted) {
	LogLine line;
	Log(&line, "end");
	LogLine_Int(&line, "pass", state.pass);
	LogLine_Int(&line, "fail", state.fail);
	LogLine_Int(&line, "interrupted", interrupted);
	DebugLog_Commit(&line);
}

// suite.txt, parsed. NULL when it is not there or has an error, which goes
// to `err` with line -1 when the file could not even be read.
static Script *Load(ScriptError *err) {
	SceIoStat stat;
	int button_count = 0;
	const LogButtonName *buttons = DebugLog_ButtonNames(&button_count);

	err->line = -1;
	err->reason[0] = '\0';

	if (sceIoGetstat(SUITE_FILE, &stat) < 0)
		return NULL;

	// One byte more than the limit is enough for the parser to say it is too
	// big, without reading the rest.
	unsigned int size = stat.st_size > SCRIPT_TEXT_MAX ? SCRIPT_TEXT_MAX + 1 : (unsigned int)stat.st_size;
	char *text = malloc(size ? size : 1);
	if (!text) {
		err->line = 0;
		strcpy(err->reason, "out of memory");
		return NULL;
	}

	int len = ReadAll(SUITE_FILE, text, size);
	Script *parsed = NULL;
	if (len < 0) {
		err->line = 0;
		strcpy(err->reason, "could not read suite.txt");
	}
	else {
		parsed = Script_Parse(text, (unsigned int)len, buttons, button_count, err);
	}

	free(text);
	return parsed;
}

// A process that ended in the middle of a scenario, closed with PS or by a
// crash: its scenario is recorded as interrupted and the suite ends there.
static void Interrupted(void) {
	ScriptError err;
	Script *parsed = Load(&err);

	if (state.current >= 0) {
		LogLine line;
		Log(&line, "result");
		const ScriptStep *scenario = parsed ? Script_Scenario(parsed, state.current, NULL) : NULL;
		if (scenario)
			LogLine_Str(&line, "name", scenario->arg);
		LogLine_Str(&line, "result", "interrupted");
		DebugLog_Commit(&line);
	}

	Script_Free(parsed);
	LogEnd(state.current >= 0);
	Finish();
}

static const char *LastState(const char *category, unsigned int *len, void *ctx) {
	return DebugLog_LastState(category, len);
}

int Suite_IsRelaunch(void) {
	SuiteState read;
	return ReadState(&read) && read.relaunch;
}

void Suite_Init(int relaunch) {
	ScriptError err;

	if (!DebugLog_On())
		return;

	int has_state = ReadState(&state);

	if (has_state && !relaunch) {
		Interrupted();
		return;
	}

	script = Load(&err);
	if (!script) {
		if (err.line >= 0)
			LogError(&err);
		else if (has_state)
			LogAbort("suite.txt missing");
		else
			return;

		Finish();
		return;
	}

	if (!has_state) {
		state.next = 0;
		state.current = -1;
		state.relaunch = 0;
		state.pass = 0;
		state.fail = 0;

		sceIoMkdir(SUITE_DIR, 0777);
		if (!Backup() || !WriteState(&state)) {
			LogAbort("could not save the suite state");
			Finish();
			return;
		}

		LogLine line;
		Log(&line, "suite");
		LogLine_Int(&line, "scenarios", script->scenario_count);
		DebugLog_Commit(&line);
	}

	// suite.txt changed under a running suite.
	if (state.next >= script->scenario_count) {
		LogEnd(0);
		Finish();
		return;
	}

	// Before Config_Load: the scenario starts from the settings before the
	// suite, whatever the one before it changed.
	Restore();

	state.current = state.next;
	state.relaunch = 0;
	if (!WriteState(&state)) {
		LogAbort("could not save the suite state");
		Finish();
		return;
	}

	const ScriptStep *scenario = Script_Scenario(script, state.current, NULL);
	LogLine line;
	Log(&line, "scenario");
	LogLine_Str(&line, "name", scenario->arg);
	LogLine_Int(&line, "index", state.current + 1);
	LogLine_Int(&line, "total", script->scenario_count);
	DebugLog_Commit(&line);

	snprintf(banner, sizeof(banner), "%d/%d  %s", state.current + 1, script->scenario_count, scenario->arg);

	ScriptRun_Init(&run, script, state.current, LastState, NULL);
	suite_running = 1;
}

void Suite_Observe(const char *text, unsigned int len) {
	if (suite_running)
		ScriptRun_Observe(&run, text, len);
}

static void LogStep(const ScriptStep *step) {
	LogLine line;
	Log(&line, "step");
	LogLine_Int(&line, "line", step->line);
	LogLine_Str(&line, "cmd", step->text);
	DebugLog_Commit(&line);
}

static void LogResult(const ScriptFrame *end) {
	const ScriptStep *scenario = Script_Scenario(script, state.current, NULL);
	LogLine line;

	Log(&line, "result");
	LogLine_Str(&line, "name", scenario->arg);
	LogLine_Str(&line, "result", end->result == SCRIPT_PASS ? "pass" : "fail");
	if (end->result == SCRIPT_FAIL) {
		LogLine_Str(&line, "reason", end->reason);
		LogLine_Int(&line, "line", end->line);
		if (end->last)
			LogLine_Str(&line, "last", end->last);
	}
	DebugLog_Commit(&line);
}

// The next scenario runs in a new process, so nothing this one left in
// memory reaches it. Only returns if the relaunch failed.
static void Relaunch(void) {
	char reason[32];

	state.next = state.current + 1;
	state.relaunch = 1;
	if (!WriteState(&state)) {
		LogAbort("could not save the suite state");
		Finish();
		return;
	}

	DebugLog_Sync();
	int ret = sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);

	snprintf(reason, sizeof(reason), "relaunch 0x%08X", (unsigned int)ret);
	LogAbort(reason);
	Finish();
}

static void EndScenario(const ScriptFrame *end) {
	LogResult(end);

	if (end->result == SCRIPT_PASS)
		state.pass++;
	else
		state.fail++;

	// Whatever the scenario changed goes back before anything else, the same
	// way whether the suite goes on or not.
	Restore();

	if (state.current + 1 < script->scenario_count) {
		Relaunch();
		return;
	}

	LogEnd(0);
	Finish();

	snprintf(banner, sizeof(banner), "Suite terminada: %d pass, %d fail  -  saliendo", state.pass, state.fail);
	ending = 1;
	ending_at = sceKernelGetProcessTimeWide() + SUMMARY_US;
}

void Suite_Ending(unsigned int pressed) {
	if (!ending)
		return;

	if (!pressed && sceKernelGetProcessTimeWide() < ending_at)
		return;

	DebugLog_Sync();
	sceKernelExitProcess(0);
}

const char *Suite_Banner(int *summary) {
	*summary = ending;

	if (ending)
		return banner;
	if (suite_running)
		return banner;
	return NULL;
}

unsigned int Suite_Tick(void) {
	if (!suite_running)
		return 0;

	frame = ScriptRun_Tick(&run, sceKernelGetProcessTimeWide());

	if (frame.started) {
		LogStep(frame.started);
		if (frame.started->cmd == SCRIPT_SEED)
			Music_SetSeed(frame.started->seed);
	}

	if (frame.result != SCRIPT_RUNNING) {
		ScriptFrame end = frame;
		EndScenario(&end);
		return 0;
	}

	return frame.buttons;
}

int Suite_Touch(int *x, int *y) {
	if (!suite_running || !frame.touching)
		return 0;

	*x = frame.x;
	*y = frame.y;
	return 1;
}

void Suite_Abort(void) {
	if (!suite_running)
		return;

	LogAbort("user");
	Finish();
}
