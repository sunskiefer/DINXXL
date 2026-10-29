// DINXXL: Twisted Electrons' BlastBeats firmware running on a virtual copy of its hardware, as an mpc_engine.
//
//   panel faders / keys  ->  hw::set_fader / hw::set_key (the pins the firmware reads)
//   firmware's LED matrix ->  lamp parameters (the skin shows them)
//   OPL3 (ymfm) output   ->  the plugin's stereo output
//
// The firmware is not modified; everything the panel does is the firmware's own behaviour (see src/fw/VENDORED.md).
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <string>

extern "C" {
#include "engine.h"
}
#include "hw.h"
#include "machine.h"
#include "pack.h"
#include "param_ids.h"

namespace {

const int SR = 44100;
const uint64_t KEY_HOLD_US = 12000;      // a tap on a momentary key is held this long (the firmware polls every 100 us)
const int FADER_HYST_RAW = 9;            // see src/hw.cpp: the firmware ignores fader changes smaller than this
const char* const VOICE_NAMES[10] = {"BD", "SD", "TOM", "CH", "OH", "CYM", "SYNTH 1", "SYNTH 2", "SYNTH 3", "SYNTH 4"};
const char* const MODE_NAMES[5] = {"KIT / PATT", "STEP", "FUNCTION", "SONG", "VGM"};
const char* const FUNC_NAMES[16] = {"SAVE", "LENGTH", "CLEAR", "WIGGLE", "SWING", "MUTE", "SOLO", "PROB",
                                    "STUTTER", "BEND", "FREEZE", "TREM", "VIBR", "PHONES", "MIX", "ROUTE"};
const char* const MKEY_NAMES[5] = {"KIT / PATT", "STEP", "FUNCTION", "TEMPO", "REC"};
const char* const WAVE_NAMES[8] = {"SINE", "HALF SINE", "ABS SINE", "PULSE SINE", "SINE EVEN", "ABS EVEN", "SQUARE", "DERIVED SQ"};
const int SCREEN_HOLD_FRAMES = SR * 2;   // a touched control stays on the screen this long, then the screen goes Home

// firmware fader groups, for the screen (key prefix -> caption)
struct GroupName { const char* prefix; const char* caption; };
const GroupName GROUPS[] = {{"bdc_", "BD CARRIER"}, {"bdm_", "BD MODULATOR"}, {"sn_", "SNARE"}, {"htc_", "HATS TOM CYMB"},
                            {"op1_", "OP 1"}, {"op2_", "OP 2"}, {"op3_", "OP 3"}, {"op4_", "OP 4"}};

struct Inst;
Inst* g_owner = nullptr;   // the firmware keeps its state in globals, so one instance at a time can be the machine

struct Inst {
  bool owner = false;
  std::string data_dir;
  float pv[P_COUNT];
  unsigned rev = 1;

  // time
  uint64_t frames = 0;           // frames rendered
  uint64_t machine_us = 0;       // virtual time given to the machine so far
  // momentary key holds
  bool hold[33] = {false};
  uint64_t release_at[33] = {0};
  bool latched[32] = {false};    // HOLD: keys kept down until tapped again (voice 0..9, step 10..25, mode 26..30, RUN 31)
  bool hold_mode = false;
  // the load window: fader values set while a project loads are positions, not moves (nothing is written to the kit)
  int64_t quiet_until = 0;       // in frames
  bool quiet_dirty = false;
  // lamps and screen
  bool lamp[32] = {false};
  std::string txt[5];
  int wave = 0, algo = 0;
  int64_t touch_until = 0, key_until = 0;
  int touch_fader = -1;
  char key_title[32] = "";
  char key_sub[32] = "";

  // host transport -> the machine's MIDI clock (what a MIDI cable from the Force would carry)
  bool host_playing = false;
  double sent_to = 0;            // ppq up to which clocks were generated
  struct MidiAt { uint64_t at_us; uint8_t byte; };
  std::vector<MidiAt> midi_due;  // bytes to hand the firmware at a machine time inside the coming block

