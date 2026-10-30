// ==========================================
// SMART ROAD INFRASTRUCTURE MONITORING
// ESP32 + FLASK WEBSITE INTEGRATION
// ==========================================

// ---------- CONFIGURATION ----------

const SENSOR_INTERVAL = 2000;
const DASHBOARD_INTERVAL = 3000;

// Prevent overlapping refresh requests
let sensorRequestRunning = false;
let dashboardRequestRunning = false;


// ==========================================
// POTHOLE SEVERITY CLASSIFICATION
// ==========================================

 
// POTHOLE SEVERITY CLASSIFICATION

function classifySeverity(depth) {

    if (depth > 5) {
        return "Severe";
    }

    else if (depth > 3) {
        return "Moderate";
    }

    else {
        return "Minor";
    }
}


// ==========================================
// LIVE ESP32 SENSOR MONITORING
// ==========================================

async function updateLiveSensors() {

    if (sensorRequestRunning) return;

    sensorRequestRunning = true;

    try {

        const response = await fetch("/api/sensor", {
            cache: "no-store"
        });

        if (!response.ok) {
            throw new Error("Unable to fetch sensor readings");
        }

        const data = await response.json();

        // Ultrasonic distance
        const ultrasonic = Number(data.ultrasonic);

        document.getElementById("ultrasonicValue").textContent =
            Number.isFinite(ultrasonic) && ultrasonic >= 0
                ? ultrasonic.toFixed(2) + " cm"
                : "No Valid Echo";

        // Pothole depth
        const depth = Math.max(0, Number(data.depth) || 0);

        document.getElementById("depthValue").textContent =
            depth.toFixed(2) + " cm";

        // Magnetic sensor
        document.getElementById("magneticValue").textContent =
            data.magnetic ?? "--";

        // Wheel rotations
        document.getElementById("rotationValue").textContent =
            data.rotations ?? 0;

        // Vehicle speed
        const speed = Number(data.speed);

        document.getElementById("speedValue").textContent =
            Number.isFinite(speed)
                ? speed.toFixed(2) + " cm/s"
                : "-- cm/s";

        // Moisture
        document.getElementById("moistureValue").textContent =
            data.moisture ?? "--";

        // GPS status
        document.getElementById("gpsStatus").textContent =
            data.gps_status === "LIVE"
                ? "LIVE GPS"
                : data.gps_status === "SIMULATED"
                    ? "SIMULATED GPS"
                    : "Waiting for GPS";

        // Detection information
        if (data.pothole === true) {

            console.log("POTHOLE DETECTED", {
                severity: classifySeverity(depth),
                depth: depth,
                moisture: data.moisture,
                latitude: data.latitude,
                longitude: data.longitude,
                gps_status: data.gps_status
            });
        }

    } catch (error) {

        console.error("Sensor monitoring error:", error);

        document.getElementById("gpsStatus").textContent =
            "Connection Error";

    } finally {

        sensorRequestRunning = false;
    }
}


// ==========================================
// DASHBOARD STATISTICS
// ==========================================

async function updateDashboard() {

    try {

        const response = await fetch("/api/dashboard", {
            cache: "no-store"
        });

        if (!response.ok) {
            throw new Error("Unable to fetch dashboard statistics");
        }

        const data = await response.json();

        document.getElementById("totalPotholes").textContent =
            data.total_potholes ?? 0;

        document.getElementById("criticalPotholes").textContent =
            data.critical_potholes ?? 0;

        document.getElementById("waterPotholes").textContent =
            data.water_potholes ?? 0;

        document.getElementById("pendingRepairs").textContent =
            data.pending_repairs ?? 0;

        document.getElementById("completedRepairs").textContent =
            data.completed_repairs ?? 0;

        const health = Number(data.road_health);

        document.getElementById("roadHealth").textContent =
            Number.isFinite(health)
                ? health.toFixed(1) + "%"
                : "--";

    } catch (error) {

        console.error("Dashboard update error:", error);
    }
}


// ==========================================
// MAINTENANCE MANAGEMENT
// ==========================================

