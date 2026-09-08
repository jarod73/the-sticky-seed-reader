# Engineering Specification & Vision: The Sticky Seed Reader
## Dedicated Firmware for Seeed Studio reTerminal Sticky

## 1. Project Vision & Architecture Objective

This repository is **The Sticky Seed Reader**—a specialized, high-performance connected e-reader and ambient intelligence operating system tailored exclusively for the **Seeed Studio reTerminal Sticky**.

While upstream CrossPoint is architected around the severe ~380 KB RAM constraints of single-core ESP32-C3 devices (lacking PSRAM and restricted to a single 48 KB framebuffer), the Seeed Studio reTerminal Sticky provides an **ESP32-S3R8 with 8 MB Octal PSRAM, 32 MB Flash, a 3.97" 800×480 4-level grayscale e-paper display, Goodix GT911 capacitive touch, a 6-axis IMU, SHT40 temperature/humidity sensor, PCF8563 RTC, BQ27220 fuel gauge, PDM microphone, and piezo buzzer**.

This firmware operates on a **hybrid edge/burst-sync model**:
* **100% Offline Core**: Page rendering, typography, Knuth-Liang hyphenation, in-memory font glyph caches, 4-level Floyd-Steinberg image dithering, offline StarDict dictionaries, and hardware sensor integration.
* **Dual-Storage & MicroSD Expansion**: 
  - **Graceful Fallback**: Operates standalone without an SD card using the internal 18 MB flash partition for settings, state, and internal books. Never halts on "SD card error".
  - **Dynamic MicroSD Support**: When a MicroSD card is inserted, the device automatically mounts it (gated by `SD_PWR_EN` GPIO10) to unlock massive storage for EPUBs, manga (CBZ/CBR), offline StarDict dictionaries, voice recordings, Anki flashcard CSVs, and Markdown notes.
* **On-Demand Burst Connectivity**: Wi-Fi powers up strictly for discrete network bursts (OPDS catalog sync, KOReader progress sync, Morning RSS/Read-It-Later compilation, and Whisper/LLM queries), immediately returning to sleep to protect the 750 mAh battery.

```
+-------------------------------------------------------------------------+
|                        Seeed reTerminal Sticky                          |
| +---------------------------------------------------------------------+ |
| |          3.97" 800x480 4-Level Grayscale e-Paper (SSD1677)          | |
| |                Capacitive Touch Screen (Goodix GT911)               | |
| +---------------------------------------------------------------------+ |
| | ESP32-S3R8: Dual-Core 240MHz, 8MB Octal PSRAM, 32MB QSPI Flash      | |
| | I2C Bus 0 (GPIO 0/1): SHT40, LSM6DS3TR-C IMU, PCF8563, BQ27220 Fuel | |
| | I2C Bus 1 (GPIO 2/3): GT911 Capacitive Touch Controller             | |
| | SPI Bus (Shared): MicroSD Slot (CS 8) & SSD1677 Display (CS 15)     | |
| | Audio / Haptics: PDM Mic (GPIO 19/20), Piezo PWM Buzzer (GPIO 48)   | |
| | Hardware Buttons: AI/Power (GPIO 4), Up (GPIO 5), Down (GPIO 6)     | |
| | Power Subsystem: 750 mAh LiPo, BQ27220 Fuel Gauge, N52 Rear Magnets  | |
+-------------------------------------------------------------------------+
```

---

## 2. Complete Peripheral & GPIO Mapping

