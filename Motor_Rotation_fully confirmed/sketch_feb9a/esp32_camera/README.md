# ESP32-CAM Setup Guide

This project sets up an ESP32-CAM module as a web camera server that you can access through your home network at a static IP address (192.168.1.5).

## Hardware Requirements

- ESP32-CAM module (AI Thinker ESP32-CAM recommended)
- FTDI programmer or USB-to-TTL converter for uploading code
- Jumper wires
- 5V power supply (for running without USB)
- Breadboard (optional but recommended)

## Pin Configuration

### Programming ESP32-CAM with FTDI Programmer

When uploading the code, connect the ESP32-CAM and FTDI programmer as follows:

| ESP32-CAM | FTDI Programmer |
|-----------|-----------------|
| 5V/VCC    | VCC (5V)        |
| GND       | GND             |
| U0R (RX)  | TX              |
| U0T (TX)  | RX              |
| IO0       | GND (only during programming) |

**Important:** 
1. To enter programming mode, connect IO0 to GND before powering up the board
2. After uploading, disconnect IO0 from GND and reset the board
3. Make sure to set the FTDI programmer to 5V mode

### ESP32-CAM Camera Pin Configuration

The camera pins are already defined in the code with the standard AI Thinker ESP32-CAM configuration:

```
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
```

The built-in LED is connected to GPIO 33, which is used for status indication.

## Setting Up the Software

1. Install the required libraries in Arduino IDE:
   - ESP32 board support
   - ESP32 Camera
   - ESPAsyncWebServer
   - AsyncTCP

2. Configure your WiFi settings in the code:
   ```c
   const char *ssid = "YOUR_WIFI_SSID";
   const char *password = "YOUR_WIFI_PASSWORD";
   ```

3. Upload the code to the ESP32-CAM (make sure IO0 is connected to GND during upload)

4. After uploading, disconnect IO0 from GND and reset the board

5. The onboard LED will indicate the connection status:
   - Fast blinking: Attempting to connect to WiFi
   - Solid for 2 seconds, then slow blinking: Successfully connected
   - Rapid blinking (5 times): Connection failed

## Accessing the Camera

Once the ESP32-CAM is connected to your WiFi network, you can access it at:

- Web Interface: `http://192.168.1.5/`
- Direct Stream: `http://192.168.1.5/stream`
- Camera Settings: `http://192.168.1.5/settings`

## DHCP Reservation (Recommended)

For most reliable operation with the static IP address when running without USB, set up a DHCP reservation in your router:

1. Note the MAC address of your ESP32-CAM (shown in the serial monitor during startup)
2. Access your router's admin panel
3. Find the DHCP reservation/static lease settings
4. Add a reservation for the ESP32-CAM's MAC address with IP 192.168.1.5

## Mounting Options

For best video quality:
- Use adequate lighting
- Mount the camera securely to prevent vibration
- Adjust the camera position before final installation
- Consider a 3D printed mount (models available online)

## Power Considerations

For standalone operation:
- Use a regulated 5V power supply capable of providing at least 500mA
- Power can be connected to the 5V and GND pins
- Consider adding a power switch for easy operation

## Integrating with the Motor Controller

This camera system is designed to work alongside the motor controller, which runs on IP address 192.168.1.4. Together they form a complete remote control system for your project. 