  // diagnostics: card/dinxxl.log beside the plugin (what happened, for a report from a device)
  std::vector<std::string> log;
  uint64_t log_next = 0; float log_peak = 0; int log_keys = 0, log_midi = 0, log_pos = 0, log_blocks = 0;
  bool log_factory = false;
  double log_cpu = 0, log_cpu_max = 0;
  int fk = -1, fs = -1, fv = -1;   // kit / synth / drum voice the on-screen faders were last matched to

  void bump() { rev++; }
};

void logline(Inst* s, const char* fmt, ...) {
  char b[256];
  va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
  s->log.push_back(b);
  if (s->log.size() > 60) s->log.erase(s->log.begin());
  if (s->data_dir.empty()) return;
  FILE* f = fopen((s->data_dir + "/dinxxl.log").c_str(), "w");
  if (!f) return;
  for (auto& l : s->log) fprintf(f, "%s\n", l.c_str());
  fclose(f);
}

int find_key(const char* k) {
  for (int i = 0; i < P_COUNT; i++)
    if (!strcmp(PARAM_KEY[i], k)) return i;
  return -1;
}

// ------------------------------------------------------------------------------------------------ the machine
void run_us(Inst* s, uint64_t us) {
  machine::run((uint32_t)us);
  s->machine_us += us;
}

void tap(Inst* s, int fw) {
  hw::set_key(fw, true);
  run_us(s, 40000);
  hw::set_key(fw, false);
  run_us(s, 40000);
}

void set_fader_pos(Inst* s, int i, int v) {
  v = v < 0 ? 0 : v > 127 ? 127 : v;
  hw::set_fader(FADER_FW[i], v);
}

// A blank machine has no kit: every voice is silent until the faders are moved. A first start is given a basic
// sound by moving the faders the way a player would (the firmware does the rest), synth by synth.
void starter_sound(Inst* s) {
  run_us(s, 1500000);
  tap(s, MODE_KEY_FW[1]);                       // step mode: the voice keys choose the voice (and synth) being edited
  // one fader at a time, so the firmware's scan sees every move (it reads one mux line per pass)
  auto sweep = [&](int lo, int hi, bool zero) {
    for (int i = lo; i < hi; i++) { set_fader_pos(s, i, zero ? 0 : FADER_DEFAULT[i]); run_us(s, 20000); }
  };
  for (int syn = 0; syn < 4; syn++) {
    tap(s, VOICE_KEY_FW[6 + syn]);
    sweep(28, 56, true);
    sweep(28, 56, false);
  }
  tap(s, VOICE_KEY_FW[0]);
  sweep(0, 28, true);
  sweep(0, 28, false);
  tap(s, VOICE_KEY_FW[6]);                      // leave synth 1 as the edited synth ...
  tap(s, VOICE_KEY_FW[0]);                      // ... and BD as the selected voice, like a fresh machine
  tap(s, MODE_KEY_FW[0]);                       // back to kit / pattern mode
  run_us(s, 1500000);                          // let the preview hits ring out before the first audio block
}

// A fresh machine boots on the factory card (card/factory.bin beside the plugin, the BlastBeats factory card
// packed by tools/pack_card.py); a project restores the factory card plus what it changed. Without the card file the
// machine gets a blank card and a starter kit.
void boot(Inst* s, const std::string* diff, const std::string* full_card) {
  bool factory = hw::sd_has_factory() || (!s->data_dir.empty() && hw::sd_load_factory_file(s->data_dir + "/factory.bin"));
  bool starter = false;
  if (full_card) hw::sd_load(*full_card);                                  // an old 'D1' project: its whole card
  else if (factory) { hw::sd_reset_to_factory(); if (diff) hw::sd_apply_diff(*diff); }
  else starter = true;
  for (int i = 0; i < N_FADER; i++) set_fader_pos(s, i, starter ? 0 : (int)s->pv[IDX_FADER + i]);
  for (int i = 0; i < 33; i++) s->hold[i] = false;
  s->midi_due.clear(); s->host_playing = false; s->sent_to = 0;
  for (int i = 0; i < 32; i++) s->latched[i] = false;
  s->hold_mode = false; s->pv[P_K_HOLD] = 0;
  machine::power_on(1);
  s->machine_us = 0;                          // the firmware's own power-on runs on its own clock ...
  if (starter) {
    machine::init_blank_card();
    starter_sound(s);
    for (int i = 0; i < N_FADER; i++) s->pv[IDX_FADER + i] = (float)FADER_DEFAULT[i];
  } else {
    run_us(s, 1500000);                       // the firmware's own power-on and card load, before the first block
  }
  s->machine_us = s->frames * 1000000ull / SR;   // ... then the machine clock rejoins the audio clock: the start-up work is not audio time
  hw::audio_trim(0);
  s->quiet_until = s->frames + SR / 2;
  s->quiet_dirty = true;
  s->log_factory = factory;
  machine::Snap sn = machine::snapshot();
  logline(s, "boot: data_dir=%s factory=%d diff=%d full=%d starter=%d | mode=%d run=%d tempo=%d kit=%d patt=%d", s->data_dir.c_str(),
          (int)factory, diff ? 1 : 0, full_card ? 1 : 0, (int)starter, sn.mode, (int)sn.run, sn.tempo, sn.kit, sn.pattern);
}

// ------------------------------------------------------------------------------------------------ screen
const char* fader_param_caption(const char* key) {
  const char* u = strchr(key, '_');
  return u ? u + 1 : key;
}

void upper(char* d, const char* s, size_t n) {
  size_t i = 0;
  for (; s[i] && i + 1 < n; i++) d[i] = (char)((s[i] >= 'a' && s[i] <= 'z') ? s[i] - 32 : s[i]);
  d[i] = 0;
}

void compose_screen(Inst* s, const machine::Snap& sn) {
  char a[64], b[64], c[64], d[64], e[64];
  a[0] = b[0] = c[0] = d[0] = e[0] = 0;
  int wave = 0, algo = 0;
  if (s->touch_until > (int64_t)s->frames && s->touch_fader >= 0) {         // a fader was touched
    int i = s->touch_fader;
    const char* key = FADER_KEY[i];
    const char* cap = "";
    for (const GroupName& g : GROUPS)
      if (!strncmp(key, g.prefix, strlen(g.prefix))) cap = g.caption;
    snprintf(a, sizeof a, "%s", cap);
    if (!strncmp(key, "op", 2)) snprintf(b, sizeof b, "SYNTH %d", sn.synth + 1);
    char nm[16];
    upper(nm, fader_param_caption(key), sizeof nm);
    snprintf(c, sizeof c, "%s", nm);
    int level = ((int)s->pv[IDX_FADER + i]) >> 3;     // the firmware sees 16 levels
    if (!strcmp(nm, "WAV")) {
      wave = (level >> 1) + 1;
      snprintf(d, sizeof d, "%d %s", wave, WAVE_NAMES[wave - 1]);
    } else if (!strcmp(nm, "ALGO")) {
      algo = level * 4 / 15 + 1;
      snprintf(d, sizeof d, "ALGO %d", algo);
    } else {
      snprintf(d, sizeof d, "%d%%", (level * 100 + 7) / 15);
    }
  } else if (s->key_until > (int64_t)s->frames) {                           // a key was tapped
    snprintf(c, sizeof c, "KEY");
    snprintf(d, sizeof d, "%s", s->key_title);
    snprintf(e, sizeof e, "%s", s->key_sub);
  } else {
    snprintf(a, sizeof a, "KIT %d", sn.kit % 10 + 1);
    snprintf(b, sizeof b, "BANK %d   PATT %d", sn.kit / 10 + 1, sn.pattern + 1);
    snprintf(c, sizeof c, "TEMPO");
    snprintf(d, sizeof d, "%d", sn.tempo);
    const char* mode = MODE_NAMES[sn.mode >= 0 && sn.mode < 5 ? sn.mode : 0];
    char fn[24] = "";
    if (sn.mode == 2 && sn.function >= 0 && sn.function < 16) snprintf(fn, sizeof fn, " %s", FUNC_NAMES[sn.function]);
    snprintf(e, sizeof e, "%s  %s%s  %s", sn.run ? (sn.rec ? "REC" : "RUN") : "STOP", mode, fn,
             VOICE_NAMES[sn.voice >= 0 && sn.voice < 10 ? sn.voice : 0]);
    // keys being held (HOLD latches and latched mode keys) replace the status line, so you always see what is down
    char held[64] = "";
    size_t hl = 0;
    auto add = [&](const char* nm) { if (hl + strlen(nm) + 2 < sizeof held) hl += snprintf(held + hl, sizeof held - hl, "%s%s", hl ? " + " : "", nm); };
    for (int i = 0; i < 5; i++) if (s->latched[26 + i]) add(MKEY_NAMES[i]);
    if (s->latched[31]) add("RUN");
    for (int i = 0; i < 10; i++) if (s->latched[i]) add(VOICE_NAMES[i]);
    for (int i = 0; i < 16; i++) if (s->latched[10 + i]) { char st[12]; snprintf(st, sizeof st, sn.mode == 1 ? "STEP %d" : "%s", sn.mode == 1 ? (int)(i + 1) : 0); if (sn.mode != 1) snprintf(st, sizeof st, "%s", FUNC_NAMES[i]); add(st); }
    if (hl) snprintf(e, sizeof e, "HELD: %s", held);
  }
  const char* t[5] = {a, b, c, d, e};
  bool changed = false;
  for (int i = 0; i < 5; i++)
    if (s->txt[i] != t[i]) { s->txt[i] = t[i]; changed = true; }
  if (wave != s->wave) { s->wave = wave; changed = true; }
  if (algo != s->algo) { s->algo = algo; changed = true; }
  if (changed) s->bump();
}

void refresh_outputs(Inst* s) {
  bool changed = false;
  auto set_lamp = [&](int slot, bool on) {
    if (s->lamp[slot] != on) { s->lamp[slot] = on; changed = true; }
  };
  for (int i = 0; i < 10; i++) set_lamp(i, hw::lamp(16 + i));
  for (int i = 0; i < 16; i++) set_lamp(10 + i, hw::lamp(i));
  for (int i = 0; i < 6; i++) set_lamp(26 + i, hw::lamp(MODE_LAMP_LED[i]));
  if (changed) s->bump();
  compose_screen(s, machine::snapshot());
}

// ------------------------------------------------------------------------------------------------ state
// The state text: "D1:" + base64(packed SD card + format tag). If it does not fit the host's 8 KB chunk it goes to a
// file beside the plugin and the text is "D1F:<id>".
const size_t CHUNK_MAX = 8000;

std::string fnv_hex(const std::string& d) {
  uint64_t h = 1469598103934665603ull;
  for (unsigned char c : d) { h ^= c; h *= 1099511628211ull; }
  char b[24];
  snprintf(b, sizeof b, "%016llx", (unsigned long long)h);
  return b;
}

int save_state(Inst* s, char* buf, int n) {
  if (!s->owner) return snprintf(buf, n, "D1:");
  // make the working song and kits part of the "card" as pressing SAVE would; the time that takes is not played
  int fill = hw::audio_fill();
  machine::flush_to_sd();
  hw::audio_trim(fill);
  bool diff = hw::sd_has_factory();
  std::string packed = pack::lzss(diff ? hw::sd_save_diff() : hw::sd_save());
  std::string text = (diff ? "D2:" : "D1:") + pack::b64(packed);
  if (text.size() >= CHUNK_MAX) {
    std::string id = fnv_hex(packed);
    if (!s->data_dir.empty()) {
      std::string path = s->data_dir + "/dinxxl-" + id + ".bin";
      FILE* f = fopen(path.c_str(), "wb");
      if (f) {
        fwrite(packed.data(), 1, packed.size(), f);
        fclose(f);
        text = (diff ? "D2F:" : "D1F:") + id;
      }
    }
    if (text.size() >= CHUNK_MAX) text = diff ? "D2:" : "D1:";   // could not store it: the project keeps its faders, not its card
  }
  return snprintf(buf, n, "%s", text.c_str());
}

void restore_state(Inst* s, const char* text) {
  if (!s->owner || !text) return;
  if (text[0] != 'D' || (text[1] != '1' && text[1] != '2')) return;
  bool diff = text[1] == '2';
  std::string packed;
  if (text[2] == 'F' && text[3] == ':') {                     // "D1F:<id>" / "D2F:<id>": a file beside the plugin
    std::string path = s->data_dir + "/dinxxl-" + (text + 4) + ".bin";
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return;
    char tmp[4096];
    size_t r;
    while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) packed.append(tmp, r);
    fclose(f);
  } else if (text[2] == ':') {
    if (!text[3]) { if (diff) boot(s, nullptr, nullptr); return; }   // an empty D2: the factory card as it was
    if (!pack::unb64(text + 3, packed)) return;
  } else return;
  std::string sd;
  if (!pack::unlzss(packed, sd)) return;
  if (diff) boot(s, &sd, nullptr); else boot(s, nullptr, &sd);
}