| Subsystem | Peripheral | Interface | Pins / Signals |
| :--- | :--- | :--- | :--- |
| **Display** | 3.97" 800×480 e-Paper (SSD1677) | SPI | SCK: `GPIO13`, SDI (MOSI): `GPIO14`, CS: `GPIO15`, DC: `GPIO16`, RST: `GPIO17`, BUSY: `GPIO18`, PWR_EN: `GPIO47` |
| **Storage** | MicroSD Slot | SPI (Shared Bus) | CS: `GPIO8`, MISO: `GPIO12`, SCK: `GPIO13`, MOSI: `GPIO14`, PWR_EN: `GPIO10` |
| **Touch** | Goodix GT911 Capacitive | I2C Bus 1 | SCL: `GPIO2`, SDA: `GPIO3`, INT: `GPIO21`, RST: `GPIO41`, PWR_EN: `GPIO42` |
| **Sensors & RTC** | SHT40, LSM6DS3TR-C, PCF8563, BQ27220 | I2C Bus 0 | SCL: `GPIO0`, SDA: `GPIO1`, IMU INT: `GPIO7` |
| **Microphone** | PDM Voice Capture | I2S / PDM | CLK: `GPIO19`, DATA: `GPIO20`, MIC_EN: `GPIO38` |
| **Buzzer** | Piezo Audio Feedback | PWM / LEDC | PWM: `GPIO48` |
| **Buttons** | Navigation & AI Key | GPIO Input | AI/Power/Wake: `GPIO4`, Up: `GPIO5`, Down: `GPIO6` |
| **Power & Battery** | 750 mAh LiPo + BQ27220 | I2C Bus 0 | BQ27220 Address: `0x55`, Charge Status: `GPIO40`, Power Hold: `GPIO45`, Power Lock: `GPIO46`, Charge EN: `GPIO39` |

> [!CAUTION]
> **CRITICAL SPI BUS RULE**: The MicroSD slot and SSD1677 display share the physical SPI clock (`GPIO13`) and MOSI (`GPIO14`). All SPI operations must be serialized using a shared FreeRTOS mutex (`spiBusMutex`) to prevent bus collision during concurrent file reads and display updates.

---

## 3. Memory Architecture & Dual-Core Allocation

### 8 MB Octal PSRAM Layout
* **Page Framebuffers (~1 MB)**: Triple-buffered 800×480 2-bit grayscale buffers (`current`, `next`, `prev`). Each 2-bit frame is 96 KB ($800 \times 480 \times 2 / 8$).
* **Glyph Bitmap Cache (~2 MB)**: FreeType rasterized glyphs for ASCII, Latin extended, and active CJK subsets cached in PSRAM.
* **Streaming Image / Comic Buffer (~2 MB)**: Working decompression scratchpad for JPEG/PNG/WebP scanlines (CBZ/CBR manga support).
* **Audio DMA Ring Buffer (~1 MB)**: 16 kHz 16-bit mono circular recording buffer (~32 seconds capacity).
* **Dynamic Heap (~2 MB)**: DOM parsing trees, SQLite FTS index caches, and HTTP TLS payloads.

### Dual-Core Thread Model
* **Core 0 (UI, Audio, Sensors & Real-Time I/O)**:
  - GT911 touch interrupt handler and gesture state machine.
  - FreeInkUI view composition and dialog popups.
  - Piezo buzzer low-latency PWM audio feedback.
  - PDM microphone DMA streaming pipeline.
  - LSM6DS3TR-C IMU orientation interrupt handling.
* **Core 1 (Compute, Storage & Background Work)**:
  - EPUB XML/CSS reflow and HTML formatting.
  - Background page pre-rendering queue ($N+1, N+2$).
  - Comic (CBZ/CBR) stream decompression and Floyd-Steinberg dithering.
  - SQLite / StarDict binary search index lookups.
  - Wi-Fi burst operations: OPDS browsing, KOSync, Whisper/LLM API calls.

---

## 4. Feature Implementation Directives

### Group A: Hardware HAL & Power Precision
1. **BQ27220 Precision Fuel Gauge (`src/hal/battery/`)**:
   - I2C Bus 0 (`GPIO0`/`GPIO1`), Address `0x55`.
   - Read State of Charge (`0x2C`), Current (`0x14`), and Time to Empty (`0x16`).
   - Render true percentage and remaining reading hours on the status bar and sleep screen instead of noisy ADC measurements.
