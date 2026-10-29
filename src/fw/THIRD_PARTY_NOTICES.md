# Third-Party Notices

This project includes or depends on the following third-party code. Commercial use is allowed by these licenses, but keep the notices and comply with the listed redistribution terms when shipping source, firmware binaries, or hardware.

## Vendored Code

| Component | Files | License | Notes |
| --- | --- | --- | --- |
| Arduino OPL2 by DhrBaksteen / Maarten Janssen | `include/OPL2*`, adapted `include/OPL2BBST*` | MIT | Keep copyright and MIT license notice with distributions. |
| uClock by Romulo Silva / midilab | `include/uClock*`, `include/platforms/stm32.h` | MIT | Keep copyright and MIT license notice with distributions. |
| miniz inflate subset by Rich Geldreich, Tenacious Software, RAD Game Tools, Valve Software | `lib/miniz/src/*` | MIT | Keep copyright and MIT license notice with distributions. |
| STM32 Arduino USB MIDI framework patch | `vendor/stm32-usb-midi/stm32/usb/*` | ST Ultimate Liberty SLA0044 / file headers | Copied into PlatformIO's STM32 Arduino framework by `scripts/set_stm32_progname.py` so clean builds get MIDI USB descriptors and endpoints. |
| PJRC Teensy 3 linker script | `linker/teensy3/mk20dx256-blastbeats.ld` | PJRC permissive license / file header | Project-local Teensy 3.1/3.2 linker script with flash origin set to `0x00005080` for the BlastBeats bootloader layout. |

## PlatformIO Dependencies

These are fetched by `platformio.ini` during builds rather than vendored in this repository.

| Dependency | Declared In | License | Notes |
| --- | --- | --- | --- |
| SdFat by Bill Greiman | `platformio.ini` / `greiman/SdFat` | MIT | Used through `include/SD.h`, a small local compatibility wrapper for the old Arduino SD API calls used by this project. |
| Arduino MIDI Library by FortySevenEffects | `platformio.ini` / `fortyseveneffects/MIDI Library` | MIT | Keep copyright and MIT license notice with distributions. |
| Teensy and STM32 Arduino cores/frameworks | `platformio.ini` platform packages | Mixed permissive/LGPL/vendor licenses | Include the exact PlatformIO package versions used for a release in build notes and keep their license notices with source or binary firmware releases. |

## Release Checklist

- Choose and add a top-level project license before publishing.
- Keep this notice file in source and firmware release packages.
- For firmware binaries shipped with hardware, publish corresponding source, build scripts, and dependency versions.
- For any LGPL components that remain through Arduino cores or future dependencies, provide relinkable object files or another compliant path for users to rebuild firmware with modified LGPL libraries.
- Confirm rights for non-code assets before distribution: `vgm/*.vgz`, `manual/assets/*`, `tool/title.png`, and files under `firmware/`.
