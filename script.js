// ==========================================
// SMART ROAD INFRASTRUCTURE MONITORING
// CUSTOM ROAD MAP + SEVERITY HEAT MAP
// ==========================================

// SIMULATED LOCATIONS

const locations = [
    {
        id: 1,
        name: "Zone 1",
        lat: 12.971600,
        lng: 77.594600,
        x: 20,
        y: 30,
        depth: 0,
        wet: false,
        status: "Normal",
        maintenance: "Not Required"
    },
    {
        id: 2,
        name: "Zone 2",
        lat: 12.971700,
        lng: 77.594700,
        x: 45,
        y: 30,
        depth: 2.5,
        wet: false,
        status: "Minor",
        maintenance: "Pending"
    },
    {
        id: 3,
        name: "Zone 3",
        lat: 12.971800,
        lng: 77.594800,
        x: 70,
        y: 55,
        depth: 5.5,
        wet: true,
        status: "Severe",
        maintenance: "Pending"
    },
    {
        id: 4,
        name: "Zone 4",
        lat: 12.971900,
        lng: 77.594900,
        x: 45,
        y: 75,
        depth: 4.0,
        wet: false,
        status: "Moderate",
        maintenance: "In Progress"
    }
];

// SEVERITY COLOURS

function getColor(status) {
    if (status === "Normal") return "#16a34a";
    if (status === "Minor") return "#eab308";
    if (status === "Moderate") return "#f97316";
    if (status === "Severe") return "#dc2626";
    return "#64748b";
}

// CREATE ROAD MAP SVG

function createRoadSVG() {
    return `
    <svg viewBox="0 0 800 450"
         xmlns="http://www.w3.org/2000/svg"
         style="width:100%;height:100%">

        <!-- Background -->
        <rect width="800" height="450" fill="#e8f0e8"/>

        <!-- City Blocks -->
        <rect x="60" y="45" width="140" height="90"
              rx="8" fill="#cbd5e1"/>

        <rect x="250" y="45" width="150" height="90"
              rx="8" fill="#d6d3d1"/>

        <rect x="470" y="45" width="170" height="90"
              rx="8" fill="#cbd5e1"/>

        <rect x="70" y="210" width="150" height="100"
              rx="8" fill="#d6d3d1"/>

        <rect x="290" y="210" width="150" height="100"
              rx="8" fill="#cbd5e1"/>

        <rect x="520" y="220" width="170" height="100"
              rx="8" fill="#d6d3d1"/>

        <!-- Roads -->
        <path d="M0 170 H800"
              stroke="#475569"
              stroke-width="55"/>

        <path d="M0 370 H800"
              stroke="#475569"
              stroke-width="55"/>

        <path d="M240 0 V450"
              stroke="#475569"
              stroke-width="55"/>

        <path d="M470 0 V450"
              stroke="#475569"
              stroke-width="55"/>

        <!-- Road Markings -->
        <path d="M0 170 H800"
              stroke="white"
              stroke-width="3"
              stroke-dasharray="20 15"/>

        <path d="M0 370 H800"
              stroke="white"
              stroke-width="3"
              stroke-dasharray="20 15"/>

        <path d="M240 0 V450"
              stroke="white"
              stroke-width="3"
              stroke-dasharray="20 15"/>

        <path d="M470 0 V450"
              stroke="white"
              stroke-width="3"
              stroke-dasharray="20 15"/>

        <!-- Buildings -->
        <text x="95" y="95" font-size="15"
              fill="#475569">BLOCK A</text>

        <text x="290" y="95" font-size="15"
              fill="#475569">BLOCK B</text>

        <text x="525" y="95" font-size="15"
              fill="#475569">BLOCK C</text>

        <text x="110" y="265" font-size="15"
              fill="#475569">BLOCK D</text>

        <text x="335" y="265" font-size="15"
              fill="#475569">BLOCK E</text>

        <text x="570" y="275" font-size="15"
              fill="#475569">BLOCK F</text>

        <!-- Map Label -->
        <text x="20" y="425" font-size="13"
              fill="#334155">
            SIMULATED ROAD NETWORK - NOT TO SCALE
        </text>
    </svg>
    `;
}

// CREATE MARKERS

