//---------------------------------------------------------
// file:    utils.h
// authors: everyone (difficulty table: Elvis, buttons: Wee Jie)
// brief:   Shared constants, math/drawing helpers, difficulty
//          settings and UI buttons used by both the menu and the game
//---------------------------------------------------------
#pragma once
#include "cprocessing.h"

//---------------------------------------------------------
// Game-wide constants
//---------------------------------------------------------
#define WINDOW_WIDTH      1280
#define WINDOW_HEIGHT     720
#define TARGET_FPS        60.0f

#define MAX_DRONES        6     // 6-drone cap from the project scope
#define MAX_ENEMIES       256
#define MAX_PROJECTILES   512
#define MAX_EFFECTS       128

#define SWARM_WAVE_COUNT  2     // 2 swarm waves, then the final boss

#define PI_F  3.14159265f
#define TAU_F 6.28318531f

//---------------------------------------------------------
// Math and drawing helpers
//---------------------------------------------------------
float     Util_MinF(float a, float b);
float     Util_MaxF(float a, float b);

// Returns a unit vector, or (0,0) if the vector is too short to normalize
CP_Vector Util_SafeNormalize(CP_Vector v);

// Keeps a circle of the given radius inside the window (fixed map, no camera)
CP_Vector Util_ClampToWindow(CP_Vector p, float radius);

int       Util_CirclesOverlap(CP_Vector a, float ra, CP_Vector b, float rb);
int       Util_IsOutsideWindow(CP_Vector p, float margin);

CP_Color  Util_Color(int r, int g, int b, int a);
CP_Color  Util_WithAlpha(CP_Color c, float alpha01);

// Notched arrowhead pointing along dir (used for drones, enemies and the boss)
void      Util_DrawArrow(CP_Vector center, CP_Vector dir, float size, CP_Color color);
void      Util_DrawTextCentered(const char* text, float x, float y, float size, CP_Color color);
// Text uses a built-in pixel font. y is the vertical middle of the text.
void      Util_DrawTextLeft(const char* text, float x, float y, float size, CP_Color color);
void      Util_DrawTextRight(const char* text, float x, float y, float size, CP_Color color);


//---------------------------------------------------------
// Difficulty (Elvis)
//---------------------------------------------------------
typedef enum DifficultyLevel
{
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_COUNT
} DifficultyLevel;

typedef struct DifficultySettings
{
	const char* name;
	float enemyHealth;     // multiplier on enemy HP
	float enemySpeed;      // multiplier on enemy move speed
	float enemyCount;      // multiplier on enemies per wave
	float spawnInterval;   // multiplier on time between spawns (lower = faster)
	float coinReward;      // multiplier on coins per kill
	float bossHealth;      // multiplier on boss HP
	float contactDamage;   // multiplier on damage enemies deal on touch
} DifficultySettings;

void                      Difficulty_Set(DifficultyLevel level);
DifficultyLevel           Difficulty_Get(void);
const DifficultySettings* Difficulty_Settings(void);
const char*               Difficulty_Name(DifficultyLevel level);

//---------------------------------------------------------
// Shared UI (Wee Jie)
//---------------------------------------------------------
// Center-positioned button. Returns 1 on the frame it is clicked.
int  UI_Button(float cx, float cy, float w, float h, const char* label, int enabled, int highlighted);
void UI_DrawDimOverlay(void);
