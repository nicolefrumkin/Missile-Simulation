void handlingWIFI();
void handlingStartTFT();
void handleRoot(); 
void handleUpload();
void handlingEndTFT(bool hit);
void showStartSimulationScreen();

#define BALLISTIC 0
#define POWERED 1

struct Missile {
  bool type;        // "ballistic" or "powered"
  int launchSpeed;  // Launch speed in m/s
  int launchAngle;  // Launch angle in degrees
  int posX;         // Current X position
  int posY;         // Current Y position
  int velX;         // Current horizontal velocity
  int velY;         // Current vertical velocity
  bool launched;      // True if missile is launched
  bool hitTarget;     // True if missile hit target
  bool hitObstacle;   // True if missile hit obstacle
  unsigned long launchTime; // Timestamp of launch (millis())
};