2. **Piezo Buzzer Micro-Clicks (`src/hal/buzzer/`)**:
   - GPIO48 using ESP32 LEDC peripheral (2.4 kHz base carrier).
   - Pulse a 10–12 ms non-blocking tone on every valid GT911 `TOUCH_DOWN` to deliver instantaneous acoustic feedback before e-ink physical refresh finishes.
3. **LSM6DS3TR-C 6-Axis IMU (`src/hal/sensors/imu.cpp`)**:
   - I2C Bus 0, Address `0x6A`/`0x6B`, INT on `GPIO7`.
   - True 4-way auto-rotate: Compute 3-axis gravity vector. Reorient screen between portrait ($480\times800$) and landscape ($800\times480$) when tilt exceeds 45° for $>500$ ms.
   - Chassis tap detection: Enable IMU hardware double-tap detection (`TAP_CFG`) to trigger `PAGE_NEXT` when mounted on a wall or fridge.
4. **Ambient Nightstand / Fridge Sleep Screen (`src/ui/screens/sleep_screen.cpp`)**:
   - Query Sensirion SHT40 and PCF8563 on I2C Bus 0 prior to entering sleep.
   - Render persistent environmental widget (temperature °C/°F, relative humidity %, battery SOC, clock, last sync timestamp) alongside book cover art.
5. **Dual-Storage & Dynamic MicroSD Insertion (`lib/hal/HalStorage`)**:
   - Enable the device to operate out-of-the-box using the internal 18 MB flash partition for settings, state, and internal books without requiring an SD card at boot.
   - Implement dynamic detection/mounting when a MicroSD card is inserted: power the SD rail via `GPIO10` (`SD_PWR_EN`), initialize SPI transactions with `GPIO8` (`SD_CS`), and mount the FAT/exFAT volume.
   - Seamlessly expose external SD storage for large EPUB libraries, CBZ/CBR manga, offline StarDict dictionaries, voice notes, and Anki review flashcards.

### Group B: Reading Engine & Zero-Latency Pipeline
1. **Zero-Latency Pre-Rendering Pipeline (`src/reader/pipeline/`)**:
   - Allocate circular framebuffers in PSRAM: `PageBuffer[current]`, `PageBuffer[next]`, `PageBuffer[prev]`.
   - On `PAGE_NEXT` touch:
     1. Instantly copy `PageBuffer[next]` to the SSD1677 controller via fast partial update waveform.
     2. Signal Core 1 task to parse and pre-render page $N+1$ into the recycled buffer.
     3. Software latency drops to 0 ms (limited only by e-paper physical transition time).
2. **In-Memory Font Glyph Caching (`src/reader/fonts/`)**:
   - Allocate all rasterized FreeType glyph bitmaps directly in Octal PSRAM (`MALLOC_CAP_SPIRAM`).
   - Eliminate SD card/flash read thrashing during reflow and flipping.
3. **Knuth-Liang Hyphenation (`src/reader/layout/`)**:
   - Integrated C++ Knuth-Liang pattern matching with flash-stored packed tables.
   - Apply breaks during text justification in `TextLayout` to eliminate character gaps.
4. **Interactive Footnotes & Nested TOC (`src/reader/navigation/`)**:
   - Footnotes: Parse `<a epub:type="noteref">`, `<aside>`, or `#fn...` tags into touch targets. On tap, open a floating modal dialog displaying note content instead of jumping chapters.
   - Nested TOC: Parse NCX/NAV tags into an N-ary tree (`TocNode`) and render in `UiListActivity` with collapsible nodes and indentation.

### Group C: Media, Search & Offline Study
1. **Comic & Manga Support (CBZ/CBR) (`src/reader/comic/`)**:
   - Use `miniz` to inspect `.cbz` archives on MicroSD.
   - Stream-decompress individual image scanlines into PSRAM without unpacking full archives to disk.
   - Scale and letterbox to $800\times480$.
