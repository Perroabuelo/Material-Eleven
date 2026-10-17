#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug_log.h"
#include "suite.h"

#ifndef APP_VERSION
#define APP_VERSION "?"
#endif

// About 650 lines of ~100 bytes: over half a minute of holding Down, while
// the writer empties it every quarter of a second.
#define DEBUG_LOG_RING_SIZE (64 * 1024)

#define DEBUG_LOG_FLUSH_US    (250 * 1000)
#define DEBUG_LOG_BLOCK_SIZE  (4 * 1024)
#define DEBUG_LOG_STACK_SIZE  (16 * 1024)
// Below the main thread (0xA0) and the audio threads (0x40): the writer only
// runs when nobody who draws or plays has anything to do.
#define DEBUG_LOG_PRIORITY    0xB0

// One block for the ring and the state channels, so that starting the log
// is one malloc and stopping it one free.
typedef struct {
	LogState states[DEBUG_STATE_COUNT];
	char ring[DEBUG_LOG_RING_SIZE];
} DebugLog_Memory;

int debug_log_on = 0;

static DebugLog_Mode mode = DEBUG_LOG_OFF;
static const char *fail_reason = NULL;
static DebugLog_Memory *memory = NULL;
static LogRing ring;
static SceKernelLwMutexWork lock;
static SceUID writer = -1;
static volatile int writer_quit = 0;
// The suite only watches lines from here: it is not thread-safe, and every
// event it waits for is logged from the main thread.
static SceUID main_thread = -1;

// The order the buttons are named in a line.
static const LogButtonName button_names[] = {
	{ SCE_CTRL_CROSS,    "CROSS"    },
	{ SCE_CTRL_CIRCLE,   "CIRCLE"   },
	{ SCE_CTRL_SQUARE,   "SQUARE"   },
	{ SCE_CTRL_TRIANGLE, "TRIANGLE" },
	{ SCE_CTRL_UP,       "UP"       },
	{ SCE_CTRL_DOWN,     "DOWN"     },
	{ SCE_CTRL_LEFT,     "LEFT"     },
	{ SCE_CTRL_RIGHT,    "RIGHT"    },
	{ SCE_CTRL_LTRIGGER, "L"        },
	{ SCE_CTRL_RTRIGGER, "R"        },
	{ SCE_CTRL_START,    "START"    },
	{ SCE_CTRL_SELECT,   "SELECT"   },
};

static void Lock(void) {
	sceKernelLockLwMutex(&lock, 1, NULL);
}

static void Unlock(void) {
	sceKernelUnlockLwMutex(&lock, 1);
}

// Moves everything in the ring to the end of debug.log. The lock is held
// only while copying a block out of the ring, never while the card works.
// Bytes that the card refuses are lost: the ring has already let them go.
static void Flush(void) {
	char block[DEBUG_LOG_BLOCK_SIZE];

	Lock();
	unsigned int len = LogRing_Read(&ring, block, sizeof(block));
	Unlock();

	if (len == 0)
		return;

	SceUID fd = sceIoOpen(DEBUG_LOG_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);

	while (len > 0) {
		if (fd >= 0)
			sceIoWrite(fd, block, len);

		Lock();
		len = LogRing_Read(&ring, block, sizeof(block));
		Unlock();
	}

	// Closing after every flush is what puts the lines on the card, so that
	// closing the app with PS loses at most the last quarter of a second.
	if (fd >= 0)
		sceIoClose(fd);
}

static int DebugLog_Writer(SceSize args, void *argp) {
	while (!writer_quit) {
		sceKernelDelayThread(DEBUG_LOG_FLUSH_US);
		Flush();
	}

	return sceKernelExitThread(0);
}

