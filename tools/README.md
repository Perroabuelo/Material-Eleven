# Development tools

Helpers for diagnosing and testing Material Eleven outside the app. They are
Python 3 scripts (3.8 or newer) that use only the standard library, so they run
in WSL, on Windows and on Linux without installing anything. External tools are
optional: when one is missing, the script does what it can and says what it
skipped.

The commands below are meant to be run from the repository root in WSL.

## `crashdump/psp2dmp.py`: read a crash dump

When an app crashes, the console writes a dump to `ux0:data/` named
`psp2core-<time>-<id>-eboot.bin.psp2dmp`. This script shows:

1. the app the dump belongs to (dumps of other homebrew land in the same
   folder; those are reported and skipped),
2. the threads, marking the ones that stopped with a fault (`0x30004` is a data
   abort),
3. the registers of each faulted thread, with the module, segment and offset
   each one points into,
4. given the ELF of the same build, the function (and line, when there is debug
   info) of `pc` and `lr`.

It checks the layout of each part of the dump as it reads it. If something does
not fit (another firmware, a truncated file), it says so and skips that part
instead of printing numbers it cannot trust.

**Needs:** Python 3. To resolve functions, the unstripped `ElevenMPV` ELF built
from the same commit as the `.vpk` that crashed, and `arm-vita-eabi-addr2line`
from VitaSDK (found in `PATH` or in `$VITASDK/bin`, or given with
`--addr2line`). With an ELF from another commit, the function names are wrong.

Copy the dumps from `ux0:data/` to `~/vita-dumps/` (VitaShell over USB or FTP),
then:

```sh
python3 tools/crashdump/psp2dmp.py ~/vita-dumps/*.psp2dmp
```

To resolve `pc` and `lr`, build the ELF from the commit (or tag) the crashing
`.vpk` came from. The build directory can live anywhere; outside `/mnt/c` it is
much faster:

```sh
export VITASDK=~/vitasdk
export PATH="$VITASDK/bin:$PATH"
cmake -S . -B ~/elevenmpv-build
make -C ~/elevenmpv-build -j"$(nproc)"
python3 tools/crashdump/psp2dmp.py ~/vita-dumps/*.psp2dmp --elf ~/elevenmpv-build/ElevenMPV
```

Example output for a heap corruption crash:

```
App: ELEVENMPV  Material Eleven

Threads (6):
  0x40010003  ELEVENMPV                        0x30004 data abort  <-- stopped
  0x4002007b  SceGxmDisplayQueue               0x00000 running
  ...

Thread 0x40010003 ELEVENMPV: data abort (0x30004)
  r0   0x812cbc38  ElevenMPV data +0xbc38
  ...
  pc   0x8112d1f0  ElevenMPV code +0x11a1f0

In /home/user/elevenmpv-build/ElevenMPV (code at 0x81000000):
  ELEVENMPV pc 0x8111a1f0  _malloc_r  ??:?
```

A fault inside `_malloc_r` or `_free_r` usually means the heap was corrupted
earlier, by a write past the end of a buffer; the function that crashed is
rarely the one at fault.

Dumps hold memory and system data from the console: do not commit them.

## `testmedia/gen_testfiles.py`: generate test files for the console

Writes one folder per purpose:

| Folder | Contents |
| --- | --- |
| `formatos-mono/` | FLAC, MP3, OGG, Opus and WAV with tags, one channel; FLAC and MP3 with a cover |
| `formatos-estereo/` | The same in stereo, with a different tone on each channel, so a lost channel is heard |
| `flac/` | Ten FLAC with covers of different colors, for fast track skipping |
| `tracker/` | MOD and XM modules |
| `danados/` | One good track and text files with audio extensions |
| `nombre/` | A WAV named `100% pure %s %d.wav` |
| `nombres-largos/` | Short WAVs with names of 100, 200 and 250 bytes, in ASCII and in UTF-8 with Japanese characters |
| `letras/` | WAVs with a well-formed `.lrc` and with broken ones (bad timestamps, unclosed bracket, very long line, empty, UTF-8 BOM, not UTF-8, CRLF) |
| `playlists/` | `.m3u` files with relative and `ux0:` paths, missing files, `#EXTINF`, CRLF, an empty one and one with 1000 entries |

**Needs:** Python 3. FLAC, MP3, OGG and Opus need `ffmpeg` (with libmp3lame,
libvorbis and libopus) in `PATH` or given with `--ffmpeg`; without it they are
skipped, along with the whole `flac/` folder, and everything else is still
written. Install it in WSL with your package manager, or run the script from
Windows where ffmpeg is installed.

```sh
python3 tools/testmedia/gen_testfiles.py ~/pruebas
```

From Windows (PowerShell), with ffmpeg installed there:

```powershell
python tools\testmedia\gen_testfiles.py C:\pruebas
```

Then copy the contents of the output folder to `ux0:/pruebas-eleven/` on the
console. The absolute paths in `playlists/` assume that location; pass
`--vita-dir` when copying somewhere else.

On Windows without long paths enabled (`LongPathsEnabled`), paths are limited
to 260 characters. The 250 byte ASCII name does not fit under that limit in any
folder, so the script skips the names that do not fit and says so; keep the
output folder short and enable long paths to get all of them.
