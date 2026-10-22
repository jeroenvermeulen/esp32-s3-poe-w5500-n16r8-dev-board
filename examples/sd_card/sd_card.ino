// Mounts the microSD card in 4-bit SDMMC mode, lists the root directory
// and writes/reads a test file.
// The card-detect switch is not wired to a GPIO, so insert the card before reset.

#include <FS.h>
#include <SD_MMC.h>

#define SD_CLK 36
#define SD_CMD 35
#define SD_D0  37
#define SD_D1  38
#define SD_D2  33
#define SD_D3  34

void listDir(fs::FS &fs, const char *dirname) {
  Serial.printf("Listing %s\n", dirname);
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("  failed to open directory");
    return;
  }
  for (File file = root.openNextFile(); file; file = root.openNextFile()) {
    Serial.printf("  %s%s  %u bytes\n", file.name(), file.isDirectory() ? "/" : "", file.size());
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  if (!SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3)) {
    Serial.println("SD_MMC.setPins() failed");
    return;
  }
  if (!SD_MMC.begin("/sdcard", false /* 4-bit mode */)) {
    Serial.println("Card mount failed - is a FAT32/exFAT card inserted?");
    return;
  }

  const char *types[] = {"none", "MMC", "SD", "SDHC/SDXC", "unknown"};
  uint8_t type = SD_MMC.cardType();
  Serial.printf("Card type: %s\n", types[type <= CARD_SDHC ? type : 4]);
  Serial.printf("Card size: %llu MB\n", SD_MMC.cardSize() / (1024 * 1024));
  Serial.printf("Used:      %llu MB of %llu MB\n", SD_MMC.usedBytes() / (1024 * 1024),
                SD_MMC.totalBytes() / (1024 * 1024));

  listDir(SD_MMC, "/");

  File f = SD_MMC.open("/hello.txt", FILE_WRITE);
  if (f) {
    f.printf("Hello from ESP32-S3 PoE W5500, uptime %lu ms\n", millis());
    f.close();
  }
  f = SD_MMC.open("/hello.txt");
  if (f) {
    Serial.print("hello.txt: ");
    while (f.available()) {
      Serial.write(f.read());
    }
    f.close();
  }
}

void loop() {
  delay(1000);
}
