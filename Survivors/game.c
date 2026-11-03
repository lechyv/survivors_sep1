#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "math.h"
#include "Swarm.h"
#include "coin-drop.h"
#include "hud.h"
#include "pause.h"
#include "gameover.h"
#include "shop.h"

extern int coinCount;

/*Player state*/
float playerX, playerY;     // arrow position
float speed;                // player (arrow) speed
float orangeArrow;          // arrow rotation angle (degrees)

float dashSpeed;            // speed while dashing
float dashDistance;         // distance of dash
float dashRemaining;        // distance left in the current dash (0 = not dashing)
float dashCooldown;         // seconds between dashes
float dashCooldownTimer;  
float invulnTimer;          // seconds of invulnerability left after being hit
#define player_invuln_time 1.0f// seconds left until the next dash is allowed

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
	// blink while invulnerable: skip drawing every other 0.1s
	if (invulnTimer > 0.0f && ((int)(invulnTimer * 10.0f) % 2 == 0))
	{
		return;
	}
	CP_Settings_Fill(CP_Color_Create(255, 165, 0, 255));
	CP_Graphics_DrawTriangleAdvanced(playerX + 30, playerY, playerX - 15, playerY - 25, playerX - 15, playerY + 25, orangeArrow);
}
//Combat and Damage
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
float runTime;   


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
	invulnTimer = 0.0f;
	for (int i = 0; i < max_bullets; i++)
	{
		bulletActive[i] = 0;
		bulletDamage[i] = 0.0f;
	}
}

