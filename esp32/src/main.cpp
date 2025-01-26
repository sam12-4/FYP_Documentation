#include <esp32cam.h>
#include <WebServer.h>
#include <WiFi.h>

// esp cam integration

WebServer server(80);

const char* ssid = "Nensi Batra";
const char* password = "waheguru.";

void handleCapture() {
  auto frame = esp32cam::capture();
  if (!frame) {
    server.send(500, "text/plain", "Failed to capture image");
    return;
  }

  const uint8_t* imageBuffer = frame->data();
  size_t imageSize = frame->size();

  server.send_P(200, "image/jpeg", (const char*)imageBuffer, imageSize);

  frame.reset();
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  esp32cam::Config cfg;
  cfg.setPins(esp32cam::pins::AiThinker);
  cfg.setResolution(esp32cam::Resolution::find(1024, 768));
  cfg.setJpeg(80);
  if (!esp32cam::Camera.begin(cfg)) {
    Serial.println("Camera init failed");
    return;
  }

  server.on("/capture", handleCapture);
  server.begin();
}

void loop() {
  server.handleClient();
}




// sonar integration 

// #define TRIG_PIN 14  // GPIO pin for Trigger
// #define ECHO_PIN 32  // GPIO pin for Echo

// void setup() {
//   Serial.begin(115200);
//   pinMode(TRIG_PIN, OUTPUT);
//   pinMode(ECHO_PIN, INPUT);

//   Serial.println("HC-SR04 Ultrasonic Sensor Example");
//   Serial.println("Measuring distances...");
// }

// void loop() {
//   // Send a 10µs HIGH pulse to the Trigger pin
//   digitalWrite(TRIG_PIN, LOW);
//   delayMicroseconds(2);
//   digitalWrite(TRIG_PIN, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(TRIG_PIN, LOW);

//   // Measure the duration of the echo pulse
//   long duration = pulseIn(ECHO_PIN, HIGH);

//   // Calculate the distance in centimeters
//   long distance = duration * 0.034 / 2;

//   // Print the result
//   if (distance > 0 && distance <= 200) {
//     Serial.print("Distance: ");
//     Serial.print(distance);
//     Serial.println(" cm");
//   } else {
//     Serial.println("Out of range");
//   }

//   delay(500); // Delay between measurements
// }




// // fan integration
// #define ENA 21
// #define IN3 22 
// #define IN4 23  

// int fanSpeed = 150;

// void setup() {
//   // Configure pins
//   pinMode(ENA, OUTPUT);
//   pinMode(IN3, OUTPUT);
//   pinMode(IN4, OUTPUT);
  
//   digitalWrite(IN3, LOW);
//   digitalWrite(IN4, HIGH);

//   Serial.begin(115200);
// }

// void loop() {
//   // Fan On with Speed Control
//   analogWrite(ENA, fanSpeed); // PWM output to control speed
//   Serial.println("Fan is running...");
//   delay(5000); // Keep fan running for 5 seconds

//   // Fan Off
//   analogWrite(ENA, 0); // Set PWM to 0 to stop the fan
//   Serial.println("Fan is stopped...");
//   delay(3000); // Keep fan stopped for 3 seconds
  
//   // Increase speed dynamically
//   for (fanSpeed = 50; fanSpeed <= 255; fanSpeed += 50) {
//     analogWrite(ENA, fanSpeed);
//     Serial.print("Fan Speed: ");
//     Serial.println(fanSpeed);
//     delay(2000);
//   }
// }
