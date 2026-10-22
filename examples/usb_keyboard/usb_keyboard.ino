// Reads a USB keyboard on the USB-A port (J5) and prints the keys on the CH340K port (J3).
// Requires the Arduino-ESP32 core 3.x. Uses the ESP-IDF USB Host Library directly.
//
// This example needs different settings from the other examples:
//   - USB CDC On Boot = Disabled, so Serial goes to UART0 and the CH340K.
//   - Connect the computer to the CH340K port (J3), and leave ESP-USB (J4) unplugged.
//     J4 and J5 share GPIO19/20, and in host mode the ESP-USB serial port is gone
//     (see docs/hardware.md#using-the-usb-a-port-as-usb-host).
//
// Only boot-protocol keyboards are decoded, with a US layout. Almost every keyboard
// supports the boot protocol. Caps Lock and Num Lock also switch the keyboard's LEDs.

#include <usb/usb_host.h>

#if ARDUINO_USB_CDC_ON_BOOT
#error "Set USB CDC On Boot = Disabled: Serial must go to the CH340K port, not to ESP-USB"
#endif

// Prefixes each log line with the uptime
#define LOG(fmt, ...) Serial.printf("\n[%8.3f] " fmt "\n", millis() / 1000.0, ##__VA_ARGS__)

// HID class requests and boot keyboard report layout
#define HID_REQ_SET_REPORT   0x09
#define HID_REQ_SET_PROTOCOL 0x0B
#define HID_SUBCLASS_BOOT    0x01
#define HID_PROTOCOL_KEYBOARD 0x01
#define HID_REPORT_SIZE      8   // modifiers, reserved, 6 key codes

#define MOD_CTRL  0x11
#define MOD_SHIFT 0x22
#define MOD_ALT   0x44
#define MOD_GUI   0x88

#define LED_NUM_LOCK  0x01
#define LED_CAPS_LOCK 0x02