// ------------------------------------------------------------------------------------------------ engine API
void* create(const char* data_dir) {
  Inst* s = new Inst();
  if (data_dir) s->data_dir = data_dir;
  for (int i = 0; i < N_FADER; i++) s->pv[i] = (float)FADER_DEFAULT[i];
  for (int i = N_FADER; i < P_COUNT; i++) s->pv[i] = 0;
  if (!g_owner) {
    g_owner = s;
    s->owner = true;
    boot(s, nullptr, nullptr);
    refresh_outputs(s);
  }
  return s;
}

void destroy(void* inst) {
  Inst* s = (Inst*)inst;
  if (g_owner == s) g_owner = nullptr;
  delete s;
}

void midi(void* inst, const uint8_t* msg, int len) {
  Inst* s = (Inst*)inst;
  if (!s->owner) return;
  s->log_midi += len;
  // The Force's pads and keys arrive on one channel. PADS mode (the default) gives every pad a voice, the way the Force's
  // own drum programs do: from C1 (36) upward, 16 pads = BD SD TOM CH OH CYM S1 S2 S3 S4 | S1' S2' S3' S4' BD SD
  // (S' = an octave up). The note is re-addressed to the voice's own channel and note, so the firmware plays it exactly as
  // a MIDI cable would. CHROMATIC mode passes the message through unchanged (channel 1-4 = synth 1-4, channel 10 = drums).
  if (len == 3 && s->pv[P_PAD_MODE] < 0.5f && (msg[0] & 0xE0) == 0x80) {
    int pad = ((int)msg[1] - 36) % 16; if (pad < 0) pad += 16;
    static const int kind[16] = {0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 22, 23, 24, 25, 0, 1};   // <6 drum, 10+n synth n, 22+n synth n +12
    int k = kind[pad];
    uint8_t m[3] = {msg[0], msg[1], msg[2]};
    if (k < 6) { m[0] = (uint8_t)((msg[0] & 0xF0) | ((machine::midi_channel(4) - 1) & 15)); m[1] = (uint8_t)machine::drum_note(k); }
    else { int syn = k >= 22 ? k - 22 : k - 10; m[0] = (uint8_t)((msg[0] & 0xF0) | ((machine::midi_channel(syn) - 1) & 15)); m[1] = (uint8_t)(k >= 22 ? 72 : 60); }
    for (int i = 0; i < 3; i++) hw::midi_in(m[i]);
    return;
  }
  for (int i = 0; i < len; i++) hw::midi_in(msg[i]);
}

