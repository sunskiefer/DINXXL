"""Printed captions of the user's panel art: where each label sits (page pixels), as read off the photos.

Live text (Chakra Petch) is placed by layout.conf from these; the rotated captions and the little triangles and group
lines are baked into the page pictures by make_art.py.
"""

# ---- fader pages: label baselines. Drum page y 294 (top row) and 620 (bottom row); synth page 291 / 617.
DRUM_BASE = {"t": 294, "b": 620}
SYNTH_BASE = {"t": 291, "b": 617}

# (text, x) singles, then ("a", "b", xl, xr) pairs written  a > b  with a triangle between
DRUM_LABELS_T = [("vol", 153), ("wav", 235), ("fbk", 314), ("mult", 395), ("att", 631), ("dec", 706),
                 ("vol", 835), ("wav", 911), ("att", 1144), ("dec", 1224)]
DRUM_PAIRS_T = [("tune", "fall", 473, 547), ("mult", "fall", 989, 1065)]
DRUM_LABELS_B = [("vol", 150), ("wav", 232), ("mult", 311), ("att", 543), ("dec", 628),
                 ("vol", 748), ("wav", 831), ("mult", 909), ("att", 1144), ("dec", 1225)]
DRUM_PAIRS_B = [("tune", "fall", 391, 462), ("tune", "fall", 987, 1061)]
DRUM_PAIRS = [((xl + xr) // 2, DRUM_BASE["t"]) for _, _, xl, xr in DRUM_PAIRS_T] + \
             [((xl + xr) // 2, DRUM_BASE["b"]) for _, _, xl, xr in DRUM_PAIRS_B]

OP1 = [("vol", 153), ("wav", 234), ("fbk", 314), ("algo", 394), ("mult", 470), ("att", 550), ("dec", 628)]
OP2 = [("vol", 750), ("wav", 831), ("mult", 910), ("trem", 990), ("vibra", 1066), ("att", 1142), ("dec", 1221)]
OP3 = [("vol", 153), ("wav", 234), ("fbk", 313), ("tune", 386), ("mult", 470), ("att", 545), ("dec", 626)]
OP4 = [("vol", 748), ("wav", 828), ("mult", 910), ("trem", 983), ("vibra", 1066), ("att", 1144), ("dec", 1223)]

# rotated captions: (x, y, w, h of the printed one to wipe, text, baseline x, centre y)
SYNTH_ROTATED = [(78, 6, 29, 140, "synth op1", 100, 76), (78, 340, 28, 145, "synth op3", 100, 412),
                 (673, 12, 29, 146, "synth op2", 695, 85), (672, 338, 29, 146, "synth op4", 694, 411)]
DRUM_ROTATED = [(82, 9, 24, 139, "bd carrier", 102, 78), (84, 335, 18, 87, "snare", 100, 378),
                (764, 12, 25, 193, "bd modulator", 787, 108), (671, 335, 31, 208, "hats tom cymb", 696, 439)]

# ---- keys page (after the keys were brought up 26 px)
BOT = ["save", "length", "clear", "wiggle", "swing", "mute", "solo", "prob", "stutter", "bend", "freeze", "trem",
       "vibr", "phones", "mix", "route"]
BOT_Y = 592           # baseline 618 - 26
VOX_Y = 335
VOX = ["bd", "sd", "tom", "ch", "oh", "cym", "synth1", "synth2", "synth3", "synth4"]
MODE_LABELS = [("kit/patt", 128, 50), ("step", 206, 50), ("function", 285, 50),
               ("run", 128, 303), ("tempo", 206, 303), ("rec", 285, 303)]
KEYS_ROTATED = [("kit / bank", 21, 402), ("patt / song", 21, 515)]
LY = 610
KEYX = [66, 144, 221, 298, 376, 454, 531, 609, 686, 764, 842, 920, 997, 1075, 1152, 1230]
GROUPS = [("edit", 0, 7, 50), ("trick", 8, 10, 62), ("rate", 11, 12, 56), ("volume", 13, 15, 88)]


def _group_lines():
    out = []
    for txt, a, b, tw in GROUPS:
        x0 = KEYX[a] - 38 + 6
        x1 = KEYX[b] + 38 - 6
        cx = (x0 + x1) / 2
        out.append((x0, cx - tw / 2 - 9, LY))
        out.append((cx + tw / 2 + 9, x1, LY))
    return out


KEYS_LINES = _group_lines()
GROUP_TEXT = [(txt, (KEYX[a] - 38 + 6 + KEYX[b] + 38 - 6) / 2) for txt, a, b, tw in GROUPS]
