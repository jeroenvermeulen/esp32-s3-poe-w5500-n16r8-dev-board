// Scans for Wi-Fi networks, connects as a station and fetches a web page.
// Requires the Arduino-ESP32 core 3.x.
// The board has no PCB antenna: connect a 2.4 GHz antenna to the SMA connector
// first, or the scan finds nothing (see docs/hardware.md#antenna).

#include <WiFi.h>
#include <NetworkClient.h>

// Fill in your network
#define WIFI_SSID     "GRS-IOT"
#define WIFI_PASSWORD "youremindme"

static bool wifiConnected = false;

// Prefixes each log line with the uptime
#define LOG(fmt, ...) Serial.printf("[%8.3f] " fmt "\n", millis() / 1000.0, ##__VA_ARGS__)

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      LOG("WiFi started");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      LOG("WiFi connected to %s, channel %u", WiFi.SSID().c_str(), WiFi.channel());
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      LOG("WiFi got IP: %s, RSSI %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
      wifiConnected = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      LOG("WiFi lost IP");
      wifiConnected = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      LOG("WiFi disconnected, reason %u", info.wifi_sta_disconnected.reason);
      wifiConnected = false;
      break;
    default:
      break;
  }
}

// A scan is a quick antenna check: with an antenna, nearby networks show
// -30 to -80 dBm. Without one, expect no networks or only very weak ones.
void scanNetworks() {
  LOG("Scanning...");
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    LOG("No networks found - is the antenna connected?");
    return;
  }
  for (int i = 0; i < n; i++) {
    Serial.printf("  %2d  %4d dBm  ch %2ld  %-7s  %s\n", i + 1, WiFi.RSSI(i), WiFi.channel(i),
                  WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "secured",
                  WiFi.SSID(i).c_str());
  }
  WiFi.scanDelete();
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
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("esp32-s3-poe");
  scanNetworks();

  // The driver reconnects by itself after a disconnect
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  if (wifiConnected) {
    LOG("RSSI %d dBm", WiFi.RSSI());
    httpGet("example.com", 80);
  }
  delay(10000);
}
