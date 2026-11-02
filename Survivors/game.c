#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "math.h"
#include "Swarm.h"
#include "coin-drop.h"
#include "hud.h"
#include "pause.h"

extern int coinCount;

/*Player state*/
float playerX, playerY;     // arrow position
float speed;                // player (arrow) speed
float orangeArrow;          // arrow rotation angle (degrees)

float dashSpeed;            // speed while dashing
float dashDistance;         // distance of dash
float dashRemaining;        // distance left in the current dash (0 = not dashing)
float dashCooldown;         // seconds between dashes
float dashCooldownTimer;    // seconds left until the next dash is allowed

#define PI 3.14159265f

/*Player controls*/
// Space: start a dash if off cooldown and not already dashing
void Player_HandleDashInput(void)
{
	if (CP_Input_KeyTriggered(KEY_SPACE) && dashRemaining <= 0.0f && dashCooldownTimer <= 0.0f)
	{
		dashRemaining = dashDistance;
		dashCooldownTimer = dashCooldown;
	}
}

void Player_Dash(void)
{/*step is a variable that tracks how much distance remains on your dash
  and makes sure any value you set for dashspeed does not overshoot. 
  since once you preess spacebar, dashremaining = dashdistance,
  it constantly gets updated frame by frame and decreases its pixels left
  because step is assigned to dashspeed to frame by frame smooth out the dash effect,
  it may overshoot when dashremaing is let's say 10pixels left and dashspeed is 20pixels.
  this would of course cause the last instance of dash frame to exceed the distance by 10 pixels.
  hence this is where step is assigned dash remaining, so the last frame instance is 10 pixels, instead of 20pixels. */
	float step = dashSpeed;
	if (step > dashRemaining)
	{
		step = dashRemaining;
	}

	// move along whatever angle the arrow is facing (works for diagonals too)
	float radians = orangeArrow * PI / 180.0f;
	/*For the next 2 lines of code, cosf(horizontal axis) and sinf(vertical axis)
	determine where the player is facing in the range of 1 to -1. Picture the dash as a 
	ray equation: P = O + td whereby playerX and playerY are starting point O, and step 
	acts as a scalar multiple for td.*/
	playerX += cosf(radians) * step;
	playerY += sinf(radians) * step;

	dashRemaining -= step;
}

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
		orangeArrow = atan2f(dy, dx) * 180.0f / PI;
	}
}

//player function
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
		Player_Move(); //player is always in this same unless dashing. Hence when dashing, the last known position of orangearrow is passed to Player_Dash

	}
}

// Draws an orange arrow
void Player_Draw(void)
{
	CP_Settings_Fill(CP_Color_Create(255, 165, 0, 255));
	CP_Graphics_DrawTriangleAdvanced(playerX + 30, playerY, playerX - 15, playerY - 25, playerX - 15, playerY + 25, orangeArrow);
}
/*Combat and Damage*/
#define max_bullets 128
#define bullet_speed 200.0f
#define bullet_radius 4.0f
#define bullet_lifetime 1.5f	//seconds before the bullet disappears
#define player_radius 20.0f
#define player_health_max 100.0f
#define enemy_contact_damage 10.0f

#define basic_drone_bullet_damage 10.0f
#define machine_gun_drone_bullet_damage 5.0f

int bulletActive[max_bullets];
float bulletX[max_bullets];
float bulletY[max_bullets];
float bulletXv[max_bullets];	// velocity
float bulletYv[max_bullets];
float bulletAge[max_bullets];	//seconds since bullet fired
float bulletDamage[max_bullets];
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
		bulletDamage[i] = 0.0f;
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
void bullet_fire(float fromX, float fromY, float targetX, float targetY, float damage)
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
			bulletAge[i] = 0.0f;
			bulletDamage[i] = damage;
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
				damage = damage + bulletDamage[i];
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

void healthbar_draw(void)
{
	float bar_width = 200.0f;
	float bar_height = 20.0f;
	float x = 20.0f;
	float y = (float)CP_System_GetWindowHeight() - 40.0f;
	float fill = player_health / player_health_max;

	CP_Settings_NoStroke();
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(CP_Color_Create(60, 60, 60, 255));
	CP_Graphics_DrawRect(x, y, bar_width, bar_height);
	CP_Settings_Fill(CP_Color_Create(220, 40, 40, 255));
	CP_Graphics_DrawRect(x, y, bar_width * fill, bar_height);
	CP_Settings_RectMode(CP_POSITION_CENTER);
}
//Starting Drone
#define drone_size 16.0f
#define drone_distance 60.0f
#define drone_orbit_speed 1.5f
#define drone_fire_rate 1.5f
#define drone_range 350.0f