2. **Hardware-Tuned 4-Level Grayscale Dithering (`src/gfx/dither/`)**:
   - Map 8-bit luminance to 2-bit values ($0, 85, 170, 255$) using Floyd-Steinberg error diffusion.
   - Mode switch: Pure Text Mode (1-bit high-contrast fast partial refresh) vs. Rich/Comic Mode (4-level dithered with occasional full-refresh inversion).
3. **Touch Word Bounding Boxes & Offline StarDict (`src/dict/`)**:
   - Log word bounding boxes $(x_0, y_0, x_1, y_1)$ for the active page.
   - Single tap on word $\rightarrow$ binary search `.idx` on SD $\rightarrow$ decompress definition from `.dict` $\rightarrow$ render definition modal in $<100$ ms.
   - Touch drag $\rightarrow$ highlight range with action bar: `[Highlight]`, `[Note]`, `[Voice]`, `[Explain]`.
4. **Flashcards & Markdown Exporter (`src/sync/notes/`)**:
   - Anki Flashcard queue: Appends word lookups to `/sdcard/flashcards/review.csv` (`word, definition, context sentence, book title, timestamp`).
   - Obsidian/Logseq Markdown export: Append highlights/notes to `/sdcard/notes/<Book_Title>.md`.
5. **Fast Full-Text Search (`src/reader/search/`)**:
   - Generate compact SQLite FTS5 index or inverted index on first book open.
   - Resolve search queries in $<500$ ms with clickable page jumps.

### Group D: Connected Ecosystem (Wi-Fi, Bluetooth BLE & USB)

1. **Multi-Network Wi-Fi Management & Web Portal (`src/network/`)**:
   - **Saved Network Profiles**: Scan and save multiple Wi-Fi networks (Home, Office, Mobile Hotspot) with automatic roaming/reconnection to the strongest available AP.
   - **Local Web Server (`http://sticky.local`)**: Built-in HTTP portal on local Wi-Fi or ad-hoc AP mode: drag-and-drop books directly from any PC, Mac, iPhone, or Android browser without cables or third-party apps.
   - **Calibre Content Server / OPDS**: XML/Atom parser to browse local or remote Calibre/Kavita/Komga libraries and download EPUBs directly.
   - **KOReader Sync (KOSync)**: Bi-directional reading progress sync on book open/close.
   - **Read-It-Later & Daily RSS Digest**: RTC wake alarm connects at a scheduled morning hour (e.g., 5:00 AM) to compile unread articles from Wallabag, Pocket, or RSS feeds into `/sdcard/books/Digest_YYYY-MM-DD.epub`, updates the screen, and returns to deep sleep.

2. **Bluetooth Low Energy (BLE 5.0) Device Ecosystem (`src/hal/bluetooth/`)**:
   - **BLE Page Turner Rings & Remotes (HID Central)**: Pair with commercial BLE scroller rings, presentation clickers, 8BitDo Micro controllers, and camera remotes for hands-free page navigation when the Sticky is magnetically mounted or on a stand.
   - **External BLE Keyboard Support**: Pair compact wireless Bluetooth keyboards for typing long reading notes, journal entries, or search queries without using the on-screen e-paper keyboard.
   - **Phone Companion Pairing (BLE Peripheral / GATT Server)**: Pair with iOS/Android via Web Bluetooth or companion app for 1-tap Wi-Fi credential sharing, clipboard text transfer, and optional quiet reading notifications (e.g., incoming call alerts).

