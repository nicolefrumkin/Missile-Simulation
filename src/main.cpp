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

// wifi:
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6
WebServer server(80);                   // HTTP server on port 80
bool waiting_for_input_from_web = true; // Flag to indicate if we are waiting for input from the web interface

// speaker:
#define SPEAKER_PIN 13

//led
#define LED_PIN 12

Missile missile;
Target target;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; // milliseconds
bool buttonState = HIGH;          // Current state
bool lastButtonState = HIGH;      // Previous state
int MISSILE_SIZE = 20;            // Size of the missile triangle

void handlingWIFI()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot); // Set up HTML handler
  server.begin();
  Serial.println("Please open the following URL in your browser: http://localhost:8180/");
  Serial.println("And provide input parameters to the server.");
  server.on("/upload", handleUpload);
}

void handleRoot()
{
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

void handleUpload()
{
  String missile_type = server.arg("type");
  String speed = server.arg("speed");
  String angle = server.arg("angle");
  String route = server.arg("route");
  String target_type = server.arg("target");

  // Print values to serial for now
  Serial.println("=== Configuration Uploaded ===");
  Serial.println("Type: " + missile_type);
  Serial.println("Speed: " + speed);
  Serial.println("Angle: " + angle);
  Serial.println("Route: " + route);
  Serial.println("Target: " + target_type);

  // setting missile parameters
  if (missile_type == "ballistic")
  {
    missile.type = BALLISTIC;
  }
  else if (missile_type == "powered")
  {
    missile.type = POWERED;
  }

  // setting target parameters
  if (target_type == "static")
  {
    target.type = STATIC;
  }
  else if (target_type == "slow")
  {
    target.type = SLOW;
  }
  else if (target_type == "random")
  {
    target.type = RANDOM;
  }

  missile.launchSpeed = speed.toInt();
  missile.launchAngle = angle.toInt();
  missile.posX = 0; // Initial position
  missile.posY = 0; // Initial position
  missile.velX = missile.launchSpeed * cos(missile.launchAngle * PI / 180);
  missile.velY = missile.launchSpeed * sin(missile.launchAngle * PI / 180);
  missile.launched = false;    // Not launched yet
  missile.hitTarget = false;   // Not hit yet
  missile.hitObstacle = false; // Not hit yet
  missile.launchTime = 0;      // No launch time yet

  // Turn on LED if needed
  waiting_for_input_from_web = false; // Set flag to false to indicate we have received input
  server.send(200, "text/html", "<h2>Configuration Received! You may now launch the missile.</h2>");
}

void handlingStartTFT()
{
  tft.begin();

  tft.setRotation(3);  // Optional: rotate for horizontal layout
  tft.fillScreen(ILI9341_BLACK);  // Clear screen

  if (!ctp.begin()) {
    Serial.println("Couldn't start touchscreen");
  }

  // First message
  tft.setCursor(55, 100);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("Pending simulation");
  tft.setCursor(35, 120);
  tft.print("configuration from WEB");
}

void handlingEndTFT(bool hit)
{
  tft.fillScreen(ILI9341_BLACK); // Clear screen

  // First message
  tft.setTextSize(2);
  if (hit)
  {
    tft.setCursor(60, 100);
    tft.setTextColor(ILI9341_GREEN);
    tft.print("Missile hit target!");
  }
  else
  {
    if(missile.hitObstacle) {
      tft.setCursor(60, 100);
      tft.setTextColor(ILI9341_YELLOW);
      tft.print("Missile hit obstacle!");
    } else {
      tft.setCursor(60, 100);
      tft.setTextColor(ILI9341_RED);
      tft.print("Missile missed target!");
    }
  }
  delay(5000);
}
void calculateMissileTriangle(float angle)
{
  missile.x0 = missile.posX + MISSILE_SIZE * cos(angle);
  missile.y0 = missile.posY + MISSILE_SIZE * sin(angle);

  missile.x1 = missile.posX + MISSILE_SIZE * cos(angle + radians(150));
  missile.y1 = missile.posY + MISSILE_SIZE * sin(angle + radians(150));

  missile.x2 = missile.posX + MISSILE_SIZE * cos(angle - radians(150));
  missile.y2 = missile.posY + MISSILE_SIZE * sin(angle - radians(150));
}

void showStartSimulationScreen()
{
  tft.fillScreen(ILI9341_BLACK);

  // Missile position
  missile.posX = MISSILE_SIZE;                // Bottom-left corner X
  missile.posY = tft.height() - MISSILE_SIZE; // Bottom-left corner Y

  // Missile angle (convert degrees to radians)
  missile.launchAngle = radians(-missile.launchAngle);

  // Calculate missile triangle points
  calculateMissileTriangle(missile.launchAngle);

  // Draw missile (red triangle)
  tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_RED);

  addObstacles();

  // Target (blue circle)
  target.radius = 25;
  target.posX = tft.width() - target.radius - 5;
  target.posY = target.radius;
  tft.fillCircle(target.posX, target.posY, target.radius, ILI9341_BLUE);
}

