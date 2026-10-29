// Dev check of the engine through the same interface the plugin wrapper uses (not shipped).
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
extern "C" {
#include "engine.h"
}
#include "param_ids.h"
#include "hw.h"
#include "machine.h"

static const mpc_engine_t* E;
static void* S;
static std::vector<int16_t> all;
static void set(const char* k, const char* v) { E->set_param(S, k, v); }
static std::string get(const char* k) { char b[512] = ""; E->get_param(S, k, b, sizeof b); return b; }
static long rendered_frames = 0;
static double render_sec(double sec, double* peak = nullptr, double* rms = nullptr) {
  rendered_frames += (long)(sec * 44100);
  int frames = (int)(sec * 44100);
  double pk = 0, ss = 0; long n = 0;
  int16_t buf[256];
  while (frames > 0) {
    int f = frames > 128 ? 128 : frames;
    E->render(S, buf, f);
    for (int i = 0; i < f * 2; i++) { double a = fabs(buf[i] / 32768.0); if (a > pk) pk = a; ss += a * a; n++; all.push_back(buf[i]); }
    frames -= f;
  }
  if (peak) *peak = pk;
  if (rms) *rms = sqrt(ss / (n ? n : 1));
  return pk;
}
static void tapk(const char* k) { set(k, "1"); render_sec(0.12); set(k, "0"); render_sec(0.05); }
static void lamps(const char* tag) {
  printf("%s lamps: voices ", tag);
  for (int i = 0; i < 10; i++) printf("%s", get(PARAM_KEY[IDX_VLAMP + i]).c_str());
  printf(" steps ");
  for (int i = 0; i < 16; i++) printf("%s", get(PARAM_KEY[IDX_SLAMP + i]).c_str());
  printf(" modes ");
  for (int i = 0; i < 6; i++) printf("%s", get(PARAM_KEY[IDX_MLAMP + i]).c_str());
  printf("\n");
}
static void screen(const char* tag) {
  printf("%s screen: [%s] [%s] [%s] [%s] [%s] wave=%s algo=%s\n", tag, get("scr_t1").c_str(), get("scr_t2").c_str(), get("scr_t3").c_str(),
         get("scr_t4").c_str(), get("scr_t5").c_str(), get("scr_wave").c_str(), get("scr_algo").c_str());
}
int main(int argc, char** argv) {
  E = mpc_engine();
  S = E->create(getenv("CARD") ? getenv("CARD") : nullptr);
  double pk, rms;
  render_sec(1.0, &pk, &rms); printf("idle: peak %.4f\n", pk);
  lamps("boot"); screen("boot");
  // enter STEP mode: latch on, off
  set("km_step", "1"); render_sec(0.2); lamps("step latched"); screen("step latched");
  set("km_step", "0"); render_sec(0.2); lamps("step released"); screen("step released");
  const char* steps[] = {"ks_1", "ks_5", "ks_9", "ks_13"};
  if (argc < 2) for (auto k : steps) tapk(k);
  lamps("after steps");
  if (getenv("DEFS")) { render_sec(0.5); for (int i = 0; i < 56; i++) { char v[8]; snprintf(v, 8, "%d", FADER_DEFAULT[i]); set(PARAM_KEY[IDX_FADER + i], v); render_sec(0.02); } }
  if (getenv("OVR")) { std::string o = getenv("OVR"); size_t p = 0; while (p < o.size()) { size_t q = o.find(';', p); if (q == std::string::npos) q = o.size(); std::string it = o.substr(p, q - p); p = q + 1; size_t c = it.find(':'); if (c == std::string::npos) continue; std::string suf = it.substr(0, c), val = it.substr(c + 1); for (int i = 0; i < 56; i++) if (strstr(FADER_KEY[i], suf.c_str())) { set(PARAM_KEY[IDX_FADER + i], val.c_str()); render_sec(0.02); } } }
  if (getenv("SUF") && getenv("SVAL")) { for (int i = 0; i < 56; i++) if (strstr(FADER_KEY[i], getenv("SUF"))) { set(PARAM_KEY[IDX_FADER + i], getenv("SVAL")); render_sec(0.02); } }
  if (getenv("LOUD")) { render_sec(0.5); for (int i = 0; i < 56; i++) { char v[8]; snprintf(v, 8, "%d", (i % 16 == 0 || i == 15) ? 120 : 90); set(PARAM_KEY[IDX_FADER + i], v); render_sec(0.02); } }
  tapk("k_run");
  render_sec(0.05);
  double mx = 0;
  for (int i = 0; i < 40; i++) { render_sec(0.1, &pk, &rms); if (pk > mx) mx = pk; if (getenv("FILL")) printf("pk=%f\n", pk); if (i % 5 == 0) lamps("run"); }
  printf("running: peak %.4f\n", mx);
  screen("run");
  set("op1_wav", "40"); render_sec(0.1); screen("touch wav");
  set("op1_algo", "100"); render_sec(0.1); screen("touch algo");
  set("bdc_vol", "127"); render_sec(0.1); screen("touch vol");
  render_sec(2.5); screen("home again");
  // state round trip
  char buf[16384]; int n = E->get_param(S, "state", buf, sizeof buf);
  printf("state: %d chars fill=%d\n", n, hw::audio_fill());
  for (int a = 0; a < 3; a++) printf("amp%d reg1=%02x reg2=%02x\n", a, hw::amp(a).reg1, hw::amp(a).reg2);
  printf("mix gain64=%d\n", hw::mix_gain64());
  FILE* f = fopen("/tmp/state.txt", "w"); fwrite(buf, 1, n, f); fclose(f);
  FILE* w = fopen("/tmp/out.raw", "wb"); fwrite(all.data(), 2, all.size(), w); fclose(w);
  E->destroy(S);
  // restore into a fresh instance: the programmed pattern and the sound must come back
  S = E->create(getenv("CARD") ? getenv("CARD") : nullptr);
  E->set_param(S, "state", buf);
  render_sec(1.0);
  lamps("restored");
  tapk("k_run");
  double rpk = 0, rr; for (int i = 0; i < 20; i++) { render_sec(0.1, &pk, &rr); if (pk > rpk) rpk = pk; }
  printf("restored: peak %.4f\n", rpk);
  int16_t ab[256];
  // host transport: play at 130 bpm for 2 s -> the machine must run (step lamps move) and follow
  { double ppq = 0; int blocks = 0; std::string chase;
    for (int i = 0; i < 2 * 44100 / 64; i++) { E->song_pos(S, ppq, 130.0, 1, 64); E->render(S, ab, 64); ppq += 64 * 130.0 / (60.0 * 44100); blocks++;
      if (i % 100 == 0) { machine::Snap sn = machine::snapshot(); char b[8]; snprintf(b, 8, "%d%s ", sn.step, sn.run ? "r" : ""); chase += b; } }
    printf("host play: step/run over time %s\n", chase.c_str());
    // RUN while slaved to the host: must stop the machine
  tapk("k_run"); { std::string c; for (int i = 0; i < 400; i++) { E->song_pos(S, ppq, 130.0, 1, 64); E->render(S, ab, 64); ppq += 64 * 130.0 / (60.0 * 44100); if (i % 100 == 0) { machine::Snap sn = machine::snapshot(); char b[8]; snprintf(b, 8, "%d%s ", sn.step, sn.run ? "r" : ""); c += b; } } printf("after RUN under host clock: %s\n", c.c_str()); }
  E->song_pos(S, ppq, 130.0, 0, 64); E->render(S, ab, 64); lamps("host stop"); }
  // HOLD: step mode, HOLD on, tap step 3 -> it stays down; move a fader -> a p-lock lands on step 3; release
  set("km_step", "1"); render_sec(0.1); set("km_step", "0"); render_sec(0.2);
  set("k_hold", "1"); render_sec(0.05); set("ks_3", "1"); render_sec(0.2); screen("held step 3"); printf("plocStep while held: %d\n", machine::snapshot().ploc_step);
  set("bdc_tune", "30"); render_sec(0.1); set("bdc_tune", "90"); render_sec(0.2);
  set("ks_3", "1"); render_sec(0.2); screen("released step 3"); printf("plocStep after release: %d\n", machine::snapshot().ploc_step); set("k_hold", "0"); render_sec(0.1); screen("hold off");
  E->destroy(S);
  fprintf(stderr, "rendered %.1f s of audio\n", rendered_frames / 44100.0);
  return 0;
}
