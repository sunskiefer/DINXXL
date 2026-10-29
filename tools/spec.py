"""DINXXL: the one source of truth for every control on the panel.

Everything else is generated from this file (tools/gen_params.py: params.json, src/param_ids.h, layout.conf).
MPC stores automation and saved values by parameter INDEX, so once a version ships the parameter list is
append-only: never reorder, rename a key or remove a parameter.

The machine is Twisted Electrons' BlastBeats; the firmware itself runs inside the plugin (src/fw), so the panel
only has to reproduce its 56 faders, 32 mux keys and RUN key, and its lamps. Coordinates are skin pixels on the
1280 x 628 plugin page (layout.conf adds 86 to y, the Force Shadow offset).
"""

# ------------------------------------------------------------------------------------------------ faders
# firmware fader number (movedFader() in fw/include/fader.h) -> panel position. Slots are 34 px wide;
# the top row of slots spans y 23..257 and the bottom row y 348..580.
SLOT_W = 34
ROW_TOP = {"t": (23, 257), "b": (348, 580)}

# (key, label, firmware fader number, slot centre x, row)
DRUM_FADERS = [
    # bd carrier
    ("bdc_vol", "BD Carrier Vol", 0, 157, "t"), ("bdc_wav", "BD Carrier Wav", 1, 238, "t"),
    ("bdc_fbk", "BD Carrier Fbk", 2, 316, "t"), ("bdc_mult", "BD Carrier Mult", 3, 392, "t"),
    ("bdc_tune", "BD Carrier Tune", 4, 471, "t"), ("bdc_fall", "BD Carrier Fall", 5, 549, "t"),
    ("bdc_att", "BD Carrier Att", 6, 630, "t"), ("bdc_dec", "BD Carrier Dec", 14, 706, "t"),
    # bd modulator
    ("bdm_vol", "BD Mod Vol", 15, 835, "t"), ("bdm_wav", "BD Mod Wav", 16, 911, "t"),
    ("bdm_mult", "BD Mod Mult", 17, 989, "t"), ("bdm_fall", "BD Mod Fall", 18, 1065, "t"),
    ("bdm_att", "BD Mod Att", 19, 1144, "t"), ("bdm_dec", "BD Mod Dec", 20, 1224, "t"),
    # snare
    ("sn_vol", "Snare Vol", 28, 150, "b"), ("sn_wav", "Snare Wav", 29, 232, "b"),
    ("sn_mult", "Snare Mult", 30, 311, "b"), ("sn_tune", "Snare Tune", 31, 391, "b"),
    ("sn_fall", "Snare Fall", 32, 462, "b"), ("sn_att", "Snare Att", 33, 543, "b"),
    ("sn_dec", "Snare Dec", 34, 628, "b"),
    # hats / tom / cymbal
    ("htc_vol", "Hats Tom Cymb Vol", 42, 748, "b"), ("htc_wav", "Hats Tom Cymb Wav", 43, 831, "b"),
    ("htc_mult", "Hats Tom Cymb Mult", 44, 909, "b"), ("htc_tune", "Hats Tom Cymb Tune", 45, 987, "b"),
    ("htc_fall", "Hats Tom Cymb Fall", 46, 1061, "b"), ("htc_att", "Hats Tom Cymb Att", 47, 1144, "b"),
    ("htc_dec", "Hats Tom Cymb Dec", 48, 1225, "b"),
]
SYNTH_FADERS = [
    # op1
    ("op1_vol", "Op1 Vol", 7, 156, "t"), ("op1_wav", "Op1 Wav", 8, 236, "t"), ("op1_fbk", "Op1 Fbk", 9, 315, "t"),
    ("op1_algo", "Op1 Algo", 10, 391, "t"), ("op1_mult", "Op1 Mult", 11, 471, "t"), ("op1_att", "Op1 Att", 12, 549, "t"),
    ("op1_dec", "Op1 Dec", 13, 630, "t"),
    # op2
    ("op2_vol", "Op2 Vol", 21, 750, "t"), ("op2_wav", "Op2 Wav", 22, 831, "t"), ("op2_mult", "Op2 Mult", 23, 910, "t"),
    ("op2_trem", "Op2 Trem", 24, 990, "t"), ("op2_vibra", "Op2 Vibra", 25, 1066, "t"), ("op2_att", "Op2 Att", 26, 1142, "t"),
    ("op2_dec", "Op2 Dec", 27, 1221, "t"),
    # op3
    ("op3_vol", "Op3 Vol", 35, 156, "b"), ("op3_wav", "Op3 Wav", 36, 236, "b"), ("op3_fbk", "Op3 Fbk", 37, 315, "b"),
    ("op3_tune", "Op3 Tune", 38, 389, "b"), ("op3_mult", "Op3 Mult", 39, 471, "b"), ("op3_att", "Op3 Att", 40, 548, "b"),
    ("op3_dec", "Op3 Dec", 41, 629, "b"),
    # op4
    ("op4_vol", "Op4 Vol", 49, 750, "b"), ("op4_wav", "Op4 Wav", 50, 831, "b"), ("op4_mult", "Op4 Mult", 51, 910, "b"),
    ("op4_trem", "Op4 Trem", 52, 986, "b"), ("op4_vibra", "Op4 Vibra", 53, 1066, "b"), ("op4_att", "Op4 Att", 54, 1144, "b"),
    ("op4_dec", "Op4 Dec", 55, 1223, "b"),
]
FADERS = DRUM_FADERS + SYNTH_FADERS