void addObstacles() {
  String route = server.arg("route");
  // Draw obstacle (green rectangle)
  if (route == "none") {
    return; // No random obstacles
  }
  for (int i = 0; i < 4; i++) {
     if(route == "fixed") {
      // Fixed obstacles
      missile.obstacles[i].posX = OBSTACLE_BOX_X_MIN + i * 10; // Fixed positions
      missile.obstacles[i].posY = OBSTACLE_BOX_Y_MIN + i * 50;
      missile.obstacles[i].hit = false; 
      tft.fillRect(missile.obstacles[i].posX, missile.obstacles[i].posY, 15, 15, ILI9341_GREEN); // Draw fixed obstacles
     } else if (route == "random") {
      // Random obstacles
      missile.obstacles[i].posX = random(OBSTACLE_BOX_X_MIN, OBSTACLE_BOX_X_MAX);
      missile.obstacles[i].posY = random(OBSTACLE_BOX_Y_MIN, OBSTACLE_BOX_Y_MAX);
      missile.obstacles[i].hit = false; 
      tft.fillRect(missile.obstacles[i].posX, missile.obstacles[i].posY, 15, 15, ILI9341_GREEN); // Draw random obstacles
    }
  } 
}

void updateTargetPosition(float dt) {
  static float directionX = 1;
  static float directionY = 1;

  if (target.type == STATIC)
  {
    return;
  }

  Target old_target = target; // Save old target position
  
  if (target.type == SLOW)
  {
    target.posY+= directionY* 30 * dt; // slow horizontal movement
    if (target.posY >= TARGET_BOX_Y_MAX || target.posY < TARGET_BOX_Y_MIN)
    {
      directionY *= -1;
    }
  }

  if (target.type == RANDOM)
  {
    target.posX += random(-75, 75) * dt;
    target.posY += random(-75, 75) * dt;

    if (target.posX < TARGET_BOX_X_MIN)
      target.posX = TARGET_BOX_X_MIN;
    if (target.posX > TARGET_BOX_X_MAX)
      target.posX = TARGET_BOX_X_MAX;
    if (target.posY < TARGET_BOX_Y_MIN)
      target.posY = TARGET_BOX_Y_MIN;
    if (target.posY > TARGET_BOX_Y_MAX)
      target.posY = TARGET_BOX_Y_MAX;
  }
  tft.fillCircle(old_target.posX, old_target.posY, old_target.radius, ILI9341_BLACK); // Clear old target position
  tft.fillCircle(target.posX, target.posY, target.radius, ILI9341_BLUE);
}

