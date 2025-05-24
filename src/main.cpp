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

  server.send(200, "text/html", "<h2>Configuration Received! You may now launch the missile.</h2>");
}

void handlingTFT(){
  tft.begin();

  tft.setCursor(26, 120);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);
  tft.println("Hello, TFT!");

  tft.setCursor(20, 160);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.println("I can has colors?");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(SPEAKER_PIN, OUTPUT);

  handlingTFT();
  handlingWIFI();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(100);
  server.handleClient(); 
  

}



  //for speaker control
  //tone(SPEAKER_PIN, 200);
  //noTone(SPEAKER_PIN);