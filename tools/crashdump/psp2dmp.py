#!/usr/bin/env python3
"""Reads a PS Vita crash dump (psp2dmp) and shows why the app stopped.

    python3 psp2dmp.py DUMP [DUMP...] [--elf build/ElevenMPV]

Shows, in this order: the app the dump belongs to, its threads with their stop
reason, the registers of every thread that stopped with a fault, and the module,
segment and offset each register points into. Given the ELF of the same build,
it also resolves pc and lr to a function with arm-vita-eabi-addr2line.

A psp2dmp is a 32-bit ELF core file, usually gzipped, whose PT_NOTE segments
hold named notes. The layouts below were read from real dumps of this console's
firmware. The script checks them as it reads and says so when something does
not fit, instead of printing numbers it cannot trust.

Python 3 standard library only.
"""

import argparse
import gzip
import os
import shutil
import struct
import subprocess
import sys

# --- what this script expects ---------------------------------------------

APP_TITLE_ID = "ELEVENMPV"     # dumps of any other app are reported and skipped
APP_MODULE = "ElevenMPV"       # module whose code segment matches the build ELF
ADDR2LINE = "arm-vita-eabi-addr2line"

# --- psp2dmp layout --------------------------------------------------------

PT_NOTE = 4
PT_LOAD = 1
PF_X = 1

# Every list note (THREAD_INFO, THREAD_REG_INFO, MODULE_INFO) starts with an
# unused u32 and the entry count.
LIST_HEADER = 8

# APP_INFO: u32, u32, then the title ID and the app name as two C strings.
APP_INFO_STRINGS = 8

# THREAD_INFO entry: u32 entry size, u32 UID, char name[32], ... stop reason.
THREAD_UID = 0x04
THREAD_NAME = 0x08
THREAD_NAME_LEN = 32
THREAD_STOP_REASON = 0x74

# THREAD_REG_INFO entry: u32 entry size, u32 UID, then r0-r12, sp, lr, pc, cpsr.
REG_UID = 0x04
REG_FIRST = 0x08
REG_NAMES = ["r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
             "sp", "lr", "pc", "cpsr"]

# MODULE_INFO entry: u32, u32 UID, ..., char name[32] at 0x24, u32 segment count
# at 0x4c, the segments from 0x50 as (u32, attributes, vaddr, size, alignment),
# then four u32 that close the entry. Zero padding may follow the last entry.
MODULE_UID = 0x04
MODULE_NAME = 0x24
MODULE_NAME_LEN = 32
MODULE_SEG_COUNT = 0x4C
MODULE_SEGS = 0x50
MODULE_SEG_SIZE = 0x14
MODULE_TAIL = 0x10
MODULE_MAX_SEGS = 8
SEG_ATTR_KIND = {0x5: "code", 0x6: "data"}  # low 16 bits of the attributes

STOP_REASONS = {
    0x00000: "running",
    0x30004: "data abort",
}


class FormatError(Exception):
    pass


# --- reading ---------------------------------------------------------------

def load(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:2] == b"\x1f\x8b":
        data = gzip.decompress(data)
    return data


def elf_program_headers(data, what):
    if data[:4] != b"\x7fELF":
        raise FormatError(f"{what} is not an ELF file")
    if data[4] != 1 or data[5] != 1:
        raise FormatError(f"{what} is not a 32-bit little-endian ELF")
    e_phoff, = struct.unpack_from("<I", data, 0x1C)
    e_phentsize, e_phnum = struct.unpack_from("<HH", data, 0x2A)
    if e_phoff + e_phnum * e_phentsize > len(data):
        raise FormatError(f"{what}: program headers run past the end of the file")
    for i in range(e_phnum):
        yield struct.unpack_from("<8I", data, e_phoff + i * e_phentsize)


def read_notes(data):
    notes = {}
    for p_type, p_offset, _, _, p_filesz, _, _, _ in elf_program_headers(data, "the dump"):
        if p_type != PT_NOTE:
            continue
        end = p_offset + p_filesz
        if end > len(data):
            raise FormatError("a PT_NOTE segment runs past the end of the dump")
        pos = p_offset
        while pos + 12 <= end:
            namesz, descsz, _ = struct.unpack_from("<III", data, pos)
            desc = pos + 12 + ((namesz + 3) & ~3)
            if desc + descsz > end:
                raise FormatError("a note runs past the end of its PT_NOTE segment")
            name = data[pos + 12:pos + 12 + namesz].rstrip(b"\0").decode("ascii", "replace")
            notes[name] = data[desc:desc + descsz]
            pos = desc + ((descsz + 3) & ~3)
    return notes


def c_string(raw, what):
    text = raw.split(b"\0", 1)[0]
    if not all(32 <= c < 127 for c in text):
        raise FormatError(f"{what} is not printable: {text!r}")
    return text.decode("ascii")


def u32(buf, off, what):
    if off + 4 > len(buf):
        raise FormatError(f"{what} runs past the end of the note")
    return struct.unpack_from("<I", buf, off)[0]


