//------------------------------------------------------------------------------
// intel_82599_eeprom_writer.ino
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2021 homelith
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------

#include <Wire.h>
#include <stdarg.h>

#define SMBUS_ADDR 0x64

bool smbus_ready = false;
bool writer_active = false;
unsigned long prev_tm = 0;
unsigned long words_written = 0;
byte hiword;
bool hiword_en = false;

// helper function for formatted serial output
long serial_printf(const char *format, ...) {
  char buf[128];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  return Serial.print(buf);
}

// issue Release EEPROM command to SMBus
byte release_eeprom() {
  Wire.beginTransmission(SMBUS_ADDR);
  Wire.write(0xc7);
  Wire.write(0x01);
  Wire.write(0xB6);
  return Wire.endTransmission(true);
}

// write EEPROM via SMBus register write command
byte write_eeprom(unsigned long offset, byte hiword, byte loword) {
  byte error;
  byte offset_loword = (byte)(offset & 0x000000ffU);
  byte offset_hiword = (byte)((offset >> 8) & 0x000000ffU);

  Wire.beginTransmission(SMBUS_ADDR);
  Wire.write(0xc8);
  Wire.write(0x07);
  Wire.write(0x01);
  Wire.write(0x01);
  Wire.write(0x14);
  Wire.write(0x00);
  Wire.write(0x00);
  // swap endianness hi <-> lo
  Wire.write(loword);
  Wire.write(hiword);
  error = Wire.endTransmission(true);
  if (error != 0) {
    return error;
  }

  Wire.beginTransmission(SMBUS_ADDR);
  Wire.write(0xc8);
  Wire.write(0x07);
  Wire.write(0x01);
  Wire.write(0x01);
  Wire.write(0x10);
  Wire.write(0x00);
  Wire.write(0x01);
  Wire.write(offset_hiword | 0x80);
  Wire.write(offset_loword);
  error = Wire.endTransmission(true);
  return error;
}

void setup() {
  Serial.begin(9600);
  serial_printf("---- intel 82599 eeprom writer ----\r\n");

  // check if SCL or SDA line tied to GND to prevent Wire.endTransmission from hanging
  serial_printf("check if SCL / SDA line works\r\n", SMBUS_ADDR);
  pinMode(SCL, INPUT);
  pinMode(SDA, INPUT);
  if (digitalRead(SCL) == LOW) {
    serial_printf(" -> SCL line seemed to be tied to GND. check SCL line.\r\n");
    return;
  }
  if (digitalRead(SDA) == LOW) {
    serial_printf(" -> SDA line seemed to be tied to GND. check SDA line.\r\n");
    return;
  }
  serial_printf(" -> OK.\r\n");

  // setup SMBus connection
  Wire.begin();
  Wire.setClock(50000);
  serial_printf("test SMBus addr 0x%02x\r\n", SMBUS_ADDR);
  Wire.beginTransmission(SMBUS_ADDR);
  if (Wire.endTransmission(true) != 0) {
    serial_printf(" -> no response detected from 0x%02x, check SMBUS_ADDR\r\n", SMBUS_ADDR);
    return;
  }
  serial_printf(" -> valid response from 0x%02x\r\n", SMBUS_ADDR);
  serial_printf("writer_ready");
  smbus_ready = true;

  return;
}

void loop() {
  if (! smbus_ready) {
    return;
  }

  // reset writer state after certain time elapsed from last serial input
  unsigned long curr_tm = millis();
  if (writer_active && curr_tm - prev_tm > 1000) {
    serial_printf("writer goes inactive after %u words (in 16bit word) written\r\n", words_written);
    serial_printf("writer_end");
    hiword_en = false;
    writer_active = false;
    words_written = 0;
  }

  // read 1 byte from serial input and dispatch
  int readout = Serial.read();
  if (readout != -1) {
    byte loword = (byte)readout;
    prev_tm = curr_tm;
    if (! writer_active) {
      serial_printf("writer start :\r\n");
      writer_active = true;
      serial_printf("send release EEPROM command (0xc7)\r\n");
      release_eeprom();
      serial_printf("send write EEPROM command (0xc8) continuously\r\n");
    }
    if (hiword_en) {
      write_eeprom(words_written, hiword, loword);
      words_written ++;
      if (words_written % 200 == 0) {
        serial_printf("%u words (in 16bit word) written\r\n", words_written);
      }
      hiword_en = false;
    } else {
      hiword = loword;
      hiword_en = true;
    }
  }
  return;
}
