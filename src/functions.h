void handlingWIFI();
void handlingStartTFT();
void handleRoot(); 
void handleUpload();
void handlingEndTFT(bool hit);
void showStartSimulationScreen();
void simulateMissileFlight();

#define BALLISTIC 0
#define POWERED 1

typedef struct Missile {
  bool type;        // "ballistic" or "powered"
  int launchSpeed;  // Launch speed in m/s
  int launchAngle;  // Launch angle in degrees
  float posX;         // Current X position
  float posY;         // Current Y position
  float velX;         // Current horizontal velocity
  float velY;         // Current vertical velocity
  bool launched;      // True if missile is launched
  bool hitTarget;     // True if missile hit target
  bool hitObstacle;   // True if missile hit obstacle
  unsigned long launchTime; // Timestamp of launch (millis())
} Missile;

typedef struct Target {
  bool type;
  float posX;         // Current X position
  float posY;         // Current Y position
  float velX;         // Current horizontal velocity
  float velY;         // Current vertical velocity
} Target;