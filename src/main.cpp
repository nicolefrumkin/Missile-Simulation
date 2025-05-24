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

//missile:
Missile missile;

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

        <label>Missile Launch Speed (1-100):</label>
        <input type="number" name="speed" min="1" max="100" value="50"><br><br>

        <label>Missile Launch Angle (0-90):</label>
        <input type="number" name="angle" min="0" max="90" value="45"><br><br>

        <label>Route Difficulty:</label>
        <select name="route">
          <option value="none">No obstacles</option>
          <option value="fixed">Fixed obstacles</option>
          <option value="random">Random obstacles</option>
        </select><br><br>

        <label>Target Difficulty:</label>
        <select name="target">
          <option value="static">Static Target</option>
          <option value="slow">Slow Moving Target</option>
          <option value="random">Random Moving Target</option>
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

  //setting missile parameters
  if (type == "ballistic") {
    missile.type = BALLISTIC;
  } else if (type == "powered") {
    missile.type = POWERED;
  }
  missile.launchSpeed = speed.toInt();
  missile.launchAngle = angle.toInt();
  missile.posX = 0;  // Initial position
  missile.posY = 0;  // Initial position
  missile.velX = missile.launchSpeed * cos(missile.launchAngle * PI / 180);
  missile.velY = missile.launchSpeed * sin(missile.launchAngle * PI / 180);
  missile.launched = false;  // Not launched yet
  missile.hitTarget = false;  // Not hit yet
  missile.hitObstacle = false;  // Not hit yet
  missile.launchTime = 0;  // No launch time yet

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

void showStartSimulationScreen() {
  tft.fillScreen(ILI9341_BLACK);

  // Missile position
  int missileX = 20;                     // Bottom-left corner X
  int missileY = tft.height() - 20;      // Bottom-left corner Y

  // Missile angle (convert degrees to radians)
  float angleRad = radians(-missile.launchAngle);  // Negative because Y-axis is inverted

  // Missile triangle dimensions
  int size = 20;

  // Calculate rotated triangle points
  int x0 = missileX + size * cos(angleRad);
  int y0 = missileY + size * sin(angleRad);

  int x1 = missileX + size * cos(angleRad + radians(150));
  int y1 = missileY + size * sin(angleRad + radians(150));

  int x2 = missileX + size * cos(angleRad - radians(150));
  int y2 = missileY + size * sin(angleRad - radians(150));

  // Draw missile (red triangle)
  tft.fillTriangle(x0, y0, x1, y1, x2, y2, ILI9341_RED);

  // Target (blue circle)
  int targetX = tft.width() - 25;
  int targetY = 25;
  tft.fillCircle(targetX, targetY, 25, ILI9341_BLUE);
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
  showStartSimulationScreen();
  delay(10000);  // Show simulation screen for 2 seconds
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