#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Fan control pins on L298N driver
#define FAN_IN1 5    // Control pin 1 for fan direction
#define FAN_IN2 18   // Control pin 2 for fan direction  
#define FAN_EN 19    // Enable pin for fan speed control

// PWM properties
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// WiFi credentials - use the same as your main project
const char *ssid = "CYBER%NET%HUSSAIN%03111243682";
const char *password = "hussain8080";

// Static IP configuration - use a different IP than your motor control
IPAddress local_IP(192, 168, 1, 9);  // Different from your motor controller
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// Create web server
AsyncWebServer server(80);

// Fan state
bool fanOn = false;
int fanSpeed = 255;  // Default to full speed

// HTML content for fan control
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Fan Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px; }
    .button { padding: 20px 40px; font-size: 24px; margin: 20px; cursor: pointer; border-radius: 10px; }
    .on { background-color: #4CAF50; color: white; }
    .off { background-color: #f44336; color: white; }
    .status { margin: 20px; padding: 20px; font-size: 30px; font-weight: bold; }
    .speed-control { margin: 20px; padding: 20px; background-color: #f0f0f0; border-radius: 5px; }
    .fan-icon { font-size: 100px; margin: 20px; }
    .rotating { animation: rotate 2s linear infinite; }
    @keyframes rotate { 100% { transform: rotate(360deg); } }
  </style>
  <script>
    function updateStatus() {
      fetch('/status')
        .then(response => response.text())
        .then(data => {
          document.getElementById('fanStatus').innerText = data;
          
          // Change fan icon animation and colors based on status
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
    
    function sendCommand(command) {
      fetch('/' + command)
        .then(response => response.text())
        .then(data => {
          updateStatus();
        });
    }
    
    function setSpeed() {
      const speed = document.getElementById('speedSlider').value;
      document.getElementById('speedValue').innerText = speed;
      fetch('/setSpeed?speed=' + speed)
        .then(response => response.text());
    }
    
    // Update status every 2 seconds
    setInterval(updateStatus, 2000);
    
    // Initial status update when page loads
    window.onload = function() {
      updateStatus();
    }
  </script>
</head>
<body>
  <h1>ESP32 Fan Control</h1>
  
  <div class="fan-icon" id="fanIcon">🌀</div>
  
  <div class="status">Fan Status: <span id="fanStatus">Loading...</span></div>
  
  <button class="button on" onclick="sendCommand('turnOn')">TURN ON</button>
  <button class="button off" onclick="sendCommand('turnOff')">TURN OFF</button>
  
  <div class="speed-control">
    <h2>Fan Speed Control</h2>
    <input type="range" min="50" max="255" value="255" id="speedSlider" onchange="setSpeed()">
    <p>Current Speed: <span id="speedValue">255</span>/255</p>
  </div>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial.println("Fan Control System Starting...");
  
  // Set up fan control pins
  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(FAN_EN, OUTPUT);
  
  // Configure PWM for fan speed control
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(FAN_EN, PWM_CHANNEL);
  
  // Initialize fan as off
  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  ledcWrite(PWM_CHANNEL, 0);
  
  // Connect to Wi-Fi with static IP
  Serial.print("Configuring static IP: ");
  Serial.println(local_IP.toString());
  
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Static IP configuration failed - will use DHCP");
  }
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed. Check credentials.");
  }
  
  // Set up web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.on("/turnOn", HTTP_GET, [](AsyncWebServerRequest *request){
    turnFanOn();
    request->send(200, "text/plain", "Fan turned on");
  });
  
  server.on("/turnOff", HTTP_GET, [](AsyncWebServerRequest *request){
    turnFanOff();
    request->send(200, "text/plain", "Fan turned off");
  });
  
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String status = fanOn ? "Fan is ON" : "Fan is OFF";
    request->send(200, "text/plain", status);
  });
  
  server.on("/setSpeed", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("speed")) {
      fanSpeed = request->getParam("speed")->value().toInt();
      // Only update if fan is on
      if (fanOn) {
        ledcWrite(PWM_CHANNEL, fanSpeed);
      }
      request->send(200, "text/plain", "Fan speed set to " + String(fanSpeed));
    } else {
      request->send(400, "text/plain", "Speed parameter missing");
    }
  });
  
  // Start server
  server.begin();
  Serial.print("HTTP server started. Visit http://");
  Serial.print(WiFi.localIP());
  Serial.println(" in your browser");
  
  Serial.println("Fan Control Setup Complete");
}

void loop() {
  // No processing needed in loop as the web server handles requests asynchronously
  
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
}

// Function to turn the fan on
void turnFanOn() {
  Serial.println("Turning fan ON");
  
  // Set fan direction (forward)
  digitalWrite(FAN_IN1, HIGH);
  digitalWrite(FAN_IN2, LOW);
  
  // Apply speed to the fan
  ledcWrite(PWM_CHANNEL, fanSpeed);
  
  fanOn = true;
  Serial.println("Fan is now ON at speed: " + String(fanSpeed));
}

// Function to turn the fan off
void turnFanOff() {
  Serial.println("Turning fan OFF");
  
  // Stop the fan by setting PWM to 0
  ledcWrite(PWM_CHANNEL, 0);
  
  // Also set control pins to LOW for lower power consumption
  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  
  fanOn = false;
  Serial.println("Fan is now OFF");
} 