#!/usr/bin/env python3
"""Builds the DINXXL skin pictures in art/ from the panel photos in art/src (the user's own panel art).

    python3 tools/make_art.py

Per page: the photo with its printed labels removed (the labels are live text widgets in layout.conf, Chakra Petch),
every painted fader cap removed (the caps are moving sprites), every lamp drawn OFF (the lamps are sprites that
light up while the machine lights them), and the vertical captions baked in. Also the lamp, cap, latch-ring and
screen pictures (8 wave symbols, 5 algorithm diagrams).
"""
import os
import sys

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import art_lib  # noqa: E402
import spec  # noqa: E402
import panel_labels as PL  # noqa: E402

ART = os.path.join(HERE, "..", "art")
SRC = os.path.join(ART, "src")
FONT = os.path.join(ART, "fonts", "ChakraPetch-Medium.ttf")
GREEN = (0x1a, 0xb4, 0x63)
SS = 4   # supersampling for the drawn pictures


def save_bgr(path, im):
    cv2.imwrite(os.path.join(ART, path), im)


def circle_alpha(h, w, cx, cy, r, feather=2.0):
    yy, xx = np.mgrid[0:h, 0:w]
    return np.clip((r - np.hypot(xx - cx, yy - cy)) / feather + 0.5, 0, 1)


# ------------------------------------------------------------------------------------------------ keys page
def make_keys():
    DY = 26
    # the panel as printed; only the lamps are recoloured (red in the photo -> the machine's green, lit by the firmware)
    im = cv2.imread(os.path.join(SRC, "keys.png"))
    im, cents = art_lib.lamps_v3(im)
    R = 17
    # the lamp disc: take an OFF and an ON one (a lamp on a black key) from the photo; every lamp is the same part
    off = next(c for c in cents if not c[2] and abs(c[0] - 144) < 3 and abs(c[1] - 398) < 3)
    on = next(c for c in cents if c[2] and abs(c[0] - 66) < 3 and abs(c[1] - 399) < 3)

    def disc(c):
        x, y = c[0], c[1]
        return im[y - R:y + R + 1, x - R:x + R + 1].copy()
    d_off, d_on = disc(off), disc(on)
    hv0 = cv2.cvtColor(d_off, cv2.COLOR_BGR2HSV).astype(np.float32)
    hv0[..., 2] *= 0.30          # an unlit lamp: the same glass, dark
    hv0[..., 1] *= 0.85
    d_off = cv2.cvtColor(hv0.astype(np.uint8), cv2.COLOR_HSV2BGR)
    al = circle_alpha(2 * R + 1, 2 * R + 1, R, R, 15.5)[..., None]
    out = im.astype(np.float32)
    for c in cents:
        x, y = c[0], c[1]
        reg = out[y - R:y + R + 1, x - R:x + R + 1]
        out[y - R:y + R + 1, x - R:x + R + 1] = reg * (1 - al) + d_off.astype(np.float32) * al
    # the REC lamp is red: its centre was measured on the photo (284, 192). OFF = the lit disc darkened.
    rx, ry, RR = 284, 192, 14
    al_r = circle_alpha(2 * RR + 1, 2 * RR + 1, RR, RR, 12.5)[..., None]
    rec_on = out[ry - RR:ry + RR + 1, rx - RR:rx + RR + 1].copy()
    hv = cv2.cvtColor(np.clip(rec_on, 0, 255).astype(np.uint8), cv2.COLOR_BGR2HSV).astype(np.float32)
    hv[..., 2] *= 0.38
    rec_off = cv2.cvtColor(hv.astype(np.uint8), cv2.COLOR_HSV2BGR).astype(np.float32)
    reg = out[ry - RR:ry + RR + 1, rx - RR:rx + RR + 1]
    out[ry - RR:ry + RR + 1, rx - RR:rx + RR + 1] = reg * (1 - al_r) + rec_off * al_r
    out = np.clip(out, 0, 255).astype(np.uint8)
    # the screen moves right to make room for a real key (HOLD, the user's own key picture) beside the mode keys
    DX = spec.SCREEN_DX
    scr = out[0:336, 370:1152].copy()                        # the bezel with its own drop shadow
    plain = out[0:336, 338:370]                                # plain panel between the mode keys and that shadow
    fill = np.concatenate([plain[:, ::-1], plain], axis=1)[:, :DX]
    out[0:336, 370:370 + DX] = fill
    out[0:336, 370 + DX:1152 + DX] = scr
    # bring the keys up: the printed bottom captions sit on the page's last pixels, so a plain band of panel between the
    # screen and the keys (rows 336..362) is moved below them; the photo's cut-off last rows go with it
    band = out[336:336 + DY]
    body = np.vstack([out[:336], out[336 + DY:626]])
    filler = np.vstack([band, band])[:628 - body.shape[0]]
    out = np.vstack([body, filler])
    # sprites (RGBA, discs with a soft edge)
    def sprite(d, name):
        rgba = np.dstack([d, (al[..., 0] * 255).astype(np.uint8)])
        cv2.imwrite(os.path.join(ART, name), rgba)
    sprite(d_on, "lamp_on.png")
    rgba = np.dstack([np.clip(rec_on, 0, 255).astype(np.uint8), (al_r[..., 0] * 255).astype(np.uint8)])
    cv2.imwrite(os.path.join(ART, "lamp_rec_on.png"), rgba)
    # the page, with the captions that are not live text (rotated ones and the group lines)
    img = Image.fromarray(cv2.cvtColor(out, cv2.COLOR_BGR2RGB)).convert("RGBA")
    d = ImageDraw.Draw(img)
    for (x0, x1, y) in PL.KEYS_LINES:   # the edit / trick / rate / volume group lines (the photo cuts them off)
        d.line([(x0, y), (x1, y)], fill=(220, 220, 216), width=3)
    img.convert("RGB").save(os.path.join(ART, "bg_keys.png"))
    return (rx, ry)


