/*
 * ESP32 WiFi + Bluetooth Dual Protocol OBD Server
 * Serves OBD data via multiple protocols for maximum compatibility
 * with Chigee XR-2 and other displays
 */

#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// Dual protocol setup
BluetoothSerial BT_Serial;
WebServer web_server(80);

// OBD2 data structure
struct OBDData {
    float speed = 0.0;
    float rpm = 0.0;
    float coolant_temp = 70.0;
    float fuel_level = 75.5;
    bool engine_running = true;
    unsigned long last_update = 0;
};

OBDData current_obd_data;
unsigned long lastStatus = 0;

// Function declarations
void setupWebRoutes();
void handleBluetoothOBD();
void updateOBDData();
void printStatus();
String generateDashboardHTML();
String generateChigeeHTML();
String generateSimpleOBDPage();
String processOBDCommand(String command);

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Dual Protocol OBD Server Starting...");
    
    // Initialize Bluetooth
    if (!BT_Serial.begin("ESP32_OBD")) {
        Serial.println("Bluetooth init failed!");
        return;
    }
    Serial.println("Bluetooth initialized: ESP32_OBD");
    
    // Initialize WiFi Access Point
    WiFi.softAP("ESP32_OBD_WiFi", "12345678");
    IPAddress IP = WiFi.softAPIP();
    Serial.println("WiFi AP started: ESP32_OBD_WiFi");
    Serial.println("IP address: " + IP.toString());
    
    // Setup web routes
    setupWebRoutes();
    
    // Start web server
    web_server.begin();
    Serial.println("Web server started on port 80");
    
    Serial.println("\n=== Ready for connections ===");
    Serial.println("Bluetooth: ESP32_OBD");
    Serial.println("WiFi: ESP32_OBD_WiFi (password: 12345678)");
    Serial.println("Web dashboard: http://" + IP.toString());
    Serial.println("Chigee interface: http://" + IP.toString() + "/chigee");
}

void loop() {
    // Handle web requests
    web_server.handleClient();
    
    // Handle Bluetooth OBD commands
    handleBluetoothOBD();
    
    // Update simulated data
    updateOBDData();
    
    // Status output every 5 seconds
    if (millis() - lastStatus > 5000) {
        lastStatus = millis();
        printStatus();
    }
    
    delay(50);
}

void setupWebRoutes() {
    // Main dashboard page
    web_server.on("/", HTTP_GET, []() {
        String html = generateDashboardHTML();
        web_server.send(200, "text/html", html);
    });
    
    // JSON API for real-time data
    web_server.on("/api/obd", HTTP_GET, []() {
        JsonDocument doc;
        doc["rpm"] = current_obd_data.rpm;
        doc["speed"] = current_obd_data.speed;
        doc["coolant_temp"] = current_obd_data.coolant_temp;
        doc["fuel_level"] = current_obd_data.fuel_level;
        doc["engine_running"] = current_obd_data.engine_running;
        
        String response;
        serializeJson(doc, response);
        web_server.send(200, "application/json", response);
    });
    
    // Chigee-specific interface
    web_server.on("/chigee", HTTP_GET, []() {
        String chigee_html = generateChigeeHTML();
        web_server.send(200, "text/html", chigee_html);
    });
    
    // Simple OBD page
    web_server.on("/simple", HTTP_GET, []() {
        String obd_page = generateSimpleOBDPage();
        web_server.send(200, "text/html", obd_page);
    });
    
    // 404 handler
    web_server.onNotFound([]() {
        web_server.send(404, "text/plain", "Page not found");
    });
}

void handleBluetoothOBD() {
    if (BT_Serial.available()) {
        String command = BT_Serial.readStringUntil('\r');
        String response = processOBDCommand(command);
        BT_Serial.print(response);
    }
}

void updateOBDData() {
    // Simulate changing data
    current_obd_data.rpm = 1500 + sin(millis() / 1000.0) * 500;
    current_obd_data.speed = 60 + sin(millis() / 2000.0) * 20;
    current_obd_data.coolant_temp = 85 + sin(millis() / 5000.0) * 10;
    current_obd_data.fuel_level = 75.5 - (millis() / 60000.0) * 0.1; // Slowly decreasing
    current_obd_data.last_update = millis();
}