// US layout for HID usages 0x04 (A) to 0x38 (/)
static const char kNormal[] = "abcdefghijklmnopqrstuvwxyz1234567890\n\x1b\b\t -=[]\\#;'`,./";
static const char kShifted[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()\n\x1b\b\t _+{}|~:\"~<>?";

struct Keyboard {
  usb_device_handle_t dev = nullptr;
  uint8_t intf = 0;
  uint8_t ep = 0;
  usb_transfer_t *ctrl = nullptr;  // SET_PROTOCOL and SET_REPORT (LEDs)
  usb_transfer_t *in = nullptr;    // interrupt IN, one report per key change
  bool ctrlBusy = false;
  bool inBusy = false;
  bool gone = false;
  uint8_t leds = 0;
  uint8_t prev[HID_REPORT_SIZE] = {};
};

static usb_host_client_handle_t client;
static Keyboard kbd;

// Addresses of new devices, filled in by the client callback
static uint8_t pendingAddr[8];
static int pendingCount = 0;

void printKey(uint8_t usage, uint8_t mods) {
  bool shift = mods & MOD_SHIFT;
  if (usage >= 0x04 && usage <= 0x1D && (kbd.leds & LED_CAPS_LOCK)) {
    shift = !shift;  // Caps Lock only affects letters
  }

  // Shortcuts with a letter or digit, such as <Ctrl+C>
  if ((mods & (MOD_CTRL | MOD_ALT | MOD_GUI)) && usage >= 0x04 && usage <= 0x27) {
    Serial.printf("<%s%s%s%c>", mods & MOD_CTRL ? "Ctrl+" : "", mods & MOD_ALT ? "Alt+" : "",
                  mods & MOD_GUI ? "GUI+" : "", toupper(kNormal[usage - 0x04]));
    return;
  }

  if (usage >= 0x04 && usage <= 0x38) {
    char c = (shift ? kShifted : kNormal)[usage - 0x04];
    switch (c) {
      case '\n': Serial.println(); break;
      case '\b': Serial.print("\b \b"); break;
      case '\x1b': Serial.print("<Esc>"); break;
      default: Serial.print(c); break;
    }
  } else if (usage >= 0x3A && usage <= 0x45) {
    Serial.printf("<F%d>", usage - 0x3A + 1);
  } else {
    switch (usage) {
      case 0x4C: Serial.print("<Del>"); break;
      case 0x4F: Serial.print("<Right>"); break;
      case 0x50: Serial.print("<Left>"); break;
      case 0x51: Serial.print("<Down>"); break;
      case 0x52: Serial.print("<Up>"); break;
      default: Serial.printf("<0x%02X>", usage); break;
    }
  }
}

void sendControl(uint8_t request, uint16_t value, const uint8_t *data, uint16_t len) {
  usb_setup_packet_t *setup = (usb_setup_packet_t *)kbd.ctrl->data_buffer;
  setup->bmRequestType = USB_BM_REQUEST_TYPE_DIR_OUT | USB_BM_REQUEST_TYPE_TYPE_CLASS |
                         USB_BM_REQUEST_TYPE_RECIP_INTERFACE;
  setup->bRequest = request;
  setup->wValue = value;
  setup->wIndex = kbd.intf;
  setup->wLength = len;
  memcpy(kbd.ctrl->data_buffer + sizeof(usb_setup_packet_t), data, len);
  kbd.ctrl->num_bytes = sizeof(usb_setup_packet_t) + len;

  esp_err_t err = usb_host_transfer_submit_control(client, kbd.ctrl);
  kbd.ctrlBusy = (err == ESP_OK);
  if (err != ESP_OK) {
    LOG("Control transfer failed: %s", esp_err_to_name(err));
  }
}

void updateLeds() {
  if (!kbd.ctrlBusy) {
    // SET_REPORT, output report 0: bit 0 Num Lock, bit 1 Caps Lock
    sendControl(HID_REQ_SET_REPORT, 0x0200, &kbd.leds, 1);
  }
}

void submitIn() {
  esp_err_t err = usb_host_transfer_submit(kbd.in);
  kbd.inBusy = (err == ESP_OK);
  if (err != ESP_OK) {
    LOG("Interrupt transfer failed: %s", esp_err_to_name(err));
  }
}

void onReport(const uint8_t *report) {
  // 0x01 in every key slot means too many keys are pressed (rollover error)
  if (report[2] == 0x01) {
    return;
  }
  for (int i = 2; i < HID_REPORT_SIZE; i++) {
    uint8_t usage = report[i];
    if (usage == 0 || memchr(kbd.prev + 2, usage, HID_REPORT_SIZE - 2)) {
      continue;  // empty slot, or a key that was already down
    }
    if (usage == 0x39 || usage == 0x53) {
      kbd.leds ^= (usage == 0x39) ? LED_CAPS_LOCK : LED_NUM_LOCK;
      updateLeds();
    } else {
      printKey(usage, report[0]);
    }
  }
  memcpy(kbd.prev, report, HID_REPORT_SIZE);
}

// Transfer callbacks run inside usb_host_client_handle_events(), in the client task
void onInTransfer(usb_transfer_t *transfer) {
  kbd.inBusy = false;
  if (transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
    if (!kbd.gone) {
      LOG("Interrupt transfer status %d", transfer->status);
    }
    return;
  }
  if (transfer->actual_num_bytes >= HID_REPORT_SIZE) {
    onReport(transfer->data_buffer);
  }
  submitIn();
}

void onCtrlTransfer(usb_transfer_t *transfer) {
  kbd.ctrlBusy = false;
  if (transfer->status != USB_TRANSFER_STATUS_COMPLETED && !kbd.gone) {
    LOG("Control transfer status %d", transfer->status);
  }
  // The first control transfer is SET_PROTOCOL. After it, start reading reports.
  if (!kbd.inBusy && !kbd.gone) {
    submitIn();
  }
}

void onClientEvent(const usb_host_client_event_msg_t *msg, void *arg) {
  if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
    if (pendingCount < (int)sizeof(pendingAddr)) {
      pendingAddr[pendingCount++] = msg->new_dev.address;
    }
  } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE && msg->dev_gone.dev_hdl == kbd.dev) {
    kbd.gone = true;
  }
}

// Prints a USB string descriptor (UTF-16LE), ASCII characters only
void printString(const usb_str_desc_t *str) {
  if (str == nullptr) {
    Serial.print("?");
    return;
  }
  for (int i = 0; i < (str->bLength - 2) / 2; i++) {
    Serial.print(str->wData[i] < 0x80 ? (char)str->wData[i] : '?');
  }
}

// Finds a boot keyboard interface and its interrupt IN endpoint
bool findKeyboard(const usb_config_desc_t *cfg, uint8_t *intf, uint8_t *ep, uint16_t *mps) {
  const uint8_t *p = (const uint8_t *)cfg;
  const uint8_t *end = p + cfg->wTotalLength;
  bool inKeyboard = false;
  for (; p + 2 <= end && p[0] > 0; p += p[0]) {
    if (p[1] == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
      const usb_intf_desc_t *i = (const usb_intf_desc_t *)p;
      inKeyboard = i->bInterfaceClass == USB_CLASS_HID && i->bInterfaceSubClass == HID_SUBCLASS_BOOT &&
                   i->bInterfaceProtocol == HID_PROTOCOL_KEYBOARD && i->bAlternateSetting == 0;
      *intf = i->bInterfaceNumber;
    } else if (p[1] == USB_B_DESCRIPTOR_TYPE_ENDPOINT && inKeyboard) {
      const usb_ep_desc_t *e = (const usb_ep_desc_t *)p;
      if ((e->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) == USB_BM_ATTRIBUTES_XFER_INT &&
          (e->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK)) {
        *ep = e->bEndpointAddress;
        *mps = USB_EP_DESC_GET_MPS(e);
        return true;
      }
    }
  }
  return false;
}

