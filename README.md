# ESP32-S3 SmallHD Tally Bridge

**32Thirteen Productions LLC** | DIT Technical Reference

HTTP tally bridge for SmallHD monitors. Receives HTTP GET requests from Bitfocus Companion (or any HTTP client) and triggers GPI tally via optocoupler. Built on the Waveshare ESP32-S3-ETH with W5500 SPI Ethernet and WiFi fallback.

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

> **Tip:** Use an Ethernet screw terminal breakout board (e.g. [Poyiccot RJ45 screw terminal](https://www.amazon.com/dp/B07WKKVZRF)) on the SmallHD end rather than cutting a cable. Direct bare-wire connections to the optocoupler are unreliable due to the thin 24AWG solid-core conductors in Ethernet cable. The screw terminal block gives a proper mechanical clamp.

---

## SmallHD OLED 22 GPI Configuration

| Setting | Value |
| :--- | :--- |
| PageOS version required | 6.x or later |
| GPI function | Tally Indicator |
| Polarity | Active High |
| GPI pin | Pin 7 (not Pin 1) |

> **Important:** Pin 1 does not behave correctly for tally on the OLED 22 — use Pin 7. Polarity is Active High. Tally behavior: open circuit = ON, contact closure = OFF. Verified on hardware running PageOS 6.3.1.
>
> Other SmallHD models in a mixed fleet may require different GPI pin and polarity settings. Bench test each model before deployment.

---

## Setup & Deployment

### First Flash

1. Open `tally_light.ino` in Arduino IDE
2. Set board to **ESP32S3 Dev Module**, ESP32 Arduino core v3.x
3. Set `DEFAULT_HOSTNAME` at the top of the sketch (e.g. `tally-cam1`)
4. Flash the board

### Per-Unit Configuration (Web Portal)

After flashing, each unit can be renamed and have its WiFi credentials set without reflashing:

1. Connect the unit via Ethernet or WiFi
2. Open a browser and go to `http://<device-ip>/` or `http://tally-light.local/`
3. Set the **Device Hostname** and **WiFi SSID/Password**
4. Hit **Save & Reboot** — settings persist in flash across reboots

### Deploying Multiple Units

- Flash all units with the same firmware
- Configure each unit's hostname via the web portal after deployment
- Use IP addresses (not `.local`) in Companion for production reliability — mDNS can be inconsistent on managed/VLAN'd networks

---

## HTTP API

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/` | GET | Web config portal |
| `/config` | GET / POST | Web config portal (same as `/`) |
| `/tally/on` | GET | Activate tally (GPIO HIGH) |
| `/tally/off` | GET | Deactivate tally (GPIO LOW) |
| `/tally/test` | GET | 1-second tally pulse for testing |
| `/status` | GET | Returns firmware version, IP, interface, and tally state |

---

## Bitfocus Companion

Use **Generic: HTTP GET** action.

| Button | URL |
| :--- | :--- |
| Record ON | `http://<device-ip>/tally/on` |
| Record OFF | `http://<device-ip>/tally/off` |
| Test | `http://<device-ip>/tally/test` |
| Status | `http://<device-ip>/status` |

For production, use IP addresses over `.local` hostnames. Assign static IPs via your DHCP server using each unit's MAC address.

---

## Known Issues

- mDNS (`.local`) resolution varies by network — always note the IP as a fallback
- With both Ethernet and WiFi active, the ESP32's mDNS stack can be flaky about which interface it advertises on
- SmallHD GPI behavior differs between monitor models — verify pin and polarity on each model before production deployment

---

*32Thirteen Productions LLC*
