# Pinout

This pinout is taken from the [vendor schematics](../schematics/). The header pin numbers are the
schematic connector numbers. Check them against the silkscreen on your board before you wire
anything.

## GPIO map

| GPIO | On-board function | Header | Notes |
|-----:|-------------------|--------|-------|
| 0  | BOOT button (S1), auto-program circuit, camera `RST` | J1-5 | Strapping pin. 10 kΩ pull-up (R3). Hold low at reset to enter download mode. |
| 1  | Camera `D0` | J1-26 | |
| 2  | Camera `D1` | J1-6 | |
| 3  | Camera `D2` | J1-25 | Strapping pin (JTAG source). 10 kΩ pull-up (R4). |
| 4  | Camera `D3` | J1-7 | |
| 5  | Camera `D4` | J1-24 | |
| 6  | Camera `D5` | J1-8 | |
| 7  | Camera `D6` | J1-23 | |
| 8  | Camera `D7` | J1-9 | |
| 9  | Camera `PCLK` | J1-22 | |
| 10 | W5500 `SCSn` (chip select) | J1-10 | Through 0 Ω R31 |
| 11 | W5500 `MOSI` | J1-21 | Through 0 Ω R28 |
| 12 | W5500 `SCLK` | J1-11 | Through 0 Ω R30 |
| 13 | W5500 `MISO` | J1-20 | Through 0 Ω R29 |
| 14 | W5500 `INTn` | J1-12 | Through 0 Ω R27 |
| 15 | W5500 `RSTn` | J1-19 | Through 0 Ω R26. **10 kΩ pull-down (R24)**, so the W5500 stays in reset until firmware drives this pin high. Also labelled `LCD_RST`. |
| 16 | Camera `HREF` | J1-13 | Also labelled `LCD_D/C` |
| 17 | Camera `SCL` (SCCB) | J1-18 | Also labelled `LCD_MOSI` |
| 18 | Camera `SDA` (SCCB) | J1-14 | Also labelled `LCD_SCLK` |
| 19 | Native USB `D-` | J1-17 | Shared by the "ESP-USB" USB-C port (J4) and the USB-A port (J5) |
| 20 | Native USB `D+` | J1-15 | Shared by J4 and J5 |
| 21 | Camera `VSYNC` | J1-16 | Also labelled `LCD_CS` |
| 26–32 | Flash and PSRAM (SPI0/1) | n/a | **Do not use.** These pins are not on the headers. |
| 33 | microSD `DAT2` | J2-15 | 51 kΩ pull-up |
| 34 | microSD `DAT3` / `CS` | J2-16 | 51 kΩ pull-up |
| 35 | microSD `CMD` | J2-14 | 51 kΩ pull-up |
| 36 | microSD `CLK` | J2-17 | |
| 37 | microSD `DAT0` | J2-13 | 51 kΩ pull-up |
| 38 | microSD `DAT1` | J2-18 | 51 kΩ pull-up |
| 39 | Free (`MTCK`) | J2-12 | JTAG |
| 40 | Free (`MTDO`) | J2-19 | JTAG |
| 41 | Free (`MTDI`) | J2-11 | JTAG |
| 42 | Free (`MTMS`) | J2-20 | JTAG |
| 43 | `U0TXD` → CH340K `RXD` | J2-10 | UART0, default console |
| 44 | `U0RXD` ← CH340K `TXD` | J2-21 | UART0, default console |
| 45 | Free | J2-9 | Strapping pin (VDD_SPI voltage). 10 kΩ pull-down (R6). It must be low at reset, or the flash gets 1.8 V. |
| 46 | Free | J2-22 | Strapping pin. 10 kΩ pull-down (R5). |
| 47 | Free | J2-8 | |
| 48 | Free | J2-23 | There is no RGB LED on this pin, unlike many other S3 boards. |

### Labels for an LCD

The main sheet labels GPIO15/16/17/18/21 as `LCD_RST`, `LCD_D/C`, `LCD_MOSI`, `LCD_SCLK` and
`LCD_CS`. None of the sheets has an LCD connector. These pins are already used by the W5500 reset
and the camera, so the labels look like leftovers from an earlier design.

### Free pins