// What a key is, in the mode the machine is in: the top row selects kits in KIT/PATT mode and voices in STEP mode, the bottom
// row is patterns, steps or functions.
void key_name(Inst* s, int slot, char* t, size_t n) {
  machine::Snap sn = machine::snapshot();
  if (slot < 10) {
    if (sn.mode == 0) snprintf(t, n, "KIT %d", slot + 1);
    else if (sn.mode == 3) snprintf(t, n, "SONG BANK %d", slot + 1);
    else snprintf(t, n, "%s", VOICE_NAMES[slot]);
  } else if (slot < 26) {
    int i = slot - 10;
    if (sn.mode == 1) snprintf(t, n, "STEP %d", i + 1);
    else if (sn.mode == 2) snprintf(t, n, "%s", FUNC_NAMES[i]);
    else if (sn.mode == 3) snprintf(t, n, "SONG %d", i + 1);
    else snprintf(t, n, "PATT %d", i + 1);
  } else if (slot < 31) snprintf(t, n, "%s", MKEY_NAMES[slot - 26]);
  else snprintf(t, n, "RUN");
}

void press(Inst* s, int slot, int fw, bool run_key) {
  if (run_key) hw::set_run_key(true); else hw::set_key(fw, true);
  s->hold[slot] = true;
  s->release_at[slot] = s->machine_us + KEY_HOLD_US;
}

