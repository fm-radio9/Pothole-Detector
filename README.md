# Smart Pothole Detection and Road Monitoring System

An ESP32-based prototype that measures surface depressions and sends sensor readings over Wi-Fi to a Flask web dashboard.
## Features

- Ultrasonic distance measurement and estimated depression depth.
- Magnetic sensor-based wheel rotation counting and speed estimation.
- Threshold-based wet or dry indication.
- Live GPS coordinates when a recent fix is available, with clearly labelled simulated coordinates for indoor demonstrations.
- Live sensor dashboard, pothole tickets, severity summaries, and maintenance status updates.

## Hardware

ESP32, ultrasonic sensor, magnetic sensor, moisture sensor, GPS module, wheel with a magnet, suitable power supply, and connecting wires.

| Signal | ESP32 pin |
| --- | --- |
| Ultrasonic trigger | GPIO 12 |
| Ultrasonic echo | GPIO 13 |
| Magnetic sensor analog output | GPIO 35 |
| Moisture sensor analog output | GPIO 34 |
| GPS TX to ESP32 RX | GPIO 16 |

Check power and signal-voltage compatibility for the actual modules before wiring. The GPS serial interface uses 9600 baud.

## Project layout

Keep `app.py`, `index.html`, `style.css`, and `script.js` together in the repository root.

```text
app.py
index.html
style.css
script.js
README.md
requirements.txt
.gitignore
esp32_final_code/esp32_final_code.ino
static/images/
static/videos/
```

Create the media folders when adding your images and videos. Keep the Arduino sketch inside a folder named `esp32_final_code`.

## Run the website on Windows

Install Python first. Download and extract the repository, or clone it with Git:

```bat
git clone https://github.com/fm-radio9/Smart_Pothole_Detection_and_Monitoring_System.git
cd Smart_Pothole_Detection_and_Monitoring_System
```

Open **Command Prompt** in the project folder and run:

```bat
py -m venv .venv
.venv\Scripts\activate
python -m pip install -r requirements.txt
python app.py
```

Open **http://localhost:5001** in your browser. Keep the terminal running; press `Ctrl+C` to stop the server. Open the dashboard through Flask, rather than opening `index.html` directly.

For later sessions:

```bat
.venv\Scripts\activate
python app.py
```

The dashboard opens without the ESP32, but live readings require sensor data from the board.

## Configure and upload the ESP32 firmware

1. Install Arduino IDE and the **esp32 by Espressif Systems** board package.
2. Install **TinyGPSPlus** through Library Manager. `WiFi` and `HTTPClient` are supplied by ESP32 board support.
3. Open `esp32_final_code/esp32_final_code.ino`.
4. Select your ESP32 board and COM port.
5. Connect the laptop and ESP32 to the same Wi-Fi network.
6. Run `ipconfig` in Command Prompt and find the IPv4 address of the laptop's active Wi-Fi adapter.
7. Set these values in the sketch, using your own network details and laptop address:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverURL = "http://192.168.1.10:5001/api/sensor";
```

The address above is an example. Use the **laptop's IP**, not the ESP32 IP or `localhost`.

8. Start `python app.py`, then upload the sketch.
9. Open Serial Monitor at **115200 baud**. Successful transmission prints `Flask HTTP Response: 200`.

If prompted by Windows Firewall, allow Python on your trusted private network. Update the server URL if the laptop's IP changes. Networks with client isolation can prevent devices from communicating.

**Before committing the sketch, replace real Wi-Fi credentials with placeholders.** The `.gitignore` does not hide credentials written inside a tracked `.ino` file.

## How it works

1. The ESP32 reads sensors and estimates depth relative to a configured reference distance.
2. It sends JSON to `POST /api/sensor` approximately every two seconds.
3. Flask updates the latest readings and creates a ticket when the received detection state changes from false to true.
4. The browser fetches live readings and summary information.
5. The maintenance table allows a repair to be marked completed.

The current reference distance is 2 cm, and detection requires estimated depth greater than 0.5 cm. Calibrate these settings for the actual mounting. The configured wheel diameter is 3.2 cm, so calculated speed is in cm/s.

## Troubleshooting

| Problem | Check |
| --- | --- |
| `No module named flask` | Activate `.venv` and run `python -m pip install -r requirements.txt`. |
| Dashboard cannot open | Keep `python app.py` running and use port `5001`. |
| ESP32 cannot send data | Check Wi-Fi, laptop IPv4 address, firewall, and network isolation. |
| GPS shows `SIMULATED` | The firmware has no recent valid GPS fix. |
| Records disappear | The current implementation uses in-memory storage. |
| Arduino reports missing ESP32 platform | Install ESP32 board support and reselect the board from the installed package. |