def list_entries(buf, note, min_size):
    """Splits a THREAD_INFO style note, whose entries start with their size."""
    count = u32(buf, 4, f"{note} header")
    entries, pos = [], LIST_HEADER
    for i in range(count):
        size = u32(buf, pos, f"{note} entry {i}")
        if size < min_size or pos + size > len(buf):
            raise FormatError(f"{note} entry {i} has size {size:#x}, which does not fit "
                              f"(minimum {min_size:#x}, {len(buf) - pos:#x} bytes left)")
        entries.append(buf[pos:pos + size])
        pos += size
    if pos != len(buf):
        raise FormatError(f"{note} declares {count} entries, but {len(buf) - pos:#x} bytes are left over")
    return entries


def parse_app(buf):
    strings = buf[APP_INFO_STRINGS:].split(b"\0")
    if len(strings) < 2:
        raise FormatError("APP_INFO does not hold a title ID and a name")
    return c_string(strings[0], "APP_INFO title ID"), c_string(strings[1], "APP_INFO name")


def parse_threads(buf):
    threads = []
    for i, e in enumerate(list_entries(buf, "THREAD_INFO", THREAD_STOP_REASON + 4)):
        threads.append({
            "uid": u32(e, THREAD_UID, "THREAD_INFO UID"),
            "name": c_string(e[THREAD_NAME:THREAD_NAME + THREAD_NAME_LEN], f"THREAD_INFO entry {i} name"),
            "stop": u32(e, THREAD_STOP_REASON, "THREAD_INFO stop reason"),
        })
    return threads


def parse_regs(buf):
    regs = {}
    for e in list_entries(buf, "THREAD_REG_INFO", REG_FIRST + 4 * len(REG_NAMES)):
        values = struct.unpack_from(f"<{len(REG_NAMES)}I", e, REG_FIRST)
        regs[u32(e, REG_UID, "THREAD_REG_INFO UID")] = dict(zip(REG_NAMES, values))
    return regs


def parse_modules(buf):
    count = u32(buf, 4, "MODULE_INFO header")
    modules, pos = [], LIST_HEADER
    for i in range(count):
        nseg = u32(buf, pos + MODULE_SEG_COUNT, f"MODULE_INFO entry {i}")
        if nseg > MODULE_MAX_SEGS:
            raise FormatError(f"MODULE_INFO entry {i} declares {nseg} segments")
        size = MODULE_SEGS + nseg * MODULE_SEG_SIZE + MODULE_TAIL
        if pos + size > len(buf):
            raise FormatError(f"MODULE_INFO entry {i} runs past the end of the note")
        segs = []
        for j in range(nseg):
            _, attr, vaddr, memsz, _ = struct.unpack_from("<5I", buf, pos + MODULE_SEGS + j * MODULE_SEG_SIZE)
            kind = SEG_ATTR_KIND.get(attr & 0xFFFF)
            if kind is None:
                raise FormatError(f"MODULE_INFO entry {i} segment {j} has attributes {attr:#x}")
            segs.append({"index": j, "kind": kind, "vaddr": vaddr, "size": memsz})
        modules.append({
            "uid": u32(buf, pos + MODULE_UID, "MODULE_INFO UID"),
            "name": c_string(buf[pos + MODULE_NAME:pos + MODULE_NAME + MODULE_NAME_LEN],
                             f"MODULE_INFO entry {i} name"),
            "segs": segs,
        })
        pos += size
    if any(buf[pos:]):
        raise FormatError(f"MODULE_INFO declares {count} modules, but non-zero bytes follow them")
    return modules


# --- showing ---------------------------------------------------------------

def stop_reason(code):
    return STOP_REASONS.get(code, "unknown")


def locate(modules, addr):
    for m in modules:
        for s in m["segs"]:
            if s["vaddr"] <= addr < s["vaddr"] + s["size"]:
                return m, s
    return None, None


def find_addr2line(given):
    if given:
        return given
    found = shutil.which(ADDR2LINE)
    if found:
        return found
    vitasdk = os.environ.get("VITASDK")
    if vitasdk:
        candidate = os.path.join(vitasdk, "bin", ADDR2LINE)
        if os.access(candidate, os.X_OK):
            return candidate
    return None


def elf_code_base(path):
    for p_type, _, p_vaddr, _, _, _, p_flags, _ in elf_program_headers(load(path), path):
        if p_type == PT_LOAD and p_flags & PF_X:
            return p_vaddr
    raise FormatError(f"{path} has no executable segment")


def resolve(addr2line, elf, addrs):
    """Returns one list of (function, location) per address, inlined frames first."""
    out = subprocess.run([addr2line, "-f", "-i", "-C", "-a", "-e", elf] + [f"{a:#x}" for a in addrs],
                         capture_output=True, text=True, check=True).stdout.splitlines()
    frames, current = [], None
    i = 0
    while i < len(out):
        if out[i].startswith("0x"):
            current = []
            frames.append(current)
            i += 1
        else:
            current.append((out[i], out[i + 1] if i + 1 < len(out) else "??:?"))
            i += 2
    return frames


