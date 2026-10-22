// Prints chip, flash, PSRAM and MAC information.
// Use this first to check your board settings: it should report 16 MB flash and ~8 MB PSRAM.
// If PSRAM shows 0, set "PSRAM: QSPI PSRAM" (not OPI) in the board settings.

#include <Arduino.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_mac.h>

void setup() {
  Serial.begin(115200);
  delay(2000);  // give the USB CDC port time to come up

  esp_chip_info_t info;
  esp_chip_info(&info);

  uint32_t flashSize = 0;
  esp_flash_get_size(NULL, &flashSize);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  Serial.println();
  Serial.println("=== ESP32-S3 PoE W5500 N16R8 ===");
  Serial.printf("Model:        %s rev v%d.%d\n", ESP.getChipModel(), info.revision / 100, info.revision % 100);
  Serial.printf("Cores:        %d @ %lu MHz\n", info.cores, ESP.getCpuFreqMHz());
  Serial.printf("Flash:        %lu MB (%s)\n", flashSize / (1024 * 1024),
                ESP.getFlashChipMode() == FM_QIO ? "QIO" : "other mode");
  Serial.printf("PSRAM:        %lu bytes total, %lu free\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("Heap:         %lu bytes free\n", ESP.getFreeHeap());
  Serial.printf("WiFi MAC:     %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("SDK:          %s\n", ESP.getSdkVersion());

  if (ESP.getPsramSize() == 0) {
    Serial.println("WARNING: no PSRAM found. Select 'QSPI PSRAM' in the board settings.");
  }
}

void loop() {
  delay(1000);
}
