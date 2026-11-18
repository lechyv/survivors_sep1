#include "cprocessing.h"
#include "boss.h"
#include "game.h"
#include "Swarm.h"
#include "math.h"
#include "diff.h"
#include <stddef.h> 

//Constants for boss behaviour - Brandon
#define boss_size 70.0f						//Diameter of the boss
#define boss_speed 180.0f					//Boss movement speed in pixels per second in phase 1
#define boss_dash_speed 1000.0f				//Boss speed while dashing in pixels per second
#define boss_dash_distance 500.0f			//Distance covered by each boss dash
#define boss_phase2_speed 225.0f			//Boss movement speed in pixels per second in phase 2
#define boss_phase2_dash_cooldown 1.25f		//Seconds between boss dashes in phase 2
#define boss_dash_cooldown 2.0f				//Seconds between boss dashes in phase 1
#define boss_lock_on_time 0.35f				//Seconds the boss takes to lock onto the player before dashing
#define boss_collision_damage 15.0f			//Damage dealt when the boss touches the player
#define boss_dash_collision_damage 30.0f	//Damage dealt when the boss touches the player while dashing
#define boss_summon_interval 2.0f			//Seconds between enemy summons in phase 2

//Variables for boss tracking - Brandon
float bossX, bossY;							//X and Y coordinates of the boss
float bossHealth;							//Current health of the boss
float bossDashTimer;						//Seconds left before the boss can start its next dash
float bossLockTimer;						//Seconds left in the lock on period before the dash
float bossdashRemaining;					//Distance left in the current dash (0 = not dashing)
float bossDashX, bossDashY;					//Target position while locking on, then the dash direction once the dash starts
float bossSummonTimer;						//Seconds left before the next enemy summon
extern float playerX;						//Player's X-coordinate from game.c
extern float playerY;						//Player's Y-coordinate from game.c

int bossActive;		//Checks if the boss is alive(1) or not(0)
int bossPhase;		//Current phase of the boss (1 or 2)
int bossState;		//Current action of the boss (0 = chasing, 1 = locking on, 2 = dashing)
CP_Image bossImage;        //boss picture (loaded only once)
int bossImageLoaded = 0;   //1 once the picture is loaded

//Constants for the boss's bullet attack
#define max_boss_bullets 96
#define boss_burst_bullets 20
#define boss_bullet_speed 180.0f
#define boss_bullet_radius 6.0f
#define boss_bullet_damage 10.0f
#define boss_bullet_lifetime 4.0f
#define boss_burst_interval 2.5f
#define boss_burst_warning_time 0.8f
#define boss_burst_dash_delay 1.0f

//Timers for the bullet attack
float bossBurstTimer;
float bossBurstWarningTimer;

//Arrays for the boss's bullets
int bossBulletActive[max_boss_bullets];
float bossBulletX[max_boss_bullets];
float bossBulletY[max_boss_bullets];
float bossBulletXv[max_boss_bullets];
float bossBulletYv[max_boss_bullets];
float bossBulletAge[max_boss_bullets];

//Function to clear all boss bullets
void boss_bullets_reset(void)
{
	for (int i = 0; i < max_boss_bullets; i++)
	{
		bossBulletActive[i] = 0; //Makes the bullet inactive
		bossBulletAge[i] = 0.0f; //Resets the bullet's age
	}
}

//Function to fire a ring of bullets around the boss
void boss_fire_burst(void)
{
	for (int bullet = 0; bullet < boss_burst_bullets; bullet++)
	{
		//Divides a full circle into evenly spaced directions
		float angle = (2.0f * 3.14159265f * bullet) / boss_burst_bullets;

		float directionX = cosf(angle);
		float directionY = sinf(angle);

		for (int i = 0; i < max_boss_bullets; i++)
		{
			if (bossBulletActive[i] == 0)
			{
				bossBulletActive[i] = 1; //Activates an unused bullet

				bossBulletX[i] = bossX; //Starts the bullet at the boss
				bossBulletY[i] = bossY;

				bossBulletXv[i] = directionX * boss_bullet_speed;
				bossBulletYv[i] = directionY * boss_bullet_speed;

				bossBulletAge[i] = 0.0f;

				break; //Stops searching after creating this bullet
			}
		}
	}
}