void key_note(Inst* s, const char* title, const char* sub) {
  snprintf(s->key_title, sizeof s->key_title, "%s", title);
  snprintf(s->key_sub, sizeof s->key_sub, "%s", sub);
  s->key_until = (int64_t)s->frames + SCREEN_HOLD_FRAMES;
  s->touch_until = 0;
}

void set_param(void* inst, const char* key, const char* val) {
  Inst* s = (Inst*)inst;
  if (!strcmp(key, "state")) { restore_state(s, val); return; }
  int p = find_key(key);
  if (p < 0) return;
  float v = (float)atof(val);
  s->pv[p] = v;
  if (!s->owner) return;
  if (p < IDX_VKEY) {                                  // a fader
    int i = p - IDX_FADER;
    int iv = (int)floorf(v + 0.5f);
    set_fader_pos(s, i, iv);
    if ((int64_t)s->frames < s->quiet_until) {         // the half second after a (re)boot: the project's stored positions,
      s->quiet_dirty = true;                            // not moves (the window is fixed: a value stream must not extend it)
    } else {
      s->touch_fader = i;
      s->touch_until = (int64_t)s->frames + SCREEN_HOLD_FRAMES;
    }
    return;
  }
  if (p <= IDX_RUN) {                                  // voice (0..9), step (10..25), mode (26..30) and RUN (31) keys
    if (v <= 0.5f) return;
    s->log_keys++;
    if ((int64_t)s->frames < s->quiet_until) return;   // a project loading: no key presses out of the stored values
    int slot = p < IDX_SKEY ? p - IDX_VKEY : p < IDX_MKEY ? 10 + (p - IDX_SKEY) : p < IDX_RUN ? 26 + (p - IDX_MKEY) : 31;
    int fw = slot < 10 ? VOICE_KEY_FW[slot] : slot < 26 ? STEP_KEY_FW[slot - 10] : slot < 31 ? MODE_KEY_FW[slot - 26] : 0;
    bool run_key = slot == 31;
    char t[24];
    key_name(s, slot, t, sizeof t);
    if (s->latched[slot]) {                            // a held key: this tap lets it go
      s->latched[slot] = false; s->hold[slot] = false;
      if (run_key) hw::set_run_key(false); else hw::set_key(fw, false);
      key_note(s, t, "RELEASED");
    } else if (s->hold_mode) {                         // HOLD is on: keep it down
      s->latched[slot] = true; s->hold[slot] = false;
      if (run_key) hw::set_run_key(true); else hw::set_key(fw, true);
      key_note(s, t, "HELD - TAP AGAIN TO RELEASE");
    } else if (run_key && machine::snapshot().midi_slave && machine::snapshot().run) {
      // slaved to the Force's transport, RUN would only restart the pattern (as the hardware under a MIDI clock); here it
      // stops the machine the way a MIDI Stop does. Press RUN again and it runs free; the next Force Play re-syncs it.
      hw::midi_in(0xFC);
      key_note(s, "RUN", "STOPPED - FREE RUN UNTIL FORCE PLAYS");
    } else {                                           // a plain tap: pressed and released, as a finger does
      press(s, slot, fw, run_key);
      key_note(s, t, "");
    }
    return;
  }
  if (p == P_K_HOLD) {
    if ((int64_t)s->frames < s->quiet_until) { s->pv[p] = 0; return; }
    s->hold_mode = v > 0.5f;
    key_note(s, "HOLD", s->hold_mode ? "NEXT VOICE / STEP KEYS STAY DOWN" : "OFF - HELD KEYS STAY UNTIL TAPPED");
    return;
  }
}

