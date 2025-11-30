#include "web_config.h"
#include <ArduinoJson.h>
#include "motion.h"  // For g_mp and g_limits

AsyncWebServer server(80);

void WebConfig_begin(){
  // Start WiFi in AP mode for configuration
  WiFi.mode(WIFI_AP);
  WiFi.softAP("PD-Blinds-Config", "config1234");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Serve HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", WebConfig_getHTML());
  });

  // API: Get current status
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<512> doc;
    doc["velocity"] = Motion_getVelocity();
    doc["acceleration"] = Motion_getAcceleration();
    doc["pdVoltage"] = (int)g_mp.pdVoltage;
    doc["measuredVoltage"] = PD_measureVoltage();
    doc["currentPosition"] = AS5600_totalCounts;
    doc["topLimit"] = g_limits.top;
    doc["bottomLimit"] = g_limits.bottom;
    doc["limitsSet"] = g_limits.set;
    doc["currentPercent"] = Motion_getCurrentPercent();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Set PD voltage
  server.on("/api/setPdVoltage", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<128> doc;
      deserializeJson(doc, (char*)data);
      
      if (doc.containsKey("voltage")) {
        int volt = doc["voltage"];
        PDVoltage pdVolt = PD_12V;
        if (volt == 5) pdVolt = PD_5V;
        else if (volt == 9) pdVolt = PD_9V;
        else if (volt == 12) pdVolt = PD_12V;
        else if (volt == 15) pdVolt = PD_15V;
        else if (volt == 20) pdVolt = PD_20V;
        
        g_mp.pdVoltage = pdVolt;
        PD_setVoltage(pdVolt);
        Motion_saveNVS();
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Missing voltage parameter\"}");
      }
    });

  // API: Set velocity
  server.on("/api/setVelocity", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<128> doc;
      deserializeJson(doc, (char*)data);
      
      if (doc.containsKey("velocity")) {
        float vel = doc["velocity"];
        Motion_setVelocity(vel);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Missing velocity parameter\"}");
      }
    });

  // API: Set acceleration
  server.on("/api/setAcceleration", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<128> doc;
      deserializeJson(doc, (char*)data);
      
      if (doc.containsKey("acceleration")) {
        float acc = doc["acceleration"];
        Motion_setAcceleration(acc);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Missing acceleration parameter\"}");
      }
    });

  // API: Set max position (encoder count)
  server.on("/api/setMaxPosition", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<128> doc;
      deserializeJson(doc, (char*)data);
      
      if (doc.containsKey("position")) {
        int32_t pos = doc["position"];
        g_limits.bottom = pos;
        g_limits.set = true;
        Motion_saveNVS();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Missing position parameter\"}");
      }
    });

  // API: Set current position as max
  server.on("/api/setCurrentAsMax", HTTP_POST, [](AsyncWebServerRequest *request){
    AS5600_readTotal();
    g_limits.bottom = AS5600_totalCounts;
    if (!g_limits.set) {
      g_limits.top = AS5600_totalCounts;
    }
    g_limits.set = true;
    Motion_saveNVS();
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // API: Find min position using stall detection
  server.on("/api/findMinPosition", HTTP_POST, [](AsyncWebServerRequest *request){
    bool stall = false;
    Motion_homeMin(stall);
    request->send(200, "application/json", stall ? "{\"status\":\"ok\",\"stall\":true}" : "{\"status\":\"ok\",\"stall\":false}");
  });

  // API: Find max position using stall detection (original homing)
  server.on("/api/findMaxPosition", HTTP_POST, [](AsyncWebServerRequest *request){
    bool stall = false;
    Motion_home(stall);
    request->send(200, "application/json", stall ? "{\"status\":\"ok\",\"stall\":true}" : "{\"status\":\"ok\",\"stall\":false}");
  });

  server.begin();
  Serial.println("Web server started");
}

  String WebConfig_getHTML(){
    // Use a raw string literal with a custom delimiter so the embedded HTML/JS doesn't
    // need escaping and won't terminate early on the default ")" sequence.
    return String(F(R"pdhtml(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PD Blinds Configuration</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
        }
        h1 {
            color: #333;
            margin-bottom: 30px;
            text-align: center;
            font-size: 2em;
        }
        .section {
            margin-bottom: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 10px;
            border-left: 4px solid #667eea;
        }
        .section h2 {
            color: #555;
            margin-bottom: 15px;
            font-size: 1.3em;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #666;
            font-weight: 500;
        }
        input[type="number"], select {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        input[type="number"]:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            width: 100%;
            margin-top: 10px;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        button:active {
            transform: translateY(0);
        }
        .status {
            margin-top: 20px;
            padding: 15px;
            background: #e8f5e9;
            border-radius: 8px;
            border-left: 4px solid #4caf50;
        }
        .status.error {
            background: #ffebee;
            border-left-color: #f44336;
        }
        .info {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-top: 15px;
        }
        .info-item {
            padding: 10px;
            background: white;
            border-radius: 6px;
            font-size: 14px;
        }
        .info-label {
            color: #999;
            font-size: 12px;
            margin-bottom: 4px;
        }
        .info-value {
            color: #333;
            font-weight: 600;
        }
        .button-group {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 PD Blinds Configuration</h1>
        
        <div class="section">
            <h2>Power Delivery Voltage</h2>
            <div class="form-group">
                <label for="pdVoltage">PD Voltage:</label>
                <select id="pdVoltage">
                    <option value="5">5V</option>
                    <option value="9">9V</option>
                    <option value="12" selected>12V</option>
                    <option value="15">15V</option>
                    <option value="20">20V</option>
                </select>
            </div>
            <button onclick="setPdVoltage()">Set PD Voltage</button>
            <div class="info">
                <div class="info-item">
                    <div class="info-label">Measured Voltage</div>
                    <div class="info-value" id="measuredVoltage">-- V</div>
                </div>
            </div>
        </div>

        <div class="section">
            <h2>Motion Parameters</h2>
            <div class="form-group">
                <label for="velocity">Velocity (counts/sec):</label>
                <input type="number" id="velocity" step="100" min="100" max="10000">
            </div>
            <div class="form-group">
                <label for="acceleration">Acceleration (counts/sec²):</label>
                <input type="number" id="acceleration" step="100" min="100" max="50000">
            </div>
            <button onclick="setMotionParams()">Save Motion Parameters</button>
        </div>

        <div class="section">
            <h2>Position Limits</h2>
            <div class="form-group">
                <label for="maxPosition">Max Position (encoder counts):</label>
                <input type="number" id="maxPosition" step="1">
            </div>
            <div class="button-group">
                <button onclick="setMaxPosition()">Set Max Position</button>
                <button onclick="setCurrentAsMax()">Use Current as Max</button>
            </div>
            <div class="button-group" style="margin-top: 10px;">
                <button onclick="findMinPosition()">Find Min (Stall Detection)</button>
                <button onclick="findMaxPosition()">Find Max (Stall Detection)</button>
            </div>
            <div class="info">
                <div class="info-item">
                    <div class="info-label">Current Position</div>
                    <div class="info-value" id="currentPosition">--</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Current %</div>
                    <div class="info-value" id="currentPercent">--%</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Top Limit</div>
                    <div class="info-value" id="topLimit">--</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Bottom Limit</div>
                    <div class="info-value" id="bottomLimit">--</div>
                </div>
            </div>
        </div>

        <div id="status"></div>
    </div>

    <script>
        let updateInterval;

        function showStatus(message, isError = false) {
            const statusDiv = document.getElementById('status');
            statusDiv.className = 'status' + (isError ? ' error' : '');
            statusDiv.textContent = message;
            setTimeout(() => {
                statusDiv.textContent = '';
            }, 5000);
        }

        async function updateStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                
                document.getElementById('velocity').value = data.velocity;
                document.getElementById('acceleration').value = data.acceleration;
                document.getElementById('pdVoltage').value = data.pdVoltage;
                document.getElementById('measuredVoltage').textContent = data.measuredVoltage.toFixed(2) + ' V';
                document.getElementById('currentPosition').textContent = data.currentPosition;
                document.getElementById('currentPercent').textContent = data.currentPercent + '%';
                document.getElementById('topLimit').textContent = data.topLimit;
                document.getElementById('bottomLimit').textContent = data.bottomLimit;
                document.getElementById('maxPosition').value = data.bottomLimit;
            } catch (error) {
                console.error('Error updating status:', error);
            }
        }

        async function setPdVoltage() {
            const voltage = parseInt(document.getElementById('pdVoltage').value);
            try {
                const response = await fetch('/api/setPdVoltage', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({voltage: voltage})
                });
                const result = await response.json();
                if (result.status === 'ok') {
                    showStatus('PD Voltage set to ' + voltage + 'V');
                    setTimeout(updateStatus, 1000);
                } else {
                    showStatus('Error: ' + result.error, true);
                }
            } catch (error) {
                showStatus('Error setting PD voltage', true);
            }
        }

        async function setMotionParams() {
            const velocity = parseFloat(document.getElementById('velocity').value);
            const acceleration = parseFloat(document.getElementById('acceleration').value);
            
            try {
                let response = await fetch('/api/setVelocity', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({velocity: velocity})
                });
                
                response = await fetch('/api/setAcceleration', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({acceleration: acceleration})
                });
                
                showStatus('Motion parameters saved');
                updateStatus();
            } catch (error) {
                showStatus('Error saving motion parameters', true);
            }
        }

        async function setMaxPosition() {
            const position = parseInt(document.getElementById('maxPosition').value);
            try {
                const response = await fetch('/api/setMaxPosition', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({position: position})
                });
                const result = await response.json();
                if (result.status === 'ok') {
                    showStatus('Max position set to ' + position);
                    updateStatus();
                } else {
                    showStatus('Error: ' + result.error, true);
                }
            } catch (error) {
                showStatus('Error setting max position', true);
            }
        }

        async function setCurrentAsMax() {
            try {
                const response = await fetch('/api/setCurrentAsMax', {method: 'POST'});
                const result = await response.json();
                if (result.status === 'ok') {
                    showStatus('Current position set as max');
                    updateStatus();
                }
            } catch (error) {
                showStatus('Error setting current as max', true);
            }
        }

        async function findMinPosition() {
            showStatus('Finding min position using stall detection...');
            try {
                const response = await fetch('/api/findMinPosition', {method: 'POST'});
                const result = await response.json();
                if (result.status === 'ok') {
                    showStatus('Min position found' + (result.stall ? ' (stall detected)' : ''));
                    updateStatus();
                }
            } catch (error) {
                showStatus('Error finding min position', true);
            }
        }

        async function findMaxPosition() {
            showStatus('Finding max position using stall detection...');
            try {
                const response = await fetch('/api/findMaxPosition', {method: 'POST'});
                const result = await response.json();
                if (result.status === 'ok') {
                    showStatus('Max position found' + (result.stall ? ' (stall detected)' : ''));
                    updateStatus();
                }
            } catch (error) {
                showStatus('Error finding max position', true);
            }
        }

        // Initialize and update status every 2 seconds
        updateStatus();
        updateInterval = setInterval(updateStatus, 2000);
    </script>
  </body>
</html>
)pdhtml"));
  }