//Function to move boss bullets and remove expired bullets
void boss_bullets_update(void)
{
	float dt = CP_System_GetDt();

	for (int i = 0; i < max_boss_bullets; i++)
	{
		if (bossBulletActive[i] == 0)
		{
			continue; //Skips inactive bullets
		}

		bossBulletX[i] = bossBulletX[i] + bossBulletXv[i] * dt;
		bossBulletY[i] = bossBulletY[i] + bossBulletYv[i] * dt;

		bossBulletAge[i] = bossBulletAge[i] + dt;

		if (bossBulletAge[i] >= boss_bullet_lifetime)
		{
			bossBulletActive[i] = 0; //Removes the expired bullet
			continue;
		}

		if (bossBulletX[i] < -boss_bullet_radius ||
			bossBulletX[i] > world_width + boss_bullet_radius ||
			bossBulletY[i] < -boss_bullet_radius ||
			bossBulletY[i] > world_height + boss_bullet_radius)
		{
			bossBulletActive[i] = 0; //Removes bullets outside the world
		}
	}
}

//Function to draw active boss bullets
void boss_bullets_draw(void)
{
	CP_Settings_NoStroke();

	for (int i = 0; i < max_boss_bullets; i++)
	{
		if (bossBulletActive[i] == 0)
		{
			continue; //Skips inactive bullets
		}

		CP_Settings_Fill(CP_Color_Create(255, 100, 30, 255));
		CP_Graphics_DrawCircle(bossBulletX[i], bossBulletY[i], boss_bullet_radius * 2.0f);

		CP_Settings_Fill(CP_Color_Create(255, 240, 180, 255));
		CP_Graphics_DrawCircle(bossBulletX[i], bossBulletY[i], boss_bullet_radius);
	}
}

//Function to return damage from boss bullets touching the player
float boss_bullets_check_if_hit(float x, float y, float radius)
{
	if (bossActive == 0)
	{
		return 0.0f; //An inactive boss cannot deal bullet damage
	}

	float totalDamage = 0.0f;
	float radius_sum = radius + boss_bullet_radius;

	for (int i = 0; i < max_boss_bullets; i++)
	{
		if (bossBulletActive[i] == 0)
		{
			continue;
		}

		float dx = bossBulletX[i] - x;
		float dy = bossBulletY[i] - y;
		float distanceSquared = (dx * dx) + (dy * dy);

		if (distanceSquared <= radius_sum * radius_sum)
		{
			bossBulletActive[i] = 0; //Removes the bullet after hitting
			totalDamage = totalDamage + boss_bullet_damage;
		}
	}

	return totalDamage;
}

//Function to start the boss fight and set the boss's starting values - Brandon
void boss_init(void)
{
	if (bossImageLoaded == 0)
	{
		bossImage = CP_Image_Load("Assets/sprites/boss.png");
		bossImageLoaded = 1;
	}

	bossX = playerX + 300.0f;			//Spawns the boss 300 pixels to the right of the player
	bossY = playerY;					//Spawns the boss at the same height as the player

	bossHealth = boss_health_max;		//Sets the boss's health to the maximum health
	bossActive = 1;						//Sets the boss to active
	bossPhase = 1;						//Starts the boss in phase 1
	bossState = 0;						//Starts the boss in the chasing state

	bossDashTimer = boss_dash_cooldown;	//Boss waits one full cooldown before its first dash
	bossLockTimer = 0.0f;				//Clears the lock-on timer
	bossdashRemaining = 0.0f;			//Clears the remaining dash distance

	bossDashX = 0.0f;	//Clears the stored dash target/direction
	bossDashY = 0.0f;

	bossSummonTimer = boss_summon_interval;	//Sets the time until the first summon

	bossBurstTimer = boss_burst_interval;
	bossBurstWarningTimer = 0.0f;

	boss_bullets_reset();
}

