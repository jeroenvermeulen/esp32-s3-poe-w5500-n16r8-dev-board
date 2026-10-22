# Getting started

## 1. Connect the board

1. Use a **USB-A to USB-C cable**. USB-C to USB-C does not work: the board gets no power and no
   serial port shows up. [hardware.md](hardware.md#usb-c-to-usb-c-cables-dont-work) explains
   why.
2. Plug it into the USB-C port marked **ESP-USB**. This is the ESP32-S3's built-in USB-Serial/JTAG.
3. Press the large **latching push switch** in the bottom-right corner so it stays down. The blue
   **LED1** turns on.
4. Check that the port shows up:

   ```console
   $ ls /dev/cu.usbmodem*          # macOS
   /dev/cu.usbmodem21401
   $ ls /dev/ttyACM*               # Linux
   ```

5. Identify the chip:

   ```console
   $ esptool --port /dev/cu.usbmodem21401 chip-id
   Chip type:          ESP32-S3 (QFN56) (revision v0.2)
   Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz
   Crystal frequency:  40MHz
   USB mode:           USB-Serial/JTAG
   MAC:                30:ed:a0:0c:29:5c

   $ esptool --port /dev/cu.usbmodem21401 flash-id
   Manufacturer: ef
   Device: 4018
   Detected flash size: 16MB
   Flash type set in eFuse: quad (4 data lines)
   Flash voltage set by a strapping pin: 3.3V
   ```

## 2. Arduino IDE

Install **esp32 by Espressif Systems** version 3.x in the Boards Manager. Then select:

| Setting | Value | Why |
|---------|-------|-----|
| Board | **ESP32S3 Dev Module** | |
| USB CDC On Boot | **Enabled** | Sends `Serial` to the ESP-USB port |
| USB Mode | Hardware CDC and JTAG | |
| Flash Size | **16MB (128Mb)** | |
| Flash Mode | QIO 80MHz | |
| PSRAM | **QSPI PSRAM** | The PSRAM is quad, not octal. With *OPI PSRAM* selected it is not found. |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) | Or any 16 MB scheme |
| Upload Mode | UART0 / Hardware CDC | |