# ------------------------------------------------------------------------------------------------ keys
# The 32 mux keys: firmware key number (buttPressed() in fw/include/buttons.h) and the panel key it is.
# Voice keys (top ten), step keys (bottom sixteen) and the modes. Positions are key centres on the keys page.
VOICE_KEYS = [  # (key, label, firmware number)
    ("v_bd", "BD", 28), ("v_sd", "SD", 0), ("v_tom", "TOM", 1), ("v_ch", "CH", 2), ("v_oh", "OH", 3),
    ("v_cym", "CYM", 4), ("v_s1", "SYNTH 1", 5), ("v_s2", "SYNTH 2", 31), ("v_s3", "SYNTH 3", 29), ("v_s4", "SYNTH 4", 6),
]
VOICE_X = [66, 144, 221, 299, 376, 454, 531, 609, 686, 764]
VOICE_Y = 402                      # key body centre (lamp at y 372)
VOICE_LAMP_Y = 372
STEP_NAMES = ["save", "length", "clear", "wiggle", "swing", "mute", "solo", "prob",
              "stutter", "bend", "freeze", "trem", "vibr", "phones", "mix", "route"]
STEP_X = [66, 144, 221, 298, 376, 454, 531, 609, 686, 764, 842, 920, 997, 1075, 1152, 1230]
STEP_Y = 512                       # key body centre (lamp at y 481)
STEP_LAMP_Y = 481
# step key s (0..15) is firmware button number 16 + s for s < 8 and s for s >= 8 (buttPressed "case 8 ... 23")
STEP_BUTTON = [16 + s if s < 8 else s for s in range(16)]
MODE_KEYS = [  # (key, label, firmware number, cx, cy, latch)
    ("m_kit", "Kit/Patt", 30, 128, 118, True), ("m_step", "Step", 26, 206, 118, True),
    ("m_func", "Function", 27, 285, 118, True),
    ("m_tempo", "Tempo", 24, 206, 227, True), ("m_rec", "Rec", 25, 285, 227, True),
]
RUN_KEY = ("m_run", "Run", 128, 227)   # its own pin on the machine (RUN_PIN)
MODE_LAMPS = [  # (key, firmware LED number, cx, cy)
    ("l_kit", 40, 130, 85), ("l_step", 41, 208, 85), ("l_func", 42, 285, 85),
    ("l_run", 50, 131, 193), ("l_tempo", 51, 208, 193), ("l_rec", 52, 285, 193),
]
KEY_W, KEY_H = 76, 98
# the plugin's one extra key: HOLD. A tap is one event on MPC OS, so HOLD + a voice or step key keeps that key down (a
# second tap releases it) for the firmware's hold combinations. Slim, in the gap between the mode keys and the screen.
SCREEN_DX = 50                                   # the screen bezel is moved right this much to make room for HOLD
HOLD_KEY = ("k_hold", "hold", 375, 124, 76, 98)    # key, label, cx, cy (page), w, h: a real key beside the mode keys

# the small printed dots between the tune/fall label pairs on the drum page (page x, label baseline y)
from panel_labels import DRUM_PAIRS as PAIR_DOTS
