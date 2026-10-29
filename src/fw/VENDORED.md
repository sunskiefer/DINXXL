# Vendored: BlastBeats firmware

- Upstream: https://github.com/twistedelectrons/BlastBeats
- Commit: `9b562d0f8675a5171e94428a9e1ec13540392a9d` ("Release firmware 7.1 with Step Stutter fix", 2026-08-07)
- Files taken: `include/`, `src/`, `lib/miniz/src/` and `THIRD_PARTY_NOTICES.md`, byte for byte (checked with `cmp` against the
  upstream tree; nothing is patched). Not taken: `manual/`, `tool/`, `vendor/`, `linker/`, `scripts/`, `platformio.ini`.
- The firmware is built as two translation units (`../fw_unit.cpp`, `../fw_unit2.cpp`) that include the upstream `.cpp`/`.h`
  files; the Arduino/STM32 APIs it calls are supplied by `../shim/` and `../hw.cpp`, which model the board (OPL3 bus, MUX
  faders and keys, LED shift registers, I²C amplifiers, SD card, EEPROM, timers, MIDI serial).

## Licence

Upstream declares no licence for its own code (no `LICENSE` file; `THIRD_PARTY_NOTICES.md`'s release checklist still says
"Choose and add a top-level project license before publishing"). The third-party parts it vendors are MIT (Arduino OPL2,
uClock, miniz). See the top-level README, "Licence", for what this means for DINXXL.

## Known upstream behaviour kept as is

This is in the hardware firmware too and is left untouched so the port stays exact:

- `getProb()` (`include/engine.h`) reads `probStack[probCounter + 1]`; when `probCounter` is at its ceiling (19) that is one
  byte past the end of `probStack[20]`. On the STM32 this reads whatever global follows; here the same happens inside the
  firmware's own statics (the port's address-sanitiser test build flags it; it never faults).