If you upload through the CH340K port instead, set **USB CDC On Boot = Disabled**. `Serial` then
goes to UART0 (GPIO43/44), which is connected to the CH340K. The
[`usb_keyboard`](../examples/usb_keyboard) example needs this, because it uses the native USB lines
for the USB-A port ([details](hardware.md#using-the-usb-a-port-as-usb-host)).

### arduino-cli

```sh
FQBN='esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,FlashMode=qio,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB'

arduino-cli compile --fqbn "$FQBN" examples/chip_info
arduino-cli upload  --fqbn "$FQBN" -p /dev/cu.usbmodem21401 examples/chip_info
arduino-cli monitor -p /dev/cu.usbmodem21401 -c baudrate=115200
```

[`scripts/build-examples.sh`](../scripts/build-examples.sh) compiles every example with these
settings.

## 3. PlatformIO

The official `espressif32` platform still ships Arduino core 2.x. The examples need core 3.x,
because they use the `ETH.begin(ETH_PHY_W5500, ...)` SPI API. Use the community
[pioarduino](https://github.com/pioarduino/platform-espressif32) platform:

```ini
[env:esp32-s3-poe-w5500]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32-s3-devkitc-1
framework = arduino
board_upload.flash_size = 16MB
board_build.flash_mode = qio
board_build.partitions = default_16MB.csv
board_build.arduino.memory_type = qio_qspi   ; quad flash + quad PSRAM
build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
monitor_speed = 115200
```

## 4. ESP-IDF

Use these settings in `sdkconfig`:

```
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ETH_SPI_ETHERNET_W5500=y
```

## Troubleshooting

### No power, no serial port

- Use a USB-A to USB-C cable. A USB-C to USB-C cable gives no power.
- Press the power switch so it latches down. LED1 must be on.
- Some USB cables carry power only. Try another cable.

### "PSRAM not found" or the board reboots at startup

PSRAM is set to OPI. Change it to **QSPI PSRAM** (`PSRAM=enabled`, or
`board_build.arduino.memory_type = qio_qspi`).

### The port disappears while uploading, or after a crash

USB-Serial/JTAG belongs to the chip. A sketch that crashes early, or one that reconfigures
GPIO19/20, can make it vanish. Force download mode: hold **BOOT**, tap **RESET**, release
**BOOT**, then upload again.

### Serial Monitor shows nothing on the ESP-USB port

Set **USB CDC On Boot = Enabled**. Otherwise `Serial` goes to UART0/CH340K.

### The CH340K port doesn't show up

**Cause on macOS:** the CH340K reports USB ID **`1a86:7522`**. The CH34x driver built into
macOS doesn't match this product ID. The chip enumerates as "USB Serial", but no driver
attaches and no `/dev/cu.*` port is created. **(verified)**

**Fix:** install WCH's own driver.

1. Download **CH34XSER_MAC** from the [WCH download page](https://www.wch-ic.com/downloads/CH34XSER_MAC_ZIP.html)
   and run the installer. It installs the `CH34xVCPDriver` app.
2. Open `CH34xVCPDriver` from Applications and click **Install**.
3. **Enable the driver.** macOS installs it switched off. Go to **System Settings → General →
   Login Items & Extensions → Driver Extensions (ⓘ)** and turn on **CH34xVCPDriver**. Then
   unplug and replug the board. Check the state with:

   ```console
   $ systemextensionsctl list | grep -i wch
   *	*	5JZGQTGU4W	cn.wch.CH34xVCPDriver (1.0/1)	cn.wch.CH34xVCPDriver	[activated enabled]
   ```

   `[activated disabled]` means the switch is still off, and no port is created. On a managed
   (MDM) Mac, IT may have to allow the extension.
4. The port appears as `/dev/cu.wchusbserial*`, for example `/dev/cu.wchusbserial2140`.
   Check it with `esptool --port /dev/cu.wchusbserial2140 chip-id`. **(verified)**

To check whether the chip is seen on USB at all:

```sh
# macOS: expect "USB Vendor ID: 0x1a86" and "USB Product ID: 0x7522"
system_profiler SPUSBHostDataType | grep -B4 -A2 -i 0x1a86   # older macOS: SPUSBDataType
# (ioreg also lists it, as "USB Serial", but prints the IDs in decimal: 6790 / 29986)
# Linux (the ch341 kernel driver supports 7522): lsusb shows 1a86:7522, port is /dev/ttyUSB0
lsusb | grep 1a86 ; dmesg | tail
```

If the chip isn't listed at all, check the cable first: it must be USB-A to USB-C. Also try the
USB-C plug the other way round, in case only one D+/D- pair is connected. If it still doesn't
show up, the problem is the hardware (soldering). The CH340K is powered from VBUS before the
power switch, so the switch position doesn't matter.

**Sanity check:** open the port at 115200 baud and press RESET. The ROM bootloader always prints
its boot log (`ESP-ROM:esp32s3-...`) on UART0, so it shows up here. Your sketch's `Serial`
output only appears on this port with **USB CDC On Boot = Disabled**.

### Ethernet doesn't come up

- GPIO15 must be driven **high**. The board has a pull-down that holds the W5500 in reset. The
  examples pass `ETH_RST = 15` to `ETH.begin()`, which handles this.
- Check that the RJ45 link LED is on.

### Ethernet link keeps going up and down

The link comes up at 100 Mbit full duplex, drops after a second or two, and comes back, over and
over. The RJ45 LEDs go off with it, and DHCP never gets an address.

On at least one board (**verified**) the W5500 can't hold a 100 Mbit link. This happened with
different cables and with both a switch and a Mac on the other end, so it isn't the network.
Polling the W5500's `PHYCFGR` register directly, without the Ethernet driver, showed:

| PHY mode | Result |
|----------|--------|
| Auto-negotiation (picks 100 Mbit full) | link drops about every 2.5 s |
| Forced 100 Mbit, full or half duplex | no link |
| Forced 10 Mbit, full or half duplex | stable link |

The cause is probably marginal analog hardware around the PHY (crystal, `+3.3A` supply or
magnetics). It hasn't been tracked down.

The workaround is to force 10 Mbit half duplex before `ETH.begin()`. The examples do this:

```cpp
ETH.setAutoNegotiation(false);
ETH.setLinkSpeed(10);
ETH.setFullDuplex(false);
```

Use half duplex, not full. When one side doesn't auto-negotiate, the other side falls back to half
duplex, so forcing full duplex causes a duplex mismatch. The cost is throughput: at most about
1 MB/s, which limits the camera stream's frame rate.

If your board holds a 100 Mbit link, remove these three lines to get the full speed.
