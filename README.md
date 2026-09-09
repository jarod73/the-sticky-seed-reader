# The Sticky Seed Reader

[![Firmware](https://img.shields.io/badge/Firmware-ESP32--S3R8-00979D?style=for-the-badge&logo=espressif&logoColor=white)](https://www.seeedstudio.com)
[![Storage](https://img.shields.io/badge/Storage-32MB_Flash_+_8MB_Octal_PSRAM-orange?style=for-the-badge)](https://www.seeedstudio.com)
[![Display](https://img.shields.io/badge/Display-3.97"_800x480_E--Ink_Touch-blue?style=for-the-badge)](https://www.seeedstudio.com)
[![License](https://img.shields.io/badge/License-GPL_v3-green?style=for-the-badge)](LICENSE)

**The Sticky Seed Reader** is a high-performance open-source connected e-reader, study station, and ambient intelligence firmware engineered exclusively for the **Seeed Studio reTerminal Sticky** (ESP32-S3).

Unlike generic multi-device firmwares, **The Sticky Seed Reader** is purpose-built to squeeze every ounce of capability from the reTerminal Sticky's rich silicon ecosystem—combining dual-core 240MHz Xtensa LX7 compute, 8MB Octal PSRAM, 32MB Flash, Goodix capacitive touch, environmental sensing, digital audio recording, and dynamic IP/GPS location tracking.

---

## ⚖️ Detailed Comparison: The Sticky Seed Reader vs. Original CrossPoint

| Capability / Feature | Original CrossPoint Reader | The Sticky Seed Reader (reTerminal Sticky) |
|---|---|---|
| **Primary Target Hardware** | Generic ESP32 boards (Xteink X3/X4, PaperS3, T5) | **Seeed Studio reTerminal Sticky (ESP32-S3R8)** |
| **Silicon Architecture** | Quad SPI Flash / standard PSRAM | **32MB Octal Flash + 8MB High-Speed OPI Octal PSRAM** |
| **Partition Layout** | Standard 4MB / 16MB layouts | **Custom 32MB Table**: Dual 6.5MB OTA slots + **18MB SPIFFS data partition** |
| **Memory Management** | Single-pass sequential rendering | **Zero-Latency PSRAM Pre-Rendering Engine** ($<1\text{ms}$ flip latency) |
| **Text Layout Engine** | Duplicated text wrapping loops across views | **Centralized `TextWrapUtils`**: Width-bounded metric wrapping & pagination |
| **Web Parser Engine** | Stream-heavy string allocations | **Zero-Copy Streaming $O(N)$ `ReadabilityExtractor`** (saved **218.8 KB Flash**) |
| **Ambient Mode** | Basic static clock / image screensaver | **The Daily Sticky**: Broadsheet morning paper with live weather, climate, & news wire |
| **Weather & Climate** | No weather integration | **Open-Meteo Integration + Onboard Sensirion SHT40 Temperature & Humidity** |
| **Battery Gauge Telemetry** | Basic voltage lookup curve | **TI BQ27220 Fuel Gauge**: Exact %, mA current draw, mV voltage, & time-to-empty |
| **Location & Timezone** | Static manual clock configuration | **Dynamic GPS & IP-Geolocation**: Auto-detects city, coordinates, & timezone on the move |
| **Power Management** | Active loop / manual sleep | **RTC-Driven Deep Sleep (<15µA draw)**: Extends battery life to **3+ weeks** untethered |
| **Touch Interaction** | Basic coordinate click mapping | **GT911 Gesture Engine**: Edge-swipes (back/home), proximity word targeting, & acoustic clicks |
| **Acoustic Haptics** | Silent | **Piezo Micro-Clicks**: 10ms 2.4kHz acoustic feedback on touch contact |
| **Digital Audio** | None | **Knowles PDM Digital Mic**: 16kHz 16-bit voice memos directly to PSRAM WAV buffer |
| **Study & Flashcards** | Manual notes | **One-Tap Anki Export (`.tsv`) + Obsidian Markdown Notes (`.md`)** with YAML metadata |
| **Public Library Ecosystem** | Basic manual OPDS entry | **Native Open Library & Internet Archive Search** + Built-in Presets (*Standard Ebooks, Gutenberg, Feedbooks*) |
| **Reading Analytics** | None | **Kobo-Style Stats**: WPM calculation, time left in book, daily streaks, & **7-Day E-Ink Heatmap Chart** |
| **Character & Topic X-Ray** | None | **Kindle-Style X-Ray & Concept Guide**: Instant 1-screen character dossiers via Wikipedia REST API |
| **Read-It-Later Sync** | None | **Pocket & Wallabag Unread Queue Sync**: Background article fetching & offline formatting |
| **Night Reading Mode** | Black text on white only | **Hardware Inverted Night Mode**: Crisp white text on deep black background |
| **Hands-Free Remotes** | Physical buttons only | **BLE Wireless Page-Turner Support**: Pair Bluetooth Low Energy HID rings & remotes |
| **Companion Protocol** | Basic mass storage | **921,600 Baud `UsbSerialCompanion`**: Drag-and-drop book sync, telemetry, & CLI |
| **Typography & UI Scaling**| Fixed UI text sizing | **Dynamic UI Scale (Small, Medium, Large, Extra Large)** with line-metric recalculation |
| **Credential Persistence** | Settings in single file (lost on wipe) | **NVS Dual Persistence**: Wi-Fi and Cloud credentials mirrored to Flash NVS |
| **Font Storage Resilience** | SD card required for font downloads | **Dual-Storage Fallback**: Caches and runs `.cpfont` typography on SD or SPIFFS |

---

## 🏆 Industry Comparison: The Sticky Seed Reader vs. Top E-Readers

How does **The Sticky Seed Reader** on the Seeed Studio reTerminal Sticky compare against commercial flagship e-readers?

| Capability / Category | The Sticky Seed Reader (ESP32-S3) | Amazon Kindle (Paperwhite / Oasis) | Rakuten Kobo (Clara / Libra) | Onyx Boox (Palma / Page) |
|---|---|---|---|---|
| **Platform Openness** | 🟢 **100% Open Source (GPL-3.0)** — Zero telemetry, no forced account login | 🔴 **Proprietary Walled Garden** (Amazon Account lock-in, locked OS) | 🟡 **Proprietary Linux** (Semi-open, sideloading supported) | 🟡 **Proprietary Android** (Google Play, proprietary launcher) |
| **Document Formats** | 🟢 **EPUB, MOBI, CBZ, FB2, PDF, TXT, HTML, XTC** | 🔴 **KFX, AZW3, MOBI (Deprecated)**; EPUB converted via cloud | 🟡 **EPUB, KEPUB, PDF, MOBI, CBZ, TXT** | 🟢 **EPUB, PDF, MOBI, CBZ, FB2, TXT, DOCX** (via Android apps) |
| **Public Library Access** | 🟢 **Native Open Library & Internet Archive direct downloads** + Pre-loaded OPDS feeds | 🟡 **OverDrive / Libby (US only)** via Amazon cloud delivery | 🟢 **OverDrive / Libby direct integration** | 🟢 **Libby / Hoopla Android Apps** via Google Play Store |
| **Ambient Intelligence** | 🟢 **The Daily Sticky**: Live weather, room climate (SHT40 Temp/Humidity), & RSS morning paper | 🔴 **None** (Static lockscreen ad/book cover) | 🔴 **None** (Static book cover screensaver) | 🟡 **Android Widgets** (High battery drain, no hardware sensors) |
| **Study & Note Export** | 🟢 **One-Tap Anki TSV (`.tsv`) + Obsidian Markdown (`.md`)** with YAML frontmatter | 🔴 **Proprietary Highlights** (Export limited by DRM to email) | 🟡 **Pocket / Readwise Export** (Requires third-party sync) | 🟢 **Android Note Apps** (Obsidian, AnkiDroid supported) |
| **Contextual Dossiers** | 🟢 **Wikipedia X-Ray Guide**: 1-screen character & historical concept summaries over Wi-Fi | 🟢 **Amazon X-Ray** (Curated Amazon metadata for select store books) | 🔴 **None** (Basic dictionary & Wikipedia web lookup) | 🟡 **Android Browser / Wikipedia App** |
| **Hardware Sensors** | 🟢 **Sensirion SHT40 Climate, TI BQ27220 Fuel Gauge, Knowles PDM Mic, Piezo Buzzer** | 🔴 **Ambient light sensor only** | 🔴 **Ambient light sensor only** | 🟡 **Microphone / Speaker on select models** (No climate sensors) |
| **Audio & Voice Memos**| 🟢 **16kHz PDM Digital Mic to PSRAM WAV** + Piezo 2.4kHz acoustic touch clicks | 🔴 **Audible Bluetooth playback only** (No microphone or voice notes) | 🔴 **Audiobooks playback only** (No microphone or voice notes) | 🟢 **Audiobook playback & voice recording via Android apps** |
| **Hands-Free Remotes** | 🟢 **Native BLE HID Remote & Ring Controller Pairing** | 🔴 **No native BLE remote support** (Requires physical page-turner clamp) | 🔴 **No native BLE remote support** (Requires physical page-turner clamp) | 🟢 **Bluetooth BLE / Presentation remotes supported** |
| **Companion Protocol** | 🟢 **921,600 Baud WebSerial CLI, Web Portal, Calibre & KOReader Sync** | 🔴 **Send-to-Kindle Cloud / MTP USB** | 🟡 **Calibre USB / Dropbox / Google Drive** | 🟢 **BooxDrop, Web Transfer, Google Drive** |
| **Battery Life & Power**| 🟢 **RTC Deep Sleep (<15µA)** — 3+ weeks standalone battery life | 🟢 **4–10 weeks** (Ultra-low-power proprietary SoC) | 🟢 **4–8 weeks** (Low-power Linux kernel) | 🔴 **1–4 days** (Heavy full Android OS background drain) |
| **Hardware Hackability**| 🟢 **Full GPIO / I²C / UART / SPI expansion**, ESP-IDF/Arduino C++ firmware | 🔴 **Locked hardware**, non-expandable | 🟡 **UART serial header accessible** (No external sensor bus) | 🔴 **Locked firmware**, non-expandable |

---

## 🍃 Onboard Hardware Specifications

| Component | Specification | Integration in Firmware |
|---|---|---|
| **MCU** | ESP32-S3R8 (Dual-Core Xtensa LX7 @ 240MHz) | Dual-core task distribution: Core 0 for async WiFi, TLS & I/O; Core 1 for e-ink rendering & display blitting |
| **Memory** | **8MB Octal PSRAM** (OPI high-speed) + 512KB SRAM | Pre-rendered page caches, font decompression tables, network buffers, and voice recordings live in PSRAM |
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

## ✨ Core Feature Highlights

### 1. 📰 The Daily Sticky (Ambient Morning Newspaper)
* Vintage broadsheet two-column layout showing headline stories and wire briefs.
* Real-time outdoor weather from **Open-Meteo** and indoor room climate from the **SHT40** sensor.
* Live RSS/Atom news feeds with one-touch browser launch into full articles.
* Scheduled RTC alarm wakeups with ultra-low-power deep sleep (<15µA) for weeks of untethered desk battery life.

### 2. 🌍 Dynamic GPS & IP-Geolocation Engine
* **Zero-Hardware IP Geolocation**: Automatically updates your city, coordinates, local time, and timezone offset whenever Wi-Fi connects.
* **NMEA 0183 GNSS Support**: Parses live `$GPRMC` and `$GPGGA` sentences from external GPS modules over the expansion serial port.

### 3. 🌐 Distraction-Free Web Reader & Wikipedia
* Single-pass HTML article extractor stripping ads, navigation bars, tracking scripts, and sidebars.
* Paginated E-Ink reading with tap-to-turn zones and offline article caching.
* Instant encyclopedic summary lookup cards via the **Wikipedia REST API**.

### 4. 🔤 Scalable Typography & Online Font Browser
* 4 selectable UI font tiers: **Small**, **Medium**, **Large (Default)**, and **Extra Large**.
* Built-in font installer downloading curated typography packages (*Bookerly*, *Literata*, *Charis SIL*, *Bitter*, *Open Sans*) directly from GitHub Releases.
* Dual-storage resilience: installs to MicroSD or onboard 18MB SPIFFS flash.

### 5. 🎓 Vocabulary Retention & Study Export
* **StarDict Dictionary**: Instant offline word definitions with phonetic guides.
* **Anki Export**: One-tap export of looked-up vocabulary to `/.crosspoint/export/anki/vocab_anki.tsv`.
* **Obsidian Notes**: Exports bookmarks, reading notes, and highlights to Markdown files with YAML frontmatter.

### 6. 🔌 High-Speed USB Serial Companion Protocol
* 921,600 baud bidirectional USB-C protocol (`UsbSerialCompanion`) supporting WebSerial book drag-and-drop (`CMD:PUT`), JSON file listing (`CMD:LS`), telemetry streaming (`CMD:STATUS`), and synthetic input injection (`CMD:KEY`).

### 7. 🎙️ Digital Voice Memos
* 16kHz 16-bit PDM digital microphone recording directly into 8MB Octal PSRAM with automatic RIFF WAV formatting.

### 8. 📚 Comprehensive Multi-Format E-Reader Engine
* **EPUB 2 / EPUB 3 (`.epub`)**: Full chapter styling, table of contents, embedded graphics, and font scaling.
* **Comic & Manga Archives (`.cbz`, `.cbr`, `.zip`)**: Instant sequential graphic decoding directly into PSRAM with aspect-ratio scaling and dithering.
* **Saved Web Articles (`.html`, `.htm`, `.xhtml`)**: Distraction-free reflowable article reader powered by the zero-copy `ReadabilityExtractor`.
* **FictionBook 2.0 (`.fb2`, `.fb2.zip`)**: Structured single-pass XML parsing for classical Russian and European e-books.
* **Mobipocket / PalmDOC (`.mobi`, `.prc`, `.azw`, `.azw3`)**: Palm Database streaming LZ77 decompression.
* **PDF Documents (`.pdf`)**: Native text operator extractor (`BT/ET`, `Tj`, `TJ`) for reading document contents.
* **Plain Text & Markdown (`.txt`, `.md`)**: High-speed line-wrapped reading with bookmarking.
* **XTC / XTCH (`.xtc`, `.xtch`)**: Pre-rendered binary e-book cache format for $<1\text{ms}$ startup.

### 9. 🏛️ Public Library Ecosystem & Open Library
* **Open Library & Internet Archive Integration**: Search millions of public domain titles and digital book loans directly over Wi-Fi, downloading EPUBs straight to your device.
* **Pre-Loaded Curated OPDS Feeds**: Immediate out-of-the-box access to *Standard Ebooks*, *Project Gutenberg*, and *Feedbooks Public Domain* catalogs with cover art and summaries.

### 10. 📊 Kobo-Style Reading Analytics & 7-Day Heatmap
* **Live Telemetry & Pacing**: Real-time reading speed computation (WPM), dynamic time-to-finish estimates for current chapter and book.
* **Visual Dashboard**: 2x2 metric cards displaying Total Reading Time, Pages Turned, Books Finished, and Daily Reading Streaks with an E-Ink optimized 7-day activity bar chart.

### 11. 🔍 Kindle-Style X-Ray & Concept Guide
* **Instant Character & Context Dossiers**: Tap or select any character, author, historical figure, or technical concept while reading to pull down a focused 1-screen summary from the Wikipedia REST API without losing your page context.

### 12. 📥 Pocket & Wallabag Saved Article Sync
* **Offline Web Reading Queue**: Connect your self-hosted Wallabag or Pocket account/feed over Wi-Fi. Unread articles are downloaded and automatically formatted into clean, standalone e-book chapters via `ReadabilityExtractor`.

### 13. 🌙 Inverted Night Reading Mode
* **Deep Contrast Dark Theme**: Toggle full-screen white-on-black inverted rendering for comfortable low-light bedtime reading with zero eye fatigue.

### 14. 💍 Hands-Free BLE Wireless Page-Turner Support
* **Bluetooth Remote & Ring Pairing**: Connect any BLE HID consumer page-turner ring or handheld presentation remote for effortless, hands-free reading in bed or on a desk stand.

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

## 📖 Complete Documentation & Instructions

For the complete, exhaustive instruction guide covering every button mapping, touch gesture, wireless transfer method, KOReader setup, and troubleshooting guide, refer to:

👉 **[Complete User & Feature Manual (USER_GUIDE.md)](USER_GUIDE.md)**

---

## 📄 License & Attribution

- **Firmware**: Licensed under the GNU General Public License v3.0 ([GPL-3.0](LICENSE)).
- **Upstream Credits**: Derived from the CrossPoint Reader project with deep modifications, hardware drivers, ambient intelligence, and connected capabilities engineered specifically for Seeed Studio reTerminal Sticky hardware.