//Function to update the boss's phase, summoning, movement and dash - Brandon
void boss_update(CP_Vector playerPos)
{
	if (bossActive == 0)	//If statement to check if the boss is inactive
	{
		return;	//Exits the function so that an inactive boss does nothing
	}

	float dt = CP_System_GetDt();	//Gets the time passed since the previous frame

	float current_boss_speed = boss_speed;	//Uses the phase 1 speed by default
	float current_boss_dash_cooldown = boss_dash_cooldown;	//Uses the phase 1 dash cooldown by default

	if (bossPhase == 2)	//If statement to check if the boss is in phase 2
	{
		current_boss_speed = boss_phase2_speed;	//Switches to the faster phase 2 speed
		current_boss_dash_cooldown = boss_phase2_dash_cooldown;	//Switches to the shorter phase 2 dash cooldown
	}

	float damage = bullet_check_if_hit(bossX, bossY, boss_size / 2.0f);	//Gets the total damage from bullets touching the boss
	
	if (damage > 0.0f)	//If statement to check if the boss was hit by any bullets
	{
		boss_take_damage(damage);	//Applies the damage to the boss
	}

	if (bossActive == 0)	//If statement to check if the boss died from the bullet damage
	{
		return;	//Exits the function so that a dead boss does not keep moving or summoning
	}

	if (bossPhase == 2)	//If statement to check if the boss is in phase 2
	{
		bossSummonTimer = bossSummonTimer - dt; //Decreases the summon timer by the time passed

		if (bossSummonTimer <= 0.0f)	//If statement to check if it is time to summon enemie
		{
			Swarm_SpawnBasic(bossX - 100.0f, bossY);	//Spawns a basic enemy to the left of the boss
			Swarm_SpawnBasic(bossX + 100.0f, bossY);	//Spawns a basic enemy to the right of the boss
			Swarm_SpawnTank(bossX, bossY + 100.0f);		//Spawns a tank enemy below the boss
			bossSummonTimer = boss_summon_interval;		//Resets the summon timer

		}
	}

	boss_bullets_update(); //Moves existing bullets during every boss state

	if (bossPhase == 2)
	{
		bossBurstTimer = bossBurstTimer - dt;

		if (bossBurstTimer <= 0.0f && bossState == 0)
		{
			bossState = 3; //Stops chasing and prepares the bullet attack
			bossBurstWarningTimer = boss_burst_warning_time;
		}
	}

	if (bossState == 0)	//State 0: boss chases the player
	{
		float dx = playerPos.x - bossX;					//Horizontal distance between the boss and the player
		float dy = playerPos.y - bossY;					//Vertical distance between the boss and the player
		float distance = sqrtf((dx * dx) + (dy * dy));	//Calculate the actual distance between the boss and the player

		if (distance > 0.01f)	//If statement to check that the boss is not extremely close to the player to avoid dividing by zero or a very small number
		{
			bossX = bossX + ((dx / distance) * current_boss_speed * dt);	//Moves the boss towards the player along the X-axis
			bossY = bossY + ((dy / distance) * current_boss_speed * dt);	//Moves the boss towards the player along the Y-axis
		}

		bossDashTimer = bossDashTimer - dt;		//Decreases the dash timer by the time passed

		if (bossDashTimer <= 0.0f)				//If statement to check if the boss is ready to dash
		{
			bossState = 1;						//Switches the boss to the lock-on state
			bossLockTimer = boss_lock_on_time;	//Starts the lock-on timer

			bossDashX = playerPos.x;	//Stores the player's current X-coordinate as the dash target
			bossDashY = playerPos.y;	//Stores the player's current X-coordinate as the dash target
		}
	}
	else if (bossState == 1)	//State 1: boss stops and locks on to the player
	{
		bossLockTimer = bossLockTimer - dt;	//Decreases the lock-on timer by the time passed

		bossDashX = playerPos.x;	//Keeps updating the target X-coordinate so the boss tracks the player during lock-on
		bossDashY = playerPos.y;	//Keeps updating the target Y-coordinate so the boss tracks the player during lock-on

		if (bossLockTimer <= 0.0f)	//If statement to check if the lock-on time has ended
		{
			float dx = bossDashX - bossX;	//Horizontal distance between the boss and the locked target
			float dy = bossDashY - bossY;	//Vertical distance between the boss and the locked target
			float distance = sqrtf((dx * dx) + (dy * dy));	//Calculate the actual distance between the boss and the locked target

			if (distance > 0.01f)	//If statement to check that the target is not extremely close to the boss
			{
				bossDashX = dx / distance;	//Converts the target position into the dash direction on the X-axis
				bossDashY = dy / distance;	//Converts the target position into the dash direction on the Y-axis

				bossdashRemaining = boss_dash_distance;	//Sets the full dash distance
				bossState = 2;	//Switches the boss to the dashing state
			}
			else
			{
				bossState = 0;	//Cancels the dash and returns the boss to chasing
				bossDashTimer = current_boss_dash_cooldown;	//Restarts the dash cooldown
			}
		}
	}
	else if (bossState == 2)	//State 2: boss dashes in a straight line
	{
		float dash_step = boss_dash_speed * dt;	//Distance the boss moves this frame

		if (dash_step > bossdashRemaining)	//If statement to check if this step would go past the dash distance
		{
			dash_step = bossdashRemaining;	//Shortens the last step so the dash does not overshoot
		}

		bossX = bossX + bossDashX * dash_step;	//Moves the boss along the dash direction on the X-axis
		bossY = bossY + bossDashY * dash_step;	//Moves the boss along the dash direction on the Y-axis

		bossdashRemaining = bossdashRemaining - dash_step;	//Decreases the remaining dash distance

		if (bossdashRemaining <= 0.0f)	//If statement to check if the dash has finished
		{
			bossState = 0;								//Returns the boss to the chasing state
			bossDashTimer = current_boss_dash_cooldown;	//Restarts the dash cooldown
		}
	}
	else if (bossState == 3)
	{
		bossBurstWarningTimer = bossBurstWarningTimer - dt;

		if (bossBurstWarningTimer <= 0.0f)
		{
			boss_fire_burst(); //Fires bullets in all directions

			bossBurstTimer = boss_burst_interval;
			bossState = 0; //Returns the boss to chasing

			bossDashTimer = boss_burst_dash_delay; //Waits before the next dash
		}
	}
}

