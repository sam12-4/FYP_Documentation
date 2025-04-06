#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Motor A (Right Motor) pins
#define IN1 27
#define IN2 26
#define ENA 23

// Motor B (Left Motor) pins
#define IN3 25
#define IN4 33
#define ENB 32

// PWM properties
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// Debug options - set to true to enable detailed debugging
#define DEBUG_ENABLED true
#define DEBUG_PINS true
#define DEBUG_MOTOR true
#define DEBUG_WIFI true

// WiFi credentials
const char *ssid = "CYBER%NET%HUSSAIN%03111243682";
const char *password = "hussain8080";

// Create an asynchronous web server object on port 80
AsyncWebServer server(80);

// Track motor state
String motorState = "stopped";

// Track motor speed
int motorSpeed = 255; // Default to full speed

// HTML content with auto-refresh and status display
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px; }
    .button { padding: 10px 20px; font-size: 24px; margin: 10px; cursor: pointer; border-radius: 5px; }
    .forward { background-color: green; color: white; }
    .backward { background-color: blue; color: white; }
    .stop { background-color: red; color: white; }
    .status { margin: 20px; padding: 20px; font-size: 30px; font-weight: bold; }
    .debug { background-color: orange; color: white; }
    #pinStates { margin: 20px; padding: 10px; font-size: 18px; background-color: #f0f0f0; }
    .speed-control { margin: 20px; padding: 20px; background-color: #f0f0f0; border-radius: 5px; }
    .turn-control { margin: 20px; padding: 20px; background-color: #f0f0f0; border-radius: 5px; }
    .debug-info { margin: 20px; padding: 10px; font-size: 18px; background-color: #f0f0f0; }
    .debug-panel { margin: 20px; padding: 20px; background-color: #f0f0f0; border-radius: 5px; }
    .diagnostic-data { margin: 20px; padding: 10px; font-size: 18px; background-color: #f0f0f0; }
    .debug-actions { margin: 20px; padding: 10px; background-color: #f0f0f0; border-radius: 5px; }
  </style>
  <script>
    function updateStatus() {
      fetch('/status')
        .then(response => response.text())
        .then(data => {
          document.getElementById('motorStatus').innerText = data;
          // Change background color based on status
          if(data === "Motor moving forward") {
            document.getElementById('motorStatus').style.backgroundColor = "lightgreen";
          } else if(data === "Motor moving backward") {
            document.getElementById('motorStatus').style.backgroundColor = "lightblue";
          } else {
            document.getElementById('motorStatus').style.backgroundColor = "lightgray";
          }
        });
    }
    
    function sendCommand(command) {
      fetch('/' + command)
        .then(response => response.text())
        .then(data => {
          updateStatus();
          checkPins(); // Update pin states after command
        });
    }
    
    function setSpeed() {
      const speed = document.getElementById('speedSlider').value;
      document.getElementById('speedValue').innerText = speed;
      fetch('/setSpeed?speed=' + speed)
        .then(response => response.text())
        .then(data => {
          checkPins(); // Update pin states after speed change
        });
    }
    
    function checkPins() {
      fetch('/pinstate')
        .then(response => response.json())
        .then(data => {
          const pinStatesDiv = document.getElementById('pinStates');
          // Format the JSON data into a readable form
          let rightMotor = data.rightMotor;
          let leftMotor = data.leftMotor;
          
          let pinStateHtml = "<strong>Right Motor:</strong> ";
          pinStateHtml += "IN1(" + rightMotor.IN1.pin + ")=" + rightMotor.IN1.state + ", ";
          pinStateHtml += "IN2(" + rightMotor.IN2.pin + ")=" + rightMotor.IN2.state + ", ";
          pinStateHtml += "ENA=" + rightMotor.ENA.pwm + "<br>";
          
          pinStateHtml += "<strong>Left Motor:</strong> ";
          pinStateHtml += "IN3(" + leftMotor.IN3.pin + ")=" + leftMotor.IN3.state + ", ";
          pinStateHtml += "IN4(" + leftMotor.IN4.pin + ")=" + leftMotor.IN4.state + ", ";
          pinStateHtml += "ENB=" + leftMotor.ENB.pwm;
          
          pinStatesDiv.innerHTML = pinStateHtml;
        });
    }
    
    function loadDiagnostics() {
      fetch('/diagnostics')
        .then(response => response.json())
        .then(data => {
          const diagnosticsDiv = document.getElementById('diagnosticsData');
          
          let html = "<h3>System Info</h3>";
          html += "<p>Uptime: " + secondsToTime(data.system.uptime) + "<br>";
          html += "ESP32 Model: " + data.system.chipId + "<br>";
          html += "CPU Freq: " + data.system.cpuFreq + " MHz<br>";
          html += "Free Memory: " + formatBytes(data.system.freeHeap) + "</p>";
          
          html += "<h3>WiFi Info</h3>";
          html += "<p>Connected: " + (data.wifi.connected ? "Yes" : "No") + "<br>";
          html += "IP Address: " + data.wifi.ip + "<br>";
          html += "Signal Strength: " + data.wifi.rssi + " dBm<br>";
          html += "MAC Address: " + data.wifi.mac + "</p>";
          
          diagnosticsDiv.innerHTML = html;
        });
    }
    
    function secondsToTime(seconds) {
      let hours = Math.floor(seconds / 3600);
      let minutes = Math.floor((seconds % 3600) / 60);
      let secs = seconds % 60;
      return hours + "h " + minutes + "m " + secs + "s";
    }
    
    function formatBytes(bytes) {
      if (bytes < 1024) return bytes + " bytes";
      else if (bytes < 1048576) return (bytes / 1024).toFixed(2) + " KB";
      else return (bytes / 1048576).toFixed(2) + " MB";
    }
    
    function toggleDebugPanel() {
      const debugPanel = document.getElementById('debugPanel');
      if (debugPanel.style.display === 'none') {
        debugPanel.style.display = 'block';
        loadDiagnostics();
      } else {
        debugPanel.style.display = 'none';
      }
    }
    
    // Update status every 2 seconds
    setInterval(updateStatus, 2000);
    setInterval(checkPins, 2000);
    
    // Initial status update when page loads
    window.onload = function() {
      updateStatus();
      checkPins();
    }
  </script>
</head>
<body>
  <h1>ESP32 Motor Control</h1>
  
  <button class="button forward" onclick="sendCommand('forward')">Forward</button>
  <button class="button stop" onclick="sendCommand('stop')">Stop</button>
  <button class="button backward" onclick="sendCommand('backward')">Backward</button>
  
  <div class="status">Motor Status: <span id="motorStatus">Loading...</span></div>
  
  <div class="speed-control">
    <h2>Speed Control</h2>
    <input type="range" min="50" max="255" value="255" id="speedSlider" onchange="setSpeed()">
    <p>Current Speed: <span id="speedValue">255</span>/255</p>
    <div>
      <button class="button speed" onclick="sendCommand('increaseSpeed')">Increase Speed</button>
      <button class="button speed" onclick="sendCommand('decreaseSpeed')">Decrease Speed</button>
    </div>
  </div>

  <div class="turn-control">
    <h2>Turn Control</h2>
    <button class="button turn-left" onclick="sendCommand('turnLeft')">Turn Left</button>
    <button class="button turn-right" onclick="sendCommand('turnRight')">Turn Right</button>
  </div>
  
  <div id="pinStates" class="debug-info">Pin States: Unknown</div>
  
  <div style="margin-top: 20px">
    <button class="button" onclick="sendCommand('test')">Run Motor Test</button>
    <button class="button" onclick="sendCommand('alternate')">Alternate Test</button>
    <button class="button debug" onclick="sendCommand('testLeftMotor')">Test Left Motor</button>
    <button class="button debug" onclick="toggleDebugPanel()">Toggle Debug Panel</button>
  </div>
  
  <div id="debugPanel" style="display: none;" class="debug-panel">
    <h2>Diagnostics & Debugging</h2>
    
    <div id="diagnosticsData" class="diagnostic-data">
      Loading diagnostics...
    </div>
    
    <div class="debug-actions">
      <button class="button debug" onclick="loadDiagnostics()">Refresh Diagnostics</button>
      <button class="button debug" onclick="sendCommand('testAllPins')">Test All Pins</button>
    </div>
  </div>
</body>
</html>
)rawliteral";

// Function prototypes
void moveForward();
void moveBackward();
void stopMotor();
void testMotor();
void alternateTest();
void setLeftMotorSpeed(int speed);
void moveLeftMotorForward();
void moveLeftMotorBackward();
void stopLeftMotor();
void turnLeft();
void turnRight();

// Debug function: Print information with timestamp
void debugPrint(String message, bool newline = true) {
  if (DEBUG_ENABLED) {
    String timestamp = String(millis() / 1000.0, 3);
    Serial.print("[" + timestamp + "s] ");
    Serial.print(message);
    if (newline) Serial.println();
  }
}

// Function to get detailed JSON of all pin states
String getPinStateJSON() {
  String json = "{";
  
  // Right motor pins
  json += "\"rightMotor\":{";
  json += "\"IN1\":{\"pin\":" + String(IN1) + ",\"state\":" + String(digitalRead(IN1)) + "},";
  json += "\"IN2\":{\"pin\":" + String(IN2) + ",\"state\":" + String(digitalRead(IN2)) + "},";
  json += "\"ENA\":{\"pin\":" + String(ENA) + ",\"pwm\":" + String(ledcRead(PWM_CHANNEL_A)) + "}";
  json += "},";
  
  // Left motor pins
  json += "\"leftMotor\":{";
  json += "\"IN3\":{\"pin\":" + String(IN3) + ",\"state\":" + String(digitalRead(IN3)) + "},";
  json += "\"IN4\":{\"pin\":" + String(IN4) + ",\"state\":" + String(digitalRead(IN4)) + "},";
  json += "\"ENB\":{\"pin\":" + String(ENB) + ",\"pwm\":" + String(ledcRead(PWM_CHANNEL_B)) + "}";
  json += "},";
  
  // System state
  json += "\"motorState\":\"" + motorState + "\"";
  
  json += "}";
  return json;
}

// Function to get full system diagnostics
String getDiagnosticsJSON() {
  String json = "{";
  
  // System info
  json += "\"system\":{";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"chipId\":\"" + String(ESP.getChipModel()) + "\",";
  json += "\"cpuFreq\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\"";
  json += "},";
  
  // WiFi info
  json += "\"wifi\":{";
  json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\"";
  json += "},";
  
  // Pin states (include the same info as pinstate)
  json += "\"pins\":" + getPinStateJSON();
  
  json += "}";
  return json;
}

// Function to test all pins for connectivity
void testAllPins() {
  debugPrint("Testing all pins for connectivity...");
  
  // Test right motor pins
  digitalWrite(IN1, HIGH);
  delay(10);
  debugPrint("IN1 set HIGH, read: " + String(digitalRead(IN1)));
  digitalWrite(IN1, LOW);
  
  digitalWrite(IN2, HIGH);
  delay(10);
  debugPrint("IN2 set HIGH, read: " + String(digitalRead(IN2)));
  digitalWrite(IN2, LOW);
  
  // Test PWM on ENA
  ledcWrite(PWM_CHANNEL_A, 128);
  delay(10);
  debugPrint("ENA PWM set 128, read: " + String(ledcRead(PWM_CHANNEL_A)));
  ledcWrite(PWM_CHANNEL_A, 0);
  
  // Enhanced debugging for left motor pins (IN3 and IN4)
  debugPrint("\n=== Testing Left Motor Pins ===");
  
  // Test IN3
  debugPrint("Testing IN3 (GPIO " + String(IN3) + "):");
  digitalWrite(IN3, HIGH);
  delay(100); // Longer delay to ensure stable reading
  int in3HighRead = digitalRead(IN3);
  debugPrint("  - Set HIGH, read: " + String(in3HighRead));
  
  digitalWrite(IN3, LOW);
  delay(100);
  int in3LowRead = digitalRead(IN3);
  debugPrint("  - Set LOW, read: " + String(in3LowRead));
  
  // Test IN4
  debugPrint("\nTesting IN4 (GPIO " + String(IN4) + "):");
  digitalWrite(IN4, HIGH);
  delay(100);
  int in4HighRead = digitalRead(IN4);
  debugPrint("  - Set HIGH, read: " + String(in4HighRead));
  
  digitalWrite(IN4, LOW);
  delay(100);
  int in4LowRead = digitalRead(IN4);
  debugPrint("  - Set LOW, read: " + String(in4LowRead));
  
  // Test PWM on ENB
  debugPrint("\nTesting ENB PWM:");
  ledcWrite(PWM_CHANNEL_B, 128);
  delay(100);
  debugPrint("  - Set PWM 128, read: " + String(ledcRead(PWM_CHANNEL_B)));
  
  ledcWrite(PWM_CHANNEL_B, 255);
  delay(100);
  debugPrint("  - Set PWM 255, read: " + String(ledcRead(PWM_CHANNEL_B)));
  
  ledcWrite(PWM_CHANNEL_B, 0);
  delay(100);
  debugPrint("  - Set PWM 0, read: " + String(ledcRead(PWM_CHANNEL_B)));
  
  // Print summary of left motor pin test
  debugPrint("\nLeft Motor Pin Test Summary:");
  debugPrint("IN3: " + String(IN3) + " - " + 
             (in3HighRead == 1 && in3LowRead == 0 ? "OK" : "PROBLEM DETECTED"));
  debugPrint("IN4: " + String(IN4) + " - " + 
             (in4HighRead == 1 && in4LowRead == 0 ? "OK" : "PROBLEM DETECTED"));
  
  debugPrint("Pin test complete");
}

// Add a new function specifically for testing left motor
void testLeftMotor() {
  debugPrint("\n=== Starting Left Motor Test ===");
  
  // Test forward movement
  debugPrint("Testing left motor forward:");
  moveLeftMotorForward();
  delay(1000);
  debugPrint("  - IN3 state: " + String(digitalRead(IN3)));
  debugPrint("  - IN4 state: " + String(digitalRead(IN4)));
  debugPrint("  - ENB PWM: " + String(ledcRead(PWM_CHANNEL_B)));
  
  // Stop briefly
  stopLeftMotor();
  delay(500);
  
  // Test backward movement
  debugPrint("\nTesting left motor backward:");
  moveLeftMotorBackward();
  delay(1000);
  debugPrint("  - IN3 state: " + String(digitalRead(IN3)));
  debugPrint("  - IN4 state: " + String(digitalRead(IN4)));
  debugPrint("  - ENB PWM: " + String(ledcRead(PWM_CHANNEL_B)));
  
  // Stop
  stopLeftMotor();
  debugPrint("\nLeft motor test complete");
}

void setup() {
  Serial.begin(115200);
  debugPrint("Setup started...");

  // Configure motor A (Right Motor) pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL_A);
  debugPrint("Right motor pins initialized");

  // Configure motor B (Left Motor) pins
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENB, PWM_CHANNEL_B);
  debugPrint("Left motor pins initialized");

  // Start with motor off - ensure pins are properly initialized
  stopMotor();
  
  debugPrint("Motor pins initialized.");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  debugPrint("Connecting to Wi-Fi: " + String(ssid));
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    Serial.print(".");
    delay(1000);
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    debugPrint("\nWi-Fi connected.");
    debugPrint("IP Address: " + WiFi.localIP().toString());
  } else {
    debugPrint("\nWi-Fi connection FAILED!");
    debugPrint("Continuing without WiFi. Check your credentials.");
  }

  // Define HTTP endpoints
  // Serve the HTML page at root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request){
    moveForward();
    motorState = "Motor moving forward";
    request->send(200, "text/plain", motorState);
  });

  server.on("/backward", HTTP_GET, [](AsyncWebServerRequest *request){
    moveBackward();
    motorState = "Motor moving backward";
    request->send(200, "text/plain", motorState);
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    stopMotor();
    motorState = "Motor stopped";
    request->send(200, "text/plain", motorState);
  });
  
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    testMotor();
    request->send(200, "text/plain", "Motor test running");
  });
  
  server.on("/alternate", HTTP_GET, [](AsyncWebServerRequest *request){
    alternateTest();
    request->send(200, "text/plain", "Alternating motor test running");
  });
  
  // Add status endpoint to get current motor state
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", motorState);
  });
  
  // Add pin state endpoint for debugging
  server.on("/pinstate", HTTP_GET, [](AsyncWebServerRequest *request){
    String pinState = getPinStateJSON();
    request->send(200, "application/json", pinState);
  });

  // Add a detailed diagnostics endpoint
  server.on("/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request){
    String diagnostics = getDiagnosticsJSON();
    request->send(200, "application/json", diagnostics);
  });

  // Add new endpoints for turning
  server.on("/turnLeft", HTTP_GET, [](AsyncWebServerRequest *request){
    turnLeft();
    request->send(200, "text/plain", "Turning left");
  });

  server.on("/turnRight", HTTP_GET, [](AsyncWebServerRequest *request){
    turnRight();
    request->send(200, "text/plain", "Turning right");
  });

  // Add pin testing endpoint
  server.on("/testAllPins", HTTP_GET, [](AsyncWebServerRequest *request){
    testAllPins();
    request->send(200, "text/plain", "Testing all pins");
  });

  // Add new endpoint for left motor testing
  server.on("/testLeftMotor", HTTP_GET, [](AsyncWebServerRequest *request){
    testLeftMotor();
    request->send(200, "text/plain", "Left motor test running");
  });

  // Add endpoints for speed control
  server.on("/setSpeed", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("speed")) {
      motorSpeed = request->getParam("speed")->value().toInt();
      // Update speed for currently running motors
      if (motorState == "Motor moving forward" || motorState == "Motor moving backward") {
        ledcWrite(PWM_CHANNEL_A, motorSpeed);
        ledcWrite(PWM_CHANNEL_B, motorSpeed);
      }
      request->send(200, "text/plain", "Speed set to " + String(motorSpeed));
    } else {
      request->send(400, "text/plain", "Speed parameter missing");
    }
  });

  server.on("/increaseSpeed", HTTP_GET, [](AsyncWebServerRequest *request){
    motorSpeed = min(255, motorSpeed + 25);
    // Update speed for currently running motors
    if (motorState == "Motor moving forward" || motorState == "Motor moving backward") {
      ledcWrite(PWM_CHANNEL_A, motorSpeed);
      ledcWrite(PWM_CHANNEL_B, motorSpeed);
    }
    request->send(200, "text/plain", "Speed increased to " + String(motorSpeed));
  });

  server.on("/decreaseSpeed", HTTP_GET, [](AsyncWebServerRequest *request){
    motorSpeed = max(50, motorSpeed - 25);
    // Update speed for currently running motors
    if (motorState == "Motor moving forward" || motorState == "Motor moving backward") {
      ledcWrite(PWM_CHANNEL_A, motorSpeed);
      ledcWrite(PWM_CHANNEL_B, motorSpeed);
    }
    request->send(200, "text/plain", "Speed decreased to " + String(motorSpeed));
  });

  // Start server
  server.begin();
  debugPrint("HTTP server started. Visit http://" + WiFi.localIP().toString() + " in your browser");
  
  // Initial status report
  debugPrint("Available GPIO pins:");
  debugPrint("IN1: GPIO " + String(IN1));
  debugPrint("IN2: GPIO " + String(IN2));
  debugPrint("ENA: GPIO " + String(ENA));
  debugPrint("IN3: GPIO " + String(IN3));
  debugPrint("IN4: GPIO " + String(IN4));
  debugPrint("ENB: GPIO " + String(ENB));
  
  // Test pins at startup to verify they're working
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Double-check that pins can be written to
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  debugPrint("Testing pins - IN1 set to HIGH, read: " + String(digitalRead(IN1)));
  digitalWrite(IN2, LOW);
  debugPrint("Testing pins - IN2 set to LOW, read: " + String(digitalRead(IN2)));
  
  // Reset to stop state
  stopMotor();

  // Run diagnostic tests on pins
  if (DEBUG_PINS) {
    testAllPins();
  }
  
  // Print system diagnostic info
  if (DEBUG_ENABLED) {
    debugPrint("System Info:");
    debugPrint("- ESP32 Chip: " + String(ESP.getChipModel()));
    debugPrint("- CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz");
    debugPrint("- SDK Version: " + String(ESP.getSdkVersion()));
    debugPrint("- Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  }
  
  debugPrint("Setup complete. Ready to operate.");
}

void loop() {
  // No loop processing is needed as AsyncWebServer handles requests asynchronously
}

// Function to move motor forward
void moveForward() {
  debugPrint("Motors moving forward");
  
  // Set both motors forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  // Apply speed to both motors
  ledcWrite(PWM_CHANNEL_A, motorSpeed);
  ledcWrite(PWM_CHANNEL_B, motorSpeed);
  
  motorState = "Motor moving forward";
  debugPrint(motorState);
}

// Function to move motor backward
void moveBackward() {
  debugPrint("Motors moving backward");
  
  // Set both motors backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  // Apply speed to both motors
  ledcWrite(PWM_CHANNEL_A, motorSpeed);
  ledcWrite(PWM_CHANNEL_B, motorSpeed);
  
  motorState = "Motor moving backward";
  debugPrint(motorState);
}

// Function to stop the motor
void stopMotor() {
  debugPrint("Motors stopped");
  
  // Stop both motors
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  // Set PWM to 0 for both motors
  ledcWrite(PWM_CHANNEL_A, 0);
  ledcWrite(PWM_CHANNEL_B, 0);
  
  motorState = "Motor stopped";
  debugPrint(motorState);
}

// Function to test motor with a sequence of movements
void testMotor() {
  debugPrint("Running motor test sequence");
  
  // Step 1: Run forward at 50% speed
  debugPrint("Test Step 1: Forward at 50% speed");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 128);
  
  delay(1000); // Wait 1 second
  
  // Step 2: Increase to full speed
  debugPrint("Test Step 2: Forward at 100% speed");
  ledcWrite(PWM_CHANNEL_A, 255);
  
  delay(1000); // Wait 1 second
  
  // Step 3: Run backward
  debugPrint("Test Step 3: Backward at 100% speed");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  ledcWrite(PWM_CHANNEL_A, 255);
  
  delay(1000); // Wait 1 second
  
  // Step 4: Stop
  debugPrint("Test Step 4: Stop motor");
  ledcWrite(PWM_CHANNEL_A, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  
  debugPrint("Motor test complete");
}

// Function for alternate testing (try this if the regular test doesn't work)
void alternateTest() {
  debugPrint("Starting alternate motor test");
  
  // Try different PIN combinations
  debugPrint("Testing PIN combination 1: IN1=HIGH, IN2=LOW");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 200); // Less than full power
  delay(1000);
  
  // Stop briefly
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 0);
  delay(500);
  
  // Try reversed direction
  debugPrint("Testing PIN combination 2: IN1=LOW, IN2=HIGH");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  ledcWrite(PWM_CHANNEL_A, 200);
  delay(1000);
  
  // Stop
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 0);
  
  debugPrint("Alternate test complete");
}

