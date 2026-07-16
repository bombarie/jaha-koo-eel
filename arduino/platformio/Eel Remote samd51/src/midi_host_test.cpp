/*
 * MIDI USB Host demo — Adafruit Feather M4 Express (SAMD51)
 *
 * The native USB port acts as a USB host; plug in any class-compliant
 * MIDI device (keyboard, controller, …). Because USB is in host mode
 * the USB-CDC Serial is unavailable — connect a USB-UART adapter to
 * the Feather's TX/RX pins (Serial1, 115200 baud) for debug output.
 *
 * Hardware note: the Feather M4's USB connector can source 5 V for
 * bus-powered devices via its built-in boost regulator.
 *
 * Library: Adafruit TinyUSB Library ≥ 3.3.0
 * Build flags required (see platformio.ini):
 *   -DUSE_TINYUSB -DCFG_TUH_ENABLED=1 -DCFG_TUH_MIDI_ENABLED=1
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_TinyUSB.h>

// Hardware UART used for debug — USB is occupied by host mode
#define DBG Serial1

// ── CC event handlers ────────────────────────────────────────────────────────

// Add application logic here; called once per incoming CC message.
void onControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
  DBG.print("CC  ch=");
  DBG.print(channel);
  DBG.print("  cc=");
  DBG.print(cc);
  DBG.print("  val=");
  DBG.println(value);

  // Example: react to specific controllers
  switch (cc) {
    case 1:   // Mod wheel
      DBG.println("  -> mod wheel");
      break;
    case 7:   // Channel volume
      DBG.println("  -> volume");
      break;
    case 11:  // Expression
      DBG.println("  -> expression");
      break;
    case 64:  // Sustain pedal
      DBG.print("  -> sustain ");
      DBG.println(value >= 64 ? "ON" : "OFF");
      break;
    default:
      break;
  }
}

// ── TinyUSB MIDI host callbacks ───────────────────────────────────────────────
// All callbacks use an interface index (idx), not a device address.

// Called when a MIDI device is plugged in.
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* info) {
  DBG.print("MIDI device mounted  idx=");
  DBG.print(idx);
  DBG.print("  addr=");
  DBG.print(info->daddr);
  DBG.print("  cables rx=");
  DBG.print(info->rx_cable_count);
  DBG.print("  tx=");
  DBG.println(info->tx_cable_count);
}

// Called when a MIDI device is unplugged.
void tuh_midi_umount_cb(uint8_t idx) {
  DBG.print("MIDI device unmounted  idx=");
  DBG.println(idx);
}

// Called when one or more MIDI packets have arrived.
// USB-MIDI packets are 4 bytes: [CIN|cable, status, data1, data2]
void tuh_midi_rx_cb(uint8_t idx, uint32_t xferred_bytes) {
  (void)xferred_bytes;
  uint8_t pkt[4];
  while (tuh_midi_packet_read(idx, pkt)) {
    uint8_t status  = pkt[1];
    uint8_t type    = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1;  // 1-based

    switch (type) {
      case 0x80:  // Note Off
        DBG.printf("Note Off  ch=%u  note=%u  vel=%u\n",
                   channel, pkt[2], pkt[3]);
        break;

      case 0x90:  // Note On (vel=0 → note off)
        DBG.printf("Note On   ch=%u  note=%u  vel=%u\n",
                   channel, pkt[2], pkt[3]);
        break;

      case 0xA0:  // Poly Aftertouch
        DBG.printf("Poly AT   ch=%u  note=%u  pres=%u\n",
                   channel, pkt[2], pkt[3]);
        break;

      case 0xB0:  // Control Change
        onControlChange(channel, pkt[2], pkt[3]);
        break;

      case 0xC0:  // Program Change
        DBG.printf("ProgChg   ch=%u  prog=%u\n", channel, pkt[2]);
        break;

      case 0xD0:  // Channel Pressure
        DBG.printf("ChanAT    ch=%u  pres=%u\n", channel, pkt[2]);
        break;

      case 0xE0:  // Pitch Bend (14-bit, LSB first)
        {
          int16_t bend = (int16_t)((pkt[3] << 7) | pkt[2]) - 8192;
          DBG.printf("PitchBend ch=%u  val=%d\n", channel, bend);
        }
        break;

      default:
        break;
    }
  }
}

// ── Arduino entry points ─────────────────────────────────────────────────────

void setup() {
  DBG.begin(115200);
  DBG.println("MIDI host ready — waiting for device...");

  // LED as visual indicator
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  tuh_task();  // drive the TinyUSB host stack

  // Blink the LED while the host stack is running
  static uint32_t lastBlink = 0;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