void printStatus() {
    Serial.println("\n=== ESP32 OBD Server Status ===");
    Serial.println("BT Clients: " + String(BT_Serial.hasClient() ? "Connected" : "None"));
    Serial.println("WiFi Clients: " + String(WiFi.softAPgetStationNum()));
    Serial.println("Current RPM: " + String(current_obd_data.rpm));
    Serial.println("Current Speed: " + String(current_obd_data.speed));
    Serial.println("Free Memory: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("===============================");
}

String processOBDCommand(String command) {
    command.trim();
    command.toUpperCase();
    
    // ELM327 AT commands
    if (command.startsWith("AT")) {
        if (command == "ATZ") return "ELM327 v1.5\r>";
        if (command == "ATE0") return "OK\r>";
        if (command == "ATL0") return "OK\r>";
        if (command == "ATS0") return "OK\r>";
        if (command == "ATH1") return "OK\r>";
        if (command == "ATSP0") return "OK\r>";
        return "OK\r>";
    }
    
    // OBD2 PIDs
    if (command == "010C") { // Engine RPM
        int rpm_hex = (int)current_obd_data.rpm * 4;
        return "41 0C " + String(rpm_hex >> 8, HEX) + " " + String(rpm_hex & 0xFF, HEX) + "\r>";
    }
    if (command == "010D") { // Vehicle Speed
        int speed_hex = (int)current_obd_data.speed;
        return "41 0D " + String(speed_hex, HEX) + "\r>";
    }
    if (command == "0105") { // Coolant Temperature
        int temp_hex = (int)current_obd_data.coolant_temp + 40;
        return "41 05 " + String(temp_hex, HEX) + "\r>";
    }
    if (command == "012F") { // Fuel Level
        int fuel_hex = (int)(current_obd_data.fuel_level * 2.55);
        return "41 2F " + String(fuel_hex, HEX) + "\r>";
    }
    
    return "NO DATA\r>";
}

String generateDashboardHTML() {
    return F(R"(<!DOCTYPE html><html><head><title>ESP32 OBD</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0}.card{background:white;border-radius:10px;padding:20px;margin:10px 0;box-shadow:0 2px 10px rgba(0,0,0,0.1)}.gauge{display:inline-block;width:150px;height:150px;border:10px solid #e0e0e0;border-radius:50%;position:relative;margin:10px}.gauge-value{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);font-size:24px;font-weight:bold}.nav{text-align:center;margin:20px 0}.nav a{margin:0 10px;padding:10px 20px;background:#007bff;color:white;text-decoration:none;border-radius:5px}</style></head><body><div class="card"><h1>ESP32 OBD Server</h1><div class="nav"><a href="/">Dashboard</a><a href="/chigee">Chigee</a><a href="/simple">Simple</a></div></div><div class="card"><h2>Engine Data</h2><div class="gauge"><div class="gauge-value" id="rpm">0</div></div>RPM<div class="gauge"><div class="gauge-value" id="speed">0</div></div>Speed<p>Temp: <span id="temp">--</span>°C | Fuel: <span id="fuel">--</span>% | Engine: <span id="engine">--</span></p></div><script>function updateData(){fetch('/api/obd').then(r=>r.json()).then(d=>{document.getElementById('rpm').textContent=Math.round(d.rpm);document.getElementById('speed').textContent=Math.round(d.speed);document.getElementById('temp').textContent=Math.round(d.coolant_temp);document.getElementById('fuel').textContent=Math.round(d.fuel_level);document.getElementById('engine').textContent=d.engine_running?'ON':'OFF'})}setInterval(updateData,1000);updateData();</script></body></html>)");
}

String generateChigeeHTML() {
    return F(R"(<!DOCTYPE html><html><head><title>Chigee XR-2</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:monospace;margin:0;padding:10px;background:#000;color:#0f0;font-size:18px}.chigee-display{border:2px solid #0f0;padding:15px;border-radius:5px;background:#010}.data-row{display:flex;justify-content:space-between;margin:5px 0;padding:5px 0;border-bottom:1px solid #040}.big-number{font-size:32px;font-weight:bold;text-align:center;margin:10px 0}.nav{text-align:center;margin:10px 0}.nav a{margin:0 5px;padding:5px 10px;background:#040;color:#0f0;text-decoration:none;border:1px solid #0f0}</style></head><body><div class="chigee-display"><h2 style="text-align:center;margin-top:0">ESP32 OBD DATA</h2><div class="nav"><a href="/">Dash</a><a href="/chigee">Chigee</a><a href="/simple">Simple</a></div><div class="big-number" id="speed-big">-- km/h</div><div class="data-row"><span>RPM:</span><span id="rpm-display">----</span></div><div class="data-row"><span>TEMP:</span><span id="temp-display">--°C</span></div><div class="data-row"><span>FUEL:</span><span id="fuel-display">--%</span></div><div class="data-row"><span>ENGINE:</span><span id="engine-display">---</span></div></div><script>function updateChigeeData(){fetch('/api/obd').then(r=>r.json()).then(d=>{document.getElementById('speed-big').textContent=Math.round(d.speed)+' km/h';document.getElementById('rpm-display').textContent=Math.round(d.rpm);document.getElementById('temp-display').textContent=Math.round(d.coolant_temp)+'°C';document.getElementById('fuel-display').textContent=Math.round(d.fuel_level)+'%';document.getElementById('engine-display').textContent=d.engine_running?'RUN':'OFF'})}setInterval(updateChigeeData,500);updateChigeeData();</script></body></html>)");
}

String generateSimpleOBDPage() {
    return F(R"(<!DOCTYPE html><html><head><title>Simple OBD</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:Arial;padding:20px;background:#f5f5f5}.data-item{background:white;padding:15px;margin:10px 0;border-radius:5px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}.nav{text-align:center;margin:20px 0}.nav a{margin:0 10px;padding:10px 20px;background:#007bff;color:white;text-decoration:none;border-radius:5px}</style></head><body><h1>Simple OBD Data</h1><div class="nav"><a href="/">Dashboard</a><a href="/chigee">Chigee</a><a href="/simple">Simple</a></div><div class="data-item"><h3>Speed: <span id="speed">--</span> km/h</h3></div><div class="data-item"><h3>RPM: <span id="rpm">--</span></h3></div><div class="data-item"><h3>Temp: <span id="temp">--</span>°C</h3></div><div class="data-item"><h3>Fuel: <span id="fuel">--</span>%</h3></div><script>function updateSimpleData(){fetch('/api/obd').then(r=>r.json()).then(d=>{document.getElementById('speed').textContent=Math.round(d.speed);document.getElementById('rpm').textContent=Math.round(d.rpm);document.getElementById('temp').textContent=Math.round(d.coolant_temp);document.getElementById('fuel').textContent=Math.round(d.fuel_level)})}setInterval(updateSimpleData,1000);updateSimpleData();</script></body></html>)");
}