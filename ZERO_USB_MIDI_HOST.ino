/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2023-2025 rppicomidi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/**
 * This demo program is designed to test the USB MIDI Host driver for a single USB
 * MIDI device connected to the USB Host port or up to 4 devices connected via a
 * USB Hub. It sends to the USB MIDI device(s) the sequence of half-steps from
 * B-flat to D whose note numbers correspond to the transport button LEDs on a Mackie
 * Control compatible control surface. It also prints to a UART serial port console
 * the messages received from each USB MIDI device.
 */
// This file is an adaptation of https://github.com/rppicomidi/EZ_USB_MIDI_HOST/blob/main/examples/arduino/EZ_USB_MIDI_HOST_example/EZ_USB_MIDI_HOST_example.ino for hardware implementation based on RP204 zero by Waveshare https://www.waveshare.com/wiki/RP2040-Zero or equivalent.

#ifndef USE_TINYUSB_HOST
#error "Please Select USB Stack: Adafruit TinyUSB Host"
#else
#warning "All Serial Monitor Output is on Serial1"
#endif
#include "EZ_USB_MIDI_HOST.h"

//#define LED_BUILTIN 25 // not defined for RPi2040_Zero
#include "Adafruit_NeoPixel.h"  // Zero has WS2812 on  GP16
Adafruit_NeoPixel strip(1, 16, NEO_GRB + NEO_KHZ800);

// Create the USB Host driver object
static Adafruit_USBH_Host USBHost;

USING_NAMESPACE_EZ_USB_MIDI_HOST
USING_NAMESPACE_MIDI

// Create the Hardware Serial object DINmidi with the default baud rate but the same interface settings as the USB host port
struct MidiHostSettings : public MidiHostSettingsDefault {
  // If you need to change the settings for either the USB host or serial port, do it
  // in this structure. For example, to change the maximum SysEx message payload size to 256
  //  static const unsigned SysExMaxSize = 256;
  //  static const unsigned MidiRxBufsize = RPPICOMIDI_EZ_USB_MIDI_HOST_GET_BUFSIZE(SysExMaxSize);
  //  static const unsigned MidiTxBufsize = RPPICOMIDI_EZ_USB_MIDI_HOST_GET_BUFSIZE(SysExMaxSize);
};
SerialMIDI<HardwareSerial, DefaultSerialSettings> serialDINmidi(Serial2);
MidiInterface<SerialMIDI<HardwareSerial, DefaultSerialSettings>, MidiHostSettings> DINmidi((SerialMIDI<HardwareSerial, DefaultSerialSettings>&)serialDINmidi);

// Create the USB MIDI Host Driver Object
RPPICOMIDI_EZ_USB_MIDI_HOST_INSTANCE(USBmidi, MidiHostSettings)

static uint8_t midiDevAddr = 0;

// DEFINE MESSAGE ROUTING
// Note that the MidiMessage data type from one MidiInterface type
// is not the same as the MidiMessage data type from another one
static void onUSBMIDIin(const MidiInterface<EZ_USB_MIDI_HOST_Transport<MidiHostSettings>>::MidiMessage& mes)
{
  //printf("usb:%02x %02x %02x %02x\r\n", mes.type, mes.data1, mes.data2, mes.channel);
  DINmidi.send(mes.type, mes.data1, mes.data2, mes.channel);
}

static void onDINMIDIin(const MidiInterface<HardwareSerial>::MidiMessage& mes)
{
  auto intf = USBmidi.getInterfaceFromDeviceAndCable(midiDevAddr, 0);
  if (intf != nullptr) {
    //printf("din:%02x %02x %02x %02x\r\n", mes.type, mes.data1, mes.data2, mes.channel);
    intf->send(mes.type, mes.data1, mes.data2, mes.channel);
  }
}

// CONNECTION MANAGEMENT
static void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables)
{
  if (midiDevAddr != 0) {
    Serial1.println("Device Ignored. This program can only handle one USB MIDI device at a time.\r\n");
  }
  Serial1.printf("MIDI device at address %u has %u IN cables and %u OUT cables\r\n", devAddr, nInCables, nOutCables);
  midiDevAddr = devAddr;
  auto intf = USBmidi.getInterfaceFromDeviceAndCable(midiDevAddr, 0);
  if (intf == nullptr)
    return;
  intf->setHandleMessage(onUSBMIDIin);
  //digitalWrite(LED_BUILTIN, HIGH); //instead use NeoPixel
    strip.setPixelColor(0, strip.Color(0,   128,   128));
    strip.show(); 
}

static void onMIDIdisconnect(uint8_t devAddr)
{
  Serial1.printf("MIDI device at address %u unplugged\r\n", devAddr);
  midiDevAddr = 0;
  //digitalWrite(LED_BUILTIN, LOW); 
        strip.setPixelColor(0, strip.Color(255,   0,   0));
        strip.show();
}

// Program initializations are in this function
void setup() {
  // Make sure the LED is off
  //digitalWrite(LED_BUILTIN, LOW); 
      strip.setPixelColor(0, strip.Color(255,   0,   0));
      strip.show(); 
  //pinMode(LED_BUILTIN, OUTPUT);
  // Enable serial printf port
  Serial1.begin(115200);

  // Enable low level USB Host driver to use the RP2040 native USB port
  USBHost.begin(0);

  // Set up UART1 on pins GP4 and GP5, which are pins 6 & 7 on a Pico board
  Serial2.setTX(4);
  Serial2.setRX(5);
  // Tell DINmidi to route all incoming messages to the onDINMIDIin function
  DINmidi.setHandleMessage(onDINMIDIin);
  // Start the serial port MIDI in device
  DINmidi.begin(MIDI_CHANNEL_OMNI);

  // Initialize USB connection management
  USBmidi.setAppOnConnect(onMIDIconnect);
  USBmidi.setAppOnDisconnect(onMIDIdisconnect);
  while(!Serial1) {}
  delay(1000);
  Serial1.println("EZ_MIDI2USB_HOST to Serial MIDI\r\n");

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50);
}

// Program main loop is here
void loop() {
  // Update USB Host transfers
  USBHost.task();
  // Poll the USB host MIDI
  USBmidi.readAll();
  // Poll the serial port MIDI
  DINmidi.read();
  // Flush any writes after reading the serial port MIDI to USB packets
  USBmidi.writeFlushAll();
}