// Keeps the previous session as debug.prev.log. Neither file existing is not
// an error; a debug.log that can be neither moved nor removed is.
static int Rotate(void) {
	SceIoStat stat;

	sceIoRemove(DEBUG_LOG_PREV);

	if (sceIoGetstat(DEBUG_LOG_FILE, &stat) < 0)
		return 1;

	if (sceIoRename(DEBUG_LOG_FILE, DEBUG_LOG_PREV) >= 0)
		return 1;

	return sceIoRemove(DEBUG_LOG_FILE) >= 0;
}

static int StartWriter(void) {
	writer_quit = 0;
	writer = sceKernelCreateThread("debug_log_thread", DebugLog_Writer, DEBUG_LOG_PRIORITY, DEBUG_LOG_STACK_SIZE, 0, 0, NULL);
	if (writer < 0 || sceKernelStartThread(writer, 0, NULL) < 0) {
		if (writer >= 0)
			sceKernelDeleteThread(writer);
		writer = -1;
		return 0;
	}

	return 1;
}

static void StopWriter(void) {
	if (writer < 0)
		return;

	writer_quit = 1;
	sceKernelWaitThreadEnd(writer, NULL, NULL);
	sceKernelDeleteThread(writer);
	writer = -1;
}

static int CreateEmptyLog(void) {
	SceUID fd = sceIoOpen(DEBUG_LOG_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);

	if (fd < 0)
		return 0;

	sceIoClose(fd);
	return 1;
}

static void Fail(const char *reason) {
	mode = DEBUG_LOG_FAILED;
	fail_reason = reason;
}

static void LogStart(void) {
	SceDateTime now;
	// Room for the widest the fields can print, not just for a sane date.
	char time[40];
	LogLine line;

	memset(&now, 0, sizeof(now));
	sceRtcGetCurrentClockLocalTime(&now);
	snprintf(time, sizeof(time), "%04u-%02u-%02u %02u:%02u:%02u",
		now.year, now.month, now.day, now.hour, now.minute, now.second);

	DebugLog_Begin(&line, "log");
	LogLine_Str(&line, "event", "start");
	LogLine_Str(&line, "version", APP_VERSION);
	LogLine_Str(&line, "time", time);
	DebugLog_Commit(&line);
}

void DebugLog_Init(int relaunch) {
	SceIoStat stat;

	if (sceIoGetstat(DEBUG_LOG_MARKER, &stat) < 0)
		return;

	memory = malloc(sizeof(*memory));
	if (!memory) {
		Fail("sin memoria");
		return;
	}

	for (int i = 0; i < DEBUG_STATE_COUNT; i++)
		LogState_Reset(&memory->states[i]);
	LogRing_Init(&ring, memory->ring, sizeof(memory->ring));

	if (sceKernelCreateLwMutex(&lock, "debug_log_lock", 0, 0, NULL) < 0) {
		free(memory);
		memory = NULL;
		Fail("no se pudo crear el lock");
		return;
	}

	// A suite that relaunches the app goes on in the same file.
	if (!relaunch && (!Rotate() || !CreateEmptyLog())) {
		sceKernelDeleteLwMutex(&lock);
		free(memory);
		memory = NULL;
		Fail("no se pudo crear debug.log");
		return;
	}

	if (!StartWriter()) {
		sceKernelDeleteLwMutex(&lock);
		free(memory);
		memory = NULL;
		Fail("no se pudo crear el hilo");
		return;
	}

	main_thread = sceKernelGetThreadId();
	mode = DEBUG_LOG_ACTIVE;
	debug_log_on = 1;
	LogStart();
}

void DebugLog_Shutdown(void) {
	if (mode != DEBUG_LOG_ACTIVE)
		return;

	debug_log_on = 0;
	StopWriter();

	// The writer is gone: whatever it had not taken yet goes from here.
	Flush();

	sceKernelDeleteLwMutex(&lock);
	free(memory);
	memory = NULL;
	mode = DEBUG_LOG_OFF;
}

void DebugLog_Sync(void) {
	if (mode != DEBUG_LOG_ACTIVE)
		return;

	// Two flushes at once could append their blocks out of order, so the
	// writer stops while this one runs.
	StopWriter();
	Flush();
	StartWriter();
}

