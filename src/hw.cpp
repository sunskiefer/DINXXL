#include "dsp/ymfm_opl.h"
#include "Arduino.h"
#include "SD.h"
#include "hw.h"
#include <vector>
#include <deque>
#include <map>
#include <stdio.h>
#include "miniz_tinfl.h"

// ---------- globals the firmware expects from the core ----------
HardwareSerial Serial;
SPIShim SPI;
TwoWireShim Wire;
TIM_TypeDef TIM4_{4}, TIM5_{5};

namespace {
// pins the firmware uses (STM32 R2 numbering, see fw/src/globals.h)
enum {
  P_MUX1 = PA2, P_MUX2 = PA3, P_MUX3 = PA0, P_MUX4 = PB13, P_MUX5 = PB14, P_MUX6 = PA4,
  P_SYNC_DETECT = PB8, P_SYNC_IN = PB9, P_RUN = PB0,
  P_RW = PB5, P_RESET = PC8, P_CS = PC11, P_A0 = PC9, P_A1 = PC10,
  P_LED_DIN = PA7, P_LED_CLK = PA5, P_LED_CS = PB11,
};

struct Tim { HardwareTimer* t = nullptr; };
std::vector<HardwareTimer*> g_timers;

uint64_t g_now = 0;          // virtual microseconds
int g_irq_off = 0;
bool g_in_isr = false;
uint32_t g_rng = 1;

uint8_t g_pin[NUM_PINS_];
uint8_t g_fader7[hw::N_FADERS];   // raw 7-bit reading
bool g_key[hw::N_KEYS];
bool g_run_key = false;

uint8_t g_leds[8];
// MAX7219 shift register capture
std::vector<uint8_t> g_sr;

hw::Amp g_amp[3];
int g_amp_sel = 0;
int g_wire_addr = 0;
std::vector<uint8_t> g_wire_buf;

std::deque<uint8_t> g_midi_in, g_midi_out;

// ---- sound chip ----
struct Iface : ymfm::ymfm_interface {};
Iface g_if;
ymfm::ymf262* g_chip = nullptr;
const double OPL_RATE = 14318181.0 / 288.0;
uint64_t g_opl_done = 0;      // native samples generated so far
std::vector<float> g_ring_l, g_ring_r;     // native-rate FIFO
size_t g_ring_head = 0;       // index of first unread sample
double g_phase = 0;           // fractional read position into FIFO
int g_ring_n = 0;

void opl_generate_to(uint64_t t_us) {
  uint64_t want = (uint64_t)((double)t_us * OPL_RATE / 1e6);
  while (g_opl_done < want) {
    ymfm::ymf262::output_data o;
    g_chip->generate(&o, 1);
    // outputs A,B,C,D -> mix bus L = A + C, R = B + D
    float a = o.data[0], b = o.data[1], c = o.data[2], d = o.data[3];
    if (g_ring_l.size() - g_ring_head > 49716) { g_ring_l.erase(g_ring_l.begin(), g_ring_l.begin() + g_ring_head); g_ring_r.erase(g_ring_r.begin(), g_ring_r.begin() + g_ring_head); g_ring_head = 0; }   // nobody listening: keep one second
    g_ring_l.push_back((a + c) * (1.0f / 32768.0f));
    g_ring_r.push_back((b + d) * (1.0f / 32768.0f));
    g_opl_done++;
  }
}

int mux_sel() {
  // DATA0..3 (PC0..PC3) double as the mux address lines
  return (g_pin[PC0] & 1) | ((g_pin[PC1] & 1) << 1) | ((g_pin[PC2] & 1) << 2) | ((g_pin[PC3] & 1) << 3);
}

void opl_latch() {
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) data |= (g_pin[PC0 + i] & 1) << i;
  bool a0 = g_pin[P_A0], a1 = g_pin[P_A1];
  opl_generate_to(g_now);
  g_chip->write((a1 ? 2 : 0) + (a0 ? 1 : 0), data);
}

