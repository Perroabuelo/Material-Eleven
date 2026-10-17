#ifndef _ELEVENMPV_SUITE_H_
#define _ELEVENMPV_SUITE_H_

// The console side of the scenario suites (dev/console-scripts): with the
// developer log on, ux0:data/ElevenMPV/suite.txt is run one scenario per
// process, with its state kept in ux0:data/ElevenMPV/suite/ in between.
// The parsing and the frame by frame run are in script.c.

#define SUITE_FILE      "ux0:data/ElevenMPV/suite.txt"
#define SUITE_DONE_FILE "ux0:data/ElevenMPV/suite.done.txt"
#define SUITE_DIR       "ux0:data/ElevenMPV/suite"

extern int suite_running;

// A scenario is running: the app gets its input from the script.
static inline int Suite_Running(void) {
	return suite_running;
}

// 1 when this process was launched by a suite between two scenarios. Only
// reads the suite's state; call before DebugLog_Init, which needs it.
int Suite_IsRelaunch(void);

// After DebugLog_Init and before Config_Load. Without the log, does nothing
// and leaves suite.txt alone. Otherwise records a scenario that a previous
// process left half done, or starts the suite or its next scenario, with
// config.cfg and lastdir.txt as they were before the suite.
void Suite_Init(int relaunch);

// A line the log just took on the main thread, for the waits.
void Suite_Observe(const char *text, unsigned int len);

#endif
