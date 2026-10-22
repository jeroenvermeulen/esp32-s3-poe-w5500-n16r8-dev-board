# Schematics

These are the vendor's schematics, copied from the AliExpress product listing. They are Altium
exports dated 2025-08-29 and have no title block or revision. They are included here for
reference only; the vendor owns the copyright.

| File | Sheet | Contents |
|------|-------|----------|
| [esp32-s3-main.jpg](esp32-s3-main.jpg) | `main.SchDoc` | ESP32-S3 (bare QFN56 chip), 40 MHz crystal, W25Q128 flash, ESP-PSRAM64H, antenna/IPEX, J1/J2 headers, microSD slot |
| [power-usb-poe.jpg](power-usb-poe.jpg) | `power.SchDoc` | 2× USB-C, USB-A host port, CH340K, auto-program transistors, BOOT/RESET buttons, power switch, AMS1117-3.3, PoE rectifiers and TPS54360 buck converter |
| [w5500-ethernet-camera.jpg](w5500-ethernet-camera.jpg) | `W5500.SchDoc` | W5500 Ethernet controller, 25 MHz crystal, HR601680 PoE magnetics, RJ45, OV2640 camera connector |

Original URLs:

- https://ae-pic-a1.aliexpress-media.com/kf/S542710a9662f46be8b34b77d8b2e70214.jpg
- https://ae-pic-a1.aliexpress-media.com/kf/S5d8151d2ce9544b0874de3bc14a1f8f9v.jpg
- https://ae-pic-a1.aliexpress-media.com/kf/Sd8e40d3f455d4579a4c4f630b41bd7acF.jpg

> The CDN serves WebP by default. To download the JPEG, send `Accept: image/jpeg`, for example
> `curl -H 'Accept: image/jpeg' -O <url>`.

[docs/hardware.md](../docs/hardware.md) has a sheet-by-sheet reading of these schematics.
