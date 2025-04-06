#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>


#define echoPin 2               // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
#define trigPin 4               // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
long duration, distance;


// WiFi credentials
const char* ssid = "CYBER%NET%HUSSAIN%03111243682";
const char* password = "hussain8080";

// Web server on port 80
WebServer server(80);
// WebSocket server on port 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Variable to track if we need to send WebSocket update
unsigned long lastSendTime = 0;

void setup(){
  Serial.begin (9600);

   // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  // Set up WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


}
void loop(){
   // Handle web client requests
  server.handleClient();
  webSocket.loop();
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = duration / 58.2;
  String disp = String(distance);

  Serial.print("Distance: ");
  Serial.print(disp);
  Serial.println(" cm");
  
  // Send distance update via WebSocket every second
  if (millis() - lastSendTime > 1000) {
    webSocket.broadcastTXT(disp);
    lastSendTime = millis();
  }
  
  delay(1000);
}

// WebSocket event handler
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

// Handler for root path
void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Distance Sensor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }";
  html += ".data { font-size: 72px; margin: 30px; font-weight: bold; }";
  html += ".unit { font-size: 36px; }";
  html += "</style>";
  html += "<script>";
  html += "var websocket;";
  html += "function init() {";
  html += "  websocket = new WebSocket('ws://' + window.location.hostname + ':81/');";
  html += "  websocket.onopen = function(evt) { console.log('WebSocket connected'); };";
  html += "  websocket.onclose = function(evt) { console.log('WebSocket disconnected'); };";
  html += "  websocket.onmessage = function(evt) {";
  html += "    document.getElementById('distance').textContent = evt.data;";
  html += "  };";
  html += "}";
  html += "window.addEventListener('load', init);";
  html += "</script></head>";
  html += "<body>";
  html += "<h1>ESP32 Distance Sensor</h1>";
  html += "<div class='data'><span id='distance'>" + String(distance) + "</span><span class='unit'> cm</span></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
} 