// SMART POTHOLE DETECTION SYSTEM
// ESP32 + Ultrasonic + Magnetic + Moisture + GPS
// Wi-Fi + Flask Website Integration

#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ---------- PIN DEFINITIONS ----------

#define TRIG_PIN 12
#define ECHO_PIN 13
#define HALL_PIN 35
#define MOISTURE_PIN 34
#define GPS_RX_PIN 16

// ---------- WIFI CONFIGURATION ----------

const char* ssid = "Easy";
const char* password = "12345qwe";

const char* serverURL =
    "http://10.146.77.146:5000/api/sensor";

const unsigned long WIFI_SEND_INTERVAL = 2000;
const unsigned long WIFI_RETRY_INTERVAL = 10000;

unsigned long lastWiFiSendTime = 0;
unsigned long lastWiFiRetryTime = 0;

// ---------- GPS ----------

TinyGPSPlus gps;

HardwareSerial gpsSerial(2);

struct GPSLocation {
  double latitude;
  double longitude;
};

// Simulated GPS locations for indoor demonstration

GPSLocation locations[4] = {
  {12.971600, 77.594600},
  {12.971700, 77.594700},
  {12.971800, 77.594800},
  {12.971900, 77.594900}
};

int currentLocation = 0;

double latitude = 0;
double longitude = 0;

bool usingSimulatedGPS = true;

// ---------- WHEEL DIMENSIONS ----------

const float wheelDiameter = 3.2;

const float wheelCircumference =
    3.14159 * wheelDiameter;

// ---------- POTHOLE DETECTION ----------

const float normalRoadDistance = 2.0;
const float potholeThreshold = 1.5;

// ---------- MOISTURE DETECTION ----------

const int MOISTURE_THRESHOLD = 1500;

// ---------- MAGNETIC SENSOR THRESHOLDS ----------

const int DETECT_HIGH = 3300;
const int DETECT_LOW = 1100;

const int RELEASE_HIGH = 2700;
const int RELEASE_LOW = 1800;

// ---------- NOISE FILTERING ----------

const int STABLE_SAMPLES = 2;
const unsigned long MIN_INTERVAL = 25;

// ---------- TIMING ----------

const unsigned long SENSOR_INTERVAL = 500;
const unsigned long DISPLAY_INTERVAL = 5000;

// ---------- VARIABLES ----------

unsigned long rotations = 0;

unsigned long lastRotationTime = 0;
unsigned long lastSpeedTime = 0;
unsigned long lastSensorTime = 0;
unsigned long lastDisplayTime = 0;

unsigned long previousRotations = 0;

bool magnetPresent = false;

int detectSamples = 0;
int releaseSamples = 0;

float speed = 0;
float distanceTravelled = 0;

float ultrasonicDistance = -1;
float depth = 0;

int moistureValue = 0;
int magneticValue = 0;

bool isWet = false;
bool potholeDetected = false;
bool previousPotholeDetected = false;

// ---------- WIFI CONNECTION ----------

void connectWiFi() {

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 15000) {

    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\nWiFi Connected!");

    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("\nWiFi Connection Failed!");
  }
}

// ---------- GPS FUNCTION ----------

void updateGPS() {

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Use live GPS only when a recent fix exists

  if (gps.location.isValid() &&
      gps.location.age() < 5000) {

    latitude = gps.location.lat();
    longitude = gps.location.lng();

    usingSimulatedGPS = false;

  } else {

    // Indoor demonstration fallback

    latitude =
        locations[currentLocation].latitude;

    longitude =
        locations[currentLocation].longitude;

    usingSimulatedGPS = true;
  }
}

// ---------- ULTRASONIC FUNCTION ----------

float readUltrasonic() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration =
      pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  float distance =
      duration * 0.0343 / 2.0;

  if (distance < 2 || distance > 400) {
    return -1;
  }

  return distance;
}

// ---------- SEND SENSOR DATA TO FLASK ----------

