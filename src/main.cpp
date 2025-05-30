#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_FT6206.h"
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
#define TOUCH_SDA  21
#define TOUCH_SCL  22
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_FT6206 ctp = Adafruit_FT6206();

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
int MISSILE_SIZE = 20;  // Size of the missile triangle

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

  if (!ctp.begin()) {
    Serial.println("Couldn't start touchscreen");
  }

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
    tft.print("Missile missed target!");
  }
  delay(5000);
}
void calculateMissileTriangle(float angle) {
  missile.x0 = missile.posX + MISSILE_SIZE * cos(angle);
  missile.y0 = missile.posY + MISSILE_SIZE * sin(angle);

  missile.x1 = missile.posX + MISSILE_SIZE * cos(angle + radians(150));
  missile.y1 = missile.posY + MISSILE_SIZE * sin(angle + radians(150));

  missile.x2 = missile.posX + MISSILE_SIZE * cos(angle - radians(150));
  missile.y2 = missile.posY + MISSILE_SIZE * sin(angle - radians(150));
}

void showStartSimulationScreen() {
  tft.fillScreen(ILI9341_BLACK);

  // Missile position
  missile.posX = MISSILE_SIZE;                     // Bottom-left corner X
  missile.posY = tft.height() - MISSILE_SIZE;      // Bottom-left corner Y

  // Missile angle (convert degrees to radians)
  missile.launchAngle = radians(-missile.launchAngle);

  // Calculate missile triangle points
  calculateMissileTriangle(missile.launchAngle);

  // Draw missile (red triangle)
  tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_RED);

  // Target (blue circle)
  target.radius = 25;
  target.posX = tft.width() - target.radius;
  target.posY = target.radius;
  tft.fillCircle(target.posX, target.posY, target.radius, ILI9341_BLUE);
}

void simulateMissileFlight() {
  float dx = target.posX - missile.posX;

  missile.velX = map(missile.velX, 1, 100, 10, 32);  // speed in m/s for 320 width screen. 10 gives 30 sec, 32 gives 10 sec **fixme
  int travelTime = map(missile.velX, 1, 100, 30, 10); // travel time in seconds
  missile.velY = missile.velX * tan(missile.launchAngle); // vertical velocity based on angle
  
  float framesPerSec = 10;
  int totalFrames = travelTime*framesPerSec;
  float g = 2.8; // gravity in m/s^2, random number need to explain

  for (int i = 0; i < totalFrames; i++) {
    // Clear old missile
    tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_BLACK);

    missile.posX += missile.velX/framesPerSec; // Update x position
    missile.posY += missile.velY/framesPerSec;
    missile.velY += g/framesPerSec; // Gravity effect (9.8 m/s^2)
    
    //calculate new angle
    Serial.print("Missile angle: ");
    Serial.println(missile.launchAngle * 180 / PI); // Convert radians to degrees for display
    missile.launchAngle = atan2(missile.velY,missile.velX);

    // Calculate rotated triangle points
    calculateMissileTriangle(missile.launchAngle);

    // Draw missile (red triangle)
    tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_RED);
    delay(1000/framesPerSec);

    if (outOfBounds()) {
      missile.hitTarget = false;
      break;
    }  
    if (hitTarget()) {
      missile.hitTarget = true;
      break;
    }
  }
  Serial.println("Flight simulation ended");
}

