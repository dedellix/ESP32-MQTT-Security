# ESP32 sensing and MQTT lab

Two ESP32 boards, each with a DHT11 sensor, a push button and two LEDs, exchange data
through an MQTT broker running on a laptop. A Node-RED dashboard displays the data, and
a humidity alarm on board 1 triggers an LED pattern on board 2.

> **Security notice:** this version is a functional baseline and is **not secure**.
> MQTT is unencrypted and the broker accepts anonymous clients. Use it only on a
> private, trusted network (for example your own phone hotspot).

## What it does
- A push button toggles an LED
- The DHT11 temperature and humidity are published over MQTT every 5 seconds
- A Node-RED dashboard shows live charts
- Humidity alarm: when board 1 reads humidity above a threshold, board 2 blinks
  red and white alternately until its button is pressed

## Architecture
```
ESP32 #1 (publisher)  ──┐
                        ├── WiFi ──► Mosquitto broker ──► Node-RED dashboard
ESP32 #2 (subscriber) ──┘            (laptop, port 1883)
```

## Hardware (per board)
| Part | Quantity |
|---|---|
| ESP32 DevKit V4 (ESP-WROOM-32) | 1 |
| DHT11 temperature and humidity sensor (bare 4-pin) | 1 |
| Push button | 1 |
| LED, white and red | 1 each |
| Resistors: about 230 ohm (LEDs, button), about 5 kohm (DHT11 data line) | 4 |
| Breadboard, jumper wires, data-capable micro-USB cable | 1 set |

| Function | GPIO |
|---|---|
| DHT11 DATA (5 kohm pull-up to 3V3) | 26 |
| Push button (external pull-down: pressed = HIGH) | 27 |
| White LED | 33 |
| Red LED | 32 |

## 1. Arduino IDE and ESP32 setup

**Requirements**
- Arduino IDE [version]
- Board package: **esp32 by Espressif Systems** [version]
- Libraries (Sketch > Include Library > Manage Libraries):
  - **DHT sensor library** by Adafruit [version]
  - **Adafruit Unified Sensor** [version]
  - **PubSubClient** by Nick O'Leary [version]

**Steps**
1. In *File > Preferences*, add this URL under *Additional Boards Manager URLs*:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Open *Tools > Board > Boards Manager*, search **esp32**, and install
   **esp32 by Espressif Systems**.
3. Select *Tools > Board > esp32 > ESP32 Dev Module*.
4. Install the three libraries listed above.
5. Plug in the board with a **data** USB cable and pick it in *Tools > Port*.
   If no port appears, install the USB-serial driver (CP210x or CH340).
6. Set the Serial Monitor to **115200 baud**.

## 2. Configure WiFi and broker address (before uploading)

Each sketch contains placeholders that you **must replace** with your own values:

```cpp
const char* WIFI_SSID   = "YOUR_WIFI_NAME";
const char* WIFI_PASS   = "YOUR_WIFI_PASSWORD";
const char* MQTT_SERVER = "YOUR_LAPTOP_IP";
const char* DEVICE_ID   = "esp32_1";   // use "esp32_2" on the second board
```

- **WiFi:** it must be a **2.4 GHz** network. The ESP32 does not support 5 GHz.
  A phone hotspot works well. Enterprise networks such as eduroam do not work
  (enterprise login and device isolation).
- **Laptop on the same network:** the laptop running the broker must be connected
  to the **same WiFi** as the boards.
- **MQTT server IP:** this is your laptop's IPv4 address on that WiFi.
  Find it with `ipconfig` (Windows, look at the Wi-Fi adapter) or `ip a` / `ifconfig`
  (Linux, macOS).
- **The IP changes!** Hotspots often assign a new address after a reconnection.
  If the boards stop connecting (Serial Monitor shows `rc=-2`), run `ipconfig` again,
  update `MQTT_SERVER` in every sketch, and re-upload.
- Change the WiFi name or password whenever you switch network, and re-upload.
- `DEVICE_ID` must be unique per board. It is also used as the MQTT client ID.

## 3. MQTT broker (Mosquitto)

