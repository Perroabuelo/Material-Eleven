#!/usr/bin/env python3
"""Generates the folders of test files to copy to the console.

    python3 gen_testfiles.py OUT_DIR [--ffmpeg PATH] [--vita-dir ux0:/pruebas-eleven]

  formatos-mono/      FLAC, MP3, OGG, Opus and WAV with tags, one channel;
                      FLAC and MP3 carry a cover
  formatos-estereo/   the same in stereo, with a different tone on each channel
                      so a lost channel is heard
  flac/               ten FLAC with covers, for fast track skipping
  tracker/            MOD and XM modules
  danados/            one good track and text files named as audio
  nombre/             a WAV called "100% pure %s %d.wav"
  nombres-largos/     short WAVs with 100, 200 and 250 byte names, in ASCII and
                      in UTF-8 with Japanese characters
  letras/             WAVs with a well-formed .lrc and with broken ones
  playlists/          .m3u files: relative and ux0: paths, missing files,
                      #EXTINF, CRLF, empty and a long one

FLAC, MP3, OGG and Opus need ffmpeg (PATH or --ffmpeg); without it they are
skipped and everything else is still written. Every case of .lrc and .m3u is a
function of its own, so the lyrics and playlist changes can adjust them.

Python 3 standard library only.
"""

import argparse
import math
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import wave

RATE = 44100
WINDOWS_PATH_MAX = 259  # MAX_PATH (260) minus the terminating NUL


# --- WAV -------------------------------------------------------------------

def tone_second(freqs, amp=8000):
    """One second of 16-bit PCM, one sine per channel. Integer frequencies make
    a whole number of cycles per second, so the second can be repeated."""
    out = bytearray()
    for i in range(RATE):
        for f in freqs:
            out += struct.pack("<h", int(amp * math.sin(2 * math.pi * f * i / RATE)))
    return bytes(out)


def info_chunk(tags):
    """RIFF LIST/INFO chunk with title (INAM), artist (IART) and album (IPRD)."""
    body = b"INFO"
    for key, fourcc in (("title", b"INAM"), ("artist", b"IART"), ("album", b"IPRD")):
        if key in tags:
            text = tags[key].encode("utf-8") + b"\0"
            body += fourcc + struct.pack("<I", len(text)) + text + (b"\0" if len(text) % 2 else b"")
    return b"LIST" + struct.pack("<I", len(body)) + body


def make_wav(path, seconds=20, freqs=(440, 440), tags=None):
    with wave.open(path, "wb") as w:
        w.setnchannels(len(freqs))
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(tone_second(freqs) * seconds)
    if tags:
        with open(path, "r+b") as f:
            f.seek(0, os.SEEK_END)
            f.write(info_chunk(tags))
            riff_size = f.tell() - 8
            f.seek(4)
            f.write(struct.pack("<I", riff_size))


def write(path, data):
    with open(path, "wb") as f:
        f.write(data)


# --- MOD and XM ------------------------------------------------------------