void simulatePoweredMissileFlight() {
  float dx = target.posX - missile.posX;

  missile.velX = map(missile.velX, 1, 100, 10, 32);  // speed in m/s for 320 width screen. 10 gives 30 sec, 32 gives 10 sec **fixme
  int travelTime = map(missile.velX, 1, 100, 30, 10); // travel time in seconds
  missile.velY = missile.velX * tan(missile.launchAngle); // vertical velocity based on angle
  
  float framesPerSec = 10;
  int totalFrames = travelTime*framesPerSec;
  float g = 2.8; // gravity in m/s^2, random number need to explain
  TS_Point touchPoint;

  for (int i = 0; i < totalFrames; i++) {
    // Clear old missile
    tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_BLACK);

    touchPoint = touchLocation(); // Touch X and Y are the opposite of the screen X Y axis so we use here X from touch as Y for screen!
    if (touchPoint.x > 0) {
      if(touchPoint.x > missile.y0) { // pressed under the missile
        missile.velY += 20/framesPerSec; // randomly testing decreasing val
      } else if (touchPoint.x < missile.y0) { // pressed above the missile
        missile.velY -= 20/framesPerSec; // randomly testing increasin val
      }
      touchPoint = TS_Point(); // Reset touch point after handling
      Serial.print("Missile Ylocation: ");
      Serial.println(missile.posY);
      Serial.print("Missile Y vel: ");
      Serial.println(missile.velY);
    }

    missile.posX += missile.velX/framesPerSec; // Update x position
    missile.posY += missile.velY/framesPerSec;
    missile.velY += g/framesPerSec; // Gravity effect (9.8 m/s^2)

    //calculate new angle
    //Serial.print("Missile angle: ");
    //Serial.println(missile.launchAngle * 180 / PI); // Convert radians to degrees for display
    missile.launchAngle = atan2(missile.velY,missile.velX);

    // Calculate rotated triangle points
    calculateMissileTriangle(missile.launchAngle);

    // Draw missile (red triangle)
    tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_RED);
    delay(1000/framesPerSec);

    if (outOfBounds()) {
      missile.hitTarget = false;
      break;
    }  
    if (hitTarget()) {
      missile.hitTarget = true;
      break;
    }
  }
  Serial.println("Flight simulation ended");
}


TS_Point touchLocation() {

  if (ctp.touched()) {
    TS_Point p = ctp.getPoint();
    Serial.print("Touch at X: "); Serial.print(p.x);
    Serial.print(", Y: "); Serial.println(p.y);
    // Example: If touch detected, launch missile
    if (!missile.launched) {
      launchSound();
      missile.launched = true;
      simulateMissileFlight();
    }
    return p;  // Return touch point
  }
  return TS_Point();  // No touch detected
}

bool outOfBounds() {
  return missile.x0 < 0 || missile.x0 >= 320 || missile.y0 < 0 || missile.y0 >= 240;
}

bool hitTarget() {
  float dx = missile.x0 - target.posX;
  float dy = missile.y0 - target.posY;
  float distance = sqrt(dx * dx + dy * dy);

  return distance <= target.radius;  
}

// Function to start launch
void launchSound() {
  tone(SPEAKER_PIN, 350,100);
  tone(SPEAKER_PIN, 500,250);
}

// Function for hitting obstacle
void hitObstacleSound() {
  tone(SPEAKER_PIN, 500,250);
  tone(SPEAKER_PIN, 350,100);
}

// Function for hitting target
void hitTargetSound() {
  tone(SPEAKER_PIN, 350,250);
  tone(SPEAKER_PIN, 425,200);
  tone(SPEAKER_PIN, 500,150);
}

// Function for missing target
void missTargetSound() {
  tone(SPEAKER_PIN, 400,200);
  tone(SPEAKER_PIN, 300,200);
  tone(SPEAKER_PIN, 200,200);
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
    if (reading != buttonState) {
      buttonState = reading;  // update stable state
      if(buttonState== LOW && !missile.launched) {  // Button pressed
        launchSound();
        Serial.println("Button Pressed! Launching Missile...");
        missile.launched = true;
        if(missile.type == POWERED) {
          simulatePoweredMissileFlight();
        } else if(missile.type == BALLISTIC) {
          simulateMissileFlight();
        }
      }
    }
  }
  lastButtonState = reading;
  
  if (missile.launched) {
    handlingEndTFT(missile.hitTarget); 
    missile.launched = false; 
    waiting_for_input_from_web = true;
    delay(3000);  // Wait for 3 seconds before next iteration
    setup();
  }
}
