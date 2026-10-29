#!/usr/bin/env python3
"""Page pictures for the README and the post images: the skin's own layout (layout.conf) drawn with real values, the way
the plugin shows it (no editor outlines). Screen text and lamps come from a 'state' dict below (what a fresh machine on
the factory card shows). Output: docs/img/keys.png, drums.png, synth.png.

    tools/make_previews.py
"""
import os, re, shlex, sys, json
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
FONT_LIVE = "art/fonts/TitilliumWeb-SemiBold.ttf"   # MPC OS draws live text in Titillium Web
if not os.path.exists(FONT_LIVE):
    FONT_LIVE = "art/fonts/ChakraPetch-SemiBold.ttf"

STATE = {   # a fresh machine on the factory card, Home page
    "scr_t1": "KIT 1", "scr_t2": "BANK 1   PATT 1", "scr_t3": "TEMPO", "scr_t4": "116", "scr_t5": "STOP  KIT / PATT  BD",
    "lv_bd": 1, "ls_1": 1, "lm_kit": 1,
}
DEFAULTS = {p["key"]: p.get("default", 0) for p in json.load(open("params.json"))["params"]}


def parse(line):
    parts = shlex.split(line)
    kind = parts[0]
    kv = {}
    for p in parts[1:]:
        if "=" in p:
            k, v = p.split("=", 1)
            kv[k] = v
    return kind, kv


def render_tab(lines):
    im = Image.new("RGBA", (1280, 628 + 86), (18, 18, 18, 255))   # layout.conf counts from under the MPC header (Y0 = 86)
    for line in lines:
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("[") or "=" in line.split()[0]:
            continue
        kind, kv = parse(line)
        if kind == "art":
            when = kv.get("when")
            if when:
                key, opt = when.split(":")
                if int(STATE.get(key, DEFAULTS.get(key, 0))) != int(opt):
                    continue
            pic = Image.open(kv["file"]).convert("RGBA").resize((int(kv["w"]), int(kv["h"])), Image.LANCZOS)
            im.alpha_composite(pic, (int(kv["x"]), int(kv["y"])))
        elif kind == "text":
            px = float(kv["size"]) * 10 / 0.91 * 0.91   # size is px/10 after the renderer's 10 % allowance
            f = ImageFont.truetype(kv.get("fontfile", FONT_LIVE), round(px * 1.1))
            d = ImageDraw.Draw(im)
            col = tuple(int(kv.get("color", "e9e9e6")[i:i + 2], 16) for i in (0, 2, 4))
            cx, cy = int(kv["cx"]), int(kv["cy"])
            align = kv.get("align", "center")
            anchor = {"center": "mm", "left": "lm", "right": "rm"}[align]
            d.text((cx, cy), kv["label"], font=f, fill=col, anchor=anchor)
        elif kind == "readout":
            txt = STATE.get(kv["key"], "")
            if not txt:
                continue
            f = ImageFont.truetype(FONT_LIVE, int(kv["tsize"]))
            d = ImageDraw.Draw(im)
            col = tuple(int(kv["tcolor"][i:i + 2], 16) for i in (0, 2, 4))
            cx, cy, w = int(kv["cx"]), int(kv["cy"]), int(kv["w"])
            x = cx - w // 2 if kv.get("talign") == "left" else cx
            d.text((x, cy), txt, font=f, fill=col, anchor="lm" if kv.get("talign") == "left" else "mm")
        elif kind == "toggle":
            on = int(STATE.get(kv["key"], DEFAULTS.get(kv["key"], 0))) == 1
            pic = Image.open(kv["img_on"] if on else kv["img"]).convert("RGBA").resize((int(kv["w"]), int(kv["h"])), Image.LANCZOS)
            im.alpha_composite(pic, (int(kv["cx"]) - int(kv["w"]) // 2, int(kv["cy"]) - int(kv["h"]) // 2))
        elif kind == "slider_v":
            w, h = int(kv["w"]), int(kv["h"])
            cap = Image.open(kv["img"]).convert("RGBA")
            th = round(cap.height * w / cap.width)
            cap = cap.resize((w, th), Image.LANCZOS)
            v = float(STATE.get(kv["key"], DEFAULTS.get(kv["key"], 0))) / 127.0
            x = int(kv["cx"]) - w // 2
            top = int(kv["cy"]) - h // 2
            y = top + round((1 - v) * (h - th))
            im.alpha_composite(cap, (x, y))
    return im.crop((0, 86, 1280, 86 + 628)).convert("RGB")


def main():
    tabs, cur = {}, None
    for line in open("layout.conf"):
        m = re.match(r"\[tab (\w+)\]", line.strip())
        if m:
            cur = m.group(1); tabs[cur] = []
        elif cur:
            tabs[cur].append(line)
    os.makedirs("docs/img", exist_ok=True)
    for tab, name in (("KEYS", "keys"), ("DRUMS", "drums"), ("SYNTH", "synth")):
        render_tab(tabs[tab]).save("docs/img/%s.png" % name, optimize=True)
        print("docs/img/%s.png" % name)


if __name__ == "__main__":
    main()