// The suite sees every line logged on the main thread, already finished.
static void Observe(LogLine *line) {
	if (!Suite_Running() || sceKernelGetThreadId() != main_thread)
		return;

	LogLine_Finish(line);
	Suite_Observe(line->text, line->len);
}

void DebugLog_Begin(LogLine *line, const char *category) {
	LogLine_Begin(line, sceKernelGetProcessTimeWide(), category);
}

void DebugLog_Commit(LogLine *line) {
	if (!debug_log_on)
		return;

	Lock();
	LogRing_Push(&ring, line);
	Unlock();

	Observe(line);
}

void DebugLog_CommitState(LogLine *line, DebugLog_StateChannel channel) {
	if (!debug_log_on || channel < 0 || channel >= DEBUG_STATE_COUNT)
		return;

	Lock();
	if (LogState_Changed(&memory->states[channel], line))
		LogRing_Push(&ring, line);
	Unlock();

	// A repeated state is not written, but it is still the screen's state.
	Observe(line);
}

static const char *const state_names[DEBUG_STATE_COUNT] = { "folders", "library", "settings", "nowplaying" };

const char *DebugLog_LastState(const char *category, unsigned int *len) {
	if (!debug_log_on)
		return NULL;

	for (int i = 0; i < DEBUG_STATE_COUNT; i++) {
		if (strcmp(category, state_names[i]) != 0)
			continue;

		// Only the main thread writes states, and only it asks.
		if (memory->states[i].len == 0)
			return NULL;

		*len = memory->states[i].len;
		return memory->states[i].body;
	}

	return NULL;
}

static const char *ScreenName(UI_Screen screen) {
	switch (screen) {
		case UI_SCREEN_FOLDERS:
			return "folders";
		case UI_SCREEN_LIBRARY:
			return "library";
		case UI_SCREEN_SETTINGS:
			return "settings";
		case UI_SCREEN_NOW_PLAYING:
			return "nowplaying";
		default:
			return "none";
	}
}

void DebugLog_Screen(UI_Screen to) {
	if (!debug_log_on)
		return;

	LogLine line;
	DebugLog_Begin(&line, "screen");
	LogLine_Str(&line, "to", ScreenName(to));
	// The overlay's HEAP figure, from the same call.
	LogLine_Int(&line, "heap_kb", (long long)(mallinfo().uordblks / 1024));

	Lock();
	for (int i = 0; i < DEBUG_STATE_COUNT; i++)
		LogState_Reset(&memory->states[i]);
	LogRing_Push(&ring, &line);
	Unlock();

	Observe(&line);
}

const LogButtonName *DebugLog_ButtonNames(int *count) {
	*count = (int)(sizeof(button_names) / sizeof(button_names[0]));
	return button_names;
}

void DebugLog_Buttons(LogLine *line, const char *key, unsigned int mask) {
	LogLine_Buttons(line, key, mask, button_names, (int)(sizeof(button_names) / sizeof(button_names[0])));
}

void DebugLog_Input(unsigned int down, unsigned int up) {
	if (!debug_log_on)
		return;

	unsigned int named = 0;
	for (unsigned int i = 0; i < sizeof(button_names) / sizeof(button_names[0]); i++)
		named |= button_names[i].mask;

	down &= named;
	up &= named;
	if (!down && !up)
		return;

	LogLine line;
	DebugLog_Begin(&line, "input");
	DebugLog_Buttons(&line, "down", down);
	DebugLog_Buttons(&line, "up", up);
	DebugLog_Commit(&line);
}

DebugLog_Status DebugLog_GetStatus(void) {
	DebugLog_Status status = { mode, fail_reason, 0, 0 };

	if (mode == DEBUG_LOG_ACTIVE) {
		Lock();
		status.lines = ring.accepted;
		status.dropped = ring.dropped;
		Unlock();
	}

	return status;
}
