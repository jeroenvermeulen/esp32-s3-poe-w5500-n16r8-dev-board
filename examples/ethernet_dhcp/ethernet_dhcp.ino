// Brings up the on-board W5500 Ethernet with DHCP and fetches a web page.
// Requires the Arduino-ESP32 core 3.x.

#include <ETH.h>
#include <NetworkClient.h>
#include <esp_system.h>

// W5500 wiring on this board (see docs/pinout.md)
#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR     1
#define ETH_PHY_CS       10
#define ETH_PHY_IRQ      14
#define ETH_PHY_RST      15  // pulled low on the board; the driver drives it high
#define ETH_PHY_SPI_HOST SPI2_HOST
#define ETH_PHY_SPI_SCK  12
#define ETH_PHY_SPI_MISO 13
#define ETH_PHY_SPI_MOSI 11

static bool ethConnected = false;
static uint32_t linkUpCount = 0;
static uint32_t linkUpSince = 0;

// Prefixes each log line with the uptime, so link drops can be lined up with reboots
#define LOG(fmt, ...) Serial.printf("[%8.3f] " fmt "\n", millis() / 1000.0, ##__VA_ARGS__)

const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:  return "power-on";
    case ESP_RST_EXT:      return "external pin";
    case ESP_RST_SW:       return "software";
    case ESP_RST_PANIC:    return "panic";
    case ESP_RST_INT_WDT:  return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT:      return "other watchdog";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_USB:      return "USB";
    default:               return "unknown";
  }
}

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      LOG("ETH started");
      ETH.setHostname("esp32-s3-poe");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      linkUpCount++;
      linkUpSince = millis();
      LOG("ETH link up (#%lu): %u Mbit, %s duplex", linkUpCount, ETH.linkSpeed(),
          ETH.fullDuplex() ? "full" : "half");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      LOG("ETH got IP: %s", ETH.localIP().toString().c_str());
      Serial.println(ETH);
      ethConnected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      LOG("ETH lost IP");
      ethConnected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      LOG("ETH link down after %.1f s up", (millis() - linkUpSince) / 1000.0);
      ethConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      LOG("ETH stopped");
      ethConnected = false;
      break;
    default:
      break;
  }
}

void httpGet(const char *host, uint16_t port) {
  Serial.printf("\nConnecting to %s\n", host);
  NetworkClient client;
  if (!client.connect(host, port)) {
    Serial.println("Connection failed");
    return;
  }
  client.printf("GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", host);
  while (client.connected() || client.available()) {
    if (client.available()) {
      Serial.write(client.read());
    }
  }
  Serial.println("\n--- done ---");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  LOG("Boot, reset reason: %s", resetReasonName(esp_reset_reason()));

  Network.onEvent(onEvent);
  // On some boards the W5500 cannot hold a 100 Mbit link: auto-negotiation picks
  // 100 Mbit and the link drops every few seconds. Forcing 10 Mbit makes it stable.
  // Use half duplex: an auto-negotiating partner falls back to half duplex when
  // this side does not negotiate, so forced full duplex would cause a mismatch.
  ETH.setAutoNegotiation(false);
  ETH.setLinkSpeed(10);
  ETH.setFullDuplex(false);
  if (!ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST,
                 ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI)) {
    Serial.println("ETH.begin() failed - check the W5500 wiring");
  }
}

void loop() {
  if (ethConnected) {
    httpGet("example.com", 80);
  }
  delay(10000);
}