void fire_due(uint64_t target) {
  for (;;) {
    HardwareTimer* best = nullptr;
    for (auto* t : g_timers)
      if (t->running_ && t->cb_ && t->period_ && (!best || t->next_ < best->next_)) best = t;
    if (!best || best->next_ > target) break;
    g_now = best->next_;
    best->next_ += best->period_;
    if (!g_in_isr && g_irq_off == 0) {
      g_in_isr = true;
      best->cb_();
      g_in_isr = false;
    }
  }
  g_now = target;
}
}  // namespace

// ---------- Arduino core ----------
void pinMode(uint8_t pin, uint8_t mode) {
  if (pin < NUM_PINS_ && mode == INPUT_PULLUP) g_pin[pin] = 1;
}
void digitalWrite(uint8_t pin, uint8_t val) {
  if (pin >= NUM_PINS_) return;
  uint8_t old = g_pin[pin];
  g_pin[pin] = val ? 1 : 0;
  if (pin == P_RW && !old && val && !g_pin[P_CS]) opl_latch();          // WR rising while CS low
  if (pin == P_RESET && !old && val) {                                   // chip reset released
    opl_generate_to(g_now);
    g_chip->reset();
  }
  if (pin == P_LED_CS) {
    if (val) {                                   // latch: [reg, value]
      if (g_sr.size() >= 2 && g_sr[0] >= 1 && g_sr[0] <= 8) g_leds[g_sr[0] - 1] = g_sr[1];
      g_sr.clear();
    } else g_sr.clear();
  }
}
int digitalRead(uint8_t pin) {
  int m = mux_sel();
  if (pin == P_MUX4) return g_key[m] ? 0 : 1;
  if (pin == P_MUX5) return g_key[m + 16] ? 0 : 1;
  if (pin == P_RUN) return g_run_key ? 0 : 1;
  if (pin == P_SYNC_DETECT || pin == P_SYNC_IN) return 1;   // no analog sync cable
  if (pin < NUM_PINS_) return g_pin[pin];
  return 1;
}
int analogRead(uint8_t pin) {
  int m = mux_sel();
  int idx = -1;
  if (pin == P_MUX1) idx = m;
  else if (pin == P_MUX2) idx = m + 16;
  else if (pin == P_MUX3) idx = m + 32;
  else if (pin == P_MUX6) idx = m < 8 ? m + 48 : -1;
  if (idx < 0) return 0;
  return ((int)g_fader7[idx] << 5) | 0x10;
}
void analogReadResolution(int) {}
void analogReadRes(int) {}
uint32_t millis() { return (uint32_t)(g_now / 1000); }
uint32_t micros() { return (uint32_t)g_now; }
void delay(uint32_t ms) { fire_due(g_now + (uint64_t)ms * 1000); }
void delayMicroseconds(uint32_t us) { fire_due(g_now + us); }
void shiftOut(uint8_t, uint8_t, uint8_t, uint8_t val) {
  if (g_pin[P_LED_CS] == 0) g_sr.push_back(val);
}
static uint32_t rnd32() { g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5; return g_rng; }
long random(long maxv) { return maxv <= 0 ? 0 : (long)(rnd32() % (uint32_t)maxv); }
long random(long minv, long maxv) { return minv >= maxv ? minv : minv + random(maxv - minv); }
void randomSeed(unsigned long s) { g_rng = s ? (uint32_t)s : 1; }
void noInterrupts() { g_irq_off++; }
void interrupts() { if (g_irq_off) g_irq_off--; }

int HardwareSerial::available() { return (int)g_midi_in.size(); }
int HardwareSerial::read() { if (g_midi_in.empty()) return -1; int b = g_midi_in.front(); g_midi_in.pop_front(); return b; }
int HardwareSerial::peek() { return g_midi_in.empty() ? -1 : g_midi_in.front(); }
size_t HardwareSerial::write(uint8_t b) {
  if (this != &Serial && g_midi_out.size() < 4096) g_midi_out.push_back(b);
  return 1;
}