def show_section(title, func, *args):
    try:
        return func(*args)
    except FormatError as e:
        print(f"{title}: cannot read it ({e}); skipped")
        return None


def show_dump(path, args):
    try:
        notes = read_notes(load(path))
    except (OSError, EOFError, gzip.BadGzipFile, FormatError) as e:
        print(f"psp2dmp.py: {path}: {e}")
        return 1

    for note in ("APP_INFO", "THREAD_INFO", "THREAD_REG_INFO", "MODULE_INFO"):
        if note not in notes:
            print(f"psp2dmp.py: {path}: the dump has no {note} note")
            return 1

    app = show_section("APP_INFO", parse_app, notes["APP_INFO"])
    if app is None:
        return 1
    title_id, app_name = app
    print(f"App: {title_id}  {app_name}")
    if title_id != APP_TITLE_ID:
        print(f"Not {APP_TITLE_ID}: this dump belongs to another app. Nothing else to read.")
        return 0

    threads = show_section("THREAD_INFO", parse_threads, notes["THREAD_INFO"])
    regs = show_section("THREAD_REG_INFO", parse_regs, notes["THREAD_REG_INFO"]) or {}
    modules = show_section("MODULE_INFO", parse_modules, notes["MODULE_INFO"]) or []
    if threads is None:
        return 1

    print(f"\nThreads ({len(threads)}):")
    for t in threads:
        mark = "  <-- stopped" if t["stop"] else ""
        print(f"  {t['uid']:#010x}  {t['name']:<32} {t['stop']:#07x} {stop_reason(t['stop'])}{mark}")

    faulted = [t for t in threads if t["stop"]]
    if not faulted:
        print("\nNo thread has a stop reason other than 0.")
    asked = []  # (thread, register, address) to resolve with the ELF
    for t in faulted:
        print(f"\nThread {t['uid']:#010x} {t['name']}: {stop_reason(t['stop'])} ({t['stop']:#x})")
        r = regs.get(t["uid"])
        if r is None:
            print("  THREAD_REG_INFO has no registers for it")
            continue
        for name in REG_NAMES:
            value = r[name]
            where = ""
            if name != "cpsr":
                m, s = locate(modules, value & ~1)
                if m:
                    where = f"  {m['name']} {s['kind']} +{(value & ~1) - s['vaddr']:#x}"
                    if name in ("pc", "lr") and m["name"] == args.module and s["kind"] == "code":
                        asked.append((t, name, value & ~1, s))
            print(f"  {name:<4} {value:#010x}{where}")

    if not args.elf:
        if asked:
            print(f"\nPass --elf with the {args.module} ELF of this build to resolve pc and lr to functions.")
        return 0
    if not asked:
        print(f"\nNeither pc nor lr points into the {args.module} code segment; nothing to resolve.")
        return 0

    base = show_section(args.elf, elf_code_base, args.elf)
    if base is None:
        return 1
    elf_addrs = [base + (addr - s["vaddr"]) for _, _, addr, s in asked]
    tool = find_addr2line(args.addr2line)
    print(f"\nIn {args.elf} (code at {base:#x}):")
    if tool is None:
        print(f"  {ADDR2LINE} not found (PATH, $VITASDK/bin); showing ELF addresses only")
        for (t, name, _, _), a in zip(asked, elf_addrs):
            print(f"  {t['name']} {name:<2} {a:#010x}")
        return 0
    try:
        frames = resolve(tool, args.elf, elf_addrs)
    except (OSError, subprocess.CalledProcessError) as e:
        print(f"  {ADDR2LINE} failed: {e}")
        return 1
    for (t, name, _, _), a, f in zip(asked, elf_addrs, frames):
        first = True
        for func, loc in f:
            label = f"{t['name']} {name:<2} {a:#010x}" if first else " " * (len(t["name"]) + 15) + "inlined in"
            print(f"  {label}  {func}  {loc}")
            first = False
    return 0


def main():
    ap = argparse.ArgumentParser(description="Show why a PS Vita app crashed, from its psp2dmp.")
    ap.add_argument("dumps", nargs="+", metavar="dump", help="psp2dmp file, gzipped or not")
    ap.add_argument("--elf", help=f"unstripped ELF of the same build (e.g. build/{APP_MODULE})")
    ap.add_argument("--addr2line", help=f"path to {ADDR2LINE} (default: PATH, then $VITASDK/bin)")
    ap.add_argument("--module", default=APP_MODULE, help=f"module the ELF belongs to (default: {APP_MODULE})")
    args = ap.parse_args()

    status = 0
    for i, path in enumerate(args.dumps):
        if len(args.dumps) > 1:
            if i:
                print()
            print(f"=== {path}")
        status |= show_dump(path, args)
    return status


if __name__ == "__main__":
    sys.exit(main())
