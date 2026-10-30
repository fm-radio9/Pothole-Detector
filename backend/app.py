from flask import Flask, jsonify, request, send_from_directory
from datetime import datetime
from pathlib import Path
from threading import Lock

app = Flask(__name__)

BASE_DIR = Path(__file__).resolve().parent
data_lock = Lock()

# ------------------------------------------
# LATEST ESP32 SENSOR DATA
# ------------------------------------------

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
    "pothole": False,
    "last_updated": None
}

# ------------------------------------------
# POTHOLE RECORDS
# ------------------------------------------

pothole_records = []

next_ticket_id = 1

previous_pothole = False

# ------------------------------------------
# POTHOLE SEVERITY
# ------------------------------------------

# ------------------------------------------
# POTHOLE SEVERITY
# ------------------------------------------

def classify_severity(depth):

    if depth > 5:
        return "Severe"

    elif depth > 3:
        return "Moderate"

    elif depth > 1:
        return "Minor"

    


# ------------------------------------------
# SERVE WEBSITE
# ------------------------------------------

@app.route("/")
def home():
    return send_from_directory(BASE_DIR, "index.html")


@app.route("/<path:filename>")
def static_files(filename):

    if filename not in ("style.css", "script.js"):
        return "Not found", 404

    return send_from_directory(BASE_DIR, filename)


# ------------------------------------------
# RECEIVE SENSOR DATA FROM ESP32
# ------------------------------------------

@app.route("/api/sensor", methods=["POST"])
def receive_sensor():

    global next_ticket_id
    global previous_pothole

    data = request.get_json(silent=True)

    if not isinstance(data, dict):
        return jsonify({"error": "Invalid JSON"}), 400

    with data_lock:

        # Update latest sensor readings
        for key in sensor_data:
            if key in data and key != "last_updated":
                sensor_data[key] = data[key]

        sensor_data["last_updated"] = datetime.now().isoformat(
            timespec="seconds"
        )

        pothole_detected = sensor_data["pothole"] is True

        # Create ticket only for a new pothole event
        if pothole_detected and not previous_pothole:

            depth = max(0, float(sensor_data["depth"]))

            severity = classify_severity(depth)

            ticket = {
                "ticket_id": f"PH-{next_ticket_id:03d}",
                "latitude": sensor_data["latitude"],
                "longitude": sensor_data["longitude"],
                "gps_status": sensor_data["gps_status"],
                "depth": depth,
                "severity": severity,
                "water_status": sensor_data["moisture"],
                "maintenance": "Pending",
                "detected_at": datetime.now().isoformat(
                    timespec="seconds"
                )
            }

            pothole_records.append(ticket)

            next_ticket_id += 1

            print(
                f"New pothole detected: "
                f"{ticket['ticket_id']} | "
                f"{severity} | "
                f"{depth:.2f} cm"
            )

        previous_pothole = pothole_detected

    return jsonify({
        "message": "Sensor data received",
        "pothole_detected": pothole_detected
    })


# ------------------------------------------
# GET LIVE SENSOR DATA
# ------------------------------------------

@app.route("/api/sensor", methods=["GET"])
def get_sensor():

    with data_lock:
        return jsonify(sensor_data.copy())


# ------------------------------------------
# GET POTHOLE RECORDS
# ------------------------------------------

@app.route("/api/potholes", methods=["GET"])
def get_potholes():

    with data_lock:

        for record in pothole_records:
            record["severity"] = classify_severity(
                float(record["depth"])
            )

        return jsonify([
            record.copy() for record in pothole_records
        ])

    with data_lock:
        return jsonify([
            record.copy() for record in pothole_records
        ])


# ------------------------------------------
# DASHBOARD STATISTICS
# ------------------------------------------

@app.route("/api/dashboard", methods=["GET"])
def get_dashboard():

    with data_lock:

        total = len(pothole_records)

        critical = sum(
            1 for p in pothole_records
            if p["severity"] == "Severe"
        )

        water_filled = sum(
            1 for p in pothole_records
            if p["water_status"] == "WET"
        )

        pending = sum(
            1 for p in pothole_records
            if p["maintenance"] in ("Pending", "In Progress")
        )

        completed = sum(
            1 for p in pothole_records
            if p["maintenance"] == "Resolved"
        )

        # Prototype road health indicator
        if total > 0:

            average_depth = sum(
                p["depth"] for p in pothole_records
            ) / total

            road_health = max(
                0,
                100 - average_depth * 10
            )

        else:
            road_health = 100

        return jsonify({
            "total_potholes": total,
            "critical_potholes": critical,
            "water_potholes": water_filled,
            "pending_repairs": pending,
            "completed_repairs": completed,
            "road_health": round(road_health, 1)
        })


# ------------------------------------------
# UPDATE MAINTENANCE STATUS
# ------------------------------------------

@app.route("/api/maintenance/<ticket_id>", methods=["PUT"])
def update_maintenance(ticket_id):

    data = request.get_json(silent=True)

    if not isinstance(data, dict):
        return jsonify({
            "error": "Invalid JSON"
        }), 400

    new_status = data.get("maintenance")

    allowed_statuses = (
        "Pending",
        "In Progress",
        "Resolved"
    )

    if new_status not in allowed_statuses:
        return jsonify({
            "error": "Invalid maintenance status"
        }), 400

    with data_lock:

        for record in pothole_records:

            if record["ticket_id"] == ticket_id:

                record["maintenance"] = new_status

                return jsonify({
                    "message": "Maintenance status updated",
                    "ticket": record.copy()
                })

    return jsonify({
        "error": "Ticket not found"
    }), 404


# ------------------------------------------
# RESET DASHBOARD
# ------------------------------------------

@app.route("/api/reset", methods=["POST"])
def reset_dashboard():

    global next_ticket_id
    global previous_pothole

    with data_lock:

        # Clear all pothole records
        pothole_records.clear()

        # Reset ticket numbering
        next_ticket_id = 1

        # Preserve current pothole detection state
        # to prevent duplicate tickets after reset
        previous_pothole = sensor_data["pothole"] is True

    print("Dashboard reset successfully!")

    return jsonify({
        "message": "Dashboard reset successfully",
        "total_potholes": 0,
        "critical_potholes": 0,
        "water_potholes": 0,
        "pending_repairs": 0,
        "completed_repairs": 0,
        "road_health": 100
    })


# ------------------------------------------
# START FLASK SERVER
# ------------------------------------------

if __name__ == "__main__":

    print("Smart Road Monitoring Backend Started")

    app.run(
        host="0.0.0.0",
        port=5001,
        debug=False
    )
