from flask import Flask, jsonify, request, send_from_directory

app = Flask(__name__)

# Store latest ESP32 sensor readings
sensor_data = {
    "ultrasonic": 0,
    "depth": 0,
    "magnetic": 0,
    "rotations": 0,
    "speed": 0,
    "moisture": "DRY",
    "gps_status": "Waiting",
    "latitude": 0,
    "longitude": 0,
    "pothole": False
}

# Serve our existing website
@app.route("/")
def home():
    return send_from_directory(".", "index.html")

@app.route("/<path:filename>")
def static_files(filename):
    if filename not in ("style.css", "script.js"):
        return "Not found", 404
    return send_from_directory(".", filename)

# Receive readings from ESP32
@app.route("/api/sensor", methods=["POST"])
def receive_sensor():
    data = request.get_json(silent=True)

    if not isinstance(data, dict):
        return jsonify({"error": "Invalid JSON"}), 400

    for key in sensor_data:
        if key in data:
            sensor_data[key] = data[key]

    return jsonify({"message": "Sensor data received"})

# Send readings to website
@app.route("/api/sensor", methods=["GET"])
def get_sensor():
    return jsonify(sensor_data)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)