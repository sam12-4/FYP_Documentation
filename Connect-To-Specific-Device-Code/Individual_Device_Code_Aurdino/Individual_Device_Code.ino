#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Replace with your WiFi credentials
const char* ssid = "CYBER%NET%HUSSAIN%03111243682";
const char* password = "hussain8080";

// Unique device code
const String deviceCode = "DEVICE12345"; // Replace with your specific device code

WebServer server(80);

// Function to handle the "/start-task" request
void handleStartTask() {
  // Set CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  if (server.hasArg("plain")) { // Check if the request body is provided
    String requestBody = server.arg("plain");
    Serial.println("Received Request Body: " + requestBody); // Debug log

    // Parse JSON payload
    StaticJsonDocument<200> doc; // Adjust size if needed
    DeserializationError error = deserializeJson(doc, requestBody);

    if (error) {
      server.send(400, "text/plain", "Bad Request: Invalid JSON.");
      Serial.println("JSON Parsing Error: " + String(error.c_str()));
      return;
    }

    // Extract the "deviceCode" field
    String receivedCode = doc["deviceCode"];
    Serial.println("Extracted Device Code: " + receivedCode); // Debug log

    // Compare the received code with the expected device code
    if (receivedCode == deviceCode) {
      server.send(200, "text/plain", "Device connected successfully! Task started.");
      Serial.println("Device verified and task started.");
    } else {
      server.send(403, "text/plain", "Invalid device code. Connection denied.");
      Serial.println("Invalid device code received.");
    }
  } else {
    server.send(400, "text/plain", "Bad Request: No request body received.");
    Serial.println("No request body received.");
  }
}

// Function to handle preflight OPTIONS requests for CORS
void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Allow all origins
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS"); // Allow specific methods
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type"); // Allow specific headers
  server.send(204); // No content
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  
  // Wait for Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Define server routes
  server.on("/start-task", HTTP_POST, handleStartTask); // POST for task handling
  server.on("/start-task", HTTP_OPTIONS, handleOptions); // OPTIONS for CORS preflight

  // Start the server
  server.begin();
  Serial.println("Server started!");
}

void loop() {
  server.handleClient(); // Handle incoming client requests
}
