#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include <Wire.h>

// Distance sensor pins
#define echoPin 2
#define trigPin 4
long duration, distance;

// Motor A (Right Motor) pins
#define IN1 27
#define IN2 26
#define ENA 23

// Motor B (Left Motor) pins
#define IN3 25
#define IN4 33
#define ENB 32

// Fan control pins on L298N driver
#define FAN_IN1 5
#define FAN_IN2 18
#define FAN_EN 19

// PWM properties for motors
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// PWM properties for fan
#define FAN_PWM_CHANNEL 2
#define FAN_PWM_FREQ 5000
#define FAN_PWM_RESOLUTION 8

// Debug options
#define DEBUG_ENABLED true
#define DEBUG_PINS true
#define DEBUG_MOTOR true
#define DEBUG_WIFI true

// WiFi credentials
const char *ssid = "CYBER%NET%HUSSAIN%03111243682";
const char *password = "hussain8080";

// Create an asynchronous web server object on port 80
AsyncWebServer server(80);

// WebSocket server on port 81 for distance sensor data
WebSocketsServer webSocket = WebSocketsServer(81);

// Track motor state
String motorState = "stopped";

// Track motor speed
int motorSpeed = 255; // Default to full speed

// Fan state
bool fanOn = false;
int fanSpeed = 255;  // Default to full speed

// Variable to track if we need to send WebSocket update
unsigned long lastSendTime = 0;

// Debug function: Print information with timestamp
void debugPrint(String message, bool newline = true) {
  if (DEBUG_ENABLED) {
    String timestamp = String(millis() / 1000.0, 3);
    Serial.print("[" + timestamp + "s] ");
    Serial.print(message);
    if (newline) Serial.println();
  }
}

