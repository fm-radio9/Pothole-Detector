// SMART POTHOLE DETECTION SYSTEM
// ESP32 + Ultrasonic + Magnetic + Moisture + GPS

#include <TinyGPSPlus.h>

// ---------- PIN DEFINITIONS ----------

#define TRIG_PIN 12
#define ECHO_PIN 13
#define HALL_PIN 25
#define MOISTURE_PIN 34
#define GPS_RX_PIN 16

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

const float normalRoadDistance = 5.0;
const float potholeThreshold = 3.0;

// ---------- MOISTURE DETECTION ----------

const int MOISTURE_THRESHOLD = 1000;

// ---------- MAGNETIC SENSOR THRESHOLDS ----------

const int DETECT_HIGH = 3300;
const int DETECT_LOW = 1100;

const int RELEASE_HIGH = 2700;
const int RELEASE_LOW = 1800;

// ---------- NOISE FILTERING ----------

const int STABLE_SAMPLES = 5;
const unsigned long MIN_INTERVAL = 300;

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

// ---------- GPS FUNCTION ----------

void updateGPS() {

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid() &&
      gps.location.age() < 5000) {

    latitude = gps.location.lat();
    longitude = gps.location.lng();

    usingSimulatedGPS = false;

  } else {

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

// ---------- SETUP ----------

void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(HALL_PIN, INPUT);
  pinMode(MOISTURE_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  analogReadResolution(12);

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
        (moistureValue < MOISTURE_THRESHOLD);

    distanceTravelled =
        rotations * wheelCircumference;

    // ---------- POTHOLE DETECTION ----------

    if (ultrasonicDistance >= 0) {

      depth =
          ultrasonicDistance - normalRoadDistance;

      potholeDetected =
          (depth > potholeThreshold);

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

      if (usingSimulatedGPS) {

        currentLocation =
            (currentLocation + 1) % 4;
      }
    }

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

  delay(10);
}