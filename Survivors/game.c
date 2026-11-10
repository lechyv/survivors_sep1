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
#include "boss.h"
#include "diff.h"
#include "wave.h"
#include "buff.h"
#include "win.h"

	extern int coinCount;

	/*Player state*/
	float playerX, playerY;     // x,y position of player
	float speed;                // player speed
#define player_radius 20.0f	    
	float player_rotation;       //rotation angle (degrees) to determine dash direction

	float dashSpeed;            // speed while dashing
	float dashDistance;         // distance of dash
	float dashRemaining;        // distance left in the current dash (0 = not dashing)
	float dashCooldown;         // seconds between dashes
	float dashCooldownTimer;	// seconds left until the next dash is allowed
	float invulnTimer;          // seconds of invulnerability left after being hit
	#define player_invuln_time 1.0f //Period of invulnerability

	#define world_width 2500.0f
	#define world_height 2500.0f

	float cameraX;
	float cameraY;

	#define swarm_duration 45.0f //duration of each swarm
	float swarmTimer;
	int currentSwarm;

	#define PI 3.14159265f

	//function to draw world border
	void world_draw_border(void)
	{
		CP_Settings_RectMode(CP_POSITION_CORNER);
		CP_Settings_NoFill();

		CP_Settings_Stroke(CP_Color_Create(255, 50, 50, 255));
		CP_Settings_StrokeWeight(5.0f);

		CP_Graphics_DrawRect(0.0f, 0.0f, world_width, world_height);
		CP_Settings_NoStroke();
	}

	//function to draw grid in background
	void world_draw_grid(void)
	{
		CP_Settings_Stroke(CP_Color_Create(90, 70, 110, 255));
		CP_Settings_StrokeWeight(1.0f);

		for (float x = 0; x < world_width; x += 100.0f)
		{
			CP_Graphics_DrawLine(x, 0.0f, x, world_height);
		}

		for (float y = 0; y < world_height; y += 100.0f)
		{
			CP_Graphics_DrawLine(0.0, y, world_width, y);
		}

		CP_Settings_NoStroke();
	}

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
		float radians = player_rotation * PI / 180.0f;
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
			// normalize so diagonal movement isn't faster than straight movement (from Week 2 Vectors Slide 24)
			float length = sqrtf(dx * dx + dy * dy); //distance formula
			dx /= length; //normalize Vsum/Vsum Magnitude
			dy /= length; //for both

			playerX += dx * speed; //normalized magnitude * speed = normalized speed
			playerY += dy * speed;

			// arrow angle in degrees (W = -90, S = 90, A = 180, D = 0, diagonals in between)
			//Slide 18 Review
			player_rotation = atan2f(dy, dx) * 180.0f / PI;
		}
	}

	//function to check if player is inside boundary
	void player_check_if_inside_boundary(void)
	{
		if (playerX < player_radius)
		{
			playerX = player_radius;
		}

		if (playerX > world_width - player_radius)
		{
			playerX = world_width - player_radius;
		}

		if (playerY < player_radius)
		{
			playerY = player_radius;
		}

		if (playerY > world_height - player_radius)   
		{
			playerY = world_height - player_radius;
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

		player_check_if_inside_boundary();
	}

	// Draws player circle
	void Player_Draw(void)
	{
		// blink while invulnerable: skip drawing every other 0.1s
		//if (int)(invulnTimer * 10.0f) evaluates to odd, draw. Evaluates to even, return (skip draw).
		if (invulnTimer > 0.0f && ((int)(invulnTimer * 10.0f) % 2 == 0))
		{
			return;
		}
		CP_Settings_NoStroke();
		CP_Settings_Fill(CP_Color_Create(64, 224, 208, 255));   // turquoise
		CP_Graphics_DrawCircle(playerX, playerY, player_radius * 2.0f);
	}
	//Combat and Damage
	#define max_bullets 128
	#define bullet_speed 450.0f
	#define bullet_radius 4.0f
	#define bullet_lifetime 2.0f	//seconds before the bullet disappears
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
				bulletDamage[i] = Buff_ScaleDamage(damage); //buff: every bullet(player's drones) gets the kill buff
				break;
			}
		}
	}

	void bullet_update(void)
	{
		float dt = CP_System_GetDt();
		float w = world_width;
		float h = world_height;

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

	int combat_get_target(float x, float y, float range, CP_Vector* target)
	{
		if (boss_get_target(x, y, range, target) == 1)
		{
			return 1;
		}
		else
		{
			return Swarm_GetNearest(x, y, range, target);
		}
	}

	#define max_rockets 32
	#define rocket_speed 300.0f
	#define rocket_radius 8.0f
	#define rocket_damage 30.0f
	#define rocket_AOE_radius 70.0f
	#define rocket_lifetime 3.5f
	#define AOE_duration 0.25f
	#define rocket_drone_range 350.0f

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
		//Swarm_AOE_Damage(x, y, rocket_AOE_radius, rocket_damage);
		//boss_take_AOE_damage(x, y, rocket_AOE_radius, rocket_damage);	
		Swarm_AOE_Damage(x, y, rocket_AOE_radius, Buff_ScaleDamage(rocket_damage)); //buff: rocket damage goes up with the kill buff
		boss_take_AOE_damage(x, y, rocket_AOE_radius, Buff_ScaleDamage(rocket_damage)); //buff: same for the boss


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
		float w = world_width;
		float h = world_height;

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

			if (combat_get_target(rocketX[i], rocketY[i], rocket_drone_range, &target) == 1)
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
		for (int i = 0; i < max_rockets; i++)
		{
			rocketActive[i] = 0;
			AOEActive[i] = 0;
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

	void Game_Start_Boss_Fight(void)
	{
		swarm_auto_spawn = 0;
		boss_init();
	}

	//swarms: called when swarm timer reaches 0.
	//swarm 1 -> shop -> swarm 2 -> shop -> boss fight
	void Game_End_Swarm(void)
	{
		Swarm_Init(Diff_GetSwarmLevel(), currentSwarm);	//clear leftover enemies and shots, coins stay
		swarm_auto_spawn = 0;							//no new enemies during the break ( must come after Swarm_Init())
		Wave_StartBreak(currentSwarm);					//5 sec to pick up coins, shows "SWARM 1 CLEARED"
	}

	void Game_Start_Next_Swarm(void) {
		if (currentSwarm == 1) {
			currentSwarm = 2;
			swarmTimer = swarm_duration;			//next swarm gets full 45s
			Swarm_Init(Diff_GetSwarmLevel(), 2); 
		}
		else if (currentSwarm == 2) {
			currentSwarm = 3;						//3 = boss fight, the swarm timer stops counting
			Game_Start_Boss_Fight();				//boss appears, swarm spawing stays off
		}
	}







	//Moving Camera
	void camera_update(void)
	{
		float window_width = CP_System_GetWindowWidth();
		float window_height = CP_System_GetWindowHeight();

		cameraX = playerX - window_width / 2.0f;
		cameraY = playerY - window_height / 2.0f;

		if (cameraX < 0.0f)
		{
			cameraX = 0.0f;
		}

		if (cameraY < 0.0f)
		{
			cameraY = 0.0f;
		}

		if (cameraX > world_width - window_width)
		{
			cameraX = world_width - window_width;
		}

		if (cameraY > world_height - window_height)
		{
			cameraY = world_height - window_height;
		}
	}


	void Game_Init(void)
	{
		//have a function for difficulty (include mainmenu.h in this file
		// arrow position on screen (spawn point)
		playerX = world_width / 2.0f;
		playerY = world_height / 2.0f;

		// speed of player
		speed = 5.0f;

		// player faces east.
		player_rotation = 0.0f;

		// dash settings
		dashSpeed = 20.0f;
		dashDistance = 150.0f;
		dashRemaining = 0.0f;
		dashCooldown = 2.0f;
		dashCooldownTimer = 0.0f;
	
	//PLEASE DONT DELETE THIS UNLESS YOU ARE DONE WITH DIFFICULTY HANDLING
	//Swarm_Init(1,2); //taking in int value of (difficulty, current stage). (Placeholder for now until someone prompts diificulty stage)
		Swarm_Init(Diff_GetSwarmLevel(), 1);// spawb rate from main menu, 1 easy/ 2 normal/ 3 hard. swarm starts on swarm 1 now
	combat_init();
	rocket_drone_init();
	Coins_Init(); //coin drop: no coins on the ground, coin count back to 0
	Shop_Init(); //shop; player starts w 1 basic drone, shop closed(Elvis)
	Wave_Init();
	Buff_Init();
	Pause_Init();
	GameOver_Init();
	win_init();
	boss_reset();
	runTime = 0.0f;

	swarmTimer = swarm_duration;
	currentSwarm = 1;

	cameraX = 0.0f;
	cameraY = 0.0f;
	camera_update();
}

void Game_Update(void)
{
	if (GameOver_IsActive() == 0 && Shop_IsOpen() == 0 && win_active() == 0) //shop: ESC closes the shop instead of pausing while it is open
	{
		Pause_HandleInput();
	}
	//shop: B opens / closes the shop, not while paused or dead (added by Elvis)
	if (GameOver_IsActive() == 0 && Pause_IsPaused() == 0 && win_active() == 0)
	{
		Shop_HandleInput();
	}

		CP_Graphics_ClearBackground(CP_Color_Create(45, 20, 75, 255));

	// ----- update: only while NOT paused and NOT dead -----
	//if (Pause_IsPaused() == 0 && GameOver_IsActive() == 0) (Brandon)
	if (Pause_IsPaused() == 0 && GameOver_IsActive() == 0 && Shop_IsOpen() == 0 && win_active() == 0) //shop: game also freezes while the shop is open (Elvis)
	{
		runTime += CP_System_GetDt();

		if (currentSwarm <= 2)
		{
			swarmTimer = swarmTimer - CP_System_GetDt();

			if (swarmTimer <= 0.0f)
			{
				swarmTimer = 0.0f;
				if (Wave_IsOnBreak() == 0) //if the wave break is not already active, start it
				{
					Game_End_Swarm();
				}
				else if (Wave_UpdateBreak() == 1)
				{
					Game_Start_Next_Swarm();
				}
			}
		}

		//temporary button to start boss fight
		if (CP_Input_KeyTriggered(KEY_M))
		{
			Game_Start_Boss_Fight();
			currentSwarm = 3; //to stop additional swarms from spawing, since we only have swarm 1 and swarm 2
			Wave_Init();
		}

		Swarm_Update(CP_Vector_Set(playerX, playerY));
		Player_Update();
		//drone_update(); (Brandon)
		//machine_gun_drone_update(); (Brandon)
		//rocket_drone_update(); (Brandon)
		Shop_UpdateDrones(); // shop; move and fires every drone in squad (Elvis)
		rocket_update();
		bullet_update();
		boss_update(CP_Vector_Set(playerX, playerY));

		if (invulnTimer > 0.0f)
		{
			invulnTimer -= CP_System_GetDt();
		}

			float contactDamage = Swarm_NumOfEnemiesTouching(playerX, playerY, player_radius) * enemy_contact_damage;
			float projectileDamage = Swarm_ProjectilesHitPlayer(playerX, playerY, player_radius);
			float bossDamage = boss_get_collision_damage(playerX, playerY, player_radius);
			combat_damage_player(Diff_ScaleDamageToPlayer(contactDamage + projectileDamage + bossDamage));
			// coin drop: kill enemies touching the player, drop a coin where each died,
			// then pick up any coin the player is touching
			Coins_Update(playerX, playerY, player_radius);

			if (currentSwarm == 3 && bossActive == 0 && player_health > 0.0f && GameOver_IsActive() == 0)
			{
				win_start();
			}
		}


	// ----- draw: always, so the game stays visible behind the overlays -----
	CP_Settings_ResetMatrix();
	CP_Graphics_ClearBackground(CP_Color_Create(45, 20, 75, 255));

	camera_update();
	
	CP_Settings_Translate(-cameraX, -cameraY);

	world_draw_grid();
	world_draw_border();

	Swarm_Draw();
	Coins_Draw();          // coins are drawn under the player
	Player_Draw();
	//drone_draw();
	//machine_gun_drone_draw(); (added by Brandon)
	//rocket_system_draw(); (added by Brandon)
	Shop_DrawDrones(); //shop: draws every drone in the squad (added by Elvis)
	rocket_projectiles_draw(); //shop: rockets and explosions (added by Elvis)
	bullet_draw();
	boss_draw();

	CP_Settings_ResetMatrix();

	HUD_Draw(player_health, player_health_max, coinCount, dashCooldownTimer, dashCooldown, runTime, swarmTimer);
	Wave_Draw(); // wave break, prints message "SWARM 1 cleared'
	Buff_Draw(); //buff: "DMG +10%" text in the bottom-left corner
	Shop_Draw(); //shop menu, press "B" on keyboard
	Pause_Draw();

		// player just died: start the game over screen
		if (player_health <= 0.0f && GameOver_IsActive() == 0 && win_active() == 0)
		{
			GameOver_Start();
		}
		GameOver_Draw(coinCount, runTime);  
		win_draw(coinCount, runTime);

		
		if (CP_Input_KeyTriggered(KEY_Q) && GameOver_IsActive() == 1)
		{
			CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
		}
	}



	void Game_Exit(void)
	{

	}