void simulateMissileFlight()
{
  float dx = target.posX - missile.posX;

  missile.velX = map(missile.velX, 1, 100, 10, 32);       // speed in m/s for 320 width screen. 10 gives 30 sec, 32 gives 10 sec **fixme
  int travelTime = map(missile.velX, 1, 100, 30, 10);     // travel time in seconds
  missile.velY = missile.velX * tan(missile.launchAngle); // vertical velocity based on angle

  float framesPerSec = 10;
  int totalFrames = travelTime * framesPerSec;
  float g = 2.8; // gravity in m/s^2, random number need to explain
  TS_Point touchPoint;

  for (int i = 0; i < totalFrames; i++)
  {
    // Clear old missile
    Missile old_missile = missile; // Save old missile position
    if(missile.type == POWERED) {
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
    }
    //digitalWrite(LED_PIN, (i % 2 == 0) ? HIGH : LOW); // Blink LED every frame
    // Blink LED every 5 iterations
    if (i % 5 == 0) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Toggle LED
    }

    missile.posX += missile.velX/framesPerSec; // Update x position
    missile.posY += missile.velY/framesPerSec;
    missile.velY += g/framesPerSec; // Gravity effect (9.8 m/s^2)

    //calculate new angle
    missile.launchAngle = atan2(missile.velY,missile.velX);

    // Calculate rotated triangle points
    calculateMissileTriangle(missile.launchAngle);

    // Update target position every 5 frames
    if (i % 5 == 0) {
      updateTargetPosition(5.0 / framesPerSec);
    }

    // Draw missile (red triangle)
    tft.fillTriangle(old_missile.x0, old_missile.y0, old_missile.x1, old_missile.y1, old_missile.x2, old_missile.y2, ILI9341_BLACK);
    tft.fillTriangle(missile.x0, missile.y0, missile.x1, missile.y1, missile.x2, missile.y2, ILI9341_RED);

    if (outOfBounds()) {
      missTargetSound();
      missile.hitTarget = false;
      break;
    }  
    if (hitTarget()) {
      hitTargetSound();
      missile.hitTarget = true;
      break;
    }
    if (hitObstacle()){
      hitObstacleSound();
      missile.hitObstacle = true; // Mark missile as hit obstacle
      missile.hitTarget = false;
      break;
    }

    delay(1000 / framesPerSec);
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


bool outOfBounds()
{
  return missile.x0 < 0 || missile.x0 >= 320 || missile.y0 < 0 || missile.y0 >= 240;
}

bool hitTarget()
{
  float dx = target.posX - missile.x0; // Adjust for target radius
  float dy = target.posY - missile.y0;
  float distance = sqrt(dx * dx + dy * dy);

  return distance <= target.radius;
}

bool hitObstacle() {
  for (int i = 0; i < 4; i++) {
    // Check if missile.x0, y0 is inside the obstacle rectangle
    if (missile.x0 >= missile.obstacles[i].posX &&
        missile.x0 <= missile.obstacles[i].posX + 15 &&
        missile.y0 >= missile.obstacles[i].posY &&
        missile.y0 <= missile.obstacles[i].posY + 15) {
      missile.obstacles[i].hit = true;  // Mark obstacle as hit
      return true;  // Missile hit an obstacle
    }
  }
  return false;  // No obstacle hit
}

// Function to start launch
void launchSound()
{
  tone(SPEAKER_PIN, 350, 100);
  tone(SPEAKER_PIN, 500, 250);
}

// Function for hitting obstacle
void hitObstacleSound()
{
  tone(SPEAKER_PIN, 500, 250);
  tone(SPEAKER_PIN, 350, 100);
}

// Function for hitting target
void hitTargetSound()
{
  tone(SPEAKER_PIN, 350, 250);
  tone(SPEAKER_PIN, 425, 200);
  tone(SPEAKER_PIN, 500, 150);
}

// Function for missing target
void missTargetSound() {
  tone(SPEAKER_PIN, 400,150);
  tone(SPEAKER_PIN, 300,200);
  tone(SPEAKER_PIN, 200,200);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);  // Turn off LED initially
  handlingStartTFT();
  handlingWIFI();
  while (waiting_for_input_from_web)
  { // Wait for web input
    delay(100);
    server.handleClient();
  }
  digitalWrite(LED_PIN, HIGH);  // Turn on LED after reciecing web input
  showStartSimulationScreen();
  delay(2000); // Show simulation screen for 2 seconds
}

void loop()
{
  // put your main code here, to run repeatedly:
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState)
  {
    lastDebounceTime = millis(); // reset debounce timer
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading; // update stable state
      if (buttonState == LOW && !missile.launched)
      { // Button pressed
        launchSound();
        Serial.println("Button Pressed! Launching Missile...");
        missile.launched = true;
        simulateMissileFlight();
      }
    }
  }
  lastButtonState = reading;

  if (missile.launched)
  {
    handlingEndTFT(missile.hitTarget);
    missile.launched = false;
    waiting_for_input_from_web = true;
    delay(3000); // Wait for 3 seconds before next iteration
    setup();
  }
}