function createMarkers(containerId, isHeatMap = false) {

    const container = document.getElementById(containerId);

    container.style.position = "relative";
    container.innerHTML = createRoadSVG();

    locations.forEach(location => {

        const marker = document.createElement("button");

        marker.className = "road-marker";

        marker.style.left = location.x + "%";
        marker.style.top = location.y + "%";

        marker.style.backgroundColor =
            getColor(location.status);

        marker.textContent = location.id;

        marker.title = location.name;

        marker.addEventListener("click", () => {
            showLocationDetails(location);
        });

        container.appendChild(marker);

        // HEAT MAP EFFECT

        if (isHeatMap && location.status !== "Normal") {

            const heat = document.createElement("div");

            heat.className = "heat-zone";

            heat.style.left = location.x + "%";
            heat.style.top = location.y + "%";

            heat.style.background =
                `radial-gradient(circle,
                ${getColor(location.status)}99 0%,
                ${getColor(location.status)}44 45%,
                transparent 75%)`;

            container.insertBefore(heat, marker);
        }
    });
}

// LOCATION DETAILS

function showLocationDetails(location) {

    alert(
        "ROAD MONITORING DETAILS\n\n" +
        "Location: " + location.name + "\n" +
        "Latitude: " + location.lat + "\n" +
        "Longitude: " + location.lng + "\n" +
        "Location Source: SIMULATED\n" +
        "Severity: " + location.status + "\n" +
        "Estimated Depth: " + location.depth + " cm\n" +
        "Water: " + (location.wet ? "Detected" : "Not Detected") + "\n" +
        "Maintenance: " + location.maintenance
    );
}

// DASHBOARD STATISTICS

function updateDashboard() {

    const potholes = locations.filter(
        l => l.status !== "Normal"
    );

    document.getElementById("totalPotholes").textContent =
        potholes.length;

    document.getElementById("criticalPotholes").textContent =
        potholes.filter(l => l.status === "Severe").length;

    document.getElementById("waterPotholes").textContent =
        potholes.filter(l => l.wet).length;

    document.getElementById("pendingRepairs").textContent =
        potholes.filter(
            l => l.maintenance === "Pending" ||
                 l.maintenance === "In Progress"
        ).length;

    document.getElementById("completedRepairs").textContent =
        potholes.filter(
            l => l.maintenance === "Resolved"
        ).length;

    const averageDepth =
        locations.reduce((sum, l) => sum + l.depth, 0)
        / locations.length;

    const health = Math.max(0, 100 - averageDepth * 10);

    document.getElementById("roadHealth").textContent =
        health.toFixed(1) + "%";
}

// MAINTENANCE TABLE

function updateMaintenanceTable() {

    const table = document.getElementById("maintenanceTable");

    table.innerHTML = "";

    locations
        .filter(l => l.status !== "Normal")
        .forEach(location => {

            const row = document.createElement("tr");

            row.innerHTML = `
                <td>PH-00${location.id}</td>
                <td>${location.name}</td>
                <td>${location.status}</td>
                <td>${location.wet ? "Wet" : "Dry"}</td>
                <td>${location.maintenance}</td>
            `;

            table.appendChild(row);
        });
}

// DEMONSTRATION SENSOR DATA

function updateDemoSensors() {

    document.getElementById("ultrasonicValue").textContent =
        "10.50 cm";

    document.getElementById("depthValue").textContent =
        "5.50 cm";

    document.getElementById("magneticValue").textContent =
        "2250";

    document.getElementById("rotationValue").textContent =
        "3";

    document.getElementById("speedValue").textContent =
        "10.05 cm/s";

    document.getElementById("moistureValue").textContent =
        "WET";

    document.getElementById("gpsStatus").textContent =
        "SIMULATED GPS";
}

// ANALYTICS

function updateAnalytics() {

    const analytics = document.getElementById(
        "analyticsContainer"
    );

    analytics.innerHTML = `
        <div style="text-align:center;padding:20px;">
            <h3>Road Condition Distribution</h3>
            <p>Normal Zones:
                ${locations.filter(l => l.status === "Normal").length}
            </p>
            <p>Minor Potholes:
                ${locations.filter(l => l.status === "Minor").length}
            </p>
            <p>Moderate Potholes:
                ${locations.filter(l => l.status === "Moderate").length}
            </p>
            <p>Severe Potholes:
                ${locations.filter(l => l.status === "Severe").length}
            </p>
            <small>Demonstration Data</small>
        </div>
    `;
}

// INITIALIZE WEBSITE

createMarkers("map");
createMarkers("heatMapContainer", true);

updateDashboard();
updateMaintenanceTable();
updateDemoSensors();
updateAnalytics();

console.log("Custom Road Monitoring Dashboard Loaded");