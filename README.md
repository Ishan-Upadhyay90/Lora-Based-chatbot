# Lora-Based-chatbot
This project demonstrates a private chatbot communication system using LoRa transceivers (REYAX RYLR896) connected to an ESP32 and an ESP8266.
Each device hosts its own Wi-Fi Access Point with a modern web-based chat UI, allowing real-time text exchange between the two boards over a LoRa link — without the need for internet or an external router.

#Features
1. Private communication line over LoRa (868 MHz band)
2. ESP32 and ESP8266 each create their own Wi-Fi hotspot
3. Web-based chat interface (responsive, modern design)
4. Peer-to-peer messaging between ESP32 and ESP8266
5. Low-power, long-range wireless communication
6. Works completely offline

#Hardware Requirements
1 × ESP32 development board
1 × ESP8266 development board
2 × REYAX RYLR896 LoRa transceiver modules
Jumper wires and breadboard (or PCB)

#Circuit Connections
*ESP32 ↔ RYLR896
ESP32 Pin	  - LoRa Pin
GPIO16 (RX2)-	TX
GPIO17 (TX2)-	RX
3.3V	      - VCC
GND	        - GND
*ESP8266 ↔ RYLR896
ESP8266 Pin	- LoRa Pin
D7 (GPIO13)	- TX
D8 (GPIO15)	- RX
3.3V	      - VCC
GND	        - GND

Note: The RYLR896 operates only at 3.3V logic. Do not connect 5V.
Software Setup

Install Arduino IDE
.

Add support for ESP32 and ESP8266 boards via Boards Manager.

Install the following libraries:

WiFi.h (built-in for ESP32)

ESP8266WiFi.h (built-in for ESP8266)

WebServer.h (ESP32)

ESPAsyncWebServer.h (ESP8266)

WebSocketsServer.h

SoftwareSerial.h (for ESP8266 → LoRa)

Open esp32_chat.ino, select ESP32 Dev Module, and upload.

Open esp8266_chat.ino, select NodeMCU/ESP8266, and upload.

How It Works

Power up ESP32 and ESP8266 with their LoRa modules connected.

Each board creates its own Wi-Fi AP:

ESP32 → ESP32_Chat (password: 12345678)

ESP8266 → ESP8266_Chat (password: 12345678)

Connect a phone or laptop to one of them and open:

ESP32: http://192.168.4.1

ESP8266: http://192.168.4.1

Type a message in the chat interface.

The message is sent over WebSocket, forwarded through LoRa, and displayed on the peer device’s chat window.

Example Serial Output
ESP32 Serial Monitor:
Waiting for LoRa module to respond...
✅ LoRa ready
📥 Hello from ESP8266

Future Improvements

Add AES encryption for secure communication

Implement mesh networking (multi-node LoRa chat)

Create a mobile app client for smoother UI

Extend with voice or AI chatbot integration