def rot_text(img, txt, x, y, font, fill):
    """Text reading bottom to top, baseline at x and centred on y (like SVG's translate(x,y) rotate(-90))."""
    l, t, r, b = font.getbbox(txt)
    w, h = r - l + 8, font.size * 2
    tmp = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(tmp).text((4 - l, h // 2), txt, font=font, fill=fill, anchor="ls")
    tmp = tmp.rotate(90, expand=True)
    # after rotation the baseline is the column at h/2 from the left
    img.alpha_composite(tmp, (int(round(x - h / 2)), int(round(y - w / 2))))


# ------------------------------------------------------------------------------------------------ fader pages
def find_caps(img, cx, row):
    """Rows of the painted cap in the slot at column cx: the rows brighter than the slot's own dark floor."""
    y0, y1 = spec.ROW_TOP[row]
    g = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY).astype(np.float32)
    ys = np.arange(y0 + 4, y1 - 6)
    m = g[ys, cx - 8:cx + 9].mean(axis=1)
    floor = np.percentile(m, 20)
    hot = np.nonzero(m > floor + 9)[0]
    if len(hot) == 0:
        return None
    runs, start, prev = [], hot[0], hot[0]
    for v in hot[1:]:
        if v - prev > 5:
            runs.append((start, prev)); start = v
        prev = v
    runs.append((start, prev))
    a, b = max(runs, key=lambda r: r[1] - r[0])
    return int(ys[a]) - 5, int(ys[b]) + 5


def erase_caps(img, faders):
    img, _ = _erase_caps(img, faders)      # a second pass takes what the first left (stripes at a slot's edge)
    return _erase_caps(img, faders)


def rebuild_slots(img, faders):
    """Each slot is redrawn clean: the slot's own cross-section (the median of its cap-free lower rows) laid down the whole
    length, with the panel's own rounded bottom end kept and mirrored for the top. Nothing is patched over a cap; the
    track is one uniform piece and the moving cap sprite sits on it like the real part on the real track."""
    out = img.copy()
    caps = {}
    for key, _, _, cx, row in faders:
        y0, y1 = spec.ROW_TOP[row]
        caps[key] = find_caps(img, cx, row)
        xa, xb = cx - 40, cx + 40                         # half the fader pitch each side: the band is rebuilt whole
        end = 14                                          # the rounded end of the slot
        clean = img[y1 - 70:y1 - end, xa:xb].astype(np.float32)   # below the lowest cap position, above the end
        profile = np.median(clean, axis=0)                # one row: the slot's cross-section, glossy edges included
        body = np.repeat(profile[None, :, :], (y1 - end) - (y0 + end), axis=0)
        # keep the gentle top-to-bottom shading of the original track (its lower part is a touch lighter)
        out[y0 + end:y1 - end, xa:xb] = np.clip(body, 0, 255).astype(np.uint8)
        bottom = img[y1 - end:y1 + 1, xa:xb]
        out[y0:y0 + end + 1, xa:xb] = bottom[::-1]       # the top end: the bottom end mirrored
    # the printed dots between a tune/fall pair sit mid-height in the band: put them back from the original
    for (mx, base) in getattr(spec, "PAIR_DOTS", []):
        y0, y1 = spec.ROW_TOP["t" if base < 400 else "b"]
        my = (y0 + y1) // 2
        out[my - 10:my + 11, mx - 10:mx + 11] = img[my - 10:my + 11, mx - 10:mx + 11]
    return out, caps


def make_fader_page(name, bg_key, faders, rot, labels_rows):
    # the panel exactly as printed (captions, rotated group names, the triangles between tune/fall): nothing is wiped
    # or redrawn; only the fader slots are rebuilt so the moving caps have a clean track
    im = cv2.imread(os.path.join(SRC, name + ".png"))
    im, caps = rebuild_slots(im, faders)
    img = Image.fromarray(cv2.cvtColor(im, cv2.COLOR_BGR2RGB)).convert("RGBA")
    img.convert("RGB").save(os.path.join(ART, bg_key))
    return caps


# ------------------------------------------------------------------------------------------------ sprites
def make_cap():
    cap = Image.open(os.path.join(SRC, "cap.png")).convert("RGBA")
    cap.resize((26, 56), Image.LANCZOS).save(os.path.join(ART, "cap.png"))


def make_clear():
    Image.new("RGBA", (spec.KEY_W, spec.KEY_H), (0, 0, 0, 0)).save(os.path.join(ART, "clear.png"))
    # while a key is pressed: a faint light veil
    Image.new("RGBA", (spec.KEY_W, spec.KEY_H), (255, 255, 255, 38)).save(os.path.join(ART, "press.png"))
    # latched modifier key: a green frame round the key (drawn with a soft outer glow)
    w, h = spec.KEY_W + 8, spec.KEY_H + 8
    big = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.rounded_rectangle([3 * SS, 3 * SS, (w - 3) * SS, (h - 3) * SS], radius=7 * SS, outline=GREEN + (255,), width=4 * SS)
    big.resize((w, h), Image.LANCZOS).save(os.path.join(ART, "latch_on.png"))
    Image.new("RGBA", (w, h), (0, 0, 0, 0)).save(os.path.join(ART, "latch_off.png"))


def make_hold():
    """HOLD: the user's key picture (art/src/hold_key.png), baked into the keys page with its lamp off; the ON state is the
    same lamp_on sprite every other key uses, at the lamp's spot. The hit area is a clear button over the key."""
    hk, hl, cx, cy, w, h = spec.HOLD_KEY
    key = Image.open(os.path.join(SRC, "hold_key.png")).convert("RGBA")
    key = key.resize((w, round(key.height * w / key.width)), Image.LANCZOS)
    h = key.height
    bg = Image.open(os.path.join(ART, "bg_keys.png")).convert("RGBA")
    # the lamp in the picture: the brightest green blob in its upper half
    arr = np.array(key).astype(int)
    g = (arr[..., 1] - np.maximum(arr[..., 0], arr[..., 2])) * (arr[..., 3] > 0)
    ys, xs = np.where(g > 40)
    lx, ly = (int(xs.mean()), int(ys.mean())) if len(xs) else (w // 2, h // 4)
    # lamp off: darken the lit disc like every other off lamp on the page
    hv = cv2.cvtColor(np.array(key.convert("RGB")), cv2.COLOR_RGB2HSV).astype(np.float32)
    yy, xx = np.mgrid[0:h, 0:w]
    disc = np.hypot(xx - lx, yy - ly) < 15.5
    hv[..., 2][disc] *= 0.30
    hv[..., 1][disc] *= 0.85
    dark = Image.fromarray(cv2.cvtColor(hv.astype(np.uint8), cv2.COLOR_HSV2RGB)).convert("RGBA")
    dark.putalpha(key.split()[3])
    bg.alpha_composite(dark, (cx - w // 2, cy - h // 2))
    bg.convert("RGB").save(os.path.join(ART, "bg_keys.png"))
    spec.HOLD_LAMP = (cx - w // 2 + lx, cy - h // 2 + ly)
    with open(os.path.join(ART, "hold_lamp.txt"), "w") as f:
        f.write("%d %d %d\n" % (spec.HOLD_LAMP[0], spec.HOLD_LAMP[1], h))


def make_pad_mode():
    """The PADS switch (text, Chakra Petch, the panel's ink and accent): what the Force's pads do."""
    w, h = 300, 36
    f = ImageFont.truetype(FONT, 26)
    for name, txt in (("pads_voices.png", "PADS = VOICES"), ("pads_chrom.png", "PADS = CHROMATIC")):
        im = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        fb = ImageFont.truetype(FONT, 26 * SS)
        d.text((w * SS // 2, h * SS // 2), txt, font=fb, fill=GREEN + (255,), anchor="mm")
        im.resize((w, h), Image.LANCZOS).save(os.path.join(ART, name))


PW, PH = 240, 100


def new_canvas():
    return Image.new("RGBA", (PW * SS, PH * SS), (0, 0, 0, 0))


def finish(big, name):
    big.resize((PW, PH), Image.LANCZOS).save(os.path.join(ART, name))


def wave_points(kind, n=400):
    """The eight OPL3 waveforms, one cycle, -1..1 (the chip's own shapes)."""
    xs = np.linspace(0, 1, n, endpoint=False)
    p = xs * 2 * np.pi
    s = np.sin(p)
    first = xs < 0.5
    if kind == 0: y = s
    elif kind == 1: y = np.where(first, s, 0)
    elif kind == 2: y = np.abs(s)
    elif kind == 3: y = np.where((xs % 0.5) < 0.25, np.abs(s), 0)
    elif kind == 4: y = np.where(first, np.sin(2 * p), 0)
    elif kind == 5: y = np.where(first, np.abs(np.sin(2 * p)), 0)
    elif kind == 6: y = np.where(first, 1.0, -1.0)
    else:
        t = (xs % 0.5) * 2          # 0..1 across each half
        mag = 2.0 ** (-8 * t)       # the chip's exponential decay in each half
        y = np.where(first, mag, -mag)
    return xs, y


def make_waves():
    Image.new("RGBA", (PW, PH), (0, 0, 0, 0)).save(os.path.join(ART, "wave_0.png"))
    for k in range(8):
        big = new_canvas()
        d = ImageDraw.Draw(big)
        xs, y = wave_points(k)
        x0, x1 = 12 * SS, (PW - 12) * SS
        mid, amp = PH * SS * 0.5, PH * SS * 0.40
        # the zero line, faint
        d.line([(x0, mid), (x1, mid)], fill=(110, 110, 110, 255), width=2 * SS)
        # abs shapes sit on the zero line; the signal is drawn about it
        pts = [(x0 + (x1 - x0) * float(a), mid - amp * float(b)) for a, b in zip(xs, y)]
        pts.append((x1, mid - amp * float(y[0])))
        d.line(pts, fill=GREEN + (255,), width=int(5.5 * SS), joint="curve")
        for p in (pts[0], pts[-1]):
            d.ellipse([p[0] - 2.7 * SS, p[1] - 2.7 * SS, p[0] + 2.7 * SS, p[1] + 2.7 * SS], fill=GREEN + (255,))
        finish(big, "wave_%d.png" % (k + 1))


# algorithm diagrams: (operators per row, rows) as firmware setAlgo() wires them (include/synth.h)
ALGOS = [
    # each: list of chains; a chain is a list of operator numbers feeding left to right, the last is a carrier;
    # algo 4 also has operator 4 alone and operator 1 alone
    [[1, 2, 3, 4]],
    [[1], [2, 3, 4]],
    [[1, 2], [3, 4]],
    [[1], [2, 3], [4]],
    [[1], [2], [3], [4]],
]
CARRIER = {0: [4], 1: [1, 4], 2: [2, 4], 3: [1, 3, 4], 4: [1, 2, 3, 4]}


def make_algos():
    Image.new("RGBA", (PW, PH), (0, 0, 0, 0)).save(os.path.join(ART, "algo_0.png"))
    f = ImageFont.truetype(FONT, 22 * SS)
    for a, chains in enumerate(ALGOS):
        big = new_canvas()
        d = ImageDraw.Draw(big)
        n = len(chains)
        if a == 4:      # four operators side by side
            chains_pos = [(i, 0) for i in range(4)]
        bw = 38 * SS if n <= 2 else 30 * SS
        gap = 22 * SS
        rowh = bw + (10 * SS)
        top = (PH * SS - (n * bw + (n - 1) * 10 * SS)) / 2
        for r, chain in enumerate(chains):
            y = top + r * rowh
            if a == 4:
                # all four on one row, each straight to the output
                bw2 = 38 * SS
                y = (PH * SS - bw2) / 2
                x = 10 * SS + r * (bw2 + 14 * SS) - 0
                box(d, f, chain[0], x, y, bw2, True)
                ax = x + bw2 / 2
                d.line([(ax, y), (ax, y - 12 * SS)], fill=(200, 200, 200, 255), width=3 * SS)
                d.polygon([(ax - 5 * SS, y - 10 * SS), (ax + 5 * SS, y - 10 * SS), (ax, y - 17 * SS)], fill=(200, 200, 200, 255))
                continue
            x = 10 * SS
            for i, op in enumerate(chain):
                carrier = op in CARRIER[a]
                box(d, f, op, x, y, bw, carrier)
                nx = x + bw
                if i < len(chain) - 1:
                    arrow(d, nx, y + bw / 2, nx + gap)
                    x = nx + gap
                else:
                    # output arrow
                    arrow(d, nx, y + bw / 2, nx + 16 * SS, out=True)
                    x = nx
        finish(big, "algo_%d.png" % (a + 1))


def box(d, f, op, x, y, s, carrier):
    fill = GREEN + (255,) if carrier else (104, 104, 104, 255)
    ink = (14, 14, 14, 255) if carrier else (240, 240, 240, 255)
    d.rounded_rectangle([x, y, x + s, y + s], radius=4 * SS, fill=fill)
    d.text((x + s / 2, y + s / 2 + 1 * SS), str(op), font=f, fill=ink, anchor="mm")


def arrow(d, x0, y, x1, out=False):
    col = (200, 200, 200, 255)
    d.line([(x0, y), (x1 - 5 * SS, y)], fill=col, width=3 * SS)
    d.polygon([(x1 - 8 * SS, y - 5 * SS), (x1, y), (x1 - 8 * SS, y + 5 * SS)], fill=col)


def main():
    make_clear()
    make_pad_mode()
    make_cap()
    make_waves()
    make_algos()
    rec = make_keys()
    make_hold()                    # bakes into bg_keys.png: after make_keys
    print("REC lamp centre (page, before shift):", rec)
    dc = make_fader_page("drum", "bg_drum.png", spec.DRUM_FADERS, PL.DRUM_ROTATED, PL.DRUM_PAIRS)
    oc = make_fader_page("op", "bg_synth.png", spec.SYNTH_FADERS, PL.SYNTH_ROTATED, [])
    for k, v in list(dc.items()) + list(oc.items()):
        if v is None:
            print("no cap found:", k)
    print("cap heights:", sorted({v[1] - v[0] + 1 for v in list(dc.values()) + list(oc.values()) if v}))


if __name__ == "__main__":
    main()