void HardwareTimer::reg_() {
  for (auto* t : g_timers) if (t == this) return;
  g_timers.push_back(this);
}
void HardwareTimer::setOverflow(uint32_t v, int fmt) { (void)fmt; period_ = v; }
void HardwareTimer::pause() { running_ = false; }
void HardwareTimer::resume() { reg_(); running_ = true; next_ = g_now + period_; }
void HardwareTimer::refresh() { next_ = g_now + period_; }
void HardwareTimer::attachInterrupt(void (*cb)()) { cb_ = cb; reg_(); }

void TwoWireShim::beginTransmission(uint8_t a) { g_wire_addr = a; g_wire_buf.clear(); }
size_t TwoWireShim::write(uint8_t b) { g_wire_buf.push_back(b); return 1; }
uint8_t TwoWireShim::endTransmission() {
  // mux 0 = out 1-4 amp, 1 = phones amp, 2 = mix amp (selected by the mux lines)
  int sel = mux_sel();
  if (g_wire_addr == 0x60 && g_wire_buf.size() >= 2 && sel <= 2) {
    if (g_wire_buf[0] == 1) g_amp[sel].reg1 = g_wire_buf[1];
    if (g_wire_buf[0] == 2) g_amp[sel].reg2 = g_wire_buf[1];
  }
  return 0;
}

// ---------- SD card ----------
namespace vfs {
std::map<std::string, std::shared_ptr<Node>>& files() { static std::map<std::string, std::shared_ptr<Node>> f; return f; }
std::vector<std::string>& dirs() { static std::vector<std::string> d; return d; }
static std::string norm(const char* p) { std::string s = p; if (s.empty() || s[0] != '/') s = "/" + s; while (s.size() > 1 && s.back() == '/') s.pop_back(); return s; }
}
int File::read(void* buf, size_t n) {
  if (!node_) return 0;
  size_t avail = pos_ < node_->data.size() ? node_->data.size() - pos_ : 0;
  size_t k = n < avail ? n : avail;
  if (k) memcpy(buf, node_->data.data() + pos_, k);
  pos_ += k;
  return (int)k;
}
size_t File::write(const uint8_t* b, size_t n) {
  if (!node_ || !write_) return 0;
  if (pos_ + n > node_->data.size()) node_->data.resize(pos_ + n);
  memcpy(node_->data.data() + pos_, b, n);
  pos_ += n;
  return n;
}
File File::openNextFile() {
  File f;
  std::string prefix = path_ == "/" ? "/" : path_ + "/";
  size_t i = 0;
  for (auto& kv : vfs::files()) {
    if (kv.first.compare(0, prefix.size(), prefix) != 0) continue;
    std::string rest = kv.first.substr(prefix.size());
    if (rest.find('/') != std::string::npos) continue;
    if (i++ < idx_) continue;
    idx_ = i;
    f.open_ = true; f.node_ = kv.second; f.name_ = rest; f.path_ = kv.first;
    return f;
  }
  for (auto& d : vfs::dirs()) {
    if (d.compare(0, prefix.size(), prefix) != 0) continue;
    std::string rest = d.substr(prefix.size());
    if (rest.empty() || rest.find('/') != std::string::npos) continue;
    if (i++ < idx_) continue;
    idx_ = i;
    f.open_ = true; f.isDir_ = true; f.name_ = rest; f.path_ = d;
    return f;
  }
  return f;
}
File BlastBeatsSDClass::open(const char* path, oflag_t mode) {
  File f;
  std::string p = vfs::norm(path);
  auto& fs = vfs::files();
  auto it = fs.find(p);
  if (mode == FILE_WRITE) {
    if (it == fs.end()) { auto n = std::make_shared<vfs::Node>(); fs[p] = n; it = fs.find(p); }
    f.write_ = true; f.node_ = it->second; f.open_ = true; f.name_ = p.substr(p.rfind('/') + 1); f.path_ = p;
    f.pos_ = f.node_->data.size();   // SD FILE_WRITE appends
    return f;
  }
  if (it != fs.end()) { f.node_ = it->second; f.open_ = true; f.name_ = p.substr(p.rfind('/') + 1); f.path_ = p; return f; }
  for (auto& d : vfs::dirs()) if (d == p) { f.open_ = true; f.isDir_ = true; f.path_ = p; f.name_ = p.substr(p.rfind('/') + 1); return f; }
  if (p == "/") { f.open_ = true; f.isDir_ = true; f.path_ = p; }
  return f;
}
bool BlastBeatsSDClass::exists(const char* path) {
  std::string p = vfs::norm(path);
  if (vfs::files().count(p) || p == "/") return true;
  for (auto& d : vfs::dirs()) if (d == p) return true;
  return false;
}
bool BlastBeatsSDClass::remove(const char* path) { return vfs::files().erase(vfs::norm(path)) > 0; }
bool BlastBeatsSDClass::rename(const char* a, const char* b) {
  auto& fs = vfs::files();
  auto it = fs.find(vfs::norm(a));
  if (it == fs.end()) return false;
  auto n = it->second; fs.erase(it); fs[vfs::norm(b)] = n; return true;
}
bool BlastBeatsSDClass::mkdir(const char* path) {
  std::string p = vfs::norm(path);
  std::string cur;
  size_t i = 1;
  while (i <= p.size()) {
    size_t j = p.find('/', i); if (j == std::string::npos) j = p.size();
    cur = p.substr(0, j);
    bool have = false; for (auto& d : vfs::dirs()) if (d == cur) have = true;
    if (!have) vfs::dirs().push_back(cur);
    i = j + 1;
  }
  return true;
}
bool BlastBeatsSDClass::rmdir(const char* path) {
  std::string p = vfs::norm(path);
  std::string prefix = p + "/";
  for (auto& kv : vfs::files()) if (kv.first.compare(0, prefix.size(), prefix) == 0) return false;
  auto& d = vfs::dirs();
  for (size_t i = 0; i < d.size(); i++) if (d[i] == p) { d.erase(d.begin() + i); return true; }
  return false;
}

