// Serves a small status page over the on-board W5500 Ethernet.
// Open http://esp32-s3-poe.local/ or the IP address printed on the serial port.

#include <ETH.h>
#include <ESPmDNS.h>
#include <WebServer.h>

#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR     1
#define ETH_PHY_CS       10
#define ETH_PHY_IRQ      14
#define ETH_PHY_RST      15
#define ETH_PHY_SPI_HOST SPI2_HOST
#define ETH_PHY_SPI_SCK  12
#define ETH_PHY_SPI_MISO 13
#define ETH_PHY_SPI_MOSI 11

#define HOSTNAME "esp32-s3-poe"

WebServer server(80);

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(HOSTNAME);
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("Open http://%s/ or http://%s.local/\n", ETH.localIP().toString().c_str(), HOSTNAME);
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH link down");
      break;
    default:
      break;
  }
}

void handleRoot() {
  char page[1024];
  snprintf(page, sizeof(page),
           "<!doctype html><html><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<meta http-equiv='refresh' content='5'><title>%s</title>"
           "<style>body{font-family:system-ui;margin:2em}td{padding:.2em 1em}</style></head><body>"
           "<h1>ESP32-S3 PoE W5500</h1><table>"
           "<tr><td>Uptime</td><td>%lu s</td></tr>"
           "<tr><td>IP</td><td>%s</td></tr>"
           "<tr><td>Link</td><td>%u Mbps %s duplex</td></tr>"
           "<tr><td>MAC</td><td>%s</td></tr>"
           "<tr><td>Free heap</td><td>%lu bytes</td></tr>"
           "<tr><td>Free PSRAM</td><td>%lu bytes</td></tr>"
           "</table></body></html>",
           HOSTNAME, millis() / 1000, ETH.localIP().toString().c_str(), ETH.linkSpeed(),
           ETH.fullDuplex() ? "full" : "half", ETH.macAddress().c_str(), ESP.getFreeHeap(),
           ESP.getFreePsram());
  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Network.onEvent(onEvent);
  // Force 10 Mbit half duplex: some boards can't hold a 100 Mbit link
  // (see docs/getting-started.md#ethernet-link-keeps-going-up-and-down)
  ETH.setAutoNegotiation(false);
  ETH.setLinkSpeed(10);
  ETH.setFullDuplex(false);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST,
            ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);

  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);

  server.on("/", handleRoot);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void loop() {
  server.handleClient();
  delay(2);
}