// HTML content for main page with tabs
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Vacuum Cleaner Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px; }
    .tab { overflow: hidden; border: 1px solid #ccc; background-color: #f1f1f1; }
    .tab button { background-color: inherit; float: left; border: none; outline: none; cursor: pointer; padding: 14px 16px; font-size: 17px; }
    .tab button:hover { background-color: #ddd; }
    .tab button.active { background-color: #ccc; }
    .tabcontent { display: none; padding: 6px 12px; border: 1px solid #ccc; border-top: none; }
    .button { padding: 10px 20px; font-size: 24px; margin: 10px; cursor: pointer; border-radius: 5px; }
    .forward { background-color: green; color: white; }
    .backward { background-color: blue; color: white; }
    .stop { background-color: red; color: white; }
    .status { margin: 20px; padding: 20px; font-size: 30px; font-weight: bold; }
    .on { background-color: #4CAF50; color: white; }
    .off { background-color: #f44336; color: white; }
    .speed-control { margin: 20px; padding: 20px; background-color: #f0f0f0; border-radius: 5px; }
    .turn-control { margin: 20px; padding: 20px; background-color: #f0f0f0; border-radius: 5px; }
    .data { font-size: 72px; margin: 30px; font-weight: bold; }
    .unit { font-size: 36px; }
    .fan-icon { font-size: 100px; margin: 20px; }
    .rotating { animation: rotate 2s linear infinite; }
    @keyframes rotate { 100% { transform: rotate(360deg); } }
    #pinStates { margin: 20px; padding: 10px; font-size: 18px; background-color: #f0f0f0; }
  </style>
  <script>
    // For tab navigation
    function openTab(evt, tabName) {
      var i, tabcontent, tablinks;
      tabcontent = document.getElementsByClassName("tabcontent");
      for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
      }
      tablinks = document.getElementsByClassName("tablinks");
      for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
      }
      document.getElementById(tabName).style.display = "block";
      evt.currentTarget.className += " active";
    }

    // Distance sensor WebSocket
    var websocket;
    function initWebSocket() {
      websocket = new WebSocket('ws://' + window.location.hostname + ':81/');
      websocket.onopen = function(evt) { console.log('WebSocket connected'); };
      websocket.onclose = function(evt) { console.log('WebSocket disconnected'); };
      websocket.onmessage = function(evt) {
        document.getElementById('distance').textContent = evt.data;
      };
    }

    // Motor Control Functions
    function updateMotorStatus() {
      fetch('/motorStatus')
        .then(response => response.text())
        .then(data => {
          document.getElementById('motorStatus').innerText = data;
          if(data === "Motor moving forward") {
            document.getElementById('motorStatus').style.backgroundColor = "lightgreen";
          } else if(data === "Motor moving backward") {
            document.getElementById('motorStatus').style.backgroundColor = "lightblue";
          } else {
            document.getElementById('motorStatus').style.backgroundColor = "lightgray";
          }
        });
    }
    
    function sendMotorCommand(command) {
      fetch('/' + command)
        .then(response => response.text())
        .then(data => {
          updateMotorStatus();
          checkPins();
        });
    }
    
    function setMotorSpeed() {
      const speed = document.getElementById('motorSpeedSlider').value;
      document.getElementById('motorSpeedValue').innerText = speed;
      fetch('/setMotorSpeed?speed=' + speed)
        .then(response => response.text())
        .then(data => {
          checkPins();
        });
    }
    
    function checkPins() {
      fetch('/pinstate')
        .then(response => response.json())
        .then(data => {
          const pinStatesDiv = document.getElementById('pinStates');
          let rightMotor = data.rightMotor;
          let leftMotor = data.leftMotor;
          
          let pinStateHtml = "<strong>Right Motor:</strong> ";
          pinStateHtml += "IN1=" + rightMotor.IN1.state + ", ";
          pinStateHtml += "IN2=" + rightMotor.IN2.state + ", ";
          pinStateHtml += "ENA=" + rightMotor.ENA.pwm + "<br>";
          
          pinStateHtml += "<strong>Left Motor:</strong> ";
          pinStateHtml += "IN3=" + leftMotor.IN3.state + ", ";
          pinStateHtml += "IN4=" + leftMotor.IN4.state + ", ";
          pinStateHtml += "ENB=" + leftMotor.ENB.pwm;
          
          pinStatesDiv.innerHTML = pinStateHtml;
        });
    }

    // Fan Control Functions
    function updateFanStatus() {
      fetch('/fanStatus')
        .then(response => response.text())
        .then(data => {
          document.getElementById('fanStatus').innerText = data;
          
          const fanIcon = document.getElementById('fanIcon');
          if(data === "Fan is ON") {
            document.getElementById('fanStatus').style.backgroundColor = "lightgreen";
            fanIcon.classList.add('rotating');
          } else {
            document.getElementById('fanStatus').style.backgroundColor = "lightgray";
            fanIcon.classList.remove('rotating');
          }
        });
    }
    
    function sendFanCommand(command) {
      fetch('/' + command)
        .then(response => response.text())
        .then(data => {
          updateFanStatus();
        });
    }
    
    function setFanSpeed() {
      const speed = document.getElementById('fanSpeedSlider').value;
      document.getElementById('fanSpeedValue').innerText = speed;
      fetch('/setFanSpeed?speed=' + speed)
        .then(response => response.text());
    }

    // Update status every 2 seconds
    setInterval(updateMotorStatus, 2000);
    setInterval(updateFanStatus, 2000);
    setInterval(checkPins, 2000);
    
    // Initial setup when page loads
    window.onload = function() {
      initWebSocket();
      updateMotorStatus();
      updateFanStatus();
      checkPins();
      // Open first tab by default
      document.getElementById("defaultTab").click();
    }
  </script>
</head>
<body>
  <h1>ESP32 Vacuum Cleaner Control</h1>
  
  <div class="tab">
    <button class="tablinks" onclick="openTab(event, 'DistanceTab')" id="defaultTab">Distance Sensor</button>
    <button class="tablinks" onclick="openTab(event, 'MotorTab')">Motor Control</button>
    <button class="tablinks" onclick="openTab(event, 'FanTab')">Fan Control</button>
  </div>
  
  <!-- Distance Sensor Tab -->
  <div id="DistanceTab" class="tabcontent">
    <h2>Distance Sensor</h2>
    <div class="data"><span id="distance">0</span><span class="unit"> cm</span></div>
  </div>
  
  <!-- Motor Control Tab -->
  <div id="MotorTab" class="tabcontent">
    <h2>Motor Control</h2>
    
    <button class="button forward" onclick="sendMotorCommand('forward')">Forward</button>
    <button class="button stop" onclick="sendMotorCommand('stop')">Stop</button>
    <button class="button backward" onclick="sendMotorCommand('backward')">Backward</button>
    
    <div class="status">Motor Status: <span id="motorStatus">Loading...</span></div>
    
    <div class="speed-control">
      <h2>Speed Control</h2>
      <input type="range" min="50" max="255" value="255" id="motorSpeedSlider" onchange="setMotorSpeed()">
      <p>Current Speed: <span id="motorSpeedValue">255</span>/255</p>
    </div>

    <div class="turn-control">
      <h2>Turn Control</h2>
      <button class="button" onclick="sendMotorCommand('turnLeft')">Turn Left</button>
      <button class="button" onclick="sendMotorCommand('turnRight')">Turn Right</button>
    </div>
    
    <div id="pinStates" class="debug-info">Pin States: Loading...</div>
  </div>
  
  <!-- Fan Control Tab -->
  <div id="FanTab" class="tabcontent">
    <h2>Fan Control</h2>
    
    <div class="fan-icon" id="fanIcon">🌀</div>
    
    <div class="status">Fan Status: <span id="fanStatus">Loading...</span></div>
    
    <button class="button on" onclick="sendFanCommand('turnFanOn')">TURN ON</button>
    <button class="button off" onclick="sendFanCommand('turnFanOff')">TURN OFF</button>
    
    <div class="speed-control">
      <h2>Fan Speed Control</h2>
      <input type="range" min="50" max="255" value="255" id="fanSpeedSlider" onchange="setFanSpeed()">
      <p>Current Speed: <span id="fanSpeedValue">255</span>/255</p>
    </div>
  </div>
</body>
</html>
)rawliteral";

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

// ------- WebSocket event handler for distance sensor --------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        
        // Send current distance immediately when a client connects
        String disp = String(distance);
        webSocket.sendTXT(num, disp);
      }
      break;
  }
}

// ---------- MOTOR FUNCTIONS ----------
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

// ---------- FAN FUNCTIONS ----------
// Function to turn the fan on
void turnFanOn() {
  Serial.println("Turning fan ON");
  
  // Set fan direction (forward)
  digitalWrite(FAN_IN1, HIGH);
  digitalWrite(FAN_IN2, LOW);
  
  // Apply speed to the fan
  ledcWrite(FAN_PWM_CHANNEL, fanSpeed);
  
  fanOn = true;
  Serial.println("Fan is now ON at speed: " + String(fanSpeed));
}

// Function to turn the fan off
void turnFanOff() {
  Serial.println("Turning fan OFF");
  
  // Stop the fan by setting PWM to 0
  ledcWrite(FAN_PWM_CHANNEL, 0);
  
  // Also set control pins to LOW for lower power consumption
  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  
  fanOn = false;
  Serial.println("Fan is now OFF");
}

// ---------- DISTANCE SENSOR FUNCTIONS ----------
void readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = duration / 58.2;
  
  // For debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}

void setup() {
  Serial.begin(115200);
  debugPrint("Integrated Vacuum Cleaner setup started...");

  // -------- Distance Sensor Setup --------
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  debugPrint("Distance sensor pins initialized");

  // -------- Motor Control Setup --------
  // Configure motor A (Right Motor) pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL_A);
  
  // Configure motor B (Left Motor) pins
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENB, PWM_CHANNEL_B);
  
  // Start with motor off
  stopMotor();
  debugPrint("Motor pins initialized");

  // -------- Fan Control Setup --------
  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(FAN_EN, OUTPUT);
  
  // Configure PWM for fan speed control
  ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
  ledcAttachPin(FAN_EN, FAN_PWM_CHANNEL);
  
  // Initialize fan as off
  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  ledcWrite(FAN_PWM_CHANNEL, 0);
  debugPrint("Fan pins initialized");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  debugPrint("Connecting to Wi-Fi: " + String(ssid));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  debugPrint("Wi-Fi connected. IP address: " + WiFi.localIP().toString());

  // -------- WebSocket Setup --------
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  debugPrint("WebSocket server started on port 81");

  // -------- Web Server Setup --------
  // Main HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  
  // ------ Motor Control Endpoints ------
  server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request) {
    moveForward();
    request->send(200, "text/plain", "Moving forward");
  });
  
  server.on("/backward", HTTP_GET, [](AsyncWebServerRequest *request) {
    moveBackward();
    request->send(200, "text/plain", "Moving backward");
  });
  
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    stopMotor();
    request->send(200, "text/plain", "Motors stopped");
  });
  
  server.on("/turnLeft", HTTP_GET, [](AsyncWebServerRequest *request) {
    turnLeft();
    request->send(200, "text/plain", "Turning left");
  });
  
  server.on("/turnRight", HTTP_GET, [](AsyncWebServerRequest *request) {
    turnRight();
    request->send(200, "text/plain", "Turning right");
  });
  
  server.on("/motorStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", motorState);
  });
  
  server.on("/setMotorSpeed", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("speed")) {
      motorSpeed = request->getParam("speed")->value().toInt();
      // Update speed for currently running motors
      if (motorState == "Motor moving forward" || motorState == "Motor moving backward" || 
          motorState == "Turning left" || motorState == "Turning right") {
        ledcWrite(PWM_CHANNEL_A, motorSpeed);
        ledcWrite(PWM_CHANNEL_B, motorSpeed);
      }
      request->send(200, "text/plain", "Speed set to " + String(motorSpeed));
    } else {
      request->send(400, "text/plain", "Speed parameter missing");
    }
  });
  
  // ------ Fan Control Endpoints ------
  server.on("/turnFanOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    turnFanOn();
    request->send(200, "text/plain", "Fan turned on");
  });
  
  server.on("/turnFanOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    turnFanOff();
    request->send(200, "text/plain", "Fan turned off");
  });
  
  server.on("/fanStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    String status = fanOn ? "Fan is ON" : "Fan is OFF";
    request->send(200, "text/plain", status);
  });
  
  server.on("/setFanSpeed", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("speed")) {
      fanSpeed = request->getParam("speed")->value().toInt();
      // Only update if fan is on
      if (fanOn) {
        ledcWrite(FAN_PWM_CHANNEL, fanSpeed);
      }
      request->send(200, "text/plain", "Fan speed set to " + String(fanSpeed));
    } else {
      request->send(400, "text/plain", "Speed parameter missing");
    }
  });
  
  // ------ Debug Endpoints ------
  server.on("/pinstate", HTTP_GET, [](AsyncWebServerRequest *request) {
    String pinState = getPinStateJSON();
    request->send(200, "application/json", pinState);
  });

  // Start server
  server.begin();
  debugPrint("HTTP server started on port 80");
  
  debugPrint("Integrated Vacuum Cleaner setup complete.");
}

void loop() {
  // Handle WebSocket communications
  webSocket.loop();
  
  // Read distance from sensor
  readDistance();
  
  // Send distance update via WebSocket every second
  if (millis() - lastSendTime > 1000) {
    String disp = String(distance);
    webSocket.broadcastTXT(disp);
    lastSendTime = millis();
  }
  
  // If WiFi disconnects, try to reconnect
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastReconnectAttempt > 10000) {
      Serial.println("Attempting to reconnect to WiFi...");
      WiFi.reconnect();
      lastReconnectAttempt = currentMillis;
    }
  }
  
  delay(100); // Small delay to prevent watchdog issues
} 