#!/usr/bin/env python3
"""Pack a BlastBeats SD card folder (or the factory-card zip) into card/factory.bin for the plugin.

    tools/pack_card.py <card folder or blastbeats-factory-card.zip> [-o card/factory.bin]

The file is a zlib stream of the plugin's own card image ('DIN1': directory list + files, the format hw::sd_save
writes), which hw.cpp inflates with the firmware's vendored miniz at boot. Skipped: the firmware updater
(SD_USB_MSC_Application.bin), macOS litter (__MACOSX, .DS_Store) and copyKit.zip (a PC-side helper, not card data).
"""
import io, os, struct, sys, zipfile, zlib

SKIP_NAMES = {".DS_Store", "SD_USB_MSC_Application.bin", "copyKit.zip"}


def read_tree(src):
    files = {}
    if os.path.isdir(src):
        for root, _, names in os.walk(src):
            for n in names:
                rel = os.path.relpath(os.path.join(root, n), src).replace(os.sep, "/")
                if n in SKIP_NAMES or rel.startswith("__MACOSX/"):
                    continue
                with open(os.path.join(root, n), "rb") as f:
                    files["/" + rel] = f.read()
    else:
        with zipfile.ZipFile(src) as z:
            for info in z.infolist():
                if info.is_dir():
                    continue
                rel = info.filename
                if rel.startswith("__MACOSX/") or os.path.basename(rel) in SKIP_NAMES:
                    continue
                files["/" + rel.lstrip("/")] = z.read(info)
    return files


def pack(files):
    dirs = []
    for path in sorted(files):
        parts = path.split("/")[1:-1]
        cur = ""
        for p in parts:
            cur += "/" + p
            if cur not in dirs:
                dirs.append(cur)
    out = io.BytesIO()
    out.write(struct.pack("<I", 0x44494E31))
    out.write(struct.pack("<I", len(dirs)))
    for d in dirs:
        b = d.encode(); out.write(struct.pack("<I", len(b))); out.write(b)
    out.write(struct.pack("<I", len(files)))
    for path in sorted(files):
        b = path.encode(); data = files[path]
        out.write(struct.pack("<I", len(b))); out.write(b)
        out.write(struct.pack("<I", len(data))); out.write(data)
    return out.getvalue()


def main():
    src = sys.argv[1]
    out = sys.argv[sys.argv.index("-o") + 1] if "-o" in sys.argv else "card/factory.bin"
    files = read_tree(src)
    raw = pack(files)
    z = zlib.compress(raw, 9)
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    with open(out, "wb") as f:
        f.write(z)
    vgm = sum(len(v) for k, v in files.items() if k.startswith("/VGM/"))
    print(f"{out}: {len(files)} files, {len(raw)/1e6:.1f} MB card ({vgm/1e6:.1f} MB VGM) -> {len(z)/1e6:.2f} MB")


if __name__ == "__main__":
    main()
