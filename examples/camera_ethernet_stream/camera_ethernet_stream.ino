// Streams the OV2640 camera as MJPEG over the on-board W5500 Ethernet.
//   http://<ip>/         -> page with the live stream
//   http://<ip>/jpg      -> single snapshot
//   http://<ip>/stream   -> raw MJPEG stream
//
// UNTESTED: the camera connector has no XCLK pin, so this only works with a camera
// module that has its own oscillator. See docs/hardware.md#camera-cam1.

#include <ETH.h>
#include <WebServer.h>
#include <esp_camera.h>

#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR     1
#define ETH_PHY_CS       10
#define ETH_PHY_IRQ      14
#define ETH_PHY_RST      15
#define ETH_PHY_SPI_HOST SPI2_HOST
#define ETH_PHY_SPI_SCK  12
#define ETH_PHY_SPI_MISO 13
#define ETH_PHY_SPI_MOSI 11

#define CAM_PWDN  -1  // tied to GND on the board
#define CAM_RESET -1  // wired to GPIO0 (BOOT); use the SCCB soft reset instead
#define CAM_XCLK  -1  // not on the connector
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

WebServer server(80);

bool initCamera() {
  camera_config_t config = {};
  config.pin_pwdn = CAM_PWDN;
  config.pin_reset = CAM_RESET;
  config.pin_xclk = CAM_XCLK;
  config.pin_sccb_sda = CAM_SIOD;
  config.pin_sccb_scl = CAM_SIOC;
  config.pin_d0 = CAM_D0;
  config.pin_d1 = CAM_D1;
  config.pin_d2 = CAM_D2;
  config.pin_d3 = CAM_D3;
  config.pin_d4 = CAM_D4;
  config.pin_d5 = CAM_D5;
  config.pin_d6 = CAM_D6;
  config.pin_d7 = CAM_D7;
  config.pin_vsync = CAM_VSYNC;
  config.pin_href = CAM_HREF;
  config.pin_pclk = CAM_PCLK;
  config.xclk_freq_hz = 20000000;
  config.ledc_timer = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  Serial.printf("Camera sensor PID: 0x%x\n", s->id.PID);
  return true;
}

void handleRoot() {
  server.send(200, "text/html",
              "<!doctype html><html><head><meta name='viewport' content='width=device-width'>"
              "<title>Camera</title></head><body style='margin:0;background:#000'>"
              "<img src='/stream' style='width:100%'></body></html>");
}

void handleJpg() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(503, "text/plain", "Camera capture failed");
    return;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStream() {
  NetworkClient client = server.client();
  client.print("HTTP/1.1 200 OK\r\n"
               "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
               "Cache-Control: no-store\r\n\r\n");
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      break;
    }
    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    esp_camera_fb_return(fb);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  if (!psramFound()) {
    Serial.println("No PSRAM - select 'QSPI PSRAM' in the board settings");
  }
  initCamera();

  Network.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
    if (event == ARDUINO_EVENT_ETH_GOT_IP) {
      Serial.printf("Open http://%s/\n", ETH.localIP().toString().c_str());
    }
  });
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST,
            ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);

  server.on("/", handleRoot);
  server.on("/jpg", handleJpg);
  server.on("/stream", handleStream);
  server.begin();
}

void loop() {
  server.handleClient();
  delay(1);
}
