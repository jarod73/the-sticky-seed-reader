# The Sticky Seed Reader

[![Firmware](https://img.shields.io/badge/Firmware-ESP32--S3R8-00979D?style=for-the-badge&logo=espressif&logoColor=white)](https://www.seeedstudio.com)
[![Storage](https://img.shields.io/badge/Storage-32MB_Flash_+_8MB_Octal_PSRAM-orange?style=for-the-badge)](https://www.seeedstudio.com)
[![Display](https://img.shields.io/badge/Display-3.97"_800x480_E--Ink_Touch-blue?style=for-the-badge)](https://www.seeedstudio.com)
[![License](https://img.shields.io/badge/License-GPL_v3-green?style=for-the-badge)](LICENSE)

**The Sticky Seed Reader** is an open-source, high-performance connected e-reader and ambient intelligence firmware engineered exclusively for the **Seeed Studio reTerminal Sticky** (ESP32-S3).

Unlike generic multi-device firmwares, **The Sticky Seed Reader** is purpose-built to squeeze every ounce of capability from the reTerminal Sticky's rich silicon ecosystem—combining dual-core 240MHz Xtensa LX7 compute, 8MB Octal PSRAM, 32MB Flash, Goodix capacitive touch, environmental sensing, and digital audio.

---

## 🍃 Hardware Architecture & Platform Integration

The Sticky Seed Reader is tailored specifically for the following hardware specifications:

| Component | Specification | Integration in Firmware |
|---|---|---|
| **MCU** | ESP32-S3R8 (Dual-Core Xtensa LX7 @ 240MHz) | Dual-core task distribution: Core 0 for async WiFi, TLS & I/O; Core 1 for e-ink rendering & display blitting |
| **Memory** | **8MB Octal PSRAM** (OPI high-speed) + 512KB SRAM | Pre-rendered page caches, font decompression tables, network buffers, and voice recordings live in PSRAM, keeping SRAM free |
| **Flash** | **32MB Octal Flash** | Custom partition layout: dual 6.5MB OTA partitions + **18MB onboard SPIFFS** data partition |
| **Display** | 3.97" 800×480 E-Ink panel (SSD1677) | Single-buffer mode with hardware-tuned partial (~400ms) and full refresh waveforms, single-pass differential clearing |
| **Touch** | Goodix GT911 Capacitive Touch | Touch word selection, finger-tap page zones, full-screen swipe navigation, on-screen keyboard |
| **Battery Gauge** | TI BQ27220 I²C Fuel Gauge | Accurate state of charge (%), real-time current draw ($\text{mA}$), voltage, and estimated time-to-empty |
| **Climate Sensor** | Sensirion SHT40 | Ambient room temperature ($\pm0.2^\circ\text{C}$) & relative humidity ($\pm1.8\%\text{RH}$) |
| **Microphone** | Knowles PDM Digital Mic | 16kHz 16-bit mono voice recording directly to PSRAM WAV buffer |
| **Audio / Haptics**| Piezo Buzzer | 10ms 2.4kHz acoustic micro-clicks on touch tap feedback |
| **Real-Time Clock**| PCF8563 RTC | Battery-backed clock with scheduled alarm wakeups for morning briefings |
| **Motion Sensor** | LSM6DS3TR-C 6-Axis IMU | Tilt detection & orientation awareness |
| **Dual Storage** | 18MB Flash + MicroSD Slot | Shared SPI serialization (`HalStorage` mutex) with dynamic hot-plug card auto-detection |

---

## 🔄 Relationship to CrossPoint Reader

**The Sticky Seed Reader** is a specialized, hardware-focused evolution of the open-source CrossPoint Reader project.

### What Carried Over from CrossPoint:
- **Core EPUB Engine**: EPUB 2/3 rendering with hyphenation, kerning, image handling, embedded style support, footnotes, and bookmarks.
- **Multi-Format Support**: Native handling for `.epub`, `.txt`, `.xtc/.xtch`, and `.bmp`.
- **StarDict Offline Dictionary**: On-device dictionary lookup definitions from `.idx` / `.dict.dz` packages.
- **Visual Design Themes**: Classic, Lyra, Lyra Extended, and RoundedRaff themes with 30+ UI translations.
- **Wireless Library & Web Portal**: Browser-based file manager, Calibre wireless sync, and OPDS catalog client.

### What is Brand New in The Sticky Seed Reader:
- 📰 **The Daily Sticky (Morning Newspaper & Ambient Briefing)**:
  - Broad-sheet vintage two-column e-paper newspaper layout with headline story and wire briefs.
  - Live outdoor weather forecasts from **Open-Meteo** (no API key required).
  - Real-time room climate banner from the onboard **SHT40** sensor & **BQ27220** battery gauge.
  - Live RSS/Atom news wire with one-tap touch launch into full articles.
  - PCF8563 RTC scheduled alarm wakeup for fresh morning news with zero idle battery drain.
- 🌐 **Distraction-Free "Readability" Web Browser & Wikipedia**:
  - Linear single-pass HTML article extractor stripping ads, navbars, sidebars, scripts, and tracking noise.
  - Paginated e-ink reading with tap-to-turn zones and offline article saving.
  - Instant encyclopedic summary lookup cards via the **Wikipedia REST API** (zero API key required).
- 🔤 **Adjustable System UI Text Size & Font Scaling**:
  - 4 selectable system UI font tiers: **Small**, **Medium**, **Large (Default)**, and **Extra Large**.
  - Dynamic FreeInkUI layout engine that recalculates row heights, padding, header dimensions, and touch bounds from font line metrics.
- 📶 **Indestructible Wi-Fi & Cloud NVS Dual-Persistence**:
  - Automatically mirrors Wi-Fi credentials and cloud API tokens to onboard Flash Non-Volatile Storage (NVS).
  - Seamlessly survives firmware updates, reboots, and SD card swaps without losing passwords.
- ⚡ **Zero-Latency PSRAM Pre-Rendering Engine**:
  - Off-screen pre-rendering pipeline in 8MB PSRAM during the 400ms reading debounce window.
  - Reduces page flip latency to `<1ms` via direct frame buffer `memcpy`.
- 👆 **Capacitive Touch & Acoustic Micro-Click Feedback**:
  - Upgraded touch word selection with dual-pass proximity targeting for finger taps.
  - 10ms 2.4kHz acoustic tap feedback from the piezo buzzer on touch contact.
  - Full touchscreen edge gestures and swipe-to-home / swipe-to-back navigation.
- 🔄 **Direct GitHub OTA Updates**:
  - Check for and install the latest firmware releases directly from the GitHub repository over Wi-Fi.
- 🎓 **Offline Study & Note Export Suite**:
  - **Anki Flashcard Export**: One-tap export of looked-up vocabulary to `/.crosspoint/export/anki/vocab_anki.tsv`.
  - **Obsidian Markdown Export**: Export book notes, quotes, progress, and highlights to `/.crosspoint/export/notes/` with YAML frontmatter.
- 🔌 **High-Speed USB Serial Companion Protocol**:
  - 921,600 baud bidirectional USB-C protocol (`UsbSerialCompanion`) supporting WebSerial book drag-and-drop (`CMD:PUT`), JSON file listing (`CMD:LS`), telemetry streaming (`CMD:STATUS`), and synthetic input injection (`CMD:KEY`).
- 🎙️ **Voice Recording Pipeline**:
  - 16kHz 16-bit PDM digital microphone streaming to PSRAM with automatic RIFF WAV formatting.
- 🛡️ **Dual-Storage Resilience & Hot-Plug Auto-Detection**:
  - Boots and functions seamlessly using the 18MB internal SPIFFS partition if no SD card is present.
  - Dynamically detects MicroSD card insertion and ejection on the fly with SPI mutex bus locking.

---

## 🚀 Getting Started

### Prerequisites
- [PlatformIO CLI](https://platformio.org/) (`pio`) or PlatformIO IDE in VS Code.
- A **Seeed Studio reTerminal Sticky** connected via USB-C.

### Build & Flash Firmware

```bash
# 1. Clone the repository
git clone https://github.com/jarod73/crosspoint-reader-sticky.git "The-Sticky-Seed-Reader"
cd "The-Sticky-Seed-Reader"

# 2. Compile the sticky environment
pio run -e sticky

# 3. Flash to the connected reTerminal Sticky
pio run -e sticky -t upload
```

### Partition Layout (`partitions_sticky.csv`)

```
# Name,    Type, SubType, Offset,   Size,    Flags
nvs,       data, nvs,     0x9000,   20K,
otadata,   data, ota,     0xe000,   8K,
app0,      app,  ota_0,   0x10000,  6656K,   # Active Firmware Slot
app1,      app,  ota_1,   0x690000, 6656K,   # OTA Update Slot
spiffs,    data, spiffs,  0xd10000, 18M,     # Onboard Standalone Storage
coredump,  data, coredump,0x1f10000,64K,
```

---

## 📖 Navigation & Controls

| Action | Touch Gesture | Hardware Buttons |
|---|---|---|
| **Turn Page Forward** | Tap Right 67% of screen / Swipe Left | Bottom Front Button / Side Down |
| **Turn Page Back** | Tap Left 33% of screen / Swipe Right | Top Front Button / Side Up |
| **Dictionary Lookup** | Long-press or tap any word | Confirm Button (on selected word) |
| **Save to Anki** | Tap `[+Anki]` in definition header | Confirm Button in Dictionary |
| **Open Web Article** | Tap any story in The Daily Sticky | Confirm Button |
| **Reader Menu** | Tap top 20% margin | Left Button |
| **Return / Home** | Swipe Down from top / Swipe Right | Back Button |

---

## 🛠️ Project Structure

```
.
├── partitions_sticky.csv      # 32MB partition table (dual 6.5MB OTA + 18MB SPIFFS)
├── platformio.ini             # PlatformIO build configuration
├── freeink-sdk/               # Low-level display, touch, and peripheral drivers
├── lib/
│   ├── EpdFont/               # Compressed vector font glyph engine
│   ├── Epub/                  # EPUB 2/3 parsing, layout, and styling
│   ├── GfxRenderer/           # Single-buffer & PSRAM pre-render graphics engine
│   ├── hal/                   # Hardware Abstraction Layer (Display, GPIO, Storage Lock)
│   ├── I18n/                  # Internationalization string tables & translations
│   └── Memory/                # PSRAM allocation helpers (makeUniqueNoThrow, psram_malloc)
└── src/
    ├── activities/
    │   ├── ambient/           # MorningNewspaperActivity (The Daily Sticky)
    │   ├── boot_sleep/        # BootActivity & SleepActivity (branded ambient climate)
    │   ├── browser/           # ReadabilityBrowserActivity & WikipediaLookupActivity
    │   ├── home/              # HomeActivity & FileBrowserActivity
    │   ├── reader/            # EpubReaderActivity, Dictionary, Bookmarks
    │   └── settings/          # SettingsActivity, OtaUpdateActivity, FontDownloadActivity
    ├── components/            # UITheme, UIScale, FreeInkApp host bindings
    ├── network/               # CloudHttpClient, CloudCredentialStore, HttpDownloader
    └── util/                  # TextWrapUtils, ReadabilityExtractor, RssFeedParser, NoteExporter, VoiceRecorder
```

---

## 📄 License & Attribution

- **Firmware**: Licensed under the GNU General Public License v3.0 ([GPL-3.0](LICENSE)).
- **Upstream Credits**: Derived from the CrossPoint Reader project with deep modifications, hardware drivers, and connected intelligence features engineered specifically for Seeed Studio reTerminal Sticky hardware.
