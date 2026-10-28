#include "swarm.h"

#define MAX_ENEMIES 200
#define ENEMY_SIZE  24.0f
#define ENEMY_SPEED 120.0f

/* One slot per enemy: same index = same enemy */
static CP_Vector enemyPos[MAX_ENEMIES];
static int enemyActive[MAX_ENEMIES];

static float spawnInterval = 1.0f;
static float spawnTimer = 0.0f;

/* Difficulty -> spawn interval (seconds between spawns) */
static void Set_Difficulty(int difficulty)
{
    switch (difficulty)
    {
    case 1:  spawnInterval = 1.5f; break;   /* easy   */
    case 2:  spawnInterval = 0.8f; break;   /* medium */
    case 3:  spawnInterval = 0.3f; break;   /* hard   */
    default: spawnInterval = 1.0f; break;
    }
}

/* Random spot on the screen edge */
static CP_Vector Get_Spawn_Location(void)
{
    float w = (float)CP_System_GetWindowWidth();
    float h = (float)CP_System_GetWindowHeight();

    switch (CP_Random_RangeInt(0, 3))
    {
    case 0:  return CP_Vector_Set(CP_Random_RangeFloat(0, w), 0);   /* top    */
    case 1:  return CP_Vector_Set(CP_Random_RangeFloat(0, w), h);   /* bottom */
    case 2:  return CP_Vector_Set(0, CP_Random_RangeFloat(0, h));   /* left   */
    default: return CP_Vector_Set(w, CP_Random_RangeFloat(0, h));   /* right  */
    }
}

static void Draw_Enemy(float x, float y)
{
    CP_Settings_Fill(CP_Color_Create(160, 60, 220, 255));
    CP_Graphics_DrawCircle(x, y, ENEMY_SIZE);
}

/* Spawn: asks the location function for a spot, then draws the enemy there */
static void Spawn_Enemy(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemyActive[i] == 0)
        {
            enemyPos[i] = Get_Spawn_Location();
            enemyActive[i] = 1;
            Draw_Enemy(enemyPos[i].x, enemyPos[i].y);
            return;
        }
    }
}

/* Movement: every enemy follows the player's current location */
static void Move_Enemies(CP_Vector playerPos)
{
    float dt = CP_System_GetDt();

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemyActive[i] == 0) continue;

        CP_Vector dir = CP_Vector_Normalize(CP_Vector_Subtract(playerPos, enemyPos[i]));
        enemyPos[i] = CP_Vector_Add(enemyPos[i], CP_Vector_Scale(dir, ENEMY_SPEED * dt));
        Draw_Enemy(enemyPos[i].x, enemyPos[i].y);
    }
}

void Swarm_Init(int difficulty)
{
    CP_Random_Seed(12345);   /* fixed seed: change the number for a different pattern */
    Set_Difficulty(difficulty);
    spawnTimer = 0.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) enemyActive[i] = 0;
}

void Swarm_Update(CP_Vector playerPos)
{
    spawnTimer += CP_System_GetDt();
    if (spawnTimer >= spawnInterval)
    {
        Spawn_Enemy();
        spawnTimer = 0.0f;
    }
    Move_Enemies(playerPos);
}

int Swarm_KillEnemiesTouching(float x, float y, float radius, float* killedX, float* killedY, int maxKilled)
{
    float touchDistance = radius + ENEMY_SIZE / 2.0f; /* ENEMY_SIZE is a diameter*/
    int killed = 0;
    int i = 0;

	while (i < MAX_ENEMIES)
	{
		if (enemyActive[i] == 1 && killed < maxKilled)
		{
			float dx = enemyPos[i].x - x;
			float dy = enemyPos[i].y - y;
			if ((dx * dx + dy * dy) <= (touchDistance * touchDistance))
			{
				enemyActive[i] = 0; /* enemy is dead */
				killedX[killed] = enemyPos[i].x; /* remember where it died*/
				killedY[killed] = enemyPos[i].y;
                killed += 1;
			}
		}
		i += 1;
	}
	return killed;
}