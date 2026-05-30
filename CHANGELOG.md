# Changelog

All notable changes to the Tally Bridge firmware.

---

## v5.3 — 2026-05-30

- Added web config portal at `/` and `/config`
- Hostname configurable at runtime via browser — no reflash needed
- WiFi SSID and password configurable via web portal
- All settings persisted in flash using `Preferences` library
- `prefs.end()` called before reboot to ensure flash commit
- `FW_VERSION` constant added — drives version string in page header and `/status`
- Default hostname changed to `tally-light`

## v5.2

- Web config portal introduced for hostname management
- `Preferences` library added for flash storage
- Root `/` now serves config page

## v5.1 — Initial release

- ESP32-S3 + W5500 SPI Ethernet with WiFi fallback
- 15-second network timeout on boot
- mDNS hostname via `MDNS.begin()`
- HTTP endpoints: `/tally/on`, `/tally/off`, `/tally/test`, `/status`
- Physical test button on GPIO3 with internal pull-up
- `HOSTNAME` variable extracted to top of file
- Serial output prints actual hostname in mDNS URLs
- Tested on Waveshare ESP32-S3-ETH with SmallHD OLED 22, PageOS 6.3.1
- ESP32 Arduino core v3.3.8