void openDevice(uint8_t addr) {
  usb_device_handle_t dev;
  if (usb_host_device_open(client, addr, &dev) != ESP_OK) {
    LOG("Can't open USB device %u", addr);
    return;
  }

  const usb_device_desc_t *desc;
  usb_device_info_t info;
  usb_host_get_device_descriptor(dev, &desc);
  usb_host_device_info(dev, &info);
  LOG("USB device %04x:%04x connected (%s speed)", desc->idVendor, desc->idProduct,
      info.speed == USB_SPEED_LOW ? "low" : "full");
  Serial.print("  ");
  printString(info.str_desc_manufacturer);
  Serial.print(" ");
  printString(info.str_desc_product);
  Serial.println();

  const usb_config_desc_t *cfg;
  uint8_t intf, ep;
  uint16_t mps;
  usb_host_get_active_config_descriptor(dev, &cfg);
  if (kbd.dev != nullptr || !findKeyboard(cfg, &intf, &ep, &mps)) {
    Serial.println(kbd.dev ? "  Ignored: a keyboard is already connected" : "  Ignored: not a boot keyboard");
    usb_host_device_close(client, dev);
    return;
  }

  esp_err_t err = usb_host_interface_claim(client, dev, intf, 0);
  if (err != ESP_OK) {
    LOG("Can't claim interface %u: %s", intf, esp_err_to_name(err));
    usb_host_device_close(client, dev);
    return;
  }

  kbd = Keyboard();
  kbd.dev = dev;
  kbd.intf = intf;
  kbd.ep = ep;
  usb_host_transfer_alloc(sizeof(usb_setup_packet_t) + 8, 0, &kbd.ctrl);
  usb_host_transfer_alloc(mps, 0, &kbd.in);

  kbd.ctrl->device_handle = dev;
  kbd.ctrl->bEndpointAddress = 0;
  kbd.ctrl->callback = onCtrlTransfer;

  kbd.in->device_handle = dev;
  kbd.in->bEndpointAddress = ep;
  kbd.in->callback = onInTransfer;
  kbd.in->num_bytes = mps;  // IN transfers must be a multiple of the max packet size

  Serial.printf("Keyboard on interface %u, endpoint 0x%02x. Start typing:\n", intf, ep);

  // Keyboards start in report protocol. Switch to the boot protocol, which always
  // uses the fixed 8-byte report this sketch decodes.
  sendControl(HID_REQ_SET_PROTOCOL, 0, nullptr, 0);
}

void closeKeyboard() {
  usb_host_endpoint_halt(kbd.dev, kbd.ep);
  usb_host_endpoint_flush(kbd.dev, kbd.ep);
  // Let the cancelled transfers finish before freeing them
  for (int i = 0; i < 50 && (kbd.inBusy || kbd.ctrlBusy); i++) {
    usb_host_client_handle_events(client, pdMS_TO_TICKS(10));
  }
  usb_host_endpoint_clear(kbd.dev, kbd.ep);
  usb_host_interface_release(client, kbd.dev, kbd.intf);
  usb_host_transfer_free(kbd.ctrl);
  usb_host_transfer_free(kbd.in);
  usb_host_device_close(client, kbd.dev);
  kbd = Keyboard();
  LOG("Keyboard disconnected");
}

// Accepts every device, with its first configuration
bool enumFilter(const usb_device_desc_t *desc, uint8_t *bConfigurationValue) {
  *bConfigurationValue = 1;
  return true;
}

// Runs the USB Host Library: enumeration, hubs and port events
void usbHostTask(void *arg) {
  while (true) {
    uint32_t flags;
    usb_host_lib_handle_events(portMAX_DELAY, &flags);
    if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      usb_host_device_free_all();
    }
  }
}

// Runs this sketch's client: device events and transfer callbacks
void usbClientTask(void *arg) {
  usb_host_client_config_t config = {};
  config.is_synchronous = false;
  config.max_num_event_msg = 5;
  config.async.client_event_callback = onClientEvent;
  ESP_ERROR_CHECK(usb_host_client_register(&config, &client));

  while (true) {
    usb_host_client_handle_events(client, portMAX_DELAY);
    while (pendingCount > 0) {
      openDevice(pendingAddr[--pendingCount]);
    }
    if (kbd.gone) {
      closeKeyboard();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  LOG("USB keyboard example. Plug a keyboard into the USB-A port.");

  // Switches the internal USB PHY from USB-Serial/JTAG to the OTG controller in host
  // mode, and turns on the port. From here on the ESP-USB port (J4) doesn't work.
  usb_host_config_t config = {};
  config.intr_flags = ESP_INTR_FLAG_LEVEL1;
  // Required since core 3.3.11: its libraries enable the enumeration filter, and without
  // a callback every device is silently rejected (espressif/arduino-esp32#12778)
  config.enum_filter_cb = enumFilter;
  ESP_ERROR_CHECK(usb_host_install(&config));

  xTaskCreate(usbHostTask, "usb_host", 4096, nullptr, 2, nullptr);
  xTaskCreate(usbClientTask, "usb_client", 4096, nullptr, 3, nullptr);
}

void loop() {
  delay(1000);
}