int get_param(void* inst, const char* key, char* buf, int n) {
  Inst* s = (Inst*)inst;
  if (!strcmp(key, "display_rev")) return snprintf(buf, n, "%u", s->rev);
  if (!strcmp(key, "state")) return save_state(s, buf, n);
  size_t kl = strlen(key);
  if ((kl > 8 && !strcmp(key + kl - 8, "_display")) || (kl > 5 && !strcmp(key + kl - 5, "_name"))) return 0;
  int p = find_key(key);
  if (p < 0) return 0;
  if (p >= IDX_SCR && p < IDX_SCR + N_SCR) return snprintf(buf, n, "%s", s->txt[p - IDX_SCR].c_str());
  if (p == IDX_WAVE) return snprintf(buf, n, "%d", s->wave);
  if (p == IDX_ALGO) return snprintf(buf, n, "%d", s->algo);
  if (p >= IDX_VLAMP && p < IDX_SCR) return snprintf(buf, n, "%d", s->lamp[p - IDX_VLAMP] ? 1 : 0);
  if (p >= IDX_VKEY && p < IDX_MKEY) return snprintf(buf, n, "0");          // momentary keys read back 0
  if (p == IDX_RUN) return snprintf(buf, n, "0");
  return snprintf(buf, n, "%g", s->pv[p]);
}

