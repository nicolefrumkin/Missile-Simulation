#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <functions.h>

//screen:
#define TFT_CS     5
#define TFT_DC     2
#define TFT_RST    4
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_SCLK   18
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//wifi:
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6
WebServer server(80);  // HTTP server on port 80
bool waiting_for_input_from_web = true;  // Flag to indicate if we are waiting for input from the web interface

//speaker:
#define SPEAKER_PIN 13


void handlingWIFI(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);  // Set up HTML handler
  server.begin();
  Serial.println("Please open the following URL in your browser: http://localhost:8180/");
  Serial.println("And provide input parameters to the server.");
  server.on("/upload", handleUpload);
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><title>Missile Config</title></head>
    <body>
      <h2>Missile Simulation Configuration</h2>

      <form action="/upload" method="GET">
        <label>Missile Type:</label>
        <select name="type">
          <option value="ballistic">Ballistic</option>
          <option value="powered">Powered</option>
        </select><br><br>

        <label>Launch Speed:</label>
        <input type="range" name="speed" min="1" max="100" value="50"><br><br>

        <label>Launch Angle (0-90):</label>
        <input type="number" name="angle" min="0" max="90" value="45"><br><br>

        <label>Route Difficulty:</label>
        <select name="route">
          <option value="none">None</option>
          <option value="fixed">Fixed</option>
          <option value="random">Random</option>
        </select><br><br>

        <label>Target Difficulty:</label>
        <select name="target">
          <option value="static">Static</option>
          <option value="slow">Slow Vertical</option>
          <option value="random">Random Box</option>
        </select><br><br>

        <input type="submit" value="Upload Simulation Configuration">
      </form>
    </body>
    </html>
  )rawliteral");
}

void handleUpload() {
  String type = server.arg("type");
  String speed = server.arg("speed");
  String angle = server.arg("angle");
  String route = server.arg("route");
  String target = server.arg("target");

  // Print values to serial for now
  Serial.println("=== Configuration Uploaded ===");
  Serial.println("Type: " + type);
  Serial.println("Speed: " + speed);
  Serial.println("Angle: " + angle);
  Serial.println("Route: " + route);
  Serial.println("Target: " + target);

  // TODO: Store values & set config ready flag
  // Turn on LED if needed
  waiting_for_input_from_web = false;  // Set flag to false to indicate we have received input
  server.send(200, "text/html", "<h2>Configuration Received! You may now launch the missile.</h2>");
}

void handlingStartTFT(){
  tft.begin();

  tft.setRotation(3);  // Optional: rotate for horizontal layout
  tft.fillScreen(ILI9341_BLACK);  // Clear screen

  // First message
  tft.setCursor(10, 50);  // X=10, Y=50
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(2);
  tft.print("Hello, pending simulation configuration from WEB");
}

void handlingEndTFT(bool hit){
  tft.fillScreen(ILI9341_BLACK);  // Clear screen

  // First message
  tft.setCursor(10, 50);  // X=10, Y=50
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(2);
  if (hit) {
    tft.print("Missile hit target!");
  } else {
    tft.print("Missile miss target!");
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(SPEAKER_PIN, OUTPUT);

  handlingStartTFT();
  handlingWIFI();
  while(waiting_for_input_from_web) { // Wait for web input
    delay(100);
    server.handleClient(); 
  }

}

void loop() {
  // put your main code here, to run repeatedly:

  handlingEndTFT(true);  // Simulate a hit for demonstration purposes
  delay(5000);
  handlingStartTFT();
  delay(5000);  // Wait for 5 seconds before next iteration
}



  //for speaker control
  //tone(SPEAKER_PIN, 200);
  //noTone(SPEAKER_PIN);