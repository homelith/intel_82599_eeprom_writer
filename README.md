# intel\_82599\_eeprom\_writer

## What is this ?

- Intel 82599 EB/ES/EN 10 Gigabit Ethernet Controller have an EEPROM recovery mechanism using SMBus, even if they lose connection via PCIe due to faulty EEPROM config.
- This toolkit includes a SMBus writer for Seeeduino v4.2 and speed-controlled EEPROM image sender script for host computer, useful for Intel 82599 EEPROM recovery via SMBus.

- *CAUTION* : Modifying any part of Intel 82599 EEPROM might cause malfunction and make your NIC completely inoperable. The author does not take any responsibility for loss of data or functionality of your equipment. Try it with your own risk.

## Licenses

- Codes are licensed under MIT license, see header of individual files.

## Prerequisites

- Intel 82599 chip with properly powered and its state kept un-initialized (D0u state)
  + Typical situation of this can be reproduced with VCC supply, PERST# de-asserted, and no PCIe connection.
  + For detailed condition to enter D0a state, see section 5.2.5.2.1 in 82599-datasheet-v3-4.pdf.
  + You'll need SMBus address of Intel 82599 if it has changed from default configuration.
    * Default value used in 82599 EEPROM-less mode is 0x64.

- Seeeduino v4.2 with its I/O voltage configured 3.3V
  + Arduino UNO R3 combined with i2c voltage translator can be alternate.
  + i2c SCL / SDA ports connected to Intel 82599 SMBus SCL / SDA ports respectively.
  + USB-serial connection with host computer.

- host computer for EEPROM image feeder
  + python3 and pyserial ($ sudo pip3 install pyserial) installed.
  + Seeeduino USB-serial connection recognized as /dev/ttyACM0 or something.

- EEPROM image to be written
  + They say you can obtain latest EEPROM image via Intel support. (see 82599-specupdate-rev3-9.pdf)
  + For backing up purpose, you can download it from working Intel 82599 via PCIe connection. Try `$ sudo ethtool -e {recognized network interface name}`.

## Usage

- Compile and download `intel_82599_eeprom_writer/intel_82599_eeprom_writer.ino` to Seeeduino v4.2
  + You can see SMBus port initialize sequence by connecting Seeeduino via generic serial console (9600bps, 8bit data, no parity, stop bit width 1, no flow control).
  + You will see `writer_ready` when writer program successfully initialized.

- Run `$ python3 rate_limited_sender.py {port_name (e.g. /dev/ttyACM0)} {image file to send} ( {target_bps (default: 300)} )` and wait until complete.

## Reference

- [Intel(R) 82599 10 Gigabit Ethernet Controller Technical Library](https://www.intel.com/content/www/us/en/design/products-and-solutions/networking-and-io/82599-10-gigabit-ethernet-controller/technical-library.html?grouping=EMT_Content%20Type&sort=title:asc)

