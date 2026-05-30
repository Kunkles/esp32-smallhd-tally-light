# SmallHD GPI Tally Setup

## OLED 22 (Verified)

PageOS 6.x or later required.

**GPI Settings:**
| Setting | Value |
| :--- | :--- |
| GPI Function | Tally Indicator |
| Polarity | Active High |
| GPI Pin | **Pin 7** |

> **Pin 1 does not work for tally on the OLED 22.** Use Pin 7. This differs from what some documentation states — verified on hardware.

**Behavior:**
- Open circuit (no signal) = Tally ON
- Contact closure = Tally OFF

This is why the firmware logic is inverted — `HIGH` on the optocoupler output opens the circuit (tally on), `LOW` closes it (tally off).

---

## Other SmallHD Models

GPI pin assignment and polarity vary by model. Bench test each model before production deployment. Settings that work on the OLED 22 may not apply to other monitors in a mixed fleet.

---

## RJ45 Pinout Reference (T568B)

| Pin | Color | Used for tally |
| :--- | :--- | :--- |
| 1 | White/Orange | — |
| 2 | Orange | — |
| 3 | White/Green | — |
| 4 | Blue | Check monitor docs |
| 5 | White/Blue | Check monitor docs |
| 6 | Green | — |
| 7 | White/Brown | **GPI active (OLED 22)** |
| 8 | Brown | GPI ground |

> Use a screw terminal RJ45 breakout board for reliable connections. Bare Ethernet wire is 24AWG solid-core and makes poor contact when inserted directly into terminals.
