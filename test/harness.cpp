// Offline check of the virtual machine (not shipped): boots the firmware, presses RUN, prints lamps, writes a WAV.
#include "../src/machine.h"
#include "../src/hw.h"
#include <stdio.h>
#include <vector>
#include <stdint.h>
static void lamps() {
  for (int r = 0; r < 8; r++) { printf("row%d ", r); for (int c = 7; c >= 0; c--) printf("%d", (hw::lamp_row(r) >> c) & 1); printf("  "); }
  printf("\n");
}
int main(int argc, char** argv) {
  machine::power_on(1);
  printf("booted t=%llu us\n", (unsigned long long)hw::now_us());
  std::vector<int16_t> pcm;
  auto render = [&](double sec) {
    int frames = (int)(sec * 44100);
    for (int done = 0; done < frames; done += 128) {
      int n = frames - done < 128 ? frames - done : 128;
      machine::run((uint32_t)(n * 1e6 / 44100.0));
      float b[256]; hw::audio_pull(b, n);
      for (int i = 0; i < 2 * n; i++) { float v = b[i] * 32767; pcm.push_back(v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v); }
    }
  };
  render(3.0);
  lamps();
  hw::set_run_key(true); render(0.05); hw::set_run_key(false);
  render(2.0);
  lamps();
  // toggle step keys 1,5,9,13 (step s<8 -> key 16+s, s>=8 -> key s), kick voice already selected
  render(0.1);
  hw::set_key(26, true); render(0.1); hw::set_key(26, false); render(0.3); printf("after STEP: "); lamps();
  { int steps[] = {16, 20, 8, 12}; auto tap = [&](int k) { hw::set_key(k, true); render(0.08); hw::set_key(k, false); render(0.08); };
    for (int i = 0; i < 56; i++) { hw::set_fader(i, (i % 16 == 0 || i == 15) ? 120 : 90); render(0.01); }
    tap(28); for (int k : steps) tap(k);
    size_t a = pcm.size(); render(3.0); int pk = 0; for (size_t i = a; i < pcm.size(); i++) { int x = pcm[i] < 0 ? -pcm[i] : pcm[i]; if (x > pk) pk = x; } printf("kick pattern peak %d\n", pk); lamps(); }
  int keys[] = {28, 0, 1, 2, 3, 4, 5, 31, 29, 6};
  for (int v = 0; v < 10; v++) { hw::set_key(keys[v], true); render(0.15); hw::set_key(keys[v], false); size_t a = pcm.size(); render(0.5); int pk = 0; for (size_t i = a; i < pcm.size(); i++) { int x = pcm[i] < 0 ? -pcm[i] : pcm[i]; if (x > pk) pk = x; } printf("voice key %d peak %d\n", v, pk); }
  FILE* f = fopen(argc > 1 ? argv[1] : "/tmp/out.wav", "wb");
  uint32_t n = pcm.size() * 2, sr = 44100, br = sr * 4; uint16_t ch = 2, bits = 16, fmt = 1, ba = 4; uint32_t sz = 36 + n, sixteen = 16;
  fwrite("RIFF", 1, 4, f); fwrite(&sz, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&sixteen, 4, 1, f); fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
  fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&n, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f);
  fclose(f);
  int peak = 0; for (auto v : pcm) { int a = v < 0 ? -v : v; if (a > peak) peak = a; }
  printf("samples %zu peak %d mixgain %d\n", pcm.size(), peak, hw::mix_gain64());
}
