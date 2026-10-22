# Hardware

This is a sheet-by-sheet reading of the [vendor schematics](../schematics/). Each point says
whether it was **verified** on a real board or only **read from the schematic**.

## Summary

| Block | Part | Source |
|-------|------|--------|
| Board revision | V1.2 (bottom silkscreen); PCB marking `634822A_P23_250515` | board photos |
| MCU | ESP32-S3 (QFN56) rev v0.2, bare chip (not a WROOM module) | verified with `esptool chip-id` |
| Flash | Winbond W25Q128, 16 MB, quad SPI, 3.3 V | verified with `esptool flash-id` (manufacturer `ef`, device `4018`) |
| PSRAM | ESP-PSRAM64H, 8 MB, **quad SPI (QSPI), not octal** | schematic |
| Ethernet | WIZnet W5500 on SPI, 25 MHz crystal | schematic |
| Magnetics | Hanrun HR601680 (PoE-capable) | schematic |
| PoE | 3 bridge rectifiers, simple PD front end, TI TPS54360 buck to 5 V | schematic |
| 3.3 V regulator | AMS1117-3.3 type LDO | schematic |
| USB-UART | WCH CH340K (`1a86:7522`) on its own USB-C port, with auto-program | verified (needs the WCH driver on macOS) |
| Native USB | ESP32-S3 USB-Serial/JTAG on a second USB-C port, plus a USB-A host port | verified |
| Storage | microSD slot, 4-bit SDMMC | schematic |
| Camera | 18-pin OV2640 connector, 8-bit DVP | schematic |
| Antenna | SMA connector (ANT1). An external 2.4 GHz antenna is required. | board photo |
| Buttons | BOOT (GPIO0), RESET (EN), latching power switch | schematic, power switch verified |
| LEDs | LED1 (power, on 3.3 V), RJ45 link/activity LEDs | verified (LED1) |