//Function to draw the boss, with its colour showing its current state - Brandon
void boss_draw(void)
{
	if (bossActive == 0)	//If statement to check if the boss is inactive
	{
		return;	//Exits the function without drawing the boss
	}

	boss_bullets_draw();

	if (bossState == 3)
	{
		//Alternates between a bright and dim warning
		int flash = (int)(bossBurstWarningTimer * 10.0f);

		CP_Settings_NoStroke();

		if (flash % 2 == 0)
		{
			CP_Settings_Fill(CP_Color_Create(255, 80, 220, 200));
		}
		else
		{
			CP_Settings_Fill(CP_Color_Create(255, 80, 220, 80));
		}

		CP_Graphics_DrawCircle(bossX, bossY, boss_size * 1.6f);
	}

	CP_Settings_NoStroke();	//Prevents an outline from appearing around the boss

	if (bossImage != NULL)	//If statement to check if the boss picture loaded
	{
		if (bossState == 1)	//Locking on: yellow glow
		{
			CP_Settings_Fill(CP_Color_Create(255, 200, 50, 140));
			CP_Graphics_DrawCircle(bossX, bossY, boss_size * 1.4f);
		}
		else if (bossState == 2)	//Dashing: red glow
		{
			CP_Settings_Fill(CP_Color_Create(255, 50, 50, 140));
			CP_Graphics_DrawCircle(bossX, bossY, boss_size * 1.4f);
		}

		CP_Settings_ImageMode(CP_POSITION_CENTER);
		CP_Image_DrawAdvanced(bossImage, bossX, bossY, boss_size, boss_size, 255, 0.0f);
		return;	//Picture drawn, skip the old circle
	}


	if (bossState == 1)	//If statement to check if the boss is locking on
	{
		CP_Settings_Fill(CP_Color_Create(255, 200, 50, 255));	//Sets the boss's colour to yellow as a warning
	}
	else if (bossState == 2)	//If statement to check if the boss is dashing
	{
		CP_Settings_Fill(CP_Color_Create(255, 50, 50, 255));	//Sets the boss's colour to red
	}
	else
	{
		CP_Settings_Fill(CP_Color_Create(180, 50, 100, 255));	//Sets the boss's colour to pink while chasing
	}

	CP_Graphics_DrawCircle(bossX, bossY, boss_size);	//Draws the boss as a circle using its current position and diameter
}