//player damage. dashing will avoid the damage.
void combat_damage_player(float amount)
{
	if (amount <= 0.0f || dashRemaining > 0.0f || invulnTimer > 0.0f)
	{
		return;
	}

	player_health = player_health - amount;
	if (player_health < 0.0f)
	{
		player_health = 0.0f;
	}

	invulnTimer = player_invuln_time;   // start invulnerability


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
#define machine_gun_drone_distance 60.0f
#define machine_gun_drone_orbit_speed 1.5f
#define machine_gun_drone_fire_rate 6.0f
#define machine_gun_drone_range 350.0f

float MGDroneX, MGDroneY;
float MGDroneAngle;
float MGDroneFireTimer;

void machine_gun_drone_init(void)
{
	MGDroneAngle = 2.09f;
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

//rocket drone
#define rocket_drone_size 18.0f
#define rocket_drone_distance 60
#define rocket_drone_orbit_speed 1.5f
#define rocket_drone_fire_rate 0.30f
#define rocket_drone_range 350.0f

#define max_rockets 32
#define rocket_speed 300.0f
#define rocket_radius 8.0f
#define rocket_damage 30.0f
#define rocket_AOE_radius 70.0f
#define rocket_lifetime 3.5f
#define AOE_duration 0.25f

float RDroneX, RDroneY;
float RDroneAngle;
float RDroneFireTimer;

int rocketActive[max_rockets];
float rocketX[max_rockets];
float rocketY[max_rockets];
float rocketXv[max_rockets];
float rocketYv[max_rockets];
float rocketAge[max_rockets];

int AOEActive[max_rockets];
float AOEX[max_rockets];
float AOEY[max_rockets];
float AOEAge[max_rockets];

void rocket_fire(float fromX, float fromY, float targetX, float targetY)
{
	float dx = targetX - fromX;
	float dy = targetY - fromY;
	float distance = sqrtf((dx * dx) + (dy * dy));

	if (distance < 0.01f)
	{
		return;
	}

	for (int i = 0; i < max_rockets; i++)
	{
		if (rocketActive[i] == 0)
		{
			rocketActive[i] = 1;

			rocketX[i] = fromX;
			rocketY[i] = fromY;

			rocketXv[i] = (dx / distance) * rocket_speed;
			rocketYv[i] = (dy / distance) * rocket_speed;

			rocketAge[i] = 0.0f;
			return;
		}
	}
}

void rocket_AOE(float x, float y)
{
	Swarm_AOE_Damage(x, y, rocket_AOE_radius, rocket_damage);

	for (int i = 0; i < max_rockets; i++)
	{
		if (AOEActive[i] == 0)
		{
			AOEActive[i] = 1;
			AOEX[i] = x;
			AOEY[i] = y;
			AOEAge[i] = 0.0f;
			return;
		}
	}
}

void rocket_update(void)
{
	float dt = CP_System_GetDt();
	float w = (float)CP_System_GetWindowWidth();
	float h = (float)CP_System_GetWindowHeight();

	for (int i = 0; i < max_rockets; i++)
	{
		if (rocketActive[i] == 0)
		{
			continue;
		}

		rocketAge[i] = rocketAge[i] + dt;

		if (rocketAge[i] > rocket_lifetime)
		{
			rocketActive[i] = 0;
			continue;
		}

		CP_Vector target;

		if (Swarm_GetNearest(rocketX[i], rocketY[i], rocket_drone_range, &target))
		{
			float dx = target.x - rocketX[i];
			float dy = target.y - rocketY[i];
			float distance = sqrtf((dx * dx) + (dy * dy));

			if (distance <= rocket_speed * dt + 26.0f)
			{
				rocketActive[i] = 0;
				rocket_AOE(target.x, target.y);
				continue;
			}

			if (distance > 0.01f)
			{
				rocketXv[i] = (dx / distance) * rocket_speed;
				rocketYv[i] = (dy / distance) * rocket_speed;
			}
		}

		rocketX[i] = rocketX[i] + rocketXv[i] * dt;
		rocketY[i] = rocketY[i] + rocketYv[i] * dt;

		if (rocketX[i] < -20.0f || rocketY[i] < -20.0f || rocketX[i] > w + 20.0f || rocketY[i] > h + 20.0f)
		{
			rocketActive[i] = 0;
		}
	}
	for (int i = 0; i < max_rockets; i++)
	{
		if (AOEActive[i] == 1)
		{
			AOEAge[i] = AOEAge[i] + dt;

			if (AOEAge[i] >= AOE_duration)
			{
				AOEActive[i] = 0;
			}
		}
	}
}

void rocket_drone_init(void)
{
	RDroneAngle = 4.19f;
	RDroneFireTimer = 0.0f;

	for (int i = 0; i < max_rockets; i++)
	{
		rocketActive[i] = 0;
		AOEActive[i] = 0;
	}
}

void rocket_drone_update(void)
{
	float dt = CP_System_GetDt();

	RDroneAngle = RDroneAngle + (rocket_drone_orbit_speed * dt);

	RDroneX = playerX + cosf(RDroneAngle) * rocket_drone_distance;
	RDroneY = playerY + sinf(RDroneAngle) * rocket_drone_distance;

	RDroneFireTimer = RDroneFireTimer - dt;

	if (RDroneFireTimer > 0.0f)
	{
		return;
	}

	CP_Vector target;

	if (Swarm_GetNearest(RDroneX, RDroneY, rocket_drone_range, &target) == 1)
	{
		rocket_fire(RDroneX, RDroneY, target.x, target.y);
		RDroneFireTimer = 1.0f / rocket_drone_fire_rate;
	}

}

void rocket_system_draw(void)
{
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(100, 160, 255, 255));
	CP_Graphics_DrawCircle(RDroneX, RDroneY, rocket_drone_size);

	CP_Settings_Fill(CP_Color_Create(255, 80, 40, 255));

	for (int i = 0; i < max_rockets; i++)
	{
		if (rocketActive[i] == 1)
		{
			CP_Graphics_DrawCircle(rocketX[i], rocketY[i], rocket_radius);
		}
	}

	for (int i = 0; i < max_rockets; i++)
	{
		if (AOEActive[i] == 1)
		{
			float AOEProgress = AOEAge[i] / AOE_duration;
			float AOESize = rocket_AOE_radius * 2.0f * AOEProgress;

			CP_Settings_Fill(CP_Color_Create(255, 140, 30, 150));
			CP_Graphics_DrawCircle(AOEX[i], AOEY[i], AOESize);
		}
	}
}
//shop: draws the rockets and explosions only, without the single rocket drone circle (added by Elvis)
//The shop now draws every drone, so rocket_system_draw is no longer called
//UI: rockets are red-orange circles, explosions are orange circles that grow, then vanish
void rocket_projectiles_draw(void)
{
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 80, 40, 255)); //UI: rocket colour (red-orange). Size is rocket_radius

	int i = 0;
	while (i < max_rockets)
	{
		if (rocketActive[i] == 1)
		{
			CP_Graphics_DrawCircle(rocketX[i], rocketY[i], rocket_radius);
		}
		i += 1;
	}

	i = 0;
	while (i < max_rockets)
	{
		if (AOEActive[i] == 1)
		{
			float AOEProgress = AOEAge[i] / AOE_duration;            //0.0 when it starts, 1.0 when it ends
			float AOESize = rocket_AOE_radius * 2.0f * AOEProgress;  //UI: grows to the full blast size (the real damage area)

			CP_Settings_Fill(CP_Color_Create(255, 140, 30, 150));   //UI: explosion colour (orange, half see-through)
			CP_Graphics_DrawCircle(AOEX[i], AOEY[i], AOESize);
		}
		i += 1;
	}
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
	dashCooldown = 2.0f;
	dashCooldownTimer = 0.0f;
	
	//PLEASE DONT DELETE THIS UNLESS YOU ARE DONE WITH DIFFICULTY HANDLING
	Swarm_Init(1,2); //taking in int value of (difficulty, current stage). (Placeholder for now until someone prompts diificulty stage)
	combat_init();
	drone_init();
	machine_gun_drone_init();
	rocket_drone_init();
	Coins_Init(); //coin drop: no coins on the ground, coin count back to 0
	Shop_Init(); //shop; player starts w 1 basic drone, shop closed(Elvis)
	Pause_Init();
	GameOver_Init();
	runTime = 0.0f;
}