"N16R8" means 16 MB flash and 8 MB PSRAM. Most N16R8 modules, such as the ESP32-S3-WROOM-1-N16R8,
use **octal** PSRAM. This board uses a separate **quad** PSRAM chip. Set PSRAM to **QSPI**, not
OPI. See [getting-started.md](getting-started.md#arduino-ide).

## Power

```
USB-C "CH340K" (J3) VBUS ─┐
USB-C "ESP-USB" (J4) VBUS ─┼── VBUS ── SW1 (power switch) ── +5V ──┬── AMS1117 ── +3.3V ── LED1
PoE buck output ── D5 ─────┘                                      ├── J1/J2 5V pins
                                                                  └── USB-A (J5) VBUS
```

- **SW1 is the main power switch.** It is the large latching push button in the corner of the
  board. It sits between `VBUS` and `+5V`. Press it so it latches down, and LED1 turns on.
  **(verified)**
- **The CH340K is powered from `VBUS`, before SW1.** It should enumerate on USB even with the
  switch off.
- **All VBUS sources share one net.** J3 VBUS, J4 VBUS and the PoE output (through diode D5) are
  joined. Some consequences:
  - With two USB-C cables to two different computers, their VBUS lines are connected to each
    other.
  - With PoE and USB both connected, the board's PoE 5 V reaches the computer's VBUS pin. D5 only
    stops current flowing back into the buck converter, not into the USB host.
- **The `5V` header pins are after SW1.** If you power the board through these pins, the switch
  does nothing. With SW1 on, that 5 V also reaches both USB-C VBUS pins.
- The 3.3 V rail comes from an AMS1117-class LDO with a 100 µF input and a 10 µF output
  capacitor. Its dropout is about 1.1 V. That is fine from USB, but tighter on PoE because of the
  D5 diode drop.
- The W5500 gets its own 3.3 V supply (`+3.3E`) through a 0 Ω resistor (R25). Its analog supply
  (`+3.3A`) is filtered by a 1 µH inductor (L3).

## USB

There are three USB connectors.

| Connector | Data lines go to | VBUS | Use it for |
|-----------|------------------|------|------------|
| J3, USB-C, "CH340K" | CH340K USB-UART → UART0 (GPIO43/44) and auto-program (EN/GPIO0) | VBUS net | Serial console, flashing with `esptool` over UART |
| J4, USB-C, "ESP-USB" | ESP32-S3 native USB (GPIO19/20) | VBUS net | Flashing and serial over USB-Serial/JTAG, JTAG debugging, TinyUSB |
| J5, USB-A female | ESP32-S3 native USB (GPIO19/20), **same lines as J4** | `+5V` (switched) | USB host (OTG) for keyboards, flash drives and similar |

### USB-C to USB-C cables don't work

**(verified)** Use a USB-A to USB-C cable.

The schematic shows the cause. On both USB-C connectors, `CC1` (A5) and `CC2` (B5) are not
connected. A USB-C device must pull each CC pin to GND with a 5.1 kΩ resistor. A USB-C host (or
charger) only turns on VBUS when it sees those resistors. Without them it never supplies power,
and the board stays dead. A USB-A to USB-C cable has the pull-up resistor built into the cable
and does not check CC, so it always supplies 5 V.

### J4 and J5 share the native USB lines

J4 (ESP-USB) and J5 (USB-A female) are wired in parallel to GPIO19/20. Don't connect a computer to J4 and a device to J5
at the same time. In host mode J5 gets its VBUS from the switched `+5V` rail.

### Using the USB-A port as USB host

**(verified)** with a USB keyboard on J5 and the computer on J3.

To use J5 (for example with [`examples/usb_keyboard`](../examples/usb_keyboard)), connect the
computer to the **CH340K port (J3)**, and leave J4 unplugged. Then flash and read the serial output
through the CH340K:

- **Serial output can't go over J4 in host mode.** The ESP32-S3 has one internal USB PHY on
  GPIO19/20. It is connected to either the USB-Serial/JTAG controller (the `usbmodem` port on J4) or
  the USB-OTG controller. `usb_host_install()` switches it to OTG, and the J4 serial port goes away.
- **J4 and J5 are one bus.** With a computer on J4 and a device on J5, two hosts drive the same
  D+/D- lines. The computer may even enumerate the device on J5 itself.
- **Flashing over the CH340K keeps working.** Its auto-program circuit drives EN and GPIO0 directly.
  On J4, once a sketch has switched the PHY to host mode, the automatic reset into download mode
  stops working, and you need BOOT + RESET for every upload.
- **One cable powers everything.** J3's VBUS goes through the power switch to `+5V`, which also
  powers J5. The USB device on J5 shares the 500 mA from the computer's port with the board.
- **Arduino setting:** set **USB CDC On Boot = Disabled**, so `Serial` goes to UART0.

### CH340K port (J3)

The CH340K is wired the standard way:

- `TXD` → `U0RXD` (GPIO44), `RXD` ← `U0TXD` (GPIO43)
- VCC comes from VBUS. The 3V3 pin is decoupled with 100 nF.
- `DTR`/`RTS` drive the usual two-transistor auto-program circuit (Q1, Q2) for EN and GPIO0.

| DTR | RTS | EN | GPIO0 |
|:---:|:---:|:--:|:-----:|
| 1 | 1 | 1 | 1 |
| 0 | 0 | 1 | 1 |
| 1 | 0 | 0 | 1 |
| 0 | 1 | 1 | 0 |

**Verified:** with the WCH driver installed, `esptool` connects and flashes through J3 without
pressing BOOT, and the ROM boot log (`ESP-ROM:esp32s3-20210327`) appears at 115200 baud after a
reset.

> **macOS needs the WCH driver.** The CH340K reports USB ID `1a86:7522`. The CH34x driver built
> into macOS doesn't match that ID, so the chip enumerates but no serial port appears
> **(verified)**. See [getting-started.md](getting-started.md#the-ch340k-port-doesnt-show-up).
> The CC problem above also applies, so use a USB-A to USB-C cable.

## Buttons and strapping pins

| Ref | Function | Circuit |
|-----|----------|---------|
| S1 | BOOT | GPIO0 to GND, 100 nF across it (C15), 10 kΩ pull-up (R3) |
| S2 | RESET | EN (CHIP_PU) to GND, 100 nF across it (C16), 10 kΩ pull-up (R2) |
| SW1 | Power | Latching push switch between VBUS and +5V |

Strapping pins:

| Pin | Board default | Effect |
|-----|---------------|--------|
| GPIO0 | High (R3) | Normal SPI boot. Hold BOOT to enter download mode. |
| GPIO3 | High (R4) | JTAG source selection. Only matters if eFuses are set. |
| GPIO45 | Low (R6) | VDD_SPI = 3.3 V. The flash is 3.3 V, so keep this low. **(verified: "Flash voltage set by a strapping pin: 3.3V")** |
| GPIO46 | Low (R5) | ROM messages and download-mode settings. Low is the normal value. |

Download mode by hand: hold **BOOT**, press and release **RESET**, then release **BOOT**. You
rarely need this. The ESP-USB port (J4) resets the chip into download mode by itself.

## Memory

- **Flash U2: W25Q128.** It is on SPI0 `CS0` (GPIO29), with D/Q/WP/HD/CLK on GPIO32/31/28/27/30,
  powered from `VDD_SPI`.
- **PSRAM U3: ESP-PSRAM64H.** It is on the same quad bus, using `CS1` (GPIO26).
- Because both chips are quad SPI, GPIO33–37 stay free. On octal-PSRAM modules those pins are
  used up, but here they go to the SD card.
- Crystal X1 is 40 MHz with 15 pF load capacitors.

## Ethernet (W5500)

- The W5500 (U7) sits on the S3's SPI2 (FSPI) bus. Every signal goes through a 0 Ω resistor
  (R26–R31), so a line can be cut and rerouted if needed.

  | W5500 | Net | GPIO |
  |-------|-----|-----:|
  | SCSn | SPI2_CS0 | 10 |
  | MOSI | SPI2_MOSI | 11 |
  | SCLK | SPI2_SCLK | 12 |
  | MISO | SPI2_MISO | 13 |
  | INTn | W5500_INT | 14 |
  | RSTn | W5500_RST | 15 |

- **RSTn has a 10 kΩ pull-down (R24).** The W5500 stays in reset until GPIO15 is driven high. If
  your library has no reset-pin setting, call `pinMode(15, OUTPUT); digitalWrite(15, HIGH);` and
  wait about 50 ms before you start the W5500.
- `PMODE0..2` are tied high, so the PHY uses auto-negotiation with all modes enabled.
- **100 Mbit isn't reliable on at least one board (verified).** The link drops every few seconds,
  while forced 10 Mbit is stable. The examples force 10 Mbit half duplex. See
  [getting-started.md](getting-started.md#ethernet-link-keeps-going-up-and-down).
- The 25 MHz crystal (X2) has 18 pF load capacitors and a 1 MΩ resistor across it.
- `EXRES1` has the 12.4 kΩ bias resistor (R33).
- `ACTLED` and `LINKLED` drive the LEDs built into the RJ45 jack, through 82 Ω resistors.
- The magnetics are a Hanrun **HR601680**, a discrete PoE-rated transformer. The TX/RX center taps
  are brought out as `TX-POE`/`RX-POE` for PoE Mode A, with 75 Ω/82 Ω and 100 nF Bob Smith
  termination.

## PoE

```
RJ45 pins 4/5 ─ D1 ┐
RJ45 pins 7/8 ─ D2 ┼─ +48VPOE ─ R17 24k (signature) ─ Q3 (switched by D4 24 V zener) ─ GND
TX/RX center taps ─ D3 ┘    │
                            └─ TPS54360 buck (4.5–60 V in) ─ L2 10 µH ─ ≈5.1 V ─ D5 ─ VBUS
```

- **Both PoE modes are supported:** Mode A (power on the data pairs, through the transformer center
  taps and D3) and Mode B (power on the spare pairs 4/5 and 7/8, through D1/D2).
- **The PD front end is discrete.** There is no PoE PD controller chip.
  - R17 (24 kΩ) is the detection signature resistor. IEEE 802.3af/at expects about 25 kΩ.
  - There is **no classification resistor**, so the switch sees the board as Class 0.
  - Q3, an N-MOSFET on the return path, is turned on by a circuit with a 24 V zener (D4) and
    R16/R18. It connects the load only after the PSE has raised the voltage above the
    detection/classification range.
- The buck converter (U6, TPS54360) sets its output with R19 = 51 kΩ over R22 + R23 = 9.4 kΩ:
  0.8 V × (1 + 51 / 9.4) ≈ **5.1 V**. RT = 200 kΩ gives roughly 480 kHz switching.
- **The PoE supply is not isolated.** The board's GND connects to the PoE negative rail through
  Q3, and the TPS54360 is a plain buck converter, not an isolated flyback. This doesn't meet the
  802.3 isolation requirement. In practice:
  - Don't connect the board to a computer over USB while it runs on PoE, unless you use a USB
    isolator. See also the VBUS note in [Power](#power).
  - Treat anything connected to the headers as connected to the PoE return.
- PoE switches and injectors that are tested to work aren't recorded yet. Contributions are
  welcome.

## microSD (TF1)

- It is wired for **4-bit SDMMC**: CLK 36, CMD 35, D0 37, D1 38, D2 33, D3 34. The S3 routes
  SDMMC through the GPIO matrix, so any pins work.
- CMD, D0–D3 and the card-detect pin have 51 kΩ pull-ups to 3.3 V.
- The card-detect switch (pin 9) is only pulled up. **It is not connected to a GPIO.**
- 1-bit SDMMC mode and SPI mode (CS = D3 = GPIO34, MOSI = CMD = GPIO35, MISO = D0 = GPIO37,
  SCK = GPIO36) also work.

## Camera (CAM1)

18-pin connector labelled `ov2640-model`:

| Cam pin | Signal | GPIO | | Cam pin | Signal | GPIO |
|--------:|--------|-----:|-|--------:|--------|-----:|
| 1  | GND   | – | | 18 | 3V3   | – |
| 2  | SCL   | 17 | | 17 | VSYNC | 21 |
| 3  | SDA   | 18 | | 16 | HREF  | 16 |
| 4  | D0    | 1 | | 15 | RST   | 0 |
| 5  | D2    | 3 | | 14 | D1    | 2 |
| 6  | D4    | 5 | | 13 | D3    | 4 |
| 7  | D6    | 7 | | 12 | D5    | 6 |
| 8  | PCLK  | 9 | | 11 | D7    | 8 |
| 9  | PWDN  | GND | | 10 | NC  | – |

- **The connector has no XCLK pin.** The camera module must have its own oscillator. A "bare"
  OV2640 module that expects XCLK from the MCU won't work without a bodge wire from a free GPIO
  (for example GPIO47) to the module's XCLK pin.
- `PWDN` is tied to GND, so the camera is always powered.
- `RST` is wired to GPIO0, the BOOT pin. GPIO0 has a pull-up, so the camera runs normally. Pressing
  BOOT also resets the camera.
- The camera uses most of GPIO1–9 and 16–21. With a camera fitted, few pins on J1 are left free.

## Antenna

The RF output (`LNA_IN`) goes through a π matching network (R1 0 Ω in series, C1/C2 3 pF to GND)
to the antenna connector, which the schematic labels `ANT1` / `IPX-4`. The
[board photo](images/board-top.jpg) shows an **SMA connector** here, and there is no PCB
antenna. Wi-Fi and Bluetooth need an external 2.4 GHz antenna.
