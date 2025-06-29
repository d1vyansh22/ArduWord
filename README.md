# ArduWord

> **Arduino-powered Wordle console with touchscreen, LEDs, physical buttons, and SD-card dictionary**

ArduWord is a self-contained hardware remake of the New York Times Wordle puzzle. It combines an **Arduino Mega 2560**, a **2.4-inch TFT resistive touchscreen**, **five WS2812B addressable LEDs**, push-buttons, and an **SD-card** to deliver a tactile, instant-feedback word-guessing game that you can hold in your hands or mount as a desk gadget. This repository contains the complete firmware, wiring information, bill of materials, and build instructions so you can reproduce the project or adapt it to your own ideas.

---

<div align="center">
  <img src="docs/images/gameplay_overview.jpg" alt="ArduWord gameplay demonstration" width="600"/>
</div>

*Figure 1 – A full game in progress showing the on-screen keyboard, LED colour feedback, and physical button interface.*

---

## Table of Contents
1. [Project Highlights](#project-highlights)
2. [Hardware Overview](#hardware-overview)
3. [Firmware Overview](#firmware-overview)
4. [Bill of Materials](#bill-of-materials)
5. [Electrical Schematic & PCB](#electrical-schematic--pcb)
6. [Mechanical Assembly](#mechanical-assembly)
7. [Getting Started](#getting-started)
8. [Gameplay & Controls](#gameplay--controls)
9. [Configuration](#configuration)
10. [Troubleshooting](#troubleshooting)
11. [Roadmap](#roadmap)
12. [Contributing](#contributing)
13. [License](#license)
14. [Authors & Acknowledgements](#authors--acknowledgements)
15. [References](#references)

---

## Project Highlights
- **Touchscreen interface** – intuitive on-screen keyboard; no external keyboard required.
- **Dynamic LED feedback** – five WS2812B LEDs mirror the Wordle colour code: **green** (correct letter & position), **yellow/blue** (correct letter wrong position), **red** (letter absent).
- **Physical buttons** – PLUS/MINUS cycle through letters, NEXT/PREV move cursor, OK submits a guess, making the game accessible even without the touchscreen.
- **SD-card dictionary** – load thousands of 5-letter English words from `english.txt`; every power-on draws a random target word.
- **Portable** – 3.7 V Li-ion battery with onboard protection powers the unit for ~6 hours.
- **Open-source** – hardware design files (KiCad) and firmware (Arduino sketch) released under the MIT License.

## Hardware Overview
| Subsystem | Part | Purpose |
|-----------|------|---------|
| MCU & I/O | Arduino Mega 2560 | Brains of the game; plenty of flash, SRAM and GPIO |
| Display & Touch | 2.4-inch TFT (ILI9341) + 4-wire resistive panel | Renders keyboard, guesses, and messages; captures touch coordinates |
| LEDs | 5 × WS2812B RGB | Colour feedback visible beyond the screen |
| Storage | Micro-SD breakout | Stores the dictionary (~3 kB) and future statistics |
| User Buttons | Five PCB-mounted tact switches | Redundant controls and improved accessibility |
| Power | 1200 mAh Li-ion + TP4056 charger | Untethered operation and USB charging |
| Carrier PCB | Two-layer FR-4, 1.6 mm | Simplifies wiring, provides mounting holes |

A complete wiring table is provided in [`docs/wiring.md`](docs/wiring.md).

## Firmware Overview
The core sketch is `ArduWord.ino` (see `src/`). Key modules:

| File | Responsibility |
|------|----------------|
| `game_logic.cpp` | Word validity check, colour evaluation, win/lose logic |
| `ui_tft.cpp` | Rendering boxes, keyboard, pop-ups |
| `io_buttons.cpp` | Debouncing & event queues for tactile buttons |
| `storage_sd.cpp` | Random target word selection, statistics future-proofing |
| `led_feedback.cpp` | Animations on WS2812B strip via FastLED |

The firmware occupies ~75 kB of flash and <40 % SRAM, leaving headroom for extensions.

## Bill of Materials
| Qty | Component | Reference | Link | Approx Cost |
|-----|-----------|-----------|------|-------------|
| 1 | Arduino Mega 2560 | U1 | [Store](https://store.arduino.cc/products/mega-2560-r3) | 15 USD |
| 1 | 2.4″ TFT ILI9341 Shield | LCD1 | – | 6 USD |
| 1 | Micro-SD module | SD1 | – | 2 USD |
| 5 | WS2812B LED 5 V | D1-D5 | – | 1 USD |
| 5 | 6 × 6 mm tact switch | SW1-SW5 | – | 0.5 USD |
| 1 | TP4056 Li-ion charger PCB | CHG1 | – | 1 USD |
| 1 | 3.7 V 1200 mAh Li-ion cell | BAT1 | – | 4 USD |
| 1 | Custom 2-layer PCB 65 × 80 mm | PCB1 | – | 5 USD |
| – | Misc passive parts, headers, screws | – | – | 3 USD |
| **Total** |  |  |  | **~37 USD** |

## Electrical Schematic & PCB
The KiCad project lives in [`hardware/kicad/`](hardware/kicad/). Generated PDFs and Gerber files are in `hardware/outputs/`.

<div align="center">
  <img src="docs/images/schematic_thumb.png" alt="Top-level schematic thumbnail" width="550"/>
</div>

*Figure 2 – High-level schematic (see PDF for details).*  

An annotated PCB top-layer render is also available under `docs/images/pcb_top.png`.

## Mechanical Assembly
1. Order the PCB (`hardware/outputs/gerbers.zip`).
2. Solder all SMD parts **before** through-hole parts.
3. Ensure TFT shield headers seat flush to avoid stress.
4. Affix Li-ion cell to PCB rear using 1 mm VHB tape.
5. Slot the assembly into a 3-D-printed enclosure (`enclosure/`);
   secure with four M2.5 × 6 mm screws.
6. Stick a diffuser above the LED strip for smoother colours.

## Getting Started
### Prerequisites
- Arduino IDE ≥ 1.8.19 or VS Code + PlatformIO.
- Libraries (install via Library Manager):
  - `Adafruit_GFX`
  - `MCUFRIEND_kbv`
  - `TouchScreen`
  - `SdFat` (by Bill Greiman)
  - `FastLED`

### Firmware Upload
```bash
# clone project and sub-modules
$ git clone https://github.com/d1vyansh22/ArduWord.git
$ cd ArduWord

# open with Arduino IDE and select:
#  • Board: "Arduino Mega or Mega 2560"
#  • Port: your device
#  • Programmer: AVR ISP

# build & upload (Ctrl+U)
```

### Preparing the SD Card
1. Format a micro-SD as FAT32.
2. Copy `assets/english.txt` to the root directory.
3. Verify the file contains one uppercase **5-letter** English word per line (no spaces).

### Power-On Test
With everything wired, insert the SD, flip the power switch. A boot screen appears, LEDs flash once, and the first row of empty letter boxes is drawn.

## Gameplay & Controls
| Action | Touchscreen | Buttons |
|--------|-------------|---------|
| Cycle letter | tap ▲ / ▼ icons | PLUS / MINUS |
| Move cursor | tap next letter box | NEXT / PREV |
| Submit guess | tap **OK** | OK |
| Reset game | touch and hold lower-left corner 3 s | hold OK 5 s |

Players have **six attempts** to guess the hidden word. Colour meanings mirror the official Wordle rules.

## Configuration
You can tweak game parameters in `src/config.h`:
- `WORD_LENGTH` – default 5.
- `MAX_TRIES` – default 6.
- `LED_BRIGHTNESS` – 0-255, default 96.
- `DICT_PATH` – path to dictionary file on SD.

## Troubleshooting
| Symptom | Possible Cause | Fix |
|---------|----------------|-----|
| TFT remains white | Shield ID not recognised | Uncomment custom ID in `setup()` |
| Touch coords inverted | Swap `XP`/`XM` or run `TouchScreen_Calibr_native.ino` |
| SD init fails | CS pin clash with TFT (default 10) | Cut trace on shield, reroute to 53 |
| LEDs random colours | 5 V rail sag or missing GND | Solder thicker 5 V trace and common ground |
| High current draw when off | Battery wired before switch | Re-route switch between battery + and load |

## Roadmap
- [ ] Stats screen: win streak, distribution.
- [ ] Six-letter & hard mode.
- [ ] Bluetooth serial logging.
- [ ] 3-D-printable case for handheld form factor.

## Contributing
Pull requests are welcome! Please fork the repo and create a feature branch:
```bash
git checkout -b feature/my-new-idea
```
Include:
1. A short description in `CHANGELOG.md`.
2. Updated docs or screenshots where relevant.
3. Follow the existing code style (`clang-format`).

## License
```
MIT License
Copyright (c) 2025 Divyansh Gupta, Akshit Jaiswal, Dakshish Jethwa
```
See `LICENSE` for the full text.

## Authors & Acknowledgements
Created by **Divyansh Gupta**, **Akshit Jaiswal**, and **Dakshish Jethwa** for the *Electronics Design Workshop* under the mentorship of **Prof. D. V. Gadre** and **Ms. Umaisa Hassan** at Netaji Subhash University of Technology, New Delhi.

Special thanks to the open-source community – particularly the authors of **MCUFRIEND_kbv**, **FastLED**, and **SdFat** – whose libraries made this project possible.

## References
Key resources consulted while building ArduWord:
- New York Times Wordle rules – <https://www.nytimes.com/games/wordle/index.html>
- `MCUFRIEND_kbv` docs – <https://github.com/prenticedavid/MCUFRIEND_kbv>
- `FastLED` wiki – <https://github.com/FastLED/FastLED/wiki>
- `SdFat` API – <https://github.com/greiman/SdFat>

*Happy guessing & hacking!*