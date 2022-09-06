# LoRa Tally
A tally system compatible with TSL UMD V5, ATEM, E1.31, MQTT &amp; LoRa

The basic operation mode of this system is providing a bidirectional relay between an IP based communication and the wireless LoRa standard. Every node in the system can be both: connected to an IP network via Ethernet or WiFi and/or LoRa. The proprietary protocol is capable of controlling up to 32 tally lights, each with two seperately addressable LEDS (RHS/LHS via TSL UMD).

## Supported Protocols

- TSL UMD V5: If a TSL UMD TCP port is configured tally states (off, PGM, PVW, REC) and brightness of up to 32 channels with different RH / LH light states are forwareded via LoRa. Text messages are only displayed on the ip connected system.
- ATEM (Tally protocol by Blackmagic Design): If a ATEM host is configured tally states (off, PGW, PVW) of up to 32 channels received via network are forwarded via LoRa.
- E1.31 (DMX via IP): Each tally light is addressed via its DMX channel. Every 3 channels make one tally: 
  - ch1: red tally 1, ch2: green tally 1, ch3: blue tally 1 
  - ch4: red tally 2, ch5: green tally 2, ch6: blue tally 2
  - ...
- E1.31: Each tally can be addressed separately via the DMX universe field:
  - universe 1: ch1: red tally 1, ch2: green tally 1, ch3: blue tally 1
  - universe 2: ch1: red tally 2, ch2: green tally 2, ch3: blue tally 2
  - ...
- MQTT: If a MQTT host is configured each system subscribes to a topic like tally/control/<tally id> (tally_id is always the one configured on the network connected device) and waits for a JSON object with these values:
  ```
  {
    "addr": 1,
    "state": 2,
    "brightness": 3
  } 
  ```
  alternatively you can set a light via RGB values:
  ```
  {
    "addr": 1,
    "r": 255,
    "g": 0,
    "b": 0
  } 
  ```
- MQTT: Status feedback of all LoRa nodes is reported back via MQTT topic tally/status/<tally id> (tally_id is always the one configured on the network connected device)
  ```
  {
    "address": 1,
    "uptime": 6340,
    "version": 3,
    "voltage": 3465,
    "msgCount": 120,
    "MAC": "65:43:21:12:34:56",
    "RSSI": 42
  } 
  ```
  
## Supported Hardware

This project is based on ESP32 boards in two different flavours:

- https://heltec.org/project/wifi-lora-32/ this board is intended as a tally light primarily connected via LoRa. Any number of WS2812 (NeoPixel) LEDs might be connected to PIN23 (PixelPin defined in main.h). Usually you connect two of them where the first one is RHS and the second one is LHS.
- https://www.olimex.com/Products/IoT/ESP32/ESP32-POE-ISO/open-source-hardware this board is intended to be used as a gateway to ethernet with PoE.
 
![Tally Device 1](https://github.com/baycom/loratally/raw/main/doc/tally-device1.jpg)
![Tally Device 2](https://github.com/baycom/loratally/raw/main/doc/tally-device2.jpg)
![Tally Device 3](https://github.com/baycom/loratally/raw/main/doc/tally-device3.jpg)
![Tally Device 4](https://github.com/baycom/loratally/raw/main/doc/tally-device4.jpg)
![Tally Device 5](https://github.com/baycom/loratally/raw/main/doc/tally-device5.jpg)

## Web Frontend & Configuration

The web frontend shows the tally state and some other data by default:

![Default Page](https://github.com/baycom/loratally/raw/main/doc/web-tally.png)

The initial setup is done via web browser by connecting to the WiFi network like Tally-123456 by entering the address 192.168.4.1

![Settings Page](https://github.com/baycom/loratally/raw/7c992fd1e20de098aaf00f8e401f0913732140b3/doc/web-setup.png)


## Button Control

On all ESP32 boards there are two buttons:
- EN/Reset: This button is being used to power the LoRa tally device on
- GPIOX: The function of this button depends on the length it is being pressed:
  - less than 1 second: Show version, tally id, ip address and status 
  - 1.5s: Power off
  - more than 10s: Factory reset
  