**Requirements:** Mosquitto [version, tested with 2.1.2]
(https://mosquitto.org/download/).
On Linux: `sudo apt install mosquitto mosquitto-clients`.
On macOS: `brew install mosquitto`.

**Configuration file** (`mosquitto.conf`):
```
listener 1883
allow_anonymous true
```
- `listener 1883` accepts connections from other devices (Mosquitto 2.x otherwise
  accepts only local connections).
- `allow_anonymous true` is **for the lab only** (no authentication).

**Start the broker** (keep the terminal open; `-v` logs every message):

Windows PowerShell:
```powershell
& "C:\Program Files\mosquitto\mosquitto.exe" -c mosquitto.conf -v
```
Windows Command Prompt, Linux, macOS:
```
mosquitto -c mosquitto.conf -v
```
Stop it with **Ctrl+C**.

**Windows notes**
- If port 1883 is already in use, the Mosquitto Windows service may be running:
  `net stop mosquitto` (administrator).
- Check what listens on the port: `netstat -ano | findstr :1883`

**Test the broker**
```powershell
# Terminal A: subscribe to everything from the boards
& "C:\Program Files\mosquitto\mosquitto_sub.exe" -h <LAPTOP_IP> -t "esp32/#" -v

# Terminal B: publish a test message
& "C:\Program Files\mosquitto\mosquitto_pub.exe" -h <LAPTOP_IP> -t test -m hello
```

## 4. Node-RED dashboard

**Requirements**
- Node.js (LTS) [version] and npm [version]: https://nodejs.org
  Check with `node -v` and `npm -v`
- Node-RED [version]
- node-red-dashboard [version]

**Install and start Node-RED**
```
npm install -g node-red
node-red
```
The editor opens at http://127.0.0.1:1880 and the dashboard at
http://127.0.0.1:1880/ui. Stop it with **Ctrl+C**.
If PowerShell blocks scripts, run once:
`Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`

**Install the dashboard nodes**
In the editor: menu (top right) > *Manage palette* > *Install* tab, search
**node-red-dashboard**, click *Install*.
(Alternative from a terminal: `cd ~/.node-red` then `npm install node-red-dashboard`,
then restart Node-RED.) The `chart` node only appears after this is installed.

**Import the flow**
1. Menu > *Import* > select `node-red/[flow-file-name].json`.
2. Click **Import**, then **Deploy**.
3. The MQTT broker node in the flow points to `127.0.0.1:1883`. Edit it if your
   broker runs on another machine.
4. Open http://127.0.0.1:1880/ui to see the charts, the threshold slider and the alarm state.

## 5. Run order
1. Connect the laptop to the WiFi (same network as the boards)
2. Start the broker
3. Start Node-RED
4. Power or reset the ESP32 boards (**EN** button)
5. Check the data with `mosquitto_sub`, then the dashboard

To shut down, press Ctrl+C in Node-RED, then in the subscribers, then in the broker.

## MQTT topics
| Topic | Direction | Payload |
|---|---|---|
| `esp32/<id>/temperature` | board to server | temperature in °C, for example `21.3` |
| `esp32/<id>/humidity` | board to server | relative humidity in %, for example `50.0` |
| `esp32/esp32_2/alarm` | server to board 2 | `1` alarm on, `0` alarm off |
| `esp32/esp32_2/ack` | board 2 to server | `1` when its button muted the alarm |

## Troubleshooting
| Symptom | Likely cause and fix |
|---|---|
| Port not found in the Arduino IDE | Charge-only cable, or missing CP210x/CH340 driver |
| `DHT read failed` | Sensor not fully seated, or the 5 kohm pull-up is missing |
| WiFi never connects | Wrong SSID/password, or a 5 GHz or enterprise network |
| MQTT `rc=-2` | Wrong `MQTT_SERVER` IP, broker not running, laptop on another network, or the firewall blocks port 1883 |
| Nothing on the dashboard | Flow not deployed, or no `chart` node (install node-red-dashboard) |

## Known limitations (security)
- MQTT traffic is plaintext and the broker allows anonymous access: anyone on the
  network can read the data and forge readings or alarm commands.
- WiFi credentials are stored in the firmware in plain text.
- The Node-RED editor has no login by default.

