// Brings up the on-board W5500 Ethernet with DHCP and fetches a web page.
// Requires the Arduino-ESP32 core 3.x.

#include <ETH.h>
#include <NetworkClient.h>

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

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH started");
      ETH.setHostname("esp32-s3-poe");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH link up");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("ETH got IP: %s\n", ETH.localIP().toString().c_str());
      Serial.println(ETH);
      ethConnected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH lost IP");
      ethConnected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH link down");
      ethConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH stopped");
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

  Network.onEvent(onEvent);
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