def square(n=64, amp=80):
    # One period of a square wave, signed 8 bits.
    return bytes([(amp if i < n // 2 else -amp) & 0xFF for i in range(n)])


MOD_PERIODS = [428, 381, 339, 320, 285, 254, 226, 214]  # C-2 .. C-3


def make_mod(title, seed):
    """ProTracker M.K. module, 4 channels."""
    sample = square()
    out = bytearray(title.encode()[:20].ljust(20, b"\0"))
    # Sample 1 loops; the other 30 are empty.
    out += b"square".ljust(22, b"\0") + struct.pack(">HBBHH", len(sample) // 2, 0, 48, 0, len(sample) // 2)
    for _ in range(30):
        out += b"\0" * 22 + struct.pack(">HBBHH", 0, 0, 0, 0, 1)
    npat = 2
    out += bytes([npat, 127]) + bytes(range(npat)).ljust(128, b"\0") + b"M.K."
    for p in range(npat):
        for row in range(64):
            for ch in range(4):
                if ch == 0 and row % 4 == 0:
                    period = MOD_PERIODS[(row // 4 + seed + p * 3) % len(MOD_PERIODS)]
                    smp = 1
                    out += bytes([(smp & 0xF0) | (period >> 8), period & 0xFF, (smp & 0x0F) << 4, 0])
                else:
                    out += b"\0\0\0\0"
    out += sample
    return bytes(out)


def make_xm(title, seed):
    """FastTracker II module, 2 channels."""
    sample = square()
    channels, rows, npat = 2, 32, 2
    out = bytearray(b"Extended Module: ")
    out += title.encode()[:20].ljust(20, b"\0") + b"\x1a" + b"gen_testfiles".ljust(20, b"\0")
    out += struct.pack("<H", 0x0104)
    hdr = struct.pack("<IHHHHHHHH", 276, npat, 0, channels, npat, 1, 1, 6, 125)
    hdr += bytes(range(npat)).ljust(256, b"\0")
    out += hdr
    for p in range(npat):
        data = bytearray()
        for row in range(rows):
            for ch in range(channels):
                if ch == 0 and row % 4 == 0:
                    note = 49 + [0, 2, 4, 5, 7, 9, 11, 12][(row // 4 + seed + p * 3) % 8]  # from C-4
                    data += bytes([note, 1, 0, 0, 0])
                else:
                    data += bytes([0x80])  # empty packed note
        out += struct.pack("<IBHH", 9, 0, rows, len(data)) + data
    # Instrument 1 with one sample.
    ins = bytearray(struct.pack("<I", 263))
    ins += b"square".ljust(22, b"\0") + b"\0" + struct.pack("<H", 1)
    ins += struct.pack("<I", 40)
    ins += b"\0" * 96               # note map: everything to sample 0
    ins += b"\0" * 48 + b"\0" * 48  # envelopes
    ins += b"\0" * 10               # points, sustain, loops, types
    ins += b"\0" * 4                # vibrato
    ins += struct.pack("<H", 0) + b"\0" * 22  # fadeout + reserved
    assert len(ins) == 263, len(ins)
    out += ins
    # Sample header: forward loop, 8 bits.
    out += struct.pack("<IIIBbBBbB", len(sample), 0, len(sample), 48, 0, 1, 128, 0, 0)
    out += b"square".ljust(22, b"\0")
    # Delta-encoded data.
    prev, delta = 0, bytearray()
    for b in sample:
        v = b - 256 if b > 127 else b
        delta.append((v - prev) & 0xFF)
        prev = v
    out += delta
    return bytes(out)


# --- ffmpeg formats --------------------------------------------------------

class Ffmpeg:
    def __init__(self, exe, scratch):
        self.exe = exe
        self.scratch = scratch

    def run(self, args):
        subprocess.run([self.exe, "-hide_banner", "-loglevel", "error", "-y"] + args, check=True)

    def cover(self, name, color):
        path = os.path.join(self.scratch, name + ".jpg")
        self.run(["-f", "lavfi", "-i", f"color=c={color}:s=500x500", "-frames:v", "1", path])
        return path

    def encode(self, path, seconds, freqs, tags, cover=None):
        """Encodes a tone, one sine per channel, with tags and an optional cover.
        The codec follows the extension."""
        expr = "|".join(f"0.25*sin({f}*2*PI*t)" for f in freqs)
        args = ["-f", "lavfi", "-i", f"aevalsrc={expr}:s={RATE}:d={seconds}"]
        if cover:
            args += ["-i", cover, "-map", "0:a", "-map", "1:v", "-c:v", "copy", "-disposition:v", "attached_pic"]
        ext = os.path.splitext(path)[1]
        args += {".flac": ["-c:a", "flac"],
                 ".mp3": ["-c:a", "libmp3lame", "-b:a", "192k", "-id3v2_version", "3"],
                 ".ogg": ["-c:a", "libvorbis", "-q:a", "5"],
                 ".opus": ["-c:a", "libopus", "-b:a", "128k"]}[ext]
        for key, value in tags.items():
            args += ["-metadata", f"{key}={value}"]
        self.run(args + [path])


# --- folders ---------------------------------------------------------------

def gen_formats(out, ff, name, label, freqs, color):
    folder = os.path.join(out, name)
    os.makedirs(folder, exist_ok=True)
    stem = name.split("-")[1]
    album = f"Formatos {label.lower()}"
    cover = ff.cover(name, color) if ff else None
    for i, ext in enumerate(["flac", "mp3", "ogg", "opus"], 1):
        if ff:
            tags = {"title": f"{label} {ext.upper()}", "artist": "Generador", "album": album, "track": str(i)}
            ff.encode(os.path.join(folder, f"{i:02d} {stem}.{ext}"), 20, freqs, tags,
                      cover if ext in ("flac", "mp3") else None)
    make_wav(os.path.join(folder, f"05 {stem}.wav"), 20, freqs,
             {"title": f"{label} WAV", "artist": "Generador", "album": album})


def gen_flac(out, ff):
    folder = os.path.join(out, "flac")
    os.makedirs(folder, exist_ok=True)
    for i in range(1, 11):
        # A different hue on every cover, so the accent changes on each skip.
        r, g, b = (int(255 * (0.5 + 0.5 * math.cos(2 * math.pi * (i / 10 + k / 3)))) for k in range(3))
        cover = ff.cover(f"flac{i}", f"0x{r:02X}{g:02X}{b:02X}")
        tags = {"title": f"FLAC {i}", "artist": "Generador", "album": "Rapido", "track": str(i)}
        ff.encode(os.path.join(folder, f"{i:02d} rapido.flac"), 20, (220 + 40 * i,), tags, cover)


def gen_tracker(out):
    folder = os.path.join(out, "tracker")
    os.makedirs(folder, exist_ok=True)
    for i in range(3):
        write(os.path.join(folder, f"{i + 1:02d} prueba.mod"), make_mod(f"prueba mod {i + 1}", i))
    for i in range(3):
        write(os.path.join(folder, f"{i + 4:02d} prueba.xm"), make_xm(f"prueba xm {i + 1}", i))


def gen_damaged(out):
    folder = os.path.join(out, "danados")
    os.makedirs(folder, exist_ok=True)
    make_wav(os.path.join(folder, "01 buena.wav"))
    junk = b"Esto no es audio, es un archivo de texto renombrado.\n" * 40
    for i, ext in enumerate(["mp3", "ogg", "opus", "xm", "mod"]):
        write(os.path.join(folder, f"{i + 2:02d} danado.{ext}"), junk)


def gen_name(out):
    folder = os.path.join(out, "nombre")
    os.makedirs(folder, exist_ok=True)
    make_wav(os.path.join(folder, "100% pure %s %d.wav"), seconds=10, freqs=(330, 330))


def long_name(prefix, filler, nbytes):
    """A .wav file name of exactly `nbytes` UTF-8 bytes."""
    name, i = prefix, 0
    while len((name + filler[i % len(filler)] + ".wav").encode("utf-8")) <= nbytes:
        name += filler[i % len(filler)]
        i += 1
    name += "x" * (nbytes - len((name + ".wav").encode("utf-8")))
    return name + ".wav"


def long_names():
    names = []
    for n, size in enumerate((100, 200, 250), 1):
        names.append(long_name(f"{n:02d} ascii {size} ", "abcdefghijklmnopqrstuvwxyz ", size))
        names.append(long_name(f"{n + 3:02d} utf8 {size} ", "長いファイル名の試験です、", size))
    return names


def windows_path_len(path):
    return len(os.path.abspath(path).encode("utf-16-le")) // 2


def windows_path_limit():
    """MAX_PATH when Windows still enforces it, None when there is no such limit
    (not Windows, or LongPathsEnabled is set; python.exe is long path aware)."""
    if os.name != "nt":
        return None
    import winreg
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\FileSystem") as key:
            if winreg.QueryValueEx(key, "LongPathsEnabled")[0]:
                return None
    except OSError:
        pass
    return WINDOWS_PATH_MAX


def gen_long_names(out):
    folder = os.path.join(out, "nombres-largos")
    os.makedirs(folder, exist_ok=True)
    limit = windows_path_limit()
    too_long, failed = [], []
    for name in long_names():
        path = os.path.join(folder, name)
        if limit and windows_path_len(path) > limit:
            too_long.append(name)
            continue
        try:
            make_wav(path, seconds=5, freqs=(392, 392))
        except OSError as e:
            failed.append(f"{name}: {e.strerror or e}")
    if too_long:
        longest = max(windows_path_len(os.path.join(folder, n)) for n in too_long)
        print(f"warning: nombres-largos/: {len(too_long)} of {len(long_names())} names do not fit in a "
              f"Windows path ({limit} characters) under {os.path.abspath(out)} and were skipped. Shorten "
              f"the output folder by {longest - limit} characters (e.g. C:\\pruebas), or enable long "
              f"paths (LongPathsEnabled), which the 250 byte ASCII name needs in any folder")
    for f in failed:
        print(f"warning: nombres-largos/: could not write {f}")
    return not too_long and not failed


# --- lyrics (.lrc) ---------------------------------------------------------
# Each case returns the bytes of its .lrc; gen_lyrics writes it next to a WAV
# with the same name.

def lrc_good():
    return ("[ti:Letra bien formada]\n[ar:Generador]\n[al:Letras]\n"
            "[00:00.00]Primera línea\n[00:04.00]Segunda línea\n[00:08.50]Tercera, con acentos: ñandú\n"
            "[00:12.00][00:16.00]Una línea con dos marcas\n").encode("utf-8")


def lrc_bad_timestamps():
    return ("[00:00.00]Esta marca es válida\n[99:99.99]Minutos y segundos fuera de rango\n"
            "[0a:1b.2c]Letras en vez de números\n[-00:01.00]Marca negativa\n[00:05]Sin centésimas\n"
            "[00:08.123456]Demasiados decimales\n").encode("utf-8")


def lrc_unclosed_bracket():
    return "[00:00.00]Bien\n[00:04.00Corchete sin cerrar\n[00:08.00]Después del error\n[ar:Sin cerrar\n".encode("utf-8")


def lrc_long_line():
    return ("[00:00.00]" + "Una línea muy larga que no cabe en la pantalla. " * 60 + "\n"
            "[00:10.00]Línea normal después\n").encode("utf-8")


def lrc_empty():
    return b""


def lrc_utf8_bom():
    return b"\xef\xbb\xbf" + lrc_good()


def lrc_not_utf8():
    return "[00:00.00]Línea en Latin-1: canción, ñandú\n[00:04.00]Otra más\n".encode("latin-1")


def lrc_crlf():
    return lrc_good().replace(b"\n", b"\r\n")


LYRICS = [
    ("01 bien", lrc_good),
    ("02 marcas invalidas", lrc_bad_timestamps),
    ("03 corchete sin cerrar", lrc_unclosed_bracket),
    ("04 linea larga", lrc_long_line),
    ("05 vacio", lrc_empty),
    ("06 bom utf8", lrc_utf8_bom),
    ("07 no utf8", lrc_not_utf8),
    ("08 crlf", lrc_crlf),
]


def gen_lyrics(out):
    folder = os.path.join(out, "letras")
    os.makedirs(folder, exist_ok=True)
    for name, case in LYRICS:
        make_wav(os.path.join(folder, name + ".wav"), seconds=20, freqs=(523, 523))
        write(os.path.join(folder, name + ".lrc"), case())


# --- playlists (.m3u) ------------------------------------------------------
# They only point to files every run writes (WAV and tracker), so a run without
# ffmpeg leaves no missing entries beyond the intended ones. Each case returns
# the lines of its playlist; gen_playlists joins them with "\n".

PLAYLIST_TARGETS = [
    "formatos-estereo/05 estereo.wav",
    "tracker/01 prueba.mod",
    "tracker/04 prueba.xm",
    "nombre/100% pure %s %d.wav",
    "formatos-mono/05 mono.wav",
]


def m3u_relative(vita_dir):
    return ["../" + t for t in PLAYLIST_TARGETS]


def m3u_absolute(vita_dir):
    return [f"{vita_dir}/{t}" for t in PLAYLIST_TARGETS]


def m3u_missing(vita_dir):
    return ["../" + PLAYLIST_TARGETS[0], "../formatos-estereo/no existe.wav",
            "../" + PLAYLIST_TARGETS[1], f"{vita_dir}/carpeta inexistente/pista.mp3",
            "ux0:/no/existe.flac", "../" + PLAYLIST_TARGETS[2]]


def m3u_extinf(vita_dir):
    lines = ["#EXTM3U"]
    for i, t in enumerate(PLAYLIST_TARGETS, 1):
        lines += [f"#EXTINF:20,Generador - Pista {i}", "../" + t]
    return lines


def m3u_empty(vita_dir):
    return []


def m3u_many(vita_dir):
    return ["../" + PLAYLIST_TARGETS[i % len(PLAYLIST_TARGETS)] for i in range(1000)]


PLAYLISTS = [
    ("01 relativas", m3u_relative, "\n"),
    ("02 absolutas", m3u_absolute, "\n"),
    ("03 inexistentes", m3u_missing, "\n"),
    ("04 extinf", m3u_extinf, "\n"),
    ("05 crlf", m3u_extinf, "\r\n"),
    ("06 vacia", m3u_empty, "\n"),
    ("07 muchas", m3u_many, "\n"),
]


def gen_playlists(out, vita_dir):
    folder = os.path.join(out, "playlists")
    os.makedirs(folder, exist_ok=True)
    for name, case, eol in PLAYLISTS:
        lines = case(vita_dir)
        write(os.path.join(folder, name + ".m3u"), "".join(l + eol for l in lines).encode("utf-8"))


# --- main ------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Generate the console test folders.")
    ap.add_argument("out", help="output folder; keep it short on Windows (e.g. C:\\pruebas)")
    ap.add_argument("--ffmpeg", help="path to ffmpeg (default: PATH)")
    ap.add_argument("--vita-dir", default="ux0:/pruebas-eleven",
                    help="where OUT will be copied on the console, for absolute .m3u paths "
                         "(default: ux0:/pruebas-eleven)")
    args = ap.parse_args()

    exe = args.ffmpeg or shutil.which("ffmpeg")
    if args.ffmpeg and not (os.path.isfile(exe) or shutil.which(exe)):
        sys.exit(f"gen_testfiles.py: ffmpeg not found at {args.ffmpeg}")
    if not exe:
        print("warning: ffmpeg not found (PATH or --ffmpeg): skipping FLAC, MP3, OGG and Opus "
              "in formatos-mono/ and formatos-estereo/, and the whole flac/ folder")

    os.makedirs(args.out, exist_ok=True)
    with tempfile.TemporaryDirectory() as scratch:
        ff = Ffmpeg(exe, scratch) if exe else None
        try:
            gen_formats(args.out, ff, "formatos-mono", "Mono", (440,), "0xD02020")
            gen_formats(args.out, ff, "formatos-estereo", "Estereo", (440, 660), "0x2050D0")
            if ff:
                gen_flac(args.out, ff)
        except subprocess.CalledProcessError as e:
            sys.exit(f"gen_testfiles.py: ffmpeg failed: {' '.join(e.cmd)}")
    gen_tracker(args.out)
    gen_damaged(args.out)
    gen_name(args.out)
    gen_long_names(args.out)
    gen_lyrics(args.out)
    gen_playlists(args.out, args.vita_dir)
    print(f"done: {os.path.abspath(args.out)}")


if __name__ == "__main__":
    main()