3. **USB Desktop & Mobile Companion Mode (`src/network/usb/`)**:
   - **Browser-Based WebSerial Drag & Drop**: Zero-software install; connect via USB-C to Windows, Mac, Linux, or Android, open a browser, and drag-and-drop books directly into internal Flash or MicroSD card at 921,600 baud.
   - **1-Click Markdown & Anki Sync**: Instant export of `/notes/[Book].md` and `/flashcards/review.csv` directly to desktop Obsidian vaults or flashcard decks.
   - **Calibre USB Driver / Companion**: Auto-detects the Sticky over serial/USB, queries library contents, and syncs books directly from Calibre.
   - **Docked "Desk Companion" Display Mode**: When plugged into USB power, disable sleep timeout and switch into an ambient desk monitor (PC system performance stats, calendar schedule, or live weather station) while charging.
   - **VBUS & Fast-Charge Latch**: Direct hardware control of the BQ25616 charger (`EN_BAT_CHGn`) to safely fast-charge the internal 750 mAh LiPo while docked.

4. **Dual-Personality OS**:
   - **Handheld**: Dedicated, distraction-free book and comic reader.
   - **Docked / Magnetically Mounted**: Ambient smart sticky, desk monitor, recipe viewer, or Home Assistant satellite.

### Group E: AI & Voice Subsystem
1. **Physical AI Button & PDM Mic Streamer (`src/hal/audio/`)**:
   - ESP32-S3 I2S in PDM RX mode: `GPIO19` (CLK), `GPIO20` (DATA), 16 kHz 16-bit mono.
   - Push-to-Talk on `GPIO4`: Fills 1 MB PSRAM ring buffer.
   - On release: Wi-Fi connects, streams audio to OpenAI Whisper or local Wyoming API endpoint, writes returned text to Markdown notes, then powers off Wi-Fi.
2. **Connected AI REST Client (`src/network/ai_client.cpp`)**:
   - **Spoiler-Free X-Ray**: Query LLM with entity name and chapter index with prompt: *"Describe {entity} in {title} strictly using details known up to chapter {chapter}. Avoid spoilers beyond this point."*
   - **Catch Me Up**: Summarize the last read chapter in 3 bullet points if unopened for $>5$ days.
   - **In-Line ELI5 / Historical Decoder**: Highlight text and prompt LLM for a $<60$-word plain language explanation.
   - **AI Cover Art Generator**: Generate cover art for sideloaded books, apply 4-level dithering, and save as $800\times480$ `.bin` cover.

---

## 5. Implementation Roadmap

* **Phase 1: Environment, Memory & Hardware Baseline**
  - Configure `[env:sticky]` in `platformio.ini` with 8 MB Octal PSRAM (`qio_opi`) and `partitions_sticky.csv` (32 MB layout).
  - Verify SPI bus lock (`spiBusMutex`) across SD Card and SSD1677 display.
  - Wire and validate BQ27220 fuel gauge, buzzer clicks, SHT40, and RTC sleep widgets.
* **Phase 2: Touch & Zero-Latency Rendering Engine**
  - Implement word bounding-box touch hit detection and StarDict popup modal.
  - Build Core 1 background pre-rendering pipeline with triple PSRAM framebuffers.
  - Enable in-memory FreeType glyph cache in PSRAM.
* **Phase 3: Media, Typography & Offline Study**
  - Implement Knuth-Liang hyphenation and floating footnotes.
  - Add CBZ/CBR comic streaming reader with 4-level Floyd-Steinberg dithering.
  - Implement Markdown notes export and Anki flashcard CSV exporter.
  - Implement SQLite FTS full-text search.
* **Phase 4: Connected Ecosystem (Wi-Fi, Bluetooth BLE & USB)**
  - Multi-network Wi-Fi management with saved profiles and local Web Server (`http://sticky.local`).
  - BLE HID central driver for wireless page-turner rings, presentation clickers, and external keyboards.
  - WebSerial browser drag-and-drop book uploader and USB docked desk companion mode.
  - Calibre OPDS catalog client and KOReader sync.
  - Morning RSS / Read-it-Later digest generator.
  - IMU 4-way auto-rotation and chassis tap detection.
* **Phase 5: Connected Voice & AI Services**
  - PDM microphone capture and Whisper transcription client.
  - Contextual X-Ray, Chapter Recaps ("Catch Me Up"), and ELI5 decoders.
  - Ambient dashboard mode for magnetic mount.
