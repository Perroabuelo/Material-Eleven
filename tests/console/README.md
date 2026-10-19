# Console suites

Scenario suites that the app runs by itself on the console, with the
developer log on. The app replays the buttons and touches of each scenario,
waits for the log lines that say it worked, and writes a `pass` or `fail` per
scenario to `ux0:data/ElevenMPV/debug.log`. The full behavior is in the
`dev/console-scripts` spec; this file is the short version.

What a suite cannot check is still checked by hand: how the screen looks,
whether it runs smoothly, how it sounds, and anything the script cannot do
(typing in the system keyboard, for one).

## The suites

| Suite | What it covers |
| --- | --- |
| `dev-console-scripts.txt` | The suites themselves: input, waits, a timeout, isolation between scenarios, the seed |
| `dev-console-scripts-error.txt` | A suite with a syntax error on line 3, which must not run at all |
| `dev-debug-log.txt` | Scenarios of `dev/debug-log` that used to be done by hand |
| `ui-visualizer.txt` | Moving between Now Playing's views and the visualizer styles, and that doing it many times keeps the heap flat |
| `playback-niceties.txt` | Going back past 3 seconds, the sleep timer's menu, its end-of-track mode and two 15-minute expiries with the library (about 37 minutes in all) |

One suite per capability, named after it. A change with console scenarios adds
them to the suite of its capability, or starts one, and its design says which
scenarios are automated in which suite and which stay manual, and why.

## Running one

1. With the console in USB mode, create the log marker, an empty file
   `ux0:data/ElevenMPV/debug`, if it is not there.
2. Set up the fixtures the suite lists in its header.
3. Copy the suite to `ux0:data/ElevenMPV/suite.txt`.
4. Leave USB mode and open the app. Do not touch anything: any button or touch
   stops the suite. A strip at the top shows the scenario running.
5. Each scenario runs in a new process: the app closes and opens again
   between them. After the last one it shows a summary for a few seconds and
   closes by itself.
6. Back in USB mode, read `debug.log`. The suite is renamed to
   `suite.done.txt`, so it does not run again.

The `script` lines of the log tell what happened:

```
3.031 script event=scenario name="Bajar dos filas" index=2 total=13
3.248 script event=step line=19 cmd="tap 38 180"
3.607 script event=result name="Bajar dos filas" result=pass
3.278 script event=end pass=12 fail=1 interrupted=0
```

A failed `wait` says which line of the suite it was and the last line of its
category the log saw, which is usually enough to tell what went wrong.

Before each scenario the app puts `config.cfg` and `lastdir.txt` back as they
were before the suite, and does it again at the end, so a suite leaves no
settings behind. Playlists, the library index and the music on the card are
not restored: a suite that creates something names it with a `suite-` prefix
and says so in its header, so it can be cleaned up.

If the app is closed in the middle of a suite (from the LiveArea, or by a
crash), the next start records that scenario as `interrupted` and the suite
ends there.

## The format

One command per line. Blank lines and anything after `#` are ignored.

| Command | What it does |
| --- | --- |
| `scenario <name>` | Starts a scenario; the steps after it belong to it |
| `press <buttons> [xN]` | Presses and releases the buttons, N times |
| `hold <buttons> <duration>` | Holds the buttons for the duration |
| `tap <x> <y>` | Touches the front panel at (x, y) for 100 ms |
| `swipe <x1> <y1> <x2> <y2> <duration>` | Drags a finger in a straight line |
| `wait <category> <key>=<value> ... [timeout=<duration>]` | Waits for a log line of that category with those fields (5 s by default) |
| `sleep <duration>` | Waits without doing anything |
| `seed <n>` | Seeds the shuffle, so the order is the same every time |

Buttons are the log's names (`CROSS`, `CIRCLE`, `SQUARE`, `TRIANGLE`, `UP`,
`DOWN`, `LEFT`, `RIGHT`, `L`, `R`, `START`, `SELECT`), joined with `+`.
Coordinates are screen pixels, 0 to 959 and 0 to 543. A duration is a whole
number with `ms` or `s`. A `wait` value with spaces goes between double
quotes, escaped the way the log escapes it.

A `wait` is met by a line logged since the last `press`, `hold`, `tap` or
`swipe` (or since the scenario started), or by the screen's current state. Two
waits after the same action may be met in either order; put an action between
them to require one.

## Fixtures

Every suite starts with a comment that lists what it needs on the card, for
example:

```
# fixtures:
#   - biblioteca de "ux0:/Mis cosas" escaneada (291 canciones);
#   - lastdir.txt en "ux0:/pruebas-eleven/", con las carpetas que genera
#     tools/testmedia;
```

The test folders come from `tools/testmedia/gen_testfiles.py` (see
`tools/README.md`), copied to `ux0:/pruebas-eleven/`. Since `lastdir.txt` is
restored to what it was before the suite, setting it is how a suite chooses
the folder that every scenario opens in.
