# Vendored: ymfm

- Upstream: https://github.com/aaronsgiles/ymfm (Aaron Giles)
- Commit: `81aec25ccbb98f4873a255f7551ac4dadac59b4a` (2026-07-27)
- Files: `src/*` of that tree, unchanged. Licence: BSD-3-Clause (`LICENSE-ymfm`).
- DINXXL uses the YMF262 (OPL3) core only; `hw.cpp` clocks it at the chip rate (14.318181 MHz / 288 = 49716 Hz), sums the
  four outputs to the two mix buses the BlastBeats board has (L = A + C, R = B + D) and resamples to 44.1 kHz.