//Function to damage the boss and switch it to phase 2 at half health - Brandon
void boss_take_damage(float damage)
{
	if (bossActive == 0 || damage <= 0.0f)	//If statement to check if the boss is inactive or the damage is 0 or less
	{
		return;	//Exits the function without damaging the boss
	}
	damage = Diff_ScaleDamageToEnemy(damage); //diff: easy = less HP, hard = more HP - Elvis
	bossHealth = bossHealth - damage;	//Reduces the boss's health by the damage received

	if (bossHealth <= 0.0f)	//If statement to check if the boss's health has reached 0
	{
		bossHealth = 0.0f;	//Sets the boss's health to 0 to prevent it from going negative
		bossActive = 0;		//Sets the boss to inactive, which ends the boss fight
		boss_bullets_reset(); //Removes remaining bullets when boss dies.
		return;				//Exits the function as the boss is already dead
	}

	if (bossHealth <= boss_health_max / 2.0f && bossPhase == 1)	//If statement to check if the boss is at or below half health and is still in phase 1
	{
		bossPhase = 2;	//Switches the boss to phase 2
		bossSummonTimer = 0.0f;	//Makes the boss summon enemies immediately when phase 2 starts
	}

	if (bossHealth <= boss_health_max / 2.0f && bossPhase == 1)
	{
		bossPhase = 2;
		bossSummonTimer = 0.0f;

		bossBurstTimer = boss_burst_interval; //Waits before the first bullet attack
	}
}

//Function to deal area of effect (AOE) damage to the boss if it is within range - Brandon
void boss_take_AOE_damage(float x, float y, float radius, float damage)
{
	if (bossActive == 0)	//If statement to check if the boss is inactive
	{
		return;	//Exits the function without damaging the boss
	}

	float dx = bossX - x;	//Horizontal distance between the boss and the explosion
	float dy = bossY - y;	//Vertical distance between the boss and the explosion
	float distance = sqrtf((dx * dx) + (dy * dy));	//Calculate the actual distance between the boss and the explosion

	if (distance <= radius + boss_size / 2.0f)	//If statement to check if the explosion radius reaches the boss's edge
	{
		boss_take_damage(damage);	//Applies the explosion damage to the boss
	}
}

//Function to return the damage the player takes from touching the boss - Brandon
float boss_get_collision_damage(float x, float y, float radius)
{
	if (bossActive == 0)	//If statement to check if the boss is inactive
	{
		return 0.0f;		//Returns 0 as an inactive boss cannot deal damage
	}

	float dx = bossX - x;	//Horizontal distance between the boss and the player
	float dy = bossY - y;	//Vertical distance between the boss and the player
	float distanceSquared = (dx * dx) + (dy * dy);	//Squared distance between the boss and the player
	float radius_sum = (boss_size / 2.0f) + radius;	//Combined radius of the boss and the player

	if (distanceSquared <= radius_sum * radius_sum)	//If statement to check if the boss and the player are touching (squared values avoid a slow sqrtf)
	{
		if (bossState == 2)	//If statement to check if the boss is dashing
		{
			return boss_dash_collision_damage;	//Returns the higher damage for a dash hit
		}

		else
		{
			return boss_collision_damage;	//Returns the normal contact damage
		}
	}
	return 0.0f;	//Returns 0 if they are not touching
}

//Function to check if the boss is within a drone's or rocket's range and give its position as the target - Brandon
int boss_get_target(float x, float y, float range, CP_Vector* target)
{
	if (bossActive == 0 || target == NULL)	//If statement to check if the boss is inactive or no target variable was given
	{
		return 0;	//Returns 0 as there is nothing to target
	}

	float dx = bossX - x;	//Horizontal distance between the boss and the shooter
	float dy = bossY - y;	//Vertical distance between the boss and the shooter
	float distance = sqrtf((dx * dx) + (dy * dy));	//Calculate the actual distance between the boss and the shooter

	if (distance <= range)	//If statement to check if the boss is within range
	{
		*target = CP_Vector_Set(bossX, bossY);	//Stores the boss's position in the target variable
		return 1;	//Returns 1 as the boss can be targeted
	}

	else
	{
		return 0;	//Returns 0 as the boss is out of range
	}
}

//Function to deactivate the boss when a new game is started - Brandon
void boss_reset(void)
{
	bossActive = 0;	//Sets the boss to inactive
	bossBurstTimer = boss_burst_interval;
	bossBurstWarningTimer = 0.0f;

	boss_bullets_reset();
}