// ---------- firmware entry points (fw_unit.cpp) ----------
void bb_setup();
void bb_loop();

namespace hw {
void reset(uint32_t seed) {
  g_timers.clear();
  g_now = 0; g_irq_off = 0; g_in_isr = false;
  memset(g_pin, 1, sizeof g_pin);
  memset(g_leds, 0, sizeof g_leds);
  memset(g_key, 0, sizeof g_key);
  g_run_key = false;
  for (auto& f : g_fader7) f = 64;
  for (auto& a : g_amp) a = Amp();
  g_sr.clear(); g_midi_in.clear(); g_midi_out.clear();
  g_rng = seed ? seed : 1;
  delete g_chip;
  g_chip = new ymfm::ymf262(g_if);
  g_chip->reset();
  g_opl_done = 0; g_ring_l.clear(); g_ring_r.clear(); g_ring_head = 0; g_phase = 0;
}
void advance(uint32_t us) { fire_due(g_now + us); opl_generate_to(g_now); }
uint64_t now_us() { return g_now; }

void set_fader(int i, int v127) { if (i >= 0 && i < N_FADERS) g_fader7[i] = (uint8_t)(127 - (v127 < 0 ? 0 : v127 > 127 ? 127 : v127)); }
int get_fader(int i) { return 127 - g_fader7[i]; }
void set_key(int i, bool d) { if (i >= 0 && i < N_KEYS) g_key[i] = d; }
void set_run_key(bool d) { g_run_key = d; }

uint8_t lamp_row(int r) { return g_leds[r & 7]; }
bool lamp(int n) {
  int row, col;
  switch (n) {
    case 0 ... 7: row = n; col = 1; break;
    case 8 ... 15: row = n - 8; col = 2; break;
    case 16: row = 4; col = 3; break;
    case 17: row = 0; col = 4; break;
    case 18: row = 1; col = 4; break;
    case 19: row = 2; col = 4; break;
    case 20: row = 3; col = 4; break;
    case 21: row = 4; col = 4; break;
    case 22: row = 5; col = 4; break;
    case 23: row = 7; col = 3; break;
    case 24: row = 5; col = 3; break;
    case 25: row = 6; col = 4; break;
    case 40: row = 6; col = 3; break;
    case 41: row = 2; col = 3; break;
    case 42: row = 3; col = 3; break;
    case 50: row = 7; col = 4; break;
    case 51: row = 0; col = 3; break;
    case 52: row = 1; col = 3; break;
    default: return false;
  }
  return (g_leds[row] >> (7 - col)) & 1;
}
const Amp& amp(int w) { return g_amp[w]; }
int mix_gain64() { return g_amp[2].reg2 & 0x3f; }

void audio_pull(float* lr, int frames) {
  // resample native OPL rate -> 44.1 kHz (linear)
  const double step = OPL_RATE / 44100.0;
  const Amp& m = g_amp[2];
  double dB = (double)(m.reg2 & 0x3f) - 63.0;   // TPA6130A2: 1 dB per step, 63 = reference
  float gain = (float)pow(10.0, dB / 20.0);
  bool muteL = m.reg2 & 0x80, muteR = m.reg2 & 0x40;
  for (int i = 0; i < frames; i++) {
    size_t base = g_ring_head;
    size_t i0 = base + (size_t)g_phase;
    float L = 0, R = 0;
    if (i0 + 1 < g_ring_l.size()) {
      float fr = (float)(g_phase - floor(g_phase));
      L = g_ring_l[i0] + (g_ring_l[i0 + 1] - g_ring_l[i0]) * fr;
      R = g_ring_r[i0] + (g_ring_r[i0 + 1] - g_ring_r[i0]) * fr;
    }
    lr[2 * i] = muteL ? 0 : L * gain;
    lr[2 * i + 1] = muteR ? 0 : R * gain;
    g_phase += step;
  }
  size_t used = (size_t)g_phase;
  if (used > g_ring_l.size() - g_ring_head) { used = g_ring_l.size() - g_ring_head; g_phase = g_phase - floor(g_phase) + (double)used; }   // underrun: never run past the data, drop the backlog
  if (used) {
    g_phase -= (double)used; if (g_phase < 0) g_phase = 0;
    g_ring_head += used;
    if (g_ring_head > 8192) {
      g_ring_l.erase(g_ring_l.begin(), g_ring_l.begin() + g_ring_head);
      g_ring_r.erase(g_ring_r.begin(), g_ring_r.begin() + g_ring_head);
      g_ring_head = 0;
    }
  }
}
void audio_trim(int keep) {   // drop the newest samples beyond `keep` (time the machine spent in a blocking save)
  int fill = (int)(g_ring_l.size() - g_ring_head);
  if (fill > keep && keep >= 0) { g_ring_l.resize(g_ring_head + keep); g_ring_r.resize(g_ring_head + keep); g_phase = 0; }
}
int audio_fill() { return (int)(g_ring_l.size() - g_ring_head); }

void midi_in(uint8_t b) { if (g_midi_in.size() < 4096) g_midi_in.push_back(b); }
bool midi_out(uint8_t* b) { if (g_midi_out.empty()) return false; *b = g_midi_out.front(); g_midi_out.pop_front(); return true; }

std::string sd_save() {
  std::string out;
  auto put32 = [&](uint32_t v) { out.append((const char*)&v, 4); };
  put32(0x44494E31);  // 'DIN1'
  put32((uint32_t)vfs::dirs().size());
  for (auto& d : vfs::dirs()) { put32((uint32_t)d.size()); out += d; }
  put32((uint32_t)vfs::files().size());
  for (auto& kv : vfs::files()) {
    put32((uint32_t)kv.first.size()); out += kv.first;
    put32((uint32_t)kv.second->data.size()); out.append((const char*)kv.second->data.data(), kv.second->data.size());
  }
  return out;
}
void sd_load(const std::string& s) {
  vfs::files().clear(); vfs::dirs().clear();
  size_t p = 0;
  auto get32 = [&](uint32_t& v) { if (p + 4 > s.size()) return false; memcpy(&v, s.data() + p, 4); p += 4; return true; };
  uint32_t magic, n;
  if (!get32(magic) || magic != 0x44494E31) return;
  if (!get32(n)) return;
  for (uint32_t i = 0; i < n; i++) { uint32_t l; if (!get32(l) || p + l > s.size()) return; vfs::dirs().push_back(s.substr(p, l)); p += l; }
  if (!get32(n)) return;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t l, dl; if (!get32(l) || p + l > s.size()) return;
    std::string name = s.substr(p, l); p += l;
    if (!get32(dl) || p + dl > s.size()) return;
    auto node = std::make_shared<vfs::Node>(); node->data.assign(s.begin() + p, s.begin() + p + dl); p += dl;
    vfs::files()[name] = node;
  }
}
}  // namespace hw

