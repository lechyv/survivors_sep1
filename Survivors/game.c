#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "math.h"
#include "Swarm.h"
/* ---------- Player state ---------- */
float playerX, playerY;     // arrow position
float speed;                // normal speed of movement
float orangeArrow;          // arrow rotation angle (degrees)

float dashSpeed;            // speed while dashing
float dashDistance;         // total distance of one dash
float dashRemaining;        // distance left in the current dash (0 = not dashing)
float dashCooldown;         // seconds between dashes
float dashCooldownTimer;    // seconds left until the next dash is allowed

/* ---------- Player controls ---------- */



// Space: start a dash if off cooldown and not already dashing
void Player_HandleDashInput(void)
{
	if (CP_Input_KeyTriggered(KEY_SPACE) && dashRemaining <= 0.0f && dashCooldownTimer <= 0.0f)
	{
		dashRemaining = dashDistance;
		dashCooldownTimer = dashCooldown;
	}
}


// Moves the arrow forward in the direction it is facing during a dash
void Player_Dash(void)
{//step is a variable that tracks how much distance remains on your dash
  //and makes sure any value you set for dashspeed does not overshoot. 
  //since once you preess spacebar, dashremaining = dashdistance,
  //it constantly gets updated frame by frame and decreases its pixels left
  //because step is assigned to dashspeed to frame by frame smooth out the dash effect,
  // it may overshoot when dashremaing is let's say 10pixels left and dashspeed is 20pixels.
  // this would of course cause the last instance of dash frame to exceed the distance by 10 pixels.
  // hence this is where step is assigned dash remaining, so the last frame instance is 10 pixels, instead of 20pixels.
	float step = dashSpeed;
	if (step > dashRemaining)
	{
		step = dashRemaining;
	}

	// move along whatever angle the arrow is facing (works for diagonals too)
	float radians = orangeArrow * 3.14159265f / 180.0f;
	playerX += cosf(radians) * step;
	playerY += sinf(radians) * step;

	dashRemaining -= step;
}
// WASD: normal movement and facing direction
// WASD: normal movement and facing direction (8 directions)
void Player_Move(void)
{
	float dx = 0.0f;
	float dy = 0.0f;

	if (CP_Input_KeyDown(KEY_W)) dy -= 1.0f;
	if (CP_Input_KeyDown(KEY_S)) dy += 1.0f;
	if (CP_Input_KeyDown(KEY_A)) dx -= 1.0f;
	if (CP_Input_KeyDown(KEY_D)) dx += 1.0f;

	// only move if at least one direction is held
	if (dx != 0.0f || dy != 0.0f)
	{
		// normalize so diagonal movement isn't faster than straight movement
		float length = sqrtf(dx * dx + dy * dy);
		dx /= length;
		dy /= length;

		playerX += dx * speed;
		playerY += dy * speed;

		// arrow angle in degrees (W = -90, S = 90, A = 180, D = 0, diagonals in between)
		orangeArrow = atan2f(dy, dx) * 180.0f / 3.14159265f;
	}
}

//  player function
 void Player_Update(void)
{
	// tick the dash cooldown down
	if (dashCooldownTimer > 0.0f)
	{
		dashCooldownTimer -= CP_System_GetDt();
	}

	Player_HandleDashInput();

	if (dashRemaining > 0.0f)
	{
		Player_Dash();      // WASD is ignored while dashing
	}
	else
	{
		Player_Move();
	}
}

// Draws an orange arrow
void Player_Draw(void)
{
	CP_Settings_Fill(CP_Color_Create(255, 165, 0, 255));
	CP_Graphics_DrawTriangleAdvanced(playerX + 30, playerY, playerX - 15, playerY - 25, playerX - 15, playerY + 25, orangeArrow);
}
/* ---------- Combat and Damage ---------- */
#define max_bullets 128
#define bullet_speed 600.0f
#define bullet_radius 4.0f
#define bullet_damage 10.0f
#define bullet_lifetime 2.0f	//seconds before the bullet disappears
#define player_radius 20.0f
#define player_health_max 100.0f

int bulletActive[max_bullets];
float bulletX[max_bullets];
float bulletY[max_bullets];
float bulletXv[max_bullets];	// velocity
float bulletYv[max_bullets];
float bulletAge[max_bullets];	//seconds since bullet fired