- **Always free:** GPIO 39, 40, 41, 42, 47, 48.
- **Free, with care:** GPIO 45 and 46 are strapping pins with pull-downs. Don't pull them high at
  reset. GPIO 43 and 44 are free if you don't need UART0 or the CH340K.
- **Free if no camera is fitted:** GPIO 1–9, 16, 17, 18, 21. GPIO 0 and 3 are strapping pins.
- **Free if the SD slot is empty:** GPIO 33–38. The 51 kΩ pull-ups stay on these pins.

## Header J1 (30 pins, 2×15)

| Pin | Signal | | Signal | Pin |
|----:|--------|-|--------|----:|
| 1  | 5V      | | 5V      | 30 |
| 2  | GND     | | GND     | 29 |
| 3  | 3.3V    | | 3.3V    | 28 |
| 4  | EN (CHIP_PU) | | 3.3V | 27 |
| 5  | GPIO0   | | GPIO1   | 26 |
| 6  | GPIO2   | | GPIO3   | 25 |
| 7  | GPIO4   | | GPIO5   | 24 |
| 8  | GPIO6   | | GPIO7   | 23 |
| 9  | GPIO8   | | GPIO9   | 22 |
| 10 | GPIO10  | | GPIO11  | 21 |
| 11 | GPIO12  | | GPIO13  | 20 |
| 12 | GPIO14  | | GPIO15  | 19 |
| 13 | GPIO16  | | GPIO17  | 18 |
| 14 | GPIO18  | | GPIO19 / USB D- | 17 |
| 15 | GPIO20 / USB D+ | | GPIO21 | 16 |

## Header J2 (30 pins, 2×15)

| Pin | Signal | | Signal | Pin |
|----:|--------|-|--------|----:|
| 1  | 5V      | | 5V      | 30 |
| 2  | 5V      | | 5V      | 29 |
| 3  | GND     | | GND     | 28 |
| 4  | GND     | | GND     | 27 |
| 5  | GND     | | GND     | 26 |
| 6  | 3.3V    | | 3.3V    | 25 |
| 7  | 3.3V    | | 3.3V    | 24 |
| 8  | GPIO47  | | GPIO48  | 23 |
| 9  | GPIO45  | | GPIO46  | 22 |
| 10 | GPIO43 / U0TXD | | GPIO44 / U0RXD | 21 |
| 11 | GPIO41  | | GPIO42  | 20 |
| 12 | GPIO39  | | GPIO40  | 19 |
| 13 | GPIO37 / SD DAT0 | | GPIO38 / SD DAT1 | 18 |
| 14 | GPIO35 / SD CMD  | | GPIO36 / SD CLK  | 17 |
| 15 | GPIO33 / SD DAT2 | | GPIO34 / SD DAT3 | 16 |

The `5V` header pins are on the **switched** `+5V` rail, after the power switch SW1. See
[hardware.md](hardware.md#power).

## Copy-paste pin definitions

```cpp
// W5500 Ethernet (SPI2 / FSPI)
#define ETH_CS    10
#define ETH_MOSI  11
#define ETH_SCLK  12
#define ETH_MISO  13
#define ETH_INT   14
#define ETH_RST   15   // pulled low on the board: drive HIGH to release reset

// microSD (SDMMC, 4-bit)
#define SD_CLK    36
#define SD_CMD    35
#define SD_D0     37
#define SD_D1     38
#define SD_D2     33
#define SD_D3     34

// Camera connector (OV2640, 8-bit DVP)
#define CAM_PWDN  -1   // tied to GND on the board
#define CAM_RESET -1   // wired to GPIO0 (BOOT): left alone, uses software reset
#define CAM_XCLK  -1   // no XCLK pin on the connector
#define CAM_SIOD  18
#define CAM_SIOC  17
#define CAM_D0    1
#define CAM_D1    2
#define CAM_D2    3
#define CAM_D3    4
#define CAM_D4    5
#define CAM_D5    6
#define CAM_D6    7
#define CAM_D7    8
#define CAM_VSYNC 21
#define CAM_HREF  16
#define CAM_PCLK  9

// Buttons
#define BOOT_BUTTON 0  // active low
```
