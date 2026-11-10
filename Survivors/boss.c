#include "cprocessing.h"
#include "boss.h"
#include "game.h"
#include "Swarm.h"
#include "math.h"
#include "diff.h"

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

//Function to start the boss fight and set the boss's starting values - Brandon
void boss_init(void)
{
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
}

//Function to draw the boss, with its colour showing its current state - Brandon
void boss_draw(void)
{
	if (bossActive == 0)	//If statement to check if the boss is inactive
	{
		return;	//Exits the function without drawing the boss
	}

	CP_Settings_NoStroke();	//Prevents an outline from appearing around the boss

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
		return;				//Exits the function as the boss is already dead
	}

	if (bossHealth <= boss_health_max / 2.0f && bossPhase == 1)	//If statement to check if the boss is at or below half health and is still in phase 1
	{
		bossPhase = 2;	//Switches the boss to phase 2
		bossSummonTimer = 0.0f;	//Makes the boss summon enemies immediately when phase 2 starts
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
}