// ---------- factory card ----------
namespace {
std::string g_factory_blob;                                   // the DIN1 image, kept to reset and to diff against
std::map<std::string, std::vector<uint8_t>> g_factory_files;  // its files
}
namespace hw {
bool sd_load_factory_file(const std::string& path) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return false;
  std::string z; char tmp[65536]; size_t r;
  while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) z.append(tmp, r);
  fclose(f);
  size_t out_len = 0;
  void* out = tinfl_decompress_mem_to_heap(z.data(), z.size(), &out_len, TINFL_FLAG_PARSE_ZLIB_HEADER);
  if (!out) return false;
  g_factory_blob.assign((const char*)out, out_len);
  free(out);
  sd_load(g_factory_blob);
  g_factory_files.clear();
  for (auto& kv : vfs::files()) g_factory_files[kv.first] = kv.second->data;
  return !g_factory_files.empty();
}
bool sd_has_factory() { return !g_factory_files.empty(); }
void sd_reset_to_factory() { if (!g_factory_blob.empty()) sd_load(g_factory_blob); }

// 'DIN2': files that differ from or are missing in the factory card, then the factory files that were deleted
std::string sd_save_diff() {
  std::string out;
  auto put32 = [&](uint32_t v) { out.append((const char*)&v, 4); };
  std::vector<const std::pair<const std::string, std::shared_ptr<vfs::Node>>*> changed;
  for (auto& kv : vfs::files()) {
    auto it = g_factory_files.find(kv.first);
    // The factory card's files are padded (512-byte .BB files, a longer EEPROM) while the firmware writes exactly the
    // bytes it reads back, so a file the firmware re-saved unchanged is the factory file's prefix: that is "unchanged".
    bool same = it != g_factory_files.end() && kv.second->data.size() <= it->second.size() &&
                memcmp(kv.second->data.data(), it->second.data(), kv.second->data.size()) == 0;
    if (!same) changed.push_back(&kv);
  }
  std::vector<const std::string*> deleted;
  for (auto& kv : g_factory_files) if (!vfs::files().count(kv.first)) deleted.push_back(&kv.first);
  put32(0x44494E32);  // 'DIN2'
  put32((uint32_t)changed.size());
  for (auto* kv : changed) {
    put32((uint32_t)kv->first.size()); out += kv->first;
    put32((uint32_t)kv->second->data.size()); out.append((const char*)kv->second->data.data(), kv->second->data.size());
  }
  put32((uint32_t)deleted.size());
  for (auto* d : deleted) { put32((uint32_t)d->size()); out += *d; }
  return out;
}
void sd_apply_diff(const std::string& s) {
  size_t p = 0;
  auto get32 = [&](uint32_t& v) { if (p + 4 > s.size()) return false; memcpy(&v, s.data() + p, 4); p += 4; return true; };
  uint32_t magic, n;
  if (!get32(magic) || magic != 0x44494E32) return;
  if (!get32(n)) return;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t l, dl; if (!get32(l) || p + l > s.size()) return;
    std::string name = s.substr(p, l); p += l;
    if (!get32(dl) || p + dl > s.size()) return;
    auto node = std::make_shared<vfs::Node>(); node->data.assign(s.begin() + p, s.begin() + p + dl); p += dl;
    vfs::files()[name] = node;
    size_t slash = name.rfind('/');
    if (slash > 0 && slash != std::string::npos) SD.mkdir(name.substr(0, slash).c_str());
  }
  if (!get32(n)) return;
  for (uint32_t i = 0; i < n; i++) { uint32_t l; if (!get32(l) || p + l > s.size()) return; vfs::files().erase(s.substr(p, l)); p += l; }
}
}  // namespace hw
