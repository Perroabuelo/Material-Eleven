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

## `promo/build.py`: build the promo video

Turns a screen recording of the console into the promo video shown in the
README. Each scene shows the console screen in a rounded frame, over a dark
canvas lit with that scene's accent colour, with a caption above it. The
scenes are joined with short crossfades, and the video ends on the app icon,
the supported formats and, when given an avatar, the author's signature.

The audio is laid over afterwards, because the capture has none. A background
song starts so that its beat drop lands on the first cut after the intro.
Under the lyrics scene, the song on screen comes in at the exact point the app's
elapsed counter shows, so the highlighted line matches the voice.

| Output | What it is |
| --- | --- |
| `material-eleven-github.mp4` | About 28 s, 1080p at 30 fps, under 10 MB: the one uploaded to the README |
| `material-eleven-full.mp4` | About 55 s, 1080p at 60 fps, with more scenes (folders, scan, settings, mini player) |
| `*-muted.mp4` | The same two videos without the audio track |
| `material-eleven.gif`, `.webp` | A frameless 640x360 loop, muted, used as the README fallback |

**Needs:** Python 3 and `ffmpeg`/`ffprobe` in `PATH`, built with libx264,
libwebp and drawtext (libfreetype). Unlike the other tools, ffmpeg is not
optional here. The fonts come from `res/`.

What goes where:

- **`tools/promo/edits/<version>.json`** describes one recording: each scene's
  start (`src`) and length (`dur`) in seconds of the recording, its caption,
  its accent, its playback speed, and which scene is the lyrics one. It also
  holds where the lyrics song started (`lyrics_song.start`, in seconds of the
  recording), the beat drop of the background song (`bed.drop`, in seconds of
  that song), and the outro text. `v3.5.json` is the edit of the 3.5 release.
- **Arguments** give the files, which stay out of the repository: the
  recording, the songs (they are copyrighted) and the avatar.

From Windows (PowerShell), where ffmpeg is usually installed:

```powershell
python tools\promo\build.py `
  --recording "D:\capturas\3.5.mkv" --edit tools\promo\edits\v3.5.json `
  --bed "D:\musica\Supernatural.flac" --lyrics-song "D:\musica\TOMBOY.flac" `
  --avatar "D:\avatar.png" --fix-obs-range --out "D:\promo"
```

Scenes are cached in `<out>/work`, keyed by their settings, so after changing
one scene only that scene is rendered again. `--only github` (repeatable) builds
just some of the outputs; the GIF needs neither song.

Do not commit the outputs: the videos with audio carry copyrighted songs, and
GitHub does not play a video stored in the repository. Upload
`material-eleven-github.mp4` by dragging it into GitHub's web editor (for
example, while editing `README.md` on the site), which gives a
`github.com/user-attachments/assets/...` URL, and put that URL on a line of its
own in the README.

### Writing the edit for a new recording

A contact sheet with one frame every two seconds, each stamped with its time,
shows where each screen starts:

```powershell
ffmpeg -i rec.mkv -vf "setparams=range=pc,fps=1/2,scale=320:180,drawtext=fontfile='C\:/Windows/Fonts/arial.ttf':text='%{pts\:hms}':x=4:y=4:fontsize=20:fontcolor=yellow:box=1:boxcolor=black,tile=8x11" -frames:v 1 sheet.jpg
```

For the lyrics song, read the elapsed counter of the lyrics view ten times a
second around a known time (here 152 s; `crop` frames the counter in a
1920x1080 recording) and note when it ticks over. The stamps count from the
`-ss` time: if the counter reaches `00:02` at stamp 0.35, that is 152.35 s into
the recording, so the song started at 150.35 s:

```powershell
ffmpeg -ss 152 -t 3 -i rec.mkv -vf "fps=10,crop=140:40:240:968,pad=240:40:100:0,drawtext=fontfile='C\:/Windows/Fonts/arial.ttf':text='%{pts\:flt}':x=4:y=8:fontsize=20:fontcolor=yellow,tile=6x5" -frames:v 1 counter.png
```

The beat drop of the background song is where its loudness jumps; this prints
the level every tenth of a second between 6 and 10 s:

```powershell
ffmpeg -ss 6 -t 4 -i song.flac -af "asetnsamples=4410,astats=metadata=1:reset=1,ametadata=print:key=lavfi.astats.Overall.RMS_level:file=-" -f null -
```

### Recording with OBS

- **Colour range.** OBS can write full-range pixels into a file tagged as
  limited range; players then crush the blacks, and the app background looks
  black instead of dark violet. Set Settings > Advanced > Color Range to match
  the capture source, or pass `--fix-obs-range` to reinterpret the recording.
  With the 3.5 recording, the app background goes from `RGB(1,1,8)` to
  `RGB(17,16,23)`, the same as in a screenshot taken on the console.
- **Sharpness.** The console draws at 960x544. Set the canvas to 1920x1088
  (exactly twice that) and the capture source's scale filter to "Point"
  (right click > Scale Filtering), so each console pixel becomes a crisp 2x2
  block instead of a blur.
- **Audio.** The capture brings no sound. Write down which songs play and when;
  the songs are added from their files.
