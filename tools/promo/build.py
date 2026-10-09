#!/usr/bin/env python3
"""Build the Material Eleven promo videos from an OBS recording of the console.

The recording-specific part (which stretch of the recording goes in each scene,
captions, accent colours, where the lyrics song starts) lives in an edit file,
see edits/v3.5.json. Paths to the recording and the music are arguments, so
nothing in the repository points at one machine. See tools/README.md.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ICON = os.path.join(REPO, "sce_sys", "icon0.png")
F_TITLE = os.path.join(REPO, "res", "Manrope.ttf")
F_MONO = os.path.join(REPO, "res", "IBMPlexMono-Medium.ttf")

W, H, FPS = 1920, 1080, 60       # every scene is rendered at this size and rate
SW, SH = 1440, 810               # the console screen on the canvas
SX, SY = (W - SW) // 2, 228
RADIUS = 28
BASE = (14, 12, 18)              # canvas colour, close to the app background
STYLE = 2                        # bump to invalidate cached scenes after a style change

OUTPUTS = {
    # name: (width, height, fps, crf, maxrate)
    "github": (1920, 1080, 30, 20, "6M"),
    "full": (1920, 1080, 60, 14, None),
}

args = None
WORK = None


def ff(cmd):
    cmd = ["ffmpeg", "-hide_banner", "-v", "error", "-y"] + cmd
    if subprocess.run(cmd).returncode:
        sys.exit("ffmpeg failed: " + " ".join(cmd))


def probe_duration(path):
    r = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration",
                        "-of", "csv=p=0", path], capture_output=True, text=True)
    try:
        return float(r.stdout.strip())
    except ValueError:  # ffprobe has no duration for animated WebP
        return None


def esc(p):
    """A path or value inside a filtergraph."""
    return p.replace("\\", "/").replace(":", "\\:")


def textfile(s):
    """drawtext reads the text from a file, which avoids escaping commas and colons."""
    p = os.path.join(WORK, "t_" + hashlib.md5(s.encode()).hexdigest()[:10] + ".txt")
    with open(p, "w", encoding="utf-8") as f:
        f.write(s)
    return esc(p)


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def key(*parts):
    return hashlib.md5(json.dumps([STYLE] + list(parts), sort_keys=True).encode()).hexdigest()[:12]


def source_filter():
    # OBS can store full-range pixels in a file tagged as limited range, which
    # crushes the blacks; reinterpreting it as full range gives the real colours.
    return "setparams=range=pc," if args.fix_obs_range else ""


def mask_png():
    p = os.path.join(WORK, "mask.png")
    if not os.path.exists(p):
        r = RADIUS
        e = (f"clip(255*({r}+0.5-hypot(max(0,abs(X-(W-1)/2)-(W/2-{r})),"
             f"max(0,abs(Y-(H-1)/2)-(H/2-{r})))),0,255)")
        ff(["-f", "lavfi", "-i", f"color=black:s={SW}x{SH}", "-vf", f"format=gray,geq=lum='{e}'",
            "-frames:v", "1", p])
    return p


def bg_png(accent):
    """Dark canvas with a soft accent glow behind the screen and a drop shadow."""
    p = os.path.join(WORK, f"bg_{accent}_{key(accent)}.png")
    if os.path.exists(p):
        return p
    ar, ag, ab = rgb(accent)
    br, bgc, bb = BASE
    cx, cy = W / 2, SY + SH / 2
    g = f"0.30*exp(-(pow((X-{cx})/900,2)+pow((Y-{cy})/520,2)))"
    vig = f"(1-0.35*pow(hypot((X-{cx})/{W},(Y-{cy})/{H}),2))"
    expr = lambda b, a: f"clip(({b}+({a}-{b})*{g})*{vig},0,255)"
    ff(["-f", "lavfi", "-i", f"color=black:s={W}x{H}", "-i", mask_png(), "-filter_complex",
        f"[0:v]format=gbrp,geq=r='{expr(br, ar)}':g='{expr(bgc, ag)}':b='{expr(bb, ab)}'[bg];"
        f"[1:v]format=gray,pad={SW + 160}:{SH + 160}:80:80:black,boxblur=40:2,"
        f"lutyuv=y='val*0.75'[sa];"
        f"color=black:s={SW + 160}x{SH + 160},format=rgba[sc];[sc][sa]alphamerge[sh];"
        f"[bg][sh]overlay={SX - 80}:{SY - 80 + 24}",
        "-frames:v", "1", p])
    return p


def caption_filters(title, sub, fade):
    a = "if(lt(t,0.15),0,min(1,(t-0.15)/0.45))" if fade else "1"
    out = []
    if title:
        out.append(f"drawtext=fontfile='{esc(F_TITLE)}':textfile='{textfile(title)}':fontsize=70:"
                   f"fontcolor=white:x=(w-text_w)/2:y=46:alpha='{a}'")
    if sub:
        out.append(f"drawtext=fontfile='{esc(F_TITLE)}':textfile='{textfile(sub)}':fontsize=38:"
                   f"fontcolor=0xE4DEEC:x=(w-text_w)/2:y=142:alpha='{a}':"
                   f"shadowcolor=black@0.6:shadowx=0:shadowy=2")
    return out


def scene_clip(sc):
    p = os.path.join(WORK, f"scene_{sc['name']}_{key(sc, args.recording, args.fix_obs_range)}.mp4")
    if os.path.exists(p):
        return p
    sp = sc["speed"]
    caps = ",".join(caption_filters(sc["title"], sc["sub"], sc["fade"]))
    fc = (f"[0:v]{source_filter()}setpts=(PTS-STARTPTS)/{sp},fps={FPS},"
          f"scale={SW}:{SH}:flags=lanczos,unsharp=5:5:0.5,format=rgba[s];"
          f"[1:v]format=gray[m];[s][m]alphamerge[sr];"
          f"[2:v]format=rgba[b];[b][sr]overlay={SX}:{SY}:shortest=0"
          + (f",{caps}" if caps else "") + ",format=yuv420p[v]")
    ff(["-ss", str(sc["src"]), "-t", str(sc["dur"] * sp), "-i", args.recording, "-i", mask_png(),
        "-loop", "1", "-framerate", str(FPS), "-t", str(sc["dur"]), "-i", bg_png(sc["accent"]),
        "-filter_complex", fc, "-map", "[v]", "-t", str(sc["dur"]), "-an",
        "-c:v", "libx264", "-crf", "8", "-preset", "fast", "-r", str(FPS), p])
    return p


def outro_clip(outro, dur, accent):
    avatar = args.avatar
    p = os.path.join(WORK, f"outro_{key(outro, dur, accent, avatar)}.mp4")
    if os.path.exists(p):
        return p
    a = lambda d: f"alpha='min(1,max(0,(t-{d})/0.5))'"
    inputs = ["-f", "lavfi", "-i", f"color=0x{''.join(f'{c:02X}' for c in BASE)}:s={W}x{H}:r={FPS}:d={dur}",
              "-loop", "1", "-framerate", str(FPS), "-t", str(dur), "-i", ICON]
    fc = (f"[0:v]format=rgba[b];"
          f"[1:v]scale=200:200:flags=lanczos,format=rgba,"
          f"geq=r='r(X,Y)':g='g(X,Y)':b='b(X,Y)':"
          f"a='alpha(X,Y)*clip(255*(42.5-hypot(max(0,abs(X-99.5)-58),max(0,abs(Y-99.5)-58))),0,255)/255',"
          f"fade=in:st=0:d=0.5:alpha=1[ic];"
          f"[b][ic]overlay=(W-w)/2:190")
    text = [
        f"drawtext=fontfile='{esc(F_TITLE)}':textfile='{textfile(outro['title'])}':fontsize=96:"
        f"fontcolor=white:x=(w-text_w)/2:y=420:{a(0.2)}",
        f"drawtext=fontfile='{esc(F_TITLE)}':textfile='{textfile(outro['subtitle'])}':fontsize=40:"
        f"fontcolor=0xE4DEEC:x=(w-text_w)/2:y=548:{a(0.4)}",
        f"drawtext=fontfile='{esc(F_MONO)}':textfile='{textfile(outro['formats'])}':fontsize=28:"
        f"fontcolor=0x{accent}:x=(w-text_w)/2:y=632:{a(0.6)}",
    ]
    if avatar:
        # signature row: round avatar, author and URL
        av, gx, gy = 128, 566, 776
        tx = gx + av + 30
        circle = f"clip(255*({av / 2}-hypot(X-{av / 2 - 0.5},Y-{av / 2 - 0.5})),0,255)"
        inputs += ["-loop", "1", "-framerate", str(FPS), "-t", str(dur), "-i", avatar]
        fc = (fc + "[b1];"
              f"color=0x2B2535:s={av}x{av}:r={FPS},format=rgba[disk];"
              f"[2:v]scale={av}:{av}:flags=lanczos,format=rgba[pf];"
              f"[disk][pf]overlay=0:0:shortest=1,"
              f"geq=r='r(X,Y)':g='g(X,Y)':b='b(X,Y)':a='{circle}',"
              f"fade=in:st=0.8:d=0.5:alpha=1[avt];"
              f"[b1][avt]overlay={gx}:{gy}:shortest=1")
        text += [
            f"drawtext=fontfile='{esc(F_TITLE)}':textfile='{textfile(outro['author'])}':fontsize=46:"
            f"fontcolor=white:x={tx}:y={gy + 14}:{a(0.8)}",
            f"drawtext=fontfile='{esc(F_MONO)}':textfile='{textfile(outro['url'])}':fontsize=28:"
            f"fontcolor=0xC9C1D6:x={tx}:y={gy + 76}:{a(0.9)}",
        ]
    else:
        text.append(f"drawtext=fontfile='{esc(F_MONO)}':textfile='{textfile(outro['url'])}':fontsize=30:"
                    f"fontcolor=0xC9C1D6:x=(w-text_w)/2:y=820:{a(0.8)}")
    fc += "," + ",".join(text) + ",format=yuv420p[v]"
    ff(inputs + ["-filter_complex", fc, "-map", "[v]", "-t", str(dur),
                 "-c:v", "libx264", "-crf", "8", "-preset", "fast", "-r", str(FPS), p])
    return p


def scenes_of(edit, cut):
    """Resolve caption and accent names of a cut into concrete scenes."""
    out = []
    for raw in edit["cuts"][cut]["scenes"]:
        title, sub = edit["captions"][raw["caption"]] if raw.get("caption") else (None, None)
        acc = raw.get("accent", "orange")
        out.append(dict(name=raw["name"], src=raw["src"], dur=raw["dur"],
                        accent=edit["accents"].get(acc, acc), title=title, sub=sub,
                        fade=raw.get("fade", True), speed=raw.get("speed", 1),
                        xf=raw.get("xf", 0.35), lyrics=raw.get("lyrics", False)))
    return out


def render(edit, cut):
    w, h, fps, crf, maxrate = OUTPUTS[cut]
    scenes = scenes_of(edit, cut)
    outro_dur = edit["cuts"][cut]["outro"]
    outro_xf = 0.6
    clips = [scene_clip(s) for s in scenes]
    clips.append(outro_clip(edit["outro"], outro_dur, edit["accents"]["orange"]))
    durs = [s["dur"] for s in scenes] + [outro_dur]
    xfs = [s["xf"] for s in scenes] + [outro_xf]

    starts, t = [], 0.0
    for i, d in enumerate(durs):
        if i:
            t -= xfs[i]
        starts.append(t)
        t += d
    total = t

    # video: a chain of short crossfades
    inputs, fc = [], []
    for c in clips:
        inputs += ["-i", c]
    prev = "0:v"
    for i in range(1, len(clips)):
        fc.append(f"[{prev}][{i}:v]xfade=transition=fade:duration={xfs[i]}:"
                  f"offset={starts[i]:.3f}[x{i}]")
        prev = f"x{i}"
    fc.append(f"[{prev}]scale={w}:{h}:flags=lanczos,fps={fps},format=yuv420p[v]")

    # audio: the bed until the lyrics scene, its drop on the first cut; then the
    # song on screen, in step with the app's elapsed counter, until the end
    n = len(clips)
    li = next(i for i, s in enumerate(scenes) if s["lyrics"])
    t_ly = starts[li]
    bed_off = edit["bed"]["drop"] - starts[1]
    bed_len = t_ly + 0.6
    fc.append(f"[{n}:a]atrim=start={bed_off:.3f}:duration={bed_len:.3f},asetpts=PTS-STARTPTS,"
              f"afade=t=in:d=0.8,afade=t=out:st={bed_len - 0.9:.3f}:d=0.9[a1]")
    ly_off = scenes[li]["src"] - edit["lyrics_song"]["start"]
    ly_len = total - t_ly + 0.3
    ms = int(t_ly * 1000)
    fc.append(f"[{n + 1}:a]atrim=start={ly_off:.3f}:duration={ly_len:.3f},asetpts=PTS-STARTPTS,"
              f"afade=t=in:d=0.4,afade=t=out:st={ly_len - 2.2:.3f}:d=2.2,adelay={ms}|{ms}[a2]")
    fc.append(f"[a1][a2]amix=inputs=2:duration=longest:normalize=0,"
              f"loudnorm=I=-15:TP=-1.5:LRA=11,aresample=48000,atrim=duration={total:.3f}[a]")

    out = os.path.join(args.out, f"material-eleven-{cut}.mp4")
    cmd = inputs + ["-i", args.bed, "-i", args.lyrics_song, "-filter_complex", ";".join(fc),
                    "-map", "[v]", "-map", "[a]", "-c:v", "libx264", "-preset", "slow",
                    "-crf", str(crf), "-pix_fmt", "yuv420p", "-color_range", "tv",
                    "-colorspace", "bt709", "-color_primaries", "bt709", "-color_trc", "bt709"]
    if maxrate:
        cmd += ["-maxrate", maxrate, "-bufsize", maxrate]
    cmd += ["-c:a", "aac", "-b:a", "128k", "-movflags", "+faststart", out]
    ff(cmd)
    muted = os.path.join(args.out, f"material-eleven-{cut}-muted.mp4")
    ff(["-i", out, "-map", "0:v", "-c:v", "copy", "-movflags", "+faststart", muted])
    for f in (out, muted):
        report(f)


def render_gif(edit):
    """Frameless, muted loop of a few shots, as GIF and WebP."""
    parts = []
    for i, (s, d) in enumerate(edit["gif"]):
        p = os.path.join(WORK, f"gif_{i}_{key(s, d, args.recording, args.fix_obs_range)}.mp4")
        if not os.path.exists(p):
            ff(["-ss", str(s), "-t", str(d), "-i", args.recording, "-vf",
                f"{source_filter()}fps=15,scale=640:360:flags=lanczos,format=yuv444p",
                "-c:v", "libx264", "-crf", "6", "-an", p])
        parts.append((p, d))
    inputs, fc, prev, acc = [], [], "0:v", parts[0][1]
    for p, _ in parts:
        inputs += ["-i", p]
    for i in range(1, len(parts)):
        off = acc - 0.2
        fc.append(f"[{prev}][{i}:v]xfade=transition=fade:duration=0.2:offset={off:.3f}[g{i}]")
        prev, acc = f"g{i}", off + parts[i][1]
    gif = os.path.join(args.out, "material-eleven.gif")
    ff(inputs + ["-filter_complex", ";".join(fc + [
        f"[{prev}]split[a][b];[a]palettegen=max_colors=192:stats_mode=diff[p];"
        f"[b][p]paletteuse=dither=sierra2_4a:diff_mode=rectangle[v]"]),
        "-map", "[v]", "-loop", "0", gif])
    webp = os.path.join(args.out, "material-eleven.webp")
    ff(inputs + ["-filter_complex", ";".join(fc + [f"[{prev}]format=yuv420p[v]"]),
                 "-map", "[v]", "-c:v", "libwebp", "-quality", "80", "-loop", "0", webp])
    for f in (gif, webp):
        report(f)


def report(path):
    d = probe_duration(path)
    print(f"{os.path.basename(path)}: " + (f"{d:.1f} s, " if d else "")
          + f"{os.path.getsize(path) / 1e6:.2f} MB")


def main():
    global args, WORK
    ap = argparse.ArgumentParser(description="Build the Material Eleven promo videos.")
    ap.add_argument("--recording", required=True, help="OBS recording of the console")
    ap.add_argument("--edit", required=True, help="edit file, e.g. tools/promo/edits/v3.5.json")
    ap.add_argument("--out", required=True, help="output folder (scenes are cached in <out>/work)")
    ap.add_argument("--bed", help="background music for the videos")
    ap.add_argument("--lyrics-song", help="the song playing in the lyrics scene")
    ap.add_argument("--avatar", help="square image for the signature in the outro (optional)")
    ap.add_argument("--fix-obs-range", action="store_true",
                    help="treat the recording as full range (OBS range mismatch)")
    ap.add_argument("--only", choices=["github", "full", "gif"], action="append",
                    help="build only these outputs (repeatable); default: all")
    args = ap.parse_args()

    only = args.only or ["github", "full", "gif"]
    if any(o in only for o in ("github", "full")) and not (args.bed and args.lyrics_song):
        ap.error("the github and full videos need --bed and --lyrics-song")
    for p in (args.recording, args.edit, args.bed, args.lyrics_song, args.avatar):
        if p and not os.path.isfile(p):
            ap.error(f"not found: {p}")

    with open(args.edit, encoding="utf-8") as f:
        edit = json.load(f)
    WORK = os.path.join(args.out, "work")
    os.makedirs(WORK, exist_ok=True)

    for cut in ("github", "full"):
        if cut in only:
            render(edit, cut)
    if "gif" in only:
        render_gif(edit)


if __name__ == "__main__":
    main()