float player_health;

// collision

int combat_circles_touch(float x1, float y1, float r1, float x2, float y2, float r2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	float distancesquared = (dx * dx) + (dy * dy);
	float radius_sum = r1 + r2;

	if (distancesquared <= (radius_sum * radius_sum))
	{
		return 1;
	}
	return 0;
}

void combat_init(void)
{
	player_health = player_health_max;
	for (int i = 0; i < max_bullets; i++)
	{
		bulletActive[i] = 0;
	}
}

//player damage. dashing will avoid the damage.
void combat_damage_player(float amount)
{
	if (dashRemaining > 0.0f)
	{
		return;
	}

	player_health = player_health - amount;

	if (player_health < 0.0f)
	{
		player_health = 0.0f;
	}
}

//drone damage
void bullet_fire(float fromX, float fromY, float targetX, float targetY)
{
	float dx = targetX - fromX;
	float dy = targetY - fromY;
	float distance = sqrtf((dx * dx) + (dy * dy));

	if (distance < 0.01f)
	{
		return;
	}

	for (int i = 0; i < max_bullets; i++)
	{
		if (bulletActive[i] == 0)
		{
			bulletActive[i] = 1;
			bulletX[i] = fromX;
			bulletY[i] = fromY;
			bulletXv[i] = (dx / distance) * bullet_speed;
			bulletYv[i] = (dy / distance) * bullet_speed;
			bulletAge[i] = 0;
			break;
		}
	}
}

void bullet_update(void)
{
	float dt = CP_System_GetDt();
	float w = (float)CP_System_GetWindowWidth();
	float h = (float)CP_System_GetWindowHeight();

	for (int i = 0; i < max_bullets; i++)
	{
		if (bulletActive[i] == 0)
		{
			continue;
		}
		bulletX[i] = bulletX[i] + bulletXv[i] * dt;
		bulletY[i] = bulletY[i] + bulletYv[i] * dt;
		bulletAge[i] = bulletAge[i] + dt;

		if (bulletAge[i] > bullet_lifetime || bulletX[i] < -20.0f || bulletY[i] < -20.0f || bulletX[i] > w + 20.0f || bulletY[i] > h + 20.0f)
		{
			bulletActive[i] = 0;
		}
	}
}

float bullet_check_if_hit(float x, float y, float radius)
{
	float damage = 0.0f;
		for (int i = 0; i < max_bullets; i++)
		{
			if (bulletActive[i] == 1 && combat_circles_touch(bulletX[i], bulletY[i], bullet_radius, x, y, radius))
			{
				bulletActive[i] = 0;
				damage = damage + bullet_damage;
			}
		}
	return damage;
}

void bullet_draw(void)
{
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 230, 150, 255));
	for (int i = 0; i < max_bullets; i++)
	{
		if (bulletActive[i] == 1)
		{
			CP_Graphics_DrawCircle(bulletX[i], bulletY[i], bullet_radius * 2.0f);
		}
	}
}


/* ---------- Game state ---------- */

void Game_Init(void)
{
	Swarm_Init(selectedDifficulty); //select difficulty (placeholder for now) include "mainmenu.c" for this
	// arrow position on screen (spawn point)
	playerX = CP_System_GetWindowWidth() / 3.0f;
	playerY = CP_System_GetWindowHeight() / 6.0f;

	// speed of player
	speed = 5.0f;

	// arrow starts facing right
	orangeArrow = 0.0f;

	// dash settings
	dashSpeed = 20.0f;
	dashDistance = 150.0f;
	dashRemaining = 0.0f;
	dashCooldown = 3.0f;
	dashCooldownTimer = 0.0f;

	combat_init();
}

void Game_Update(void)
{
	// black background
	CP_Graphics_ClearBackground(CP_Color_Create(0, 0, 0, 255));

	Swarm_Update(CP_Vector_Set(playerX, playerY));
	Player_Update();
	bullet_update();
	Player_Draw();
	bullet_draw();

	//program termination
	if (CP_Input_KeyTriggered(KEY_Q))
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}
}

void Game_Exit(void)
{

}