void sendSensorData() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  HTTPClient http;

  http.begin(serverURL);

  http.setTimeout(1500);

  http.addHeader(
      "Content-Type",
      "application/json"
  );

  String jsonData = "{";

  jsonData += "\"ultrasonic\":";
  jsonData += String(ultrasonicDistance, 2);

  jsonData += ",\"depth\":";
  jsonData += String(depth, 2);

  jsonData += ",\"magnetic\":";
  jsonData += String(magneticValue);

  jsonData += ",\"rotations\":";
  jsonData += String(rotations);

  jsonData += ",\"speed\":";
  jsonData += String(speed, 2);

  jsonData += ",\"moisture\":\"";
  jsonData += (isWet ? "WET" : "DRY");
  jsonData += "\"";

  jsonData += ",\"gps_status\":\"";
  jsonData += (usingSimulatedGPS ? "SIMULATED" : "LIVE");
  jsonData += "\"";

  jsonData += ",\"latitude\":";
  jsonData += String(latitude, 6);

  jsonData += ",\"longitude\":";
  jsonData += String(longitude, 6);

  jsonData += ",\"pothole\":";
  jsonData += (potholeDetected ? "true" : "false");

  jsonData += "}";
  Serial.print("WiFi Status: ");
Serial.println(WiFi.status());

Serial.print("ESP32 IP: ");
Serial.println(WiFi.localIP());

Serial.print("Flask Server: ");
Serial.println(serverURL);

  int responseCode = http.POST(jsonData);
  if (responseCode < 0) {
    Serial.print("HTTP Error: ");
    Serial.println(http.errorToString(responseCode));
}

  Serial.print("Flask HTTP Response: ");
  Serial.println(responseCode);

  if (responseCode != 200) {
    Serial.println("Sensor data transmission failed");
  }

  http.end();
}

// ---------- SETUP ----------

