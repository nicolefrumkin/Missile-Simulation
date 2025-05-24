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
#define BUTTON_PIN 15
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//wifi:
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6
WebServer server(80);  // HTTP server on port 80
bool waiting_for_input_from_web = true;  // Flag to indicate if we are waiting for input from the web interface

//speaker:
#define SPEAKER_PIN 13

Missile missile;
Target target;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;  // milliseconds
bool buttonState = HIGH;            // Current state
bool lastButtonState = HIGH;        // Previous state

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
  delay(5000);
}

void showStartSimulationScreen() {
  tft.fillScreen(ILI9341_BLACK);

  // Missile position
  missile.posX = 20;                     // Bottom-left corner X
  missile.posY = tft.height() - 20;      // Bottom-left corner Y

  // Missile angle (convert degrees to radians)
  missile.launchAngle = radians(-missile.launchAngle);
  // Missile triangle dimensions
  int size = 20;

  // Calculate rotated triangle points
  missile.x0 = missile.posX + size * cos(missile.launchAngle);
  missile.y0 = missile.posY + size * sin(missile.launchAngle);

  missile.x1 = missile.posX + size * cos(missile.launchAngle + radians(150));
  missile.y1 = missile.posY + size * sin(missile.launchAngle + radians(150));

  missile.x2 = missile.posX + size * cos(missile.launchAngle - radians(150));
  missile.y2 = missile.posY + size * sin(missile.launchAngle - radians(150));

  // Draw missile (red triangle)
  tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_RED);

  // Target (blue circle)
  target.posX = tft.width() - 25;
  target.posY = 25;
  tft.fillCircle(target.posX, target.posY, 25, ILI9341_BLUE);
}

void simulateMissileFlight() {
  float dx = target.posX - missile.posX;

  int travelTime = map(missile.launchSpeed, 1, 100, 60, 20);  // seconds
  int totalFrames = travelTime*10;
  float dxPerFrame = dx / totalFrames;

  for (int i = 0; i < totalFrames; i++) {
    float start_time = millis();

    // Clear old missile
    tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_BLACK);
    // Update position
    missile.posX += dxPerFrame;
    // simple arc with parabolic Y motion
    //missile.posY = (tft.height() - 20) - (0.002 * (i * dxPerFrame) * (i * dxPerFrame));  // simple parabola
    float pixelToMeter = 0.002;
    int t = 1;
    int a = 9.8;
    missile.velY -= a*(travelTime/totalFrames);
    missile.posY -= missile.velY*pixelToMeter;
    Serial.println("missile.velY: ");
    Serial.println(missile.velY);
    Serial.println("missile.posY: ");
    Serial.println(missile.posY);
    // Draw missile
    // Missile triangle dimensions
    int size = 20;
    missile.launchAngle = (-1)*atan2(missile.velY,missile.velX);
    // Calculate rotated triangle points
    missile.x0 = missile.posX + size * cos(missile.launchAngle);
    missile.y0 = missile.posY + size * sin(missile.launchAngle);

    missile.x1 = missile.posX + size * cos(missile.launchAngle + radians(150));
    missile.y1 = missile.posY + size * sin(missile.launchAngle + radians(150));

    missile.x2 = missile.posX + size * cos(missile.launchAngle - radians(150));
    missile.y2 = missile.posY + size * sin(missile.launchAngle - radians(150));

    // Draw missile (red triangle)
    tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_RED);
    delay(100);  
  }
  float dist = sqrt(pow(missile.posX - target.posX, 2) + pow(missile.posY - target.posY, 2));
  if (dist < 25) {
    handlingEndTFT(true);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  handlingStartTFT();
  handlingWIFI();
  while(waiting_for_input_from_web) { // Wait for web input
    delay(100);
    server.handleClient(); 
  }
  showStartSimulationScreen();
  delay(2000);  // Show simulation screen for 2 seconds
}

void loop() {
  // put your main code here, to run repeatedly:
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();  // reset debounce timer
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && buttonState == HIGH) {
      // Button was just pressed (transition from HIGH to LOW)
      missile.launched = true;
      Serial.println("Missile Launched!");
      simulateMissileFlight();
    }
    buttonState = reading;  // update stable state
  }
  lastButtonState = reading;

  //handlingEndTFT(true);  // Simulate a hit for demonstration purposes
  //handlingStartTFT();
  //delay(5000);  // Wait for 5 seconds before next iteration
}

  //for speaker control
  //tone(SPEAKER_PIN, 200);
  //noTone(SPEAKER_PIN);