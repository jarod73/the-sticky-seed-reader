# The Sticky Seed Reader — Complete User & Feature Manual

Welcome to **The Sticky Seed Reader**, the next-generation firmware engineered exclusively for the **Seeed Studio reTerminal Sticky (ESP32-S3)**. This comprehensive guide covers all onboard features, touch interactions, ambient modes, cloud integrations, and study tools.

---

## 📑 Table of Contents

1. [Hardware Overview & Architecture](#1-hardware-overview--architecture)
2. [Touch Gestures & Button Navigation](#2-touch-gestures--button-navigation)
3. [Power, Startup & Ambient Deep Sleep](#3-power-startup--ambient-deep-sleep)
4. [Home Screen & Dual-Storage File Browser](#4-home-screen--dual-storage-file-browser)
5. [E-Book Reading Experience](#5-e-book-reading-experience)
6. [Interactive Dictionary, Anki & Obsidian Notes](#6-interactive-dictionary-anki--obsidian-notes)
7. [The Daily Sticky (Morning Newspaper & Ambient Display)](#7-the-daily-sticky-morning-newspaper--ambient-display)
8. [Dynamic GPS & IP-Geolocation Engine](#8-dynamic-gps--ip-geolocation-engine)
9. [Distraction-Free Web Browser & Wikipedia Lookup](#9-distraction-free-web-browser--wikipedia-lookup)
10. [Typography, Custom Fonts & UI Scaling](#10-typography-custom-fonts--ui-scaling)
11. [Voice Recording (PDM Digital Microphone)](#11-voice-recording-pdm-digital-microphone)
12. [High-Speed USB Serial Companion Protocol](#12-high-speed-usb-serial-companion-protocol)
13. [Wireless Library Management (Calibre, Web Portal, OPDS)](#13-wireless-library-management-calibre-web-portal-opds)
14. [KOReader Cloud Reading Sync](#14-koreader-cloud-reading-sync)
15. [Over-The-Air (OTA) Firmware Updates](#15-over-the-air-ota-firmware-updates)
16. [Troubleshooting & Recovery Guide](#16-troubleshooting--recovery-guide)

---

## 1. Hardware Overview & Architecture

The Seeed Studio reTerminal Sticky features an advanced suite of embedded peripherals integrated seamlessly into the firmware:

```
┌────────────────────────────────────────────────────────┐
│               THE STICKY SEED READER                   │
│   3.97" 800x480 E-Ink (SSD1677) + GT911 Touch         │
├────────────────────────────────────────────────────────┤
│ • ESP32-S3R8 (Dual-Core 240MHz, 8MB PSRAM, 32MB Flash) │
│ • Sensirion SHT40 Temperature & Humidity Sensor        │
│ • TI BQ27220 Precision Li-ion Battery Fuel Gauge       │
│ • Knowles PDM Digital MEMS Microphone                  │
│ • Piezo Buzzer (2.4kHz Acoustic Feedback Clicks)       │
│ • PCF8563 Real-Time Clock with Battery Backup          │
│ • Dual-Storage: 18MB Internal Flash + MicroSD Slot     │
└────────────────────────────────────────────────────────┘
```

### Onboard Peripherals & Specifications

| Component | Hardware Feature | How Firmware Leverages It |
|---|---|---|
| **MCU & PSRAM** | ESP32-S3R8 (8MB Octal PSRAM) | Off-screen page pre-rendering, font glyph decompression caching, and uncompressed audio buffers. |
| **Flash Partitioning** | 32MB Octal Flash | Dual 6.5MB OTA partitions + 18MB SPIFFS internal storage partition. |
| **Display** | 3.97" 800×480 E-Ink (SSD1677) | Single-buffer mode with hardware-tuned fast partial (~400ms) and full refresh waveforms. |
| **Touchscreen** | Goodix GT911 Capacitive Touch | Multi-zone tap navigation, text selection proximity targeting, and edge-swipe gestures. |
| **Indoor Climate** | Sensirion SHT40 | Real-time temperature ($\pm0.2^\circ\text{C}$) and relative humidity ($\pm1.8\%\text{RH}$) reporting. |
| **Battery Gauge** | TI BQ27220 I²C Fuel Gauge | Precise state-of-charge (%), instantaneous current ($\text{mA}$), voltage ($\text{mV}$), and time-to-empty. |
| **Microphone** | Knowles PDM Digital Mic | 16kHz 16-bit mono voice recording directly to PSRAM WAV buffers. |
| **Acoustic Feedback**| Piezo Buzzer | 10ms 2.4kHz micro-clicks providing physical-feeling confirmation on touch taps. |
| **Real-Time Clock**| PCF8563 RTC | Scheduled alarm wakeups for automatic morning news updates with zero idle battery drain. |

---

## 2. Touch Gestures & Button Navigation

The reTerminal Sticky can be navigated entirely via capacitive touch gestures or physical buttons.

### Touch Navigation Map

```
┌────────────────────────────────────────────────────────┐
│ [Top 15% Margin] -> Reader Menu / Settings Overlay     │
├─────────────────────────┬──────────────────────────────┤
│                         │                              │
│                         │                              │
│       PAGE BACK         │         PAGE FORWARD         │
│     (Left 33% Zone)     │       (Right 67% Zone)       │
│                         │                              │
│                         │                              │
├─────────────────────────┴──────────────────────────────┤
│ [Bottom Edge Swipe Up]  -> Return to Home Dashboard    │
└────────────────────────────────────────────────────────┘
```

### Supported Gestures & Actions

* **Turn Page Forward**: Tap anywhere in the right 67% of the screen, or swipe from right to left.
* **Turn Page Back**: Tap anywhere in the left 33% of the screen, or swipe from left to right.
* **Reader Menu / Quick HUD**: Tap the top 15% margin of the screen, or swipe down from the top edge.
* **Return Home**: Swipe up from the bottom bezel, or swipe right from the extreme left bezel ($X < 40\text{px}$).
* **Word Lookup**: Tap or long-press any word in a book to trigger the offline dictionary popup.
* **Acoustic Click Feedback**: Every valid touch tap produces a crisp, instantaneous 10ms acoustic click from the piezo buzzer.

---

## 3. Power, Startup & Ambient Deep Sleep

### Power Controls
* **Power On**: Press and hold the **Power Button** for 0.5 seconds.
* **Sleep / Wake**: Quick click of the **Power Button** transitions between active reading and sleep mode.
* **Forced Reset**: Press and hold the **Reset Button** alongside the **Power Button** for 5 seconds.

### Ambient Deep-Sleep Power Profiles
When running in **Ambient Desk Mode** or displaying **The Daily Sticky**, the firmware utilizes an ultra-low-power deep sleep architecture:
1. **Scheduled Wakeup**: Wakes automatically every 15, 30, or 60 minutes via the PCF8563 RTC alarm.
2. **Fast Wi-Fi Burst**: Connects to Wi-Fi, fetches live weather/news updates in under 2 seconds, refreshes the E-Ink screen, and immediately shuts down the Wi-Fi radio.
3. **Deep Sleep (<15µA Draw)**: The ESP32-S3 enters deep sleep with only the GT911 touch interrupt and power button armed.
4. **Instant Touch Wake**: Tapping the screen or pressing any button instantly wakes the device into full interactive reading mode.
5. **Battery Longevity**: Extends untethered desk battery life from **8 hours to over 3 weeks** on a single charge.

---

## 4. Home Screen & Dual-Storage File Browser

The Home Screen displays your active library, reading statistics, battery telemetry, and quick-launch activities.

```
┌────────────────────────────────────────────────────────┐
│ THE STICKY SEED READER                🔋 94%  ⛅ 72°F │
├────────────────────────────────────────────────────────┤
│ [📖 Continue: Dune]                  Ch. 4 (42% read)  │
├────────────────────────────────────────────────────────┤
│ 📰 The Daily Sticky (Morning Paper)                    │
│ 📁 Dual-Storage File Browser (Internal Flash + SD)     │
│ 🌐 Distraction-Free Web Browser & Wikipedia            │
│ 🎙️ Voice Memo Recorder                                 │
│ ⚙️ System Settings & UI Scale                          │
└────────────────────────────────────────────────────────┘
```

### Dual-Storage Resilience
* **MicroSD Card**: Insert any FAT32 or exFAT MicroSD card containing `.epub`, `.txt`, `.xtc`, or `.bmp` files. Cards are hot-plug auto-detected upon insertion.
* **18MB Internal SPIFFS**: If no SD card is inserted, the firmware boots cleanly and stores standalone books, settings, and downloaded fonts directly on internal flash.
* **Safe SPI Arbitration**: Storage operations and display refreshes are guarded by the `HalStorage` recursive mutex, preventing bus collisions.

---

## 5. E-Book Reading Experience

The Sticky Seed Reader delivers a premium, distraction-free reading experience:

### Reading Features
* **Zero-Latency PSRAM Pre-Rendering**: While you are reading a page, the next page is pre-rendered off-screen in 8MB PSRAM. Page turns happen in `<1ms` via direct frame buffer transfer.
* **Advanced Typography**: Full support for soft hyphenation, kerning, line-height adjustment, paragraph spacing, and custom margin insets.
* **Footnote Popups**: Tapping footnote numbers `[1]` opens an overlay card containing the note text without losing your place in the chapter.
* **Comprehensive 8-Format Multi-Format Engine**:
  - **EPUB 2 / EPUB 3 (`.epub`)**: Full chapter parsing, embedded images, and CSS stylesheet styling.
  - **Comic & Manga Archives (`.cbz`, `.cbr`, `.zip`)**: Sequential image decoding directly into PSRAM with automatic aspect-ratio scaling, Floyd-Steinberg dithering, and page count HUD.
  - **Cleaned Web Articles (`.html`, `.htm`, `.xhtml`)**: Strips advertisements, navigation bars, and scripts via `ReadabilityExtractor`, formatting local web articles into reflowable, paginated chapters.
  - **FictionBook 2.0 (`.fb2`, `.fb2.zip`)**: Lightweight single-pass XML parser for structured headings, body paragraphs, and chapter metadata.
  - **Mobipocket / PalmDOC (`.mobi`, `.prc`, `.azw`, `.azw3`)**: High-speed streaming LZ77 decompression supporting classic unencrypted PalmDOC e-books.
  - **PDF Documents (`.pdf`)**: Native text operator extractor (`BT/ET`, `Tj`, `TJ`) for reading document contents without heavy desktop PDF engines.
  - **Plain Text & Markdown (`.txt`, `.md`)**: High-speed line wrapping, customizable margin insets, and dynamic bookmarking.
  - **XTC / XTCH (`.xtc`, `.xtch`)**: Pre-rendered binary book format for instant startup and zero-latency page turns.

---

## 6. Interactive Dictionary, Anki & Obsidian Notes

Enhance your vocabulary and retain insights effortlessly while reading.

### Instant Word Lookup
1. **Tap Any Word**: Tap or hold any word on the page. Proximity targeting identifies the closest word boundaries.
2. **StarDict Definition**: The definition is retrieved instantly from offline StarDict dictionary files stored in `/dict/` or `/.dict/`.
3. **Pronunciation & Etymology**: View phonetic guides, grammatical forms, and complete definitions.

### Anki Flashcard Export (`.tsv`)
* Tap the **`[+Anki]`** button in any dictionary popup.
* The word, phonetic transcription, definition, book title, and sentence context are automatically appended to:
  ```
  /.crosspoint/export/anki/vocab_anki.tsv
  ```
* Import directly into the [Anki](https://apps.ankiweb.net/) desktop or mobile app with one click.

### Obsidian Markdown Note Export (`.md`)
* Export reading notes, bookmarks, progress, and highlights directly into clean Markdown files with YAML frontmatter:
  ```
  /.crosspoint/export/notes/<Book_Title>_notes.md
  ```
* Ready to drop straight into your [Obsidian](https://obsidian.md/) vault.

---

## 7. The Daily Sticky (Morning Newspaper & Ambient Display)

Transform your reTerminal Sticky into a vintage, ambient smart desk display.

```
┌────────────────────────────────────────────────────────┐
│  THE STICKY GAZETTE  •  TUESDAY, SEP 8  •  MORNING ED. │
│  Indoor: 71.4°F  48% RH   │  Outdoor: 68°F  ⛅ Sunny   │
├───────────────────────────┴────────────────────────────┤
│ 📰 HEADLINE: NASA Unveils New Deep Space Observatory   │
│ Scientists have revealed breakthrough imagery from the │
│ next-generation optical array orbiting Lagrange L2...  │
│                                                        │
│ • Global Markets Post Steady Gains in Tech Rally       │
│ • Breakthrough in Ambient Thermoelectric Materials     │
│ • Open-Source Firmware Modernizes Vintage E-Paper      │
├────────────────────────────────────────────────────────┤
│ [Tap any story to read full article in Web Browser]    │
└────────────────────────────────────────────────────────┘
```

### Ambient Newspaper Highlights
* **Broadsheet Layout**: Elegant two-column layout using the modular `TextWrapUtils` engine.
* **Outdoor Weather Forecasts**: Live temperature, weather conditions, and daily highs/lows from **Open-Meteo** (no API key required).
* **Indoor Climate Telemetry**: Live temperature and relative humidity from the onboard **SHT40** sensor.
* **Battery Gauge Telemetry**: Live battery percentage, current draw ($\text{mA}$), and voltage.
* **RSS/Atom Syndication**: Automatically parses RSS 2.0 and Atom 1.0 feeds from your configured news wire.
* **One-Touch Article Reader**: Tap any headline or brief on the screen to immediately fetch and read the full article in the distraction-free web browser.

---

## 8. Dynamic GPS & IP-Geolocation Engine

The device automatically keeps your time, timezone, and weather forecast updated as you travel across the globe.

### How Location Synchronization Works
1. **Zero-Hardware IP Geolocation (Default)**:
   - When connected to any Wi-Fi network (home, office, or phone hotspot), the `GpsLocationService` queries geolocation APIs to detect your city, latitude, longitude, and IANA timezone name.
   - Updates `halClock` (PCF8563 RTC) with the correct local time and UTC offset.
   - Feeds new coordinates into `OpenMeteoClient` to fetch local weather automatically.
2. **NMEA 0183 GNSS Serial Stream (Optional Hardware)**:
   - Plug an external GPS/GNSS receiver into the expansion UART header.
   - The asynchronous `$GPRMC` and `$GPGGA` parser decodes live coordinates, speed, and atomic UTC time on the move.

---

## 9. Distraction-Free Web Browser & Wikipedia Lookup

Browse articles and encyclopedic entries on E-Ink without ads, popups, or distractions.

### Readability Article Browser
* **Single-Pass $O(N)$ Parser**: The `ReadabilityExtractor` streams web HTML, targeting `<article>` and `<main>` tags while stripping navigation bars, ads, tracking scripts, and sidebars.
* **E-Ink Pagination**: Converts articles into clean, paginated chapters with tap-to-turn zones and progress tracking.
* **Offline Article Caching**: Saves articles to local storage for offline reading during commutes.

### Instant Wikipedia Lookup
* Access the entire world's knowledge via the **Wikipedia REST API**.
* Search any topic, term, or historical event to view a clean summary card with key facts, dates, and overviews.

---

## 10. Typography, Custom Fonts & UI Scaling

Customize your reading experience with scalable typography and custom font packages.

### System UI Text Size Scaling
Adjust the system text size across all menus, file browsers, and dashboard headers:
* **Small**: Maximum information density for large directory structures.
* **Medium**: Balanced compactness.
* **Large (Default)**: High legibility with comfortable touch targets.
* **Extra Large**: High accessibility for low-light or distant viewing.

### Online Font Downloader & Custom `.cpfont` Packages
1. Open **Settings** $\rightarrow$ **Download Fonts**.
2. Connect to Wi-Fi. The device downloads the latest font manifest from GitHub Releases:
   ```
   https://github.com/crosspoint-reader/crosspoint-fonts/releases
   ```
3. Browse curated font families (e.g. *Bookerly*, *Literata*, *Charis SIL*, *Bitter*, *Open Sans*, *Noto Sans CJK*).
4. Select any family to install all point sizes directly to `/.fonts/` on your SD card or onboard flash.

---

## 11. Voice Recording (PDM Digital Microphone)

Capture quick voice memos, thoughts, and book reflections using the onboard Knowles PDM microphone.

### Voice Memo Workflow
1. Launch **Voice Memo Recorder** from the Home Screen or Reader Menu.
2. Tap **Record** (or press Confirm). The 16kHz 16-bit PDM stream records audio into 8MB Octal PSRAM.
3. Tap **Stop**. The recording is formatted into a standard RIFF WAV file and saved to:
   ```
   /recordings/memo_YYYYMMDD_HHMMSS.wav
   ```
4. Play back or transfer recordings over USB-C to your computer.

---

## 12. High-Speed USB Serial Companion Protocol

Connect your reTerminal Sticky to a PC, Mac, Chromebook, or Android device via USB-C for high-speed management at **921,600 baud**.

### Companion Protocol Commands

| Command | Action | Description |
|---|---|---|
| `CMD:PING` | Connectivity Check | Returns `PONG:STICKY_SEED_READER`. |
| `CMD:STATUS` | Live Telemetry | Returns JSON with battery %, voltage, temperature, humidity, and free heap. |
| `CMD:LS:<path>` | Directory Listing | Lists files and directories with byte sizes in JSON format. |
| `CMD:PUT:<path>:<size>` | High-Speed File Upload | Streams `.epub` books, fonts, or dictionaries directly to storage. |
| `CMD:GET:<path>` | File Download | Streams recordings or screenshots to the host computer. |
| `CMD:KEY:<key_name>` | Synthetic Input Injection | Remotely triggers button presses (`UP`, `DOWN`, `CONFIRM`, `BACK`). |

---

## 13. Wireless Library Management (Calibre, Web Portal, OPDS)

Wirelessly manage and upload books over your local Wi-Fi network.

### 1. Calibre Wireless Plugin
* Install the official CrossPoint / FreeInk plugin in [Calibre](https://calibre-ebook.com/).
* Click "Send to Device" over Wi-Fi to sync books, covers, and metadata automatically.

### 2. Built-In Web Management Portal
* Connect to the same Wi-Fi network as your reTerminal Sticky.
* Open the on-screen IP address (e.g., `http://192.168.1.120`) in any web browser.
* Drag and drop EPUB files, manage folders, and delete finished books directly.

### 3. OPDS Catalogs
* Connect to multiple OPDS catalog servers (Calibre Content Server, Kavita, Komga, Project Gutenberg).
* Search, browse covers, and download books directly on the device.

---

## 14. KOReader Cloud Reading Sync

Seamlessly synchronize reading progress across your reTerminal Sticky, Android tablet, and PC running KOReader.

### Setup Instructions
1. Open **Settings** $\rightarrow$ **KOReader Sync**.
2. Enter your sync server URL (default: `https://sync.crosspointreader.com` or self-hosted).
3. Enter your Username and User Key.
4. When reading any book, progress is automatically synchronized in the background upon chapter completion or book exit.

---

## 15. Over-The-Air (OTA) Firmware Updates

Keep your device up to date with the latest features, performance improvements, and security patches over Wi-Fi.

### Updating Firmware Wirelessly
1. Ensure your device is connected to Wi-Fi.
2. Open **Settings** $\rightarrow$ **Check for Updates**.
3. The firmware queries the GitHub release API for the latest binary tag.
4. If a new version is available, tap **Download & Install**. The device streams the update to the secondary 6.5MB OTA partition (`app1`), verifies the SHA256 checksum, and silently restarts into the new release.

---

## 16. Troubleshooting & Recovery Guide

### 1. Wi-Fi Password Missing After Update
* **Resolution**: The firmware features **NVS Dual Persistence**. Wi-Fi credentials and cloud tokens are mirrored to the non-volatile storage partition (`cp_wifi`). If an SD card is reformatted or removed, settings restore automatically upon boot.

### 2. MicroSD Card Not Detected
* **Resolution**: If an SD card is inserted while the device is powered on, the dynamic hot-plug scanner mounts the card automatically. If it does not appear, ensure the card is formatted as FAT32 or exFAT.

### 3. Online Font Browser Shows No Fonts
* **Resolution**: Ensure Wi-Fi is connected and an SD card or internal storage is available. The font manifest requires write access to `/fonts_manifest.tmp` to parse the catalog.

### 4. Recovery Firmware Mode
* If a corrupted file or infinite loop occurs:
  1. Hold **Button Down** (GPIO 7) while pressing the **Power Button** from a powered-off state.
  2. The device boots into clean Recovery Safe Mode, bypassing startup activity loads.