// The Force's transport, as MIDI: Start/Stop on the play state, 24 clocks per beat on the song-position grid
// (docs/MIDI_TIMING.md: continue from where the last block ended, re-anchor on a jump). The firmware then slaves to it as the
// hardware does to a MIDI cable.
void song_pos(void* inst, double ppq, double bpm, int playing, int frames) {
  Inst* s = (Inst*)inst;
  if (!s->owner) return;
  s->log_pos++;
  uint64_t block_us = s->frames * 1000000ull / SR;
  auto due = [&](double frac, uint8_t b) {
    if (frac < 0) frac = 0; if (frac > 1) frac = 1;
    s->midi_due.push_back({block_us + (uint64_t)(frac * frames * 1000000.0 / SR), b});
  };
  if (ppq < 0 || bpm <= 0) {                                   // no position from the host: only the play state matters
    if (!playing && s->host_playing) { due(0, 0xFC); s->host_playing = false; }
    return;
  }
  double blk = frames * bpm / (60.0 * SR);
  if (playing && !s->host_playing) { due(0, 0xFA); s->sent_to = ppq; s->host_playing = true; }
  else if (!playing && s->host_playing) { due(0, 0xFC); s->host_playing = false; return; }
  if (!playing) return;
  if (fabs(ppq - s->sent_to) > 1.0) s->sent_to = ppq - 0.002;  // a locate or an unreported loop: re-anchor
  double start = s->sent_to, end = ppq + blk;
  const double tick = 1.0 / 24.0;
  double t = ceil(start / tick - 1e-9) * tick;
  for (; t < end; t += tick) due((t - ppq) / blk, 0xF8);
  s->sent_to = end;
}


// The faders on the panel show the settings of the kit now loaded (the machine's own faders are physical and stay put; a
// panel can do better): when the kit, the synth or the drum voice changes, move every fader to the stored value. The
// values already are what the firmware holds, so nothing sounds different.
void follow_kit(Inst* s) {
  if ((int64_t)s->frames < s->quiet_until || s->touch_until > (int64_t)s->frames) return;
  machine::Snap sn = machine::snapshot();
  if (sn.kit == s->fk && sn.synth == s->fs && sn.voice == s->fv) return;
  s->fk = sn.kit; s->fs = sn.synth; s->fv = sn.voice;
  bool moved = false;
  for (int i = 0; i < N_FADER; i++) {
    int lv = machine::fader_level(FADER_FW[i]);
    if (lv < 0) continue;
    if (((int)s->pv[IDX_FADER + i] >> 3) == lv) continue;
    s->pv[IDX_FADER + i] = (float)(lv * 8 + 4);
    set_fader_pos(s, i, lv * 8 + 4);
    moved = true;
  }
  if (moved) { machine::sync_faders(); s->bump(); }
}