// Function to set left motor speed without changing direction
void setLeftMotorSpeed(int speed) {
  // Constrain speed to valid range
  speed = constrain(speed, 0, 255);
  
  // Only update speed if motor is running
  if (motorState == "Motor moving forward" || motorState == "Motor moving backward") {
    ledcWrite(PWM_CHANNEL_B, speed);
  }
  
  debugPrint("Left motor speed set to: " + String(speed));
}

// Function to move left motor forward
void moveLeftMotorForward() {
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_B, 255);
}

// Function to move left motor backward
void moveLeftMotorBackward() {
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  ledcWrite(PWM_CHANNEL_B, 255);
}

// Function to stop left motor
void stopLeftMotor() {
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_B, 0);
}

// Function to turn left
void turnLeft() {
  debugPrint("Turning left");
  
  // Right motor forward, left motor backward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  // Apply speed to both motors
  ledcWrite(PWM_CHANNEL_A, motorSpeed);
  ledcWrite(PWM_CHANNEL_B, motorSpeed);
  
  motorState = "Turning left";
  debugPrint(motorState);
}

// Function to turn right
void turnRight() {
  debugPrint("Turning right");
  
  // Right motor backward, left motor forward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  // Apply speed to both motors
  ledcWrite(PWM_CHANNEL_A, motorSpeed);
  ledcWrite(PWM_CHANNEL_B, motorSpeed);
  
  motorState = "Turning right";
  debugPrint(motorState);
}