float droneX, droneY;
float droneAngle;
float droneFireTimer;

void drone_init(void)
{
	droneAngle = 0.0f;
	droneFireTimer = 0.0f;
}

void drone_update(void)
{
	float dt = CP_System_GetDt();
	droneAngle = droneAngle + drone_orbit_speed * dt;
	droneX = playerX + cosf(droneAngle) * drone_distance;
	droneY = playerY + sinf(droneAngle) * drone_distance;

	droneFireTimer = droneFireTimer - dt;
	if (droneFireTimer > 0.0f)	// this restricts the fire rate so that it does not shoot constantly
	{
		return;
	}

	CP_Vector target;
	if (Swarm_GetNearest(droneX, droneY, drone_range, &target))
	{
		bullet_fire(droneX, droneY, target.x, target.y, basic_drone_bullet_damage);
		droneFireTimer = 1.0f / drone_fire_rate;
	}

}

void drone_draw(void)
{
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(80, 220, 170, 244));
	CP_Graphics_DrawCircle(droneX, droneY, drone_size);
}

//machine gun drone

#define machine_gun_drone_size 14.0f
#define machine_gun_drone_distance 90.0f
#define machine_gun_drone_orbit_speed 1.2f
#define machine_gun_drone_fire_rate 6.0f
#define machine_gun_drone_range 350.0f

float MGDroneX, MGDroneY;
float MGDroneAngle;
float MGDroneFireTimer;

void machine_gun_drone_init(void)
{
	MGDroneAngle = 3.14f;
	MGDroneFireTimer = 0.0f;
}

void machine_gun_drone_update(void)
{
	float dt = CP_System_GetDt();

	MGDroneAngle = MGDroneAngle + machine_gun_drone_orbit_speed * dt;

	MGDroneX = playerX + cosf(MGDroneAngle) * machine_gun_drone_distance;
	MGDroneY = playerY + sinf(MGDroneAngle) * machine_gun_drone_distance;

	MGDroneFireTimer = MGDroneFireTimer - dt;

	if (MGDroneFireTimer > 0.0f)
	{
		return;
	}

	CP_Vector target;

	if (Swarm_GetNearest(MGDroneX, MGDroneY, machine_gun_drone_range, &target))
	{
		bullet_fire(MGDroneX, MGDroneY, target.x, target.y, machine_gun_drone_bullet_damage);
		MGDroneFireTimer = 1.0f / machine_gun_drone_fire_rate;
	}
}

void machine_gun_drone_draw(void)
{
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 180, 50, 255));
	CP_Graphics_DrawCircle(MGDroneX, MGDroneY, machine_gun_drone_size);
}


/*Game state*/

void Game_Init(void)
{
	//have a function for difficulty (include mainmenu.h in this file
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
	
	//PLEASE DONT DELETE THIS UNLESS YOU ARE DONE WITH DIFFICULTY HANDLING
	Swarm_Init(1,2); //taking in int value of (difficulty, current stage). (Placeholder for now until someone prompts diificulty stage)
	combat_init();
	drone_init();
	machine_gun_drone_init();
	Coins_Init(); //coin drop: no coins on the ground, coin count back to 0
	Pause_Init();
}

void Game_Update(void)
{
	Pause_HandleInput();   // ESC or P toggles pause

	// black background
	CP_Graphics_ClearBackground(CP_Color_Create(0, 0, 0, 255));

	// ----- update: only while NOT paused -----
	if (Pause_IsPaused() == 0)
	{
		Swarm_Update(CP_Vector_Set(playerX, playerY));
		Player_Update();
		drone_update();
		machine_gun_drone_update();
		bullet_update();

		int enemies_touching_player = Swarm_NumOfEnemiesTouching(playerX, playerY, player_radius);
		combat_damage_player(enemies_touching_player * enemy_contact_damage);
		combat_damage_player(Swarm_ProjectilesHitPlayer(playerX, playerY, player_radius));
		// coin drop: kill enemies touching the player, drop a coin where each died,
		// then pick up any coin the player is touching
		Coins_KillAndDrop(playerX, playerY, player_radius);
		Coins_Update(playerX, playerY, player_radius);
	}

	// ----- draw: always, so the game stays visible behind the pause menu -----
	Swarm_Draw();
	Coins_Draw();          // coins are drawn under the player
	Player_Draw();
	drone_draw();
	machine_gun_drone_draw();
	bullet_draw();
	HUD_Draw(player_health, player_health_max, coinCount, dashCooldownTimer, dashCooldown);

	Pause_Draw();    

	if (player_health <= 0.0f)
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}
}


void Game_Exit(void)
{

}