#include <time.h>
static double now_s() { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
void render(void* inst, int16_t* out, int frames) {
  Inst* s = (Inst*)inst;
  double t_start = now_s();
  if (!s->owner) { memset(out, 0, sizeof(int16_t) * 2 * frames); return; }
  if (s->quiet_dirty) {                       // faders were set while loading: tell the firmware where they are now
    machine::sync_faders();
    s->quiet_dirty = false;
  }
  uint64_t target = (s->frames + (uint64_t)frames) * 1000000ull / SR;
  while (s->machine_us < target) {
    // run to the next key release (or the end of the block)
    uint64_t stop = target;
    for (int i = 0; i < 33; i++)
      if (s->hold[i] && s->release_at[i] < stop) stop = s->release_at[i] > s->machine_us ? s->release_at[i] : s->machine_us + 1;
    for (auto& m : s->midi_due)
      if (m.at_us < stop) stop = m.at_us > s->machine_us ? m.at_us : s->machine_us + 1;
    run_us(s, stop - s->machine_us);
    for (size_t i = 0; i < s->midi_due.size();)
      if (s->midi_due[i].at_us <= s->machine_us) { hw::midi_in(s->midi_due[i].byte); s->midi_due.erase(s->midi_due.begin() + i); }
      else i++;
    for (int i = 0; i < 33; i++)
      if (s->hold[i] && s->machine_us >= s->release_at[i]) {
        s->hold[i] = false;
        if (i == 31) hw::set_run_key(false);
        else if (i < 10) hw::set_key(VOICE_KEY_FW[i], false);
        else if (i < 26) hw::set_key(STEP_KEY_FW[i - 10], false);
        else hw::set_key(MODE_KEY_FW[i - 26], false);
      }
  }
  float lr[256];
  hw::audio_pull(lr, frames);
  const float G = 12.0f;                      // calibrated so a full-volume voice peaks near -8 dBFS; the soft limiter catches sums
  for (int i = 0; i < frames * 2; i++) {
    float x = lr[i] * G;
    x = x / (1.0f + fabsf(x) * 0.35f);         // soft limiter: never clips
    int v = (int)lrintf(x * 32767.0f);
    out[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
  }
  for (int i = 0; i < frames * 2; i++) { float a = fabsf(out[i] / 32768.0f); if (a > s->log_peak) s->log_peak = a; }
  s->frames += frames;
  s->log_blocks++;
  { double dt = now_s() - t_start; s->log_cpu += dt; if (dt > s->log_cpu_max) s->log_cpu_max = dt; }
  if (s->frames >= s->log_next) {
    machine::Snap sn = machine::snapshot();
    double audio_s = 5.0;
    logline(s, "t=%llus blocks=%d cpu=%.1f%% worst=%.1fms peak=%.3f keys=%d midi=%d pos=%d fill=%d | mode=%d run=%d step=%d tempo=%d kit=%d patt=%d voice=%d",
            (unsigned long long)(s->frames / SR), s->log_blocks, 100.0 * s->log_cpu / audio_s, 1000.0 * s->log_cpu_max, s->log_peak,
            s->log_keys, s->log_midi, s->log_pos, hw::audio_fill(), sn.mode, (int)sn.run, sn.step, sn.tempo, sn.kit, sn.pattern, sn.voice);
    s->log_next = s->frames + 5 * SR; s->log_peak = 0; s->log_blocks = 0; s->log_cpu = 0; s->log_cpu_max = 0;
  }
  follow_kit(s);
  refresh_outputs(s);
}

const mpc_engine_t API = {create, destroy, midi, set_param, get_param, render, nullptr, song_pos};

}  // namespace

extern "C" const mpc_engine_t* mpc_engine(void) { return &API; }
