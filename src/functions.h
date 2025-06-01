#define TARGET_BOX_X_MIN 220
#define TARGET_BOX_X_MAX 295
#define TARGET_BOX_Y_MIN 25
#define TARGET_BOX_Y_MAX 215
#define OBSTACLE_BOX_X_MIN 150
#define OBSTACLE_BOX_X_MAX 220
#define OBSTACLE_BOX_Y_MIN 25
#define OBSTACLE_BOX_Y_MAX 215
#define BALLISTIC 0
#define POWERED 1
#define NUMBER_OF_OBSTACLES 4

typedef enum
{
  STATIC = 0,
  SLOW = 1,
  RANDOM = 2
} TargetType;

typedef struct Obstacle {
  float posX;      // X coordinate
  float posY;      // Y coordinate
  bool hit;        // Whether the missile has hit the obstacle
} Obstacle;

typedef struct Missile
{
  bool type;                // "ballistic" or "powered"
  int launchSpeed;          // Launch speed in m/s
  float launchAngle;        // Launch angle in degrees
  float posX;               // Current X position
  float posY;               // Current Y position
  float velX;               // Current horizontal velocity
  float velY;               // Current vertical velocity
  bool launched;            // True if missile is launched
  bool hitTarget;           // True if missile hit target
  bool hitObstacle;         // True if missile hit obstacle
  unsigned long launchTime; // Timestamp of launch (millis())
  // triangle points
  int x0;
  int y0;
  int x1;
  int y1;
  int x2;
  int y2;
  Obstacle obstacles[4] ;
} Missile;

typedef struct Target
{
  TargetType type; // "static", "slow", or "random"
  float posX; // Current X position
  float posY; // Current Y position
  float velX; // Current horizontal velocity
  float velY; // Current vertical velocity
  int radius;
} Target;

void handlingWIFI();
void handlingStartTFT();
void handleRoot();
void handleUpload();
void handlingEndTFT(bool hit);
void showStartSimulationScreen();
void simulateMissileFlight();
bool outOfBounds();
bool hitTarget();
void launchSound();
void hitObstacleSound();
void hitTargetSound();
void missTargetSound();
TS_Point touchLocation();
void updateTargetPosition(float dt);
void addObstacles();
bool hitObstacle();
