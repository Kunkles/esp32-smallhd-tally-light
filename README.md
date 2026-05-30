# ESP32-S3 SmallHD Tally Bridge

**32Thirteen Productions LLC** | DIT Technical Reference

HTTP tally bridge for SmallHD monitors. Receives HTTP GET requests from Bitfocus Companion (or any HTTP client) and triggers GPI tally via optocoupler. Built on the Waveshare ESP32-S3-ETH with W5500 SPI Ethernet and WiFi fallback.

---

## Setup & Deployment

### First Flash

Flash the firmware once — all configuration is done through the browser after that. No need to edit the sketch per unit.

1. Open `tally_light.ino` in Arduino IDE
2. Set board to **ESP32S3 Dev Module**, ESP32 Arduino core v3.x
3. Under **Tools → Erase All Flash Before Sketch Upload → Enabled** (ensures a clean state)
4. Flash the board

### Configure via Web Portal

After flashing, open a browser and go to:

```
http://tally-light.local/
```

or use the IP address shown in the serial monitor if mDNS isn't resolving.

From the portal you can set:

- **Device Hostname** — unique name for this unit (e.g. `tally-cam1`). Becomes the `.local` address.
- **IP Mode** — DHCP or Static. Static reveals fields for IP, Gateway, Subnet, and DNS.
- **WiFi SSID & Password** — fallback network if Ethernet is not connected.

Hit **Save & Reboot** — all settings are stored in flash and survive reboots and reflashes.

### Deploying Multiple Units

1. Flash all units with the same firmware (no per-unit sketch changes needed)
2. Connect each unit and open `http://tally-light.local/` in a browser
3. Set a unique hostname for each unit (e.g. `tally-cam1`, `tally-cam2`)
4. Optionally assign a static IP for each unit for reliable Companion integration
5. Use IP addresses in Companion, not `.local` hostnames — mDNS can be unreliable on managed networks

### Factory Reset

To wipe all saved settings and return a unit to defaults, open the web portal and click **Reset to Defaults** at the bottom of the page. The device reboots as `tally-light` with DHCP enabled.

Alternatively, enable **Erase All Flash Before Sketch Upload** in Arduino IDE before reflashing for a full clean state.

---

## Hardware

| Component | Value |
| :--- | :--- |
| Board | Waveshare ESP32-S3-ETH |
| Ethernet chip | W5500 (SPI) |
| Tally output pin | GPIO18 |
| Test button pin | GPIO3 |
| Optocoupler | NOYITO 1-channel PC817 module |
| ESP32 Arduino core | v3.x required |
| Board selection | ESP32S3 Dev Module |

### W5500 Pin Mapping

| GPIO | Function |
| :--- | :--- |
| GPIO9 | W5500 RST |
| GPIO10 | W5500 INT |
| GPIO11 | SPI MOSI |
| GPIO12 | SPI MISO |
| GPIO13 | SPI CLK |
| GPIO14 | SPI CS |

> **Note:** GPIO18 is the tally output. If a pin is damaged, change `TALLY_PIN` at the top of the sketch — GPIO15 is a confirmed working substitute.

### Optocoupler Wiring

**Input side (ESP32 → Optocoupler):**
| Optocoupler | ESP32 |
| :--- | :--- |
| `+` | GPIO18 |
| `-` | GND |

**Output side (Optocoupler → SmallHD):**
| Optocoupler | SmallHD RJ45 |
| :--- | :--- |
| `VCC` | 3.3V (ESP32) |
| `OUT` | GPI active pin |
| `GND` | GPI ground pin |

> **Tip:** Use an Ethernet screw terminal breakout board (e.g. [Poyiccot RJ45 screw terminal](https://www.amazon.com/dp/B07WKKVZRF)) on the SmallHD end rather than cutting a cable. Direct bare-wire connections are unreliable due to the thin 24AWG solid-core conductors in Ethernet cable. The screw terminal block gives a proper mechanical clamp.

---

## SmallHD OLED 22 GPI Configuration

| Setting | Value |
| :--- | :--- |
| PageOS version required | 6.x or later |
| GPI function | Tally Indicator |
| Polarity | Active High |
| GPI pin | **Pin 7** (not Pin 1) |

> **Important:** Pin 1 does not behave correctly for tally on the OLED 22 — use Pin 7. Polarity is Active High. Tally behavior: open circuit = ON, contact closure = OFF. Verified on hardware running PageOS 6.3.1.
>
> Other SmallHD models in a mixed fleet may require different GPI pin and polarity settings. Bench test each model before deployment.

---

## HTTP API

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/` | GET | Web config portal |
| `/config` | GET / POST | Web config portal (same as `/`) |
| `/reset` | POST | Clear all saved settings and reboot to defaults |
| `/tally/on` | GET | Activate tally (GPIO HIGH) |
| `/tally/off` | GET | Deactivate tally (GPIO LOW) |
| `/tally/test` | GET | 1-second tally pulse for testing |
| `/status` | GET | Returns firmware version, IP, interface, IP mode, and tally state |

---

## Bitfocus Companion

Use **Generic: HTTP GET** action.

| Button | URL |
| :--- | :--- |
| Record ON | `http://<device-ip>/tally/on` |
| Record OFF | `http://<device-ip>/tally/off` |
| Test | `http://<device-ip>/tally/test` |
| Status | `http://<device-ip>/status` |

Use static IPs and enter them directly in Companion. mDNS `.local` addresses are convenient for setup but unreliable in production on managed or VLAN'd networks.

---

## Known Issues

- mDNS (`.local`) resolution varies by network — always note the IP as a fallback
- With both Ethernet and WiFi active, the ESP32's mDNS stack can be flaky about which interface it advertises on
- SmallHD GPI behavior differs between monitor models — verify pin and polarity on each model before production deployment

---

*32Thirteen Productions LLC*
