# Changelog

All notable changes to the Tally Bridge firmware.

---

## v5.6 — 2026-05-30

- Tally control card added to web portal
- Live indicator (red dot + label) reflects current tally state
- ON / OFF buttons toggle tally instantly via fetch() — no page reload
- Indicator updates dynamically in the browser without a refresh

## v5.5 — 2026-05-30

- Tally output pin configurable via web portal
- Pin field is locked by default with a checkbox to unlock — prevents accidental changes
- Active pin shown in status card and serial output on boot
- `DEFAULT_TALLY_PIN` replaces hardcoded constant; active pin loaded from flash at boot

## v5.4 — 2026-05-30

- Static IP support — IP, Gateway, Subnet, DNS configurable via web portal
- IP mode (DHCP/Static) shown in status card and `/status` endpoint
- Factory Reset button added to web portal — clears all NVS settings and reboots to defaults
- Static fields show/hide dynamically based on IP mode selection

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