void Game_Update(void)
{
	if (GameOver_IsActive() == 0 && Shop_IsOpen() == 0) //shop: ESC closes the shop instead of pausing while it is open
	{
		Pause_HandleInput();
	}
	//shop: B opens / closes the shop, not while paused or dead (added by Elvis)
	if (GameOver_IsActive() == 0 && Pause_IsPaused() == 0)
	{
		Shop_HandleInput();
	}

	CP_Graphics_ClearBackground(CP_Color_Create(45, 20, 75, 255));

	// ----- update: only while NOT paused and NOT dead -----
	//if (Pause_IsPaused() == 0 && GameOver_IsActive() == 0) (Brandon)
	if (Pause_IsPaused() == 0 && GameOver_IsActive() == 0 && Shop_IsOpen() == 0) //shop: game also freezes while the shop is open (Elvis)
	{
		runTime += CP_System_GetDt();

		Swarm_Update(CP_Vector_Set(playerX, playerY));
		Player_Update();
		//drone_update(); (Brandon)
		//machine_gun_drone_update(); (Brandon)
		//rocket_drone_update(); (Brandon)
		Shop_UpdateDrones(); // shop; move and fires every drone in squad (Elvis)
		rocket_update();
		bullet_update();

		if (invulnTimer > 0.0f)
		{
			invulnTimer -= CP_System_GetDt();
		}

		float contactDamage = Swarm_NumOfEnemiesTouching(playerX, playerY, player_radius) * enemy_contact_damage;
		float projectileDamage = Swarm_ProjectilesHitPlayer(playerX, playerY, player_radius);
		combat_damage_player(contactDamage + projectileDamage);
		// coin drop: kill enemies touching the player, drop a coin where each died,
		// then pick up any coin the player is touching
		Coins_Update(playerX, playerY, player_radius);
	}

	// ----- draw: always, so the game stays visible behind the overlays -----
	Swarm_Draw();
	Coins_Draw();          // coins are drawn under the player
	Player_Draw();
	//drone_draw();
	//machine_gun_drone_draw(); (added by Brandon)
	//rocket_system_draw(); (added by Brandon)
	Shop_DrawDrones(); //shop: draws every drone in the squad (added by Elvis)
	rocket_projectiles_draw(); //shop: rockets and explosions (added by Elvis)
	bullet_draw();
	HUD_Draw(player_health, player_health_max, coinCount, dashCooldownTimer, dashCooldown, runTime);
	
	Shop_Draw(); //shop menu, press "B" on keyboard
	Pause_Draw();

	// player just died: start the game over screen
	if (player_health <= 0.0f && GameOver_IsActive() == 0)
	{
		GameOver_Start();
	}
	GameOver_Draw(coinCount, runTime);  

	// back to menu
	if (CP_Input_KeyTriggered(KEY_Q))
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}
}



void Game_Exit(void)
{

}