async function updateMaintenanceTable() {

    const table = document.getElementById("maintenanceTable");

    try {

        const response = await fetch("/api/potholes", {
            cache: "no-store"
        });

        if (!response.ok) {
            throw new Error("Unable to fetch maintenance records");
        }

        const potholes = await response.json();

        table.innerHTML = "";

        if (!Array.isArray(potholes) || potholes.length === 0) {

            const row = document.createElement("tr");
            const cell = document.createElement("td");

            cell.colSpan = 6;
            cell.textContent = "No maintenance tickets available.";

            row.appendChild(cell);
            table.appendChild(row);

            return;
        }

        potholes.forEach(pothole => {

            const row = document.createElement("tr");

            const latitude = Number(pothole.latitude);
            const longitude = Number(pothole.longitude);

            const coordinates =
                Number.isFinite(latitude) &&
                Number.isFinite(longitude)
                    ? `${latitude.toFixed(6)}, ${longitude.toFixed(6)}`
                    : "Unavailable";

            const locationText =
                `${coordinates} (${pothole.gps_status || "Unknown"})`;

            const values = [
                pothole.ticket_id,
                locationText,
                pothole.severity,
                pothole.water_status,
                pothole.maintenance
            ];

            values.forEach(value => {

                const cell = document.createElement("td");

                cell.textContent = value ?? "--";

                row.appendChild(cell);
            });

            // ---------- ACTION BUTTON ----------

            const actionCell = document.createElement("td");

            if (pothole.maintenance === "Resolved") {

                actionCell.textContent = "Completed";

            } else {

                const button = document.createElement("button");

                button.type = "button";
                button.textContent = "Mark as Completed";
                button.className = "complete-btn";

                button.addEventListener("click", () => {
                    markRepairCompleted(pothole.ticket_id);
                });

                actionCell.appendChild(button);
            }

            row.appendChild(actionCell);
            table.appendChild(row);
        });

    } catch (error) {

        console.error("Maintenance table error:", error);

        const row = document.createElement("tr");
        const cell = document.createElement("td");

        cell.colSpan = 6;
        cell.textContent = "Unable to load maintenance records.";

        row.appendChild(cell);
        table.replaceChildren(row);
    }
}


// ==========================================
// MARK REPAIR AS COMPLETED
// ==========================================

async function markRepairCompleted(ticketId) {

    const confirmed = confirm(
        `Mark repair ${ticketId} as completed?`
    );

    if (!confirmed) return;

    try {

        const response = await fetch(
            `/api/maintenance/${encodeURIComponent(ticketId)}`,
            {
                method: "PUT",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    maintenance: "Resolved"
                })
            }
        );

        if (!response.ok) {
            throw new Error("Unable to update maintenance status");
        }

        alert(`${ticketId} marked as completed successfully!`);

        await Promise.all([
            updateDashboard(),
            updateMaintenanceTable(),
            updateAnalytics()
        ]);

    } catch (error) {

        console.error("Repair update error:", error);

        alert("Failed to update repair status.");
    }
}


// ==========================================
// ROAD INFRASTRUCTURE ANALYTICS
// ==========================================

async function updateAnalytics() {

    const analytics = document.getElementById("analyticsContainer");

    try {

        const response = await fetch("/api/potholes", {
            cache: "no-store"
        });

        if (!response.ok) {
            throw new Error("Unable to fetch analytics records");
        }

        const potholes = await response.json();

        if (!Array.isArray(potholes)) {
            throw new Error("Invalid analytics data");
        }

        const minor = potholes.filter(
            p => p.severity === "Minor"
        ).length;

        const moderate = potholes.filter(
            p => p.severity === "Moderate"
        ).length;

        const severe = potholes.filter(
            p => p.severity === "Severe"
        ).length;

        const wet = potholes.filter(
            p => p.water_status === "WET"
        ).length;

        const resolved = potholes.filter(
            p => p.maintenance === "Resolved"
        ).length;

        analytics.innerHTML = `
            <div style="text-align:center;padding:20px;">

                <h3>Road Condition Distribution</h3>

                <p>Minor Potholes: ${minor}</p>

                <p>Moderate Potholes: ${moderate}</p>

                <p>Severe Potholes: ${severe}</p>

                <p>Water-Filled Potholes: ${wet}</p>

                <p>Completed Repairs: ${resolved}</p>

                <br>

                <small>Recorded ESP32 Detection Events</small>

            </div>
        `;

    } catch (error) {

        console.error("Analytics error:", error);

        analytics.textContent = "Unable to load analytics.";
    }
}


// ==========================================
// REFRESH DASHBOARD AND RECORDS
// ==========================================

async function refreshDashboardData() {

    if (dashboardRequestRunning) return;

    dashboardRequestRunning = true;

    try {

        await Promise.all([
            updateDashboard(),
            updateMaintenanceTable(),
            updateAnalytics()
        ]);

    } finally {

        dashboardRequestRunning = false;
    }
}


// ==========================================
// INITIALIZE WEBSITE
// ==========================================

updateLiveSensors();

refreshDashboardData();

// Refresh live sensor readings every 2 seconds
setInterval(updateLiveSensors, SENSOR_INTERVAL);

// Refresh dashboard, maintenance and analytics every 3 seconds
setInterval(refreshDashboardData, DASHBOARD_INTERVAL);

console.log(
    "Smart Road Infrastructure Monitoring Portal Loaded"
);
// RESET DASHBOARD

async function resetDashboard() {

    const confirmed = confirm(
        "Are you sure you want to reset all dashboard values?"
    );

    if (!confirmed) return;

    try {
        const response = await fetch("/api/reset", {
            method: "POST"
        });

        if (!response.ok) {
            throw new Error("Reset failed");
        }

        await refreshDashboardData();

        alert("Dashboard reset successfully!");

    } catch (error) {
        console.error(error);
        alert("Unable to reset dashboard!");
    }
}

// CONNECT RESET BUTTON

document.getElementById("resetDashboardBtn")
    .addEventListener("click", resetDashboard);