void setup() {

  Serial.begin(115200);

  // Ultrasonic

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Magnetic

  pinMode(HALL_PIN, INPUT);

  // Moisture

  pinMode(MOISTURE_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  analogReadResolution(12);

  // GPS UART

  gpsSerial.begin(
      9600,
      SERIAL_8N1,
      GPS_RX_PIN,
      -1
  );

  lastSpeedTime = millis();
  lastSensorTime = millis();
  lastDisplayTime = millis();

  Serial.println("================================");
  Serial.println("SMART POTHOLE DETECTION SYSTEM");
  Serial.println("ULTRASONIC + MAGNETIC + MOISTURE + GPS");
  Serial.println("================================");

  Serial.println("GPS Module Initialized");
  Serial.println("Waiting for GPS Fix...");

  // Connect ESP32 to Wi-Fi

  connectWiFi();

  lastWiFiSendTime = millis();
  lastWiFiRetryTime = millis();
}

// ---------- MAIN LOOP ----------

void loop() {

  unsigned long currentTime = millis();

  // Update GPS continuously

  updateGPS();

  // ---------- MAGNETIC SENSOR ----------

  magneticValue = analogRead(HALL_PIN);

  bool strongField =
      (magneticValue < DETECT_LOW ||
       magneticValue > DETECT_HIGH);

  bool normalField =
      (magneticValue > RELEASE_LOW &&
       magneticValue < RELEASE_HIGH);

  // ---------- WHEEL ROTATION COUNTING ----------

  if (!magnetPresent) {

    if (strongField) {
      detectSamples++;
    } else {
      detectSamples = 0;
    }

    if (detectSamples >= STABLE_SAMPLES) {

      magnetPresent = true;
      detectSamples = 0;

      if (currentTime - lastRotationTime >= MIN_INTERVAL) {

        rotations++;
        lastRotationTime = currentTime;
      }
    }

  } else {

    if (normalField) {
      releaseSamples++;
    } else {
      releaseSamples = 0;
    }

    if (releaseSamples >= STABLE_SAMPLES) {

      magnetPresent = false;
      releaseSamples = 0;
    }
  }

  // ---------- SPEED CALCULATION ----------

  if (currentTime - lastSpeedTime >= 1000) {

    unsigned long elapsed =
        currentTime - lastSpeedTime;

    unsigned long newRotations =
        rotations - previousRotations;

    speed =
        (newRotations * wheelCircumference) /
        (elapsed / 1000.0);

    previousRotations = rotations;
    lastSpeedTime = currentTime;
  }

  // ---------- SENSOR READINGS EVERY 500 ms ----------

  if (currentTime - lastSensorTime >= SENSOR_INTERVAL) {

    lastSensorTime = currentTime;

    ultrasonicDistance = readUltrasonic();

    moistureValue = analogRead(MOISTURE_PIN);

    isWet =
        (moistureValue > MOISTURE_THRESHOLD);

    distanceTravelled =
        rotations * wheelCircumference;

    // ---------- POTHOLE DETECTION ----------

    if (ultrasonicDistance >= 0) {

   depth = max(0.0f, ultrasonicDistance - normalRoadDistance);

   potholeDetected = (depth > potholeThreshold);

    } else {

      depth = 0;
      potholeDetected = false;
    }

    // ---------- NEW POTHOLE ALERT ----------

    if (ultrasonicDistance >= 0 &&
        potholeDetected &&
        !previousPotholeDetected) {

      Serial.println();
      Serial.println("****************************");
      Serial.println("NEW POTHOLE ALERT!");

      Serial.print("Latitude: ");
      Serial.println(latitude, 6);

      Serial.print("Longitude: ");
      Serial.println(longitude, 6);

      Serial.print("Estimated Depth: ");
      Serial.print(depth, 2);
      Serial.println(" cm");

      Serial.print("Water Status: ");
      Serial.println(isWet ? "WET" : "DRY");

      Serial.print("Location Source: ");
      Serial.println(
          usingSimulatedGPS ? "SIMULATED" : "LIVE GPS"
      );

      Serial.println("****************************");

      // Move to next simulated location

      if (usingSimulatedGPS) {

        currentLocation =
            (currentLocation + 1) % 4;
      }
    }

    // Reset event detection when road is normal

    if (ultrasonicDistance >= 0) {
      previousPotholeDetected = potholeDetected;
    }
  }

  // ---------- DISPLAY EVERY 5 SECONDS ----------

  if (currentTime - lastDisplayTime >= DISPLAY_INTERVAL) {

    lastDisplayTime = currentTime;

    Serial.println();
    Serial.println("------------------------------");
    Serial.println("SMART POTHOLE MONITORING");
    Serial.println("------------------------------");

    // Ultrasonic

    if (ultrasonicDistance < 0) {

      Serial.println("Ultrasonic: No Valid Echo");
      Serial.println("Pothole Status: UNKNOWN");

    } else {

      Serial.print("Ultrasonic Distance: ");
      Serial.print(ultrasonicDistance, 2);
      Serial.println(" cm");

      if (potholeDetected) {

        Serial.println("Pothole Status: POSSIBLE POTHOLE");

        Serial.print("Estimated Depth: ");
        Serial.print(depth, 2);
        Serial.println(" cm");

      } else {

        Serial.println("Pothole Status: NORMAL ROAD");
      }
    }

    // Magnetic

    Serial.print("Magnetic Value: ");
    Serial.println(magneticValue);

    Serial.print("Wheel Rotations: ");
    Serial.println(rotations);

    Serial.print("Distance Travelled: ");
    Serial.print(distanceTravelled, 2);
    Serial.println(" cm");

    Serial.print("Wheel Speed: ");
    Serial.print(speed, 2);
    Serial.println(" cm/s");

    // Moisture

    Serial.print("Moisture Value: ");
    Serial.println(moistureValue);

    Serial.print("Water Status: ");
    Serial.println(isWet ? "WET" : "DRY");

    // GPS

    Serial.println("------ GPS INFORMATION ------");

    Serial.print("GPS Mode: ");
    Serial.println(
        usingSimulatedGPS ? "SIMULATED" : "LIVE"
    );

    Serial.print("Latitude: ");
    Serial.println(latitude, 6);

    Serial.print("Longitude: ");
    Serial.println(longitude, 6);

    if (gps.satellites.isValid()) {

      Serial.print("Satellites: ");
      Serial.println(gps.satellites.value());

    } else {

      Serial.println("Satellites: Not Available");
    }

    // Road condition

    Serial.println("------ ROAD CONDITION ------");

    if (ultrasonicDistance < 0) {

      Serial.println("Road Condition: UNKNOWN");

    } else if (potholeDetected && isWet) {

      Serial.println("POSSIBLE WATER-FILLED POTHOLE!");

    } else if (potholeDetected) {

      Serial.println("POSSIBLE DRY POTHOLE!");

    } else if (isWet) {

      Serial.println("WET ROAD DETECTED!");

    } else {

      Serial.println("NORMAL ROAD");
    }

    Serial.println("------------------------------");
  }

  // ---------- WIFI RECONNECTION ----------

  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastWiFiRetryTime >= WIFI_RETRY_INTERVAL) {

    lastWiFiRetryTime = millis();

    WiFi.reconnect();
  }

  // ---------- SEND DATA TO FLASK EVERY 2 SECONDS ----------

  if (millis() - lastWiFiSendTime >= WIFI_SEND_INTERVAL) {

    lastWiFiSendTime = millis();

    sendSensorData();
  }

  delay(10);
}