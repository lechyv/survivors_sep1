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

	//World Dimensions in pixels - Brandon
	#define world_width 2500.0f
	#define world_height 2500.0f

	//Camera position in world - Brandon
	float cameraX;
	float cameraY;

	//Constant to define duration of each swarm - Brandon
	#define swarm_duration 45.0f

	//Variables for the swarm timer - Brandon
	float swarmTimer;	//Time remaining in current swarm
	int currentSwarm;	//Current stage of the game (Swarm 1, 2, or Boss fight)

	//Variable for tracking the total game time
	float runTime;	//Total time the player has survived in seconds

	#define PI 3.14159265f

	//Function to draw the world borders - Brandon
	void world_draw_border(void)
	{
		CP_Settings_RectMode(CP_POSITION_CORNER);	//Sets the rectangle position to start from the top left corner
		CP_Settings_NoFill();	//Removes the fill colour so that only the rectangle outline is visible

		CP_Settings_Stroke(CP_Color_Create(255, 50, 50, 255));	//Sets the border colour to red
		CP_Settings_StrokeWeight(5.0f);	//Sets the thickness of the border to 5 pixels.

		CP_Graphics_DrawRect(0.0f, 0.0f, world_width, world_height);	//Draws the border from (0,0) using the world width and world height
		CP_Settings_NoStroke();	//Ensures that the red outline is not drawn for objects drawn after the border
	}

	//Function to draw the background grid - Brandon
	void world_draw_grid(void)
	{
		CP_Settings_Stroke(CP_Color_Create(90, 70, 110, 255));	//Set the grid lines to purple
		CP_Settings_StrokeWeight(1.0f);	//Set the thickness of the grid lines to 1 pixel

		for (float x = 0; x < world_width; x += 100.0f)	//For loop to draw vertical lines every 100 pixels
		{
			CP_Graphics_DrawLine(x, 0.0f, x, world_height);	//Draws a line from the top to the bottom of the map
		}

		for (float y = 0; y < world_height; y += 100.0f)	//For loop to draw horizontal lines every 100 pixels
		{
			CP_Graphics_DrawLine(0.0, y, world_width, y);	//Draws a line from the left to right of the map
		}

		CP_Settings_NoStroke();	//Ensures that the purple outline is not drawn for objects drawn after the grid lines.
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

	//Function to prevent player from going outside the world boundaries - Brandon 
	void player_check_if_inside_boundary(void)
	{
		if (playerX < player_radius)	//If statement to check if player has moved past the left boundary
		{
			playerX = player_radius;	//Moves the player back inside the left boundary
		}

		if (playerX > world_width - player_radius)	//If statement to check if player has moved past the right boundary
		{
			playerX = world_width - player_radius;	//Moves the player back inside the right boundary
		}

		if (playerY < player_radius)	//If statement to check if the player has moved past the top boundary
		{
			playerY = player_radius;	//Moves the player back inside the top boundary
		}

		if (playerY > world_height - player_radius)	//If statement to check if the player has moved past the bottom boundary
		{
			playerY = world_height - player_radius;	//Moves the player back inside the bottom boundary
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
	//Bullet Firing, Targeting, and Collision/Damage System - Brandon

	//Constants for bullet behaviour
	#define max_bullets 128			//Maximum number of bullets allowed at once
	#define bullet_speed 450.0f		//Speed of bullets in pixels per second
	#define bullet_radius 4.0f		//Radius of each bullet
	#define bullet_lifetime 2.0f	//Seconds before a bullet disappears

	//Variables for bullet tracking
	int bulletActive[max_bullets];		//Checks whether each bullet is active(1) or inactive(0)
	float bulletX[max_bullets];			//X-coordinate of each bullet
	float bulletY[max_bullets];			//Y-coordinate of each bullet
	float bulletXv[max_bullets];		//Horizontal velocity of each bullet
	float bulletYv[max_bullets];		//Vertical velocity of each bullet
	float bulletAge[max_bullets];		//Seconds since bullet fired
	float bulletDamage[max_bullets];	//Damage dealt by each bullet

	//Constants for player health and damage taken
	#define player_health_max 100.0f	//Maximum health of the player
	#define enemy_contact_damage 10.0f	//Damage dealt when an enemy touches the player

	//Variable for player's current health
	float player_health;

	
	//Function to check if 2 circles are touching - Brandon
	int combat_circles_touch(float x1, float y1, float r1, float x2, float y2, float r2) 
	{
		float dx = x1 - x2;	//Horizontal distance between the 2 objects
		float dy = y1 - y2;	//Vertical distance between the 2 objects
		float distancesquared = (dx * dx) + (dy * dy);	//Squared distance between the objects
		float radius_sum = r1 + r2;	//Combined radius of both objects

		if (distancesquared <= (radius_sum * radius_sum))	//If statement to check if the squared distance between objects is smaller than or equal to the squared combined radius
		{
			return 1;	//Returns 1 if the objects are touching
		}
		return 0;	//Returns 0 if they are not touching
	}

	//Function to initialize the player health and bullet data when the game starts - Brandon
	void combat_init(void)
	{
		player_health = player_health_max;	//Resets the player health to the maximum health
		invulnTimer = 0.0f;	//Removes the remaining invulnerability time
		for (int i = 0; i < max_bullets; i++)	//For loop to loop through all bullet indexes and reset them
		{
			bulletActive[i] = 0;	//Makes every bullet inactive
			bulletDamage[i] = 0.0f;	//Resets the damage stored for each bullet
		}
	}

	//Function for handling damage taken by player - Brandon
	void combat_damage_player(float amount)
	{
		if (amount <= 0.0f || dashRemaining > 0.0f || invulnTimer > 0.0f)	//Prevents damage if the amount is 0 or less, the player is dashing, or if the player is invulnerable
		{
			return;	//Exits the function without damaging the player
		}

		player_health = player_health - amount;	//Reduces the player's health by the damage received
		if (player_health < 0.0f)	//If statement to check if player's health goes below 0
		{
			player_health = 0.0f;	//Sets the player's health to 0 to prevent it from going negative
		}

		invulnTimer = player_invuln_time;	//Gives the player invulnerability after taking damage
	}

	//Function to fire bullets from drones to a target - Brandon
	void bullet_fire(float fromX, float fromY, float targetX, float targetY, float damage)
	{
		float dx = targetX - fromX;	//Horizontal distance between the drone and the target
		float dy = targetY - fromY;	//Vertical distance between the drone and the target
		float distance = sqrtf((dx * dx) + (dy * dy));	//It is the actual distance between the drone and the target

		if (distance < 0.01f)	//If statement to check if the drone and the target are extremely close to prevent division by zero or a very small number. 
		{						
			return;	//Exits the function without firing a bullet
		}

		for (int i = 0; i < max_bullets; i++)	//For loop to loop through all bullet indexes to find an inactive bullet
		{
			if (bulletActive[i] == 0)	//If statement to check if the bullet index is available
			{
				bulletActive[i] = 1;	//Sets the bullet to active
				bulletX[i] = fromX;		//Sets the bullet's starting X-coordinate to the drone's X-coordinate
				bulletY[i] = fromY;		//Sets the bullet's starting Y-coordinate to the drone's Y-coordinate
				bulletXv[i] = (dx / distance) * bullet_speed;	//Sets the bullet's horizontal velocity along the X-axis towards the target
				bulletYv[i] = (dy / distance) * bullet_speed;	//Sets the bullet's vertical velocity along the Y-axis towards the target
				bulletAge[i] = 0.0f;	//Resets the bullet's age to 0 when fired
				bulletDamage[i] = Buff_ScaleDamage(damage); //Applies the damage buff to the bullet
				break;	//Stops the for loop after firing a bullet
			}
		}
	}

	//Function to update bullet positions and remove expired bullets - Brandon
	void bullet_update(void)
	{
		float dt = CP_System_GetDt();	//Gets the time passed since the previous frame
		float w = world_width;			//Sets world_width as w
		float h = world_height;			//Sets world_height as h

		for (int i = 0; i < max_bullets; i++)	//For loop to loop through all bullet indexes
		{
			if (bulletActive[i] == 0)	//If statement to check if the bullet is inactive
			{
				continue;	//Skips the bullet if it's inactive and moves on to the next index
			}
			bulletX[i] = bulletX[i] + bulletXv[i] * dt;	//Updates the bullet's X-coordinate based on its horizontal velocity
			bulletY[i] = bulletY[i] + bulletYv[i] * dt;	//Updates the bullet's Y-coordinate based on its vertical velocity
			bulletAge[i] = bulletAge[i] + dt;	//Increases the bullet's age by the time passed

			if (bulletAge[i] > bullet_lifetime || bulletX[i] < -20.0f || bulletY[i] < -20.0f || bulletX[i] > w + 20.0f || bulletY[i] > h + 20.0f)	//Checks if the bullet has expired or moved outside the world boundaries
			{
				bulletActive[i] = 0;	//Sets the bullet to inactive
			}
		}
	}

	//Function to check if bullets hit an enemy or the boss, and calculate the damage - Brandon
	float bullet_check_if_hit(float x, float y, float radius)
	{
		float damage = 0.0f;	//Sets the total damage done to 0
			for (int i = 0; i < max_bullets; i++)	//For loop to loop through all bullet indexes
			{
				if (bulletActive[i] == 1 && combat_circles_touch(bulletX[i], bulletY[i], bullet_radius, x, y, radius))	//If statement to check if the bullet is active and whether it's touching an enemy or the boss
				{
					bulletActive[i] = 0;	//Sets the bullet to inactive after hitting the target
					damage = damage + bulletDamage[i];	//Adds the bullet's damage to the total damage
				}
			}
		return damage;	//Returns the total damage done to the target
	}

	//Function to draw active bullets on the screen - Brandon
	void bullet_draw(void)
	{
		CP_Settings_NoStroke();	//Prevents outlines from appearing around the bullets
		CP_Settings_Fill(CP_Color_Create(255, 230, 150, 255));	//Sets the bullet's colour to light yellow
		for (int i = 0; i < max_bullets; i++)	//For loop to loop through all bullet indexes
		{
			if (bulletActive[i] == 1)	//If statement to check if the bullet is active
			{
				CP_Graphics_DrawCircle(bulletX[i], bulletY[i], bullet_radius * 2.0f);	//Draws the bullet as a circle using its current position and diameter
			}
		}
	}

	//Function for targeting system for the drones, prioritizing the boss - Brandon
	int combat_get_target(float x, float y, float range, CP_Vector* target)
	{
		if (boss_get_target(x, y, range, target) == 1)	//If statement to check if the boss is active and within the drone's attack range
		{
			return 1;	//Returns 1 if the boss is targeted
		}
		else
		{
			return Swarm_GetNearest(x, y, range, target);	//If the boss is not targeted, Swarm_GetNearest searches for the nearest swarm enemy and returns 1 if an enemy is found or returns 0 if no enemies are found
		}
	}

	//Rocket Firing, Homing, and Area of Effect(AOE) Damage System - Brandon 

	//Constants for rocket behaviour
	#define max_rockets 32				//Maximum number of rockets allowed at once
	#define rocket_speed 300.0f			//Speed of rockets in pixels per second
	#define rocket_radius 8.0f			//Radius of each rocket
	#define rocket_damage 30.0f			//Damage dealt by each rocket explosion
	#define rocket_AOE_radius 70.0f		//Radius of the rocket's explosion
	#define rocket_lifetime 3.5f		//Seconds before a rocket disappears
	#define AOE_duration 0.25f			//Duration of the AOE effect in seconds
	#define rocket_drone_range 350.0f	//Maximum distance for detecting targets

	//Variables for rocket and explosion tracking
	int rocketActive[max_rockets];		//Checks if each rocket is active(1) or inactive(0)
	float rocketX[max_rockets];			//X-coordinate of each rocket
	float rocketY[max_rockets];			//Y-coordinate of each rocket
	float rocketXv[max_rockets];		//Horizontal velocity of each rocket
	float rocketYv[max_rockets];		//vertical velocity of each rocket
	float rocketAge[max_rockets];		//Seconds since each rocket was fired

	int AOEActive[max_rockets];			//Checks if each explosion is active(1) or inactive(0)
	float AOEX[max_rockets];			//X-coordinate of each explosion
	float AOEY[max_rockets];			//Y-coordinate of each explosion
	float AOEAge[max_rockets];			//Seconds since each explosion started

	//Function to fire a rocket from the drone towards a target - Brandon
	void rocket_fire(float fromX, float fromY, float targetX, float targetY)
	{
		float dx = targetX - fromX;	//Horizontal distance between the drone and the target
		float dy = targetY - fromY;	//Vertical distance between the drone and the target
		float distance = sqrtf((dx * dx) + (dy * dy));	//Calculate the actual distance between the drone and the target

		if (distance < 0.01f)	//Checks if the drone and target are extremely close
		{
			return;	//Exits the function without firing a rocket to avoid dividing by zero or a very small number
		}

		for (int i = 0; i < max_rockets; i++)	//For loop to loop through all rocket indexes
		{
			if (rocketActive[i] == 0) //If statement to check if the rocket index is inactive
			{
				rocketActive[i] = 1;	//Sets the rocket index to active

				rocketX[i] = fromX;	//Sets the rocket's starting X-coordinate to the drone's X-coordinate
				rocketY[i] = fromY;	//Sets the rocket's starting Y-coordinate to the drone's Y-coordinate

				rocketXv[i] = (dx / distance) * rocket_speed;	//Sets the rocket's horizontal velocity along the X-axis towards the target
				rocketYv[i] = (dy / distance) * rocket_speed;	//Sets the rocket's vertical velocity along the Y-axis towards the target 

				rocketAge[i] = 0.0f;	//Resets the rocket's age to 0 when fired
				break;	//Stops the for loop after firing a rocket
			}
		}
	}

	//Function to deal AOE damage to nearby enemies and the boss - Brandon
	void rocket_AOE(float x, float y)
	{
		Swarm_AOE_Damage(x, y, rocket_AOE_radius, Buff_ScaleDamage(rocket_damage));	//Deals AOE damage to nearby swarm enemies and applies the damage buff
		boss_take_AOE_damage(x, y, rocket_AOE_radius, Buff_ScaleDamage(rocket_damage));	//Deals AOE damage to the boss if it's within range and applies the damage buff

		for (int i = 0; i < max_rockets; i++)	//For loop to loop through all AOE indexes
		{
			if (AOEActive[i] == 0)	//If statement to check if the AOE is inactive
			{
				AOEActive[i] = 1;	//Sets the AOE to active
				AOEX[i] = x;		//Sets the AOE's X-coordinate	
				AOEY[i] = y;		//Sets the AOE's Y-coordinate
				AOEAge[i] = 0.0f;	//Resets the AOE's age to 0
				return;				//Exits the function after creating the explosion effect
			}
		}
	}

	//Function to update homing rockets and explosion effects - Brandon
	void rocket_update(void)
	{
		float dt = CP_System_GetDt();	//Gets the time passed since the previous frame
		float w = world_width;	//Sets the world_width as w
		float h = world_height;	//Sets the world_height as h

		for (int i = 0; i < max_rockets; i++)	//For loop to loop through all rocket indexes
		{
			if (rocketActive[i] == 0)	//If statement to check if the rocket is inactive
			{
				continue;	//Skips the inactive rockets and move to the next index
			}

			rocketAge[i] = rocketAge[i] + dt;	//Increases the rocket's age by the time passed

			if (rocketAge[i] > rocket_lifetime)	//If statement to check if the rocket has expired
			{
				rocketActive[i] = 0;	//Sets the rocket to inactive
				continue;	//Skips the rocket and moves to the next index
			}

			CP_Vector target;	//Variable to store the position of the target 

			if (combat_get_target(rocketX[i], rocketY[i], rocket_drone_range, &target) == 1)	//If statement to check if there is a boss or swarm enemy within the rocket's range and gets its position
			{
				float dx = target.x - rocketX[i];	//Horizontal distance between the rocket and the target
				float dy = target.y - rocketY[i];	//Vertical distance between the rocket and the target
				float distance = sqrtf((dx * dx) + (dy * dy));	//Calculate the actual distance between the rocket and the target

				if (distance <= rocket_speed * dt + 26.0f)	//If statement to check if the rocket is close enough to hit the target 
				{
					rocketActive[i] = 0;	//Sets the rocket to inactive
					rocket_AOE(target.x, target.y);	//Creates an explosion at the target's position
					continue;	//Moves to the next rocket index
				}

				if (distance > 0.01f)	//If statement to check that the rocket is not extremely close to the target 
				{
					rocketXv[i] = (dx / distance) * rocket_speed;	//Updates the horizontal velocity to follow the target
					rocketYv[i] = (dy / distance) * rocket_speed;	//Updates the vertical velocity to follow the target 
				}
			}

			rocketX[i] = rocketX[i] + rocketXv[i] * dt;	//Updates the rocket's X-coordinate
			rocketY[i] = rocketY[i] + rocketYv[i] * dt;	//Updates the rocket's Y-coordinate

			if (rocketX[i] < -20.0f || rocketY[i] < -20.0f || rocketX[i] > w + 20.0f || rocketY[i] > h + 20.0f)	//If statement to check if the rocket has moved outside the world boundaries
			{
				rocketActive[i] = 0;	//Sets the rocket to inactive
			}
		}

		for (int i = 0; i < max_rockets; i++)	//For loop to loop through all AOE indexes
		{
			if (AOEActive[i] == 1)	//If statement to check if AOE is active
			{
				AOEAge[i] = AOEAge[i] + dt;	//Increases the AOE's age by the time passed

				if (AOEAge[i] >= AOE_duration)	//If statement to check if the AOE has expired
				{
					AOEActive[i] = 0;	//Sets the AOE to inactive
				}
			}
		}
	}

	//Function to reset all rocket and AOE data when the game starts - Brandon 
	void rocket_drone_init(void)
	{
		for (int i = 0; i < max_rockets; i++)	//For loop to loop through all AOE and rocket indexes
		{
			rocketActive[i] = 0;	//Sets every rocket index to inactive
			AOEActive[i] = 0;	//Sets every AOE index to inactive
		}
	}

	//shop: draws the rockets and explosions only, without the single rocket drone circle (added by Elvis)
	//The shop now draws every drone, so rocket_system_draw is no longer called
	//UI: rockets are red-orange circles, explosions are orange circles that grow, then vanish

	//Function to draw rockets and AOE explosion effects without drawing the drones - Brandon  
	void rocket_projectiles_draw(void)
	{
		CP_Settings_NoStroke();	//Removes the outlines from the rockets and explosions
		CP_Settings_Fill(CP_Color_Create(255, 80, 40, 255)); //UI: rocket colour (red-orange). Size is rocket_radius

		int i = 0;
		while (i < max_rockets)	//While loop to loop through all rocket indexes
		{
			if (rocketActive[i] == 1)	//If statement to check if the rocket is active
			{
				CP_Graphics_DrawCircle(rocketX[i], rocketY[i], rocket_radius);	//Draws the rocket at its current position
			}
			i += 1;	//Moves to the next rocket index
		}

		i = 0;	//Resets i to 0 to loop through AOE indexes
		while (i < max_rockets)	//While loop to loop through all AOE indexes
		{
			if (AOEActive[i] == 1)	//If statement to check if AOE is active
			{
				float AOEProgress = AOEAge[i] / AOE_duration;            //0.0 when it starts, 1.0 when it ends
				float AOESize = rocket_AOE_radius * 2.0f * AOEProgress;  //UI: grows to the full blast size (the real damage area)

				CP_Settings_Fill(CP_Color_Create(255, 140, 30, 150));   //UI: explosion colour (orange, half see-through)
				CP_Graphics_DrawCircle(AOEX[i], AOEY[i], AOESize);
			}
			i += 1;	//Moves to the next AOE index
		}
	}

	/*Game state*/

	//Function to stop swarm enemies from spawning and start the boss fight - Brandon
	void Game_Start_Boss_Fight(void)
	{
		swarm_auto_spawn = 0;	//Stops new swarm enemies from spawning
		boss_init();			//Initializes the boss and starts the boss fight
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

	//Function to update the camera position to follow the player - Brandon
	void camera_update(void)
	{
		float window_width = CP_System_GetWindowWidth();	//Gets the width of the game window
		float window_height = CP_System_GetWindowHeight();	//Gets the height of the game window

		cameraX = playerX - window_width / 2.0f;	//Centers the camera horizontally on the player
		cameraY = playerY - window_height / 2.0f;	//Centers the camera vertically on the player

		if (cameraX < 0.0f)	//If statement to check if the camera moves past the left world boundary
		{
			cameraX = 0.0f;	//Stops the camera at the left boundary
		}

		if (cameraY < 0.0f)	//If statement to check if the camera moves past the top world boundary
		{
			cameraY = 0.0f;	//Stops the camera at the top boundary
		}

		if (cameraX > world_width - window_width)	//If statement to check if the camera moves past the right world boundary
		{
			cameraX = world_width - window_width;	//Stops the camera at the right boundary
		}

		if (cameraY > world_height - window_height)	//If statement to check if the camera moves past the bottom world boundary
		{
			cameraY = world_height - window_height;	//Stops the camera at the bottom boundary
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
	combat_init();	//Initializes the player health and combat system - Brandon
	rocket_drone_init();	//Resets all rockets and AOE effects - Brandon
	Coins_Init(); //coin drop: no coins on the ground, coin count back to 0
	Shop_Init(); //shop; player starts w 1 basic drone, shop closed(Elvis)
	Wave_Init();
	Buff_Init();
	Pause_Init();
	GameOver_Init();
	win_init();	//Initializes the victory screen - Brandon
	boss_reset();	//Resets the boss when a new game is started - Brandon
	runTime = 0.0f;

	swarmTimer = swarm_duration;	//Sets the swarm timer to 45 seconds - Brandon
	currentSwarm = 1;				//Sets the starting stage to swarm 1 - Brandon

	cameraX = 0.0f;		//Resets the camera's X-coordinate - Brandon
	cameraY = 0.0f;		//Resets the camera's Y-coordinate - Brandon
	camera_update();	//Updates the camera position to follow the player - Brandon
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
	if (Pause_IsPaused() == 0 && GameOver_IsActive() == 0 && Shop_IsOpen() == 0 && win_active() == 0) //shop: game also freezes while the shop is open (Elvis)
	{
		runTime += CP_System_GetDt();

		if (currentSwarm <= 2) //If statement to check if the game is currently in Swarm 1 or 2
		{
			swarmTimer = swarmTimer - CP_System_GetDt();	//Decrease the swarm timer by the amount of time that has passed

			if (swarmTimer <= 0.0f)	//If statement to check if the swarm timer has reached 0
			{
				swarmTimer = 0.0f;	//Sets the swarm timer to 0 to prevent it from going below 0
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
		Shop_UpdateDrones(); // shop; move and fires every drone in squad (Elvis)
		rocket_update();	//Updates homing rockets and AOE effects - Brandon
		bullet_update();	//Updates the bullet positions and remove expired bullets - Brandon
		boss_update(CP_Vector_Set(playerX, playerY));	//Updates the boss using the player's position - Brandon

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

			if (currentSwarm == 3 && bossActive == 0 && player_health > 0.0f && GameOver_IsActive() == 0)	//If statement to check if the boss is defeated and the player is still alive - Brandon
			{
				win_start();	//Starts the victory screen - Brandon
			}
		}


	// ----- draw: always, so the game stays visible behind the overlays -----
	CP_Settings_ResetMatrix();
	CP_Graphics_ClearBackground(CP_Color_Create(45, 20, 75, 255));

	camera_update();	//Updates the camera position to follow the player - Brandon
	
	CP_Settings_Translate(-cameraX, -cameraY);	//Shifts the view to follow the camera position - Brandon

	world_draw_grid();		//Draws the background grid - Brandon
	world_draw_border();	//Draws the world boundaries - Brandon

	Swarm_Draw();
	Coins_Draw();          // coins are drawn under the player
	Player_Draw();

	Shop_DrawDrones(); //shop: draws every drone in the squad (added by Elvis)
	rocket_projectiles_draw(); //shop: rockets and explosions (added by Elvis)
	bullet_draw();	//Draws active bullets - Brandon
	boss_draw();	//Draws the boss - Brandon

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
