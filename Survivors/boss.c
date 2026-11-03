#include "cprocessing.h"
#include "boss.h"
#include "game.h"
#include "Swarm.h"
#include "math.h"

#define boss_size 70.0f
#define boss_health_max 500.0f
#define boss_speed 80.0f
#define boss_dash_speed 450.0f
#define boss_dash_distance 250.0f
#define boss_dash_cooldown 4.0f
#define boss_lock_on_time 1.0f
#define boss_collision_damage 10.0f
#define boss_dash_collision_damage 25.0f
#define boss_summon_interval 8.0f

float bossX, bossY;
float bossHealth;
float bossDashTimer;
float bossLockTimer;
float bossdashRemaining;
float bossDashX, bossDashY;
float bossSummonTimer;

int bossActive;
int bossPhase;
int bossState;

void boss_init(void)
{
	bossX = CP_System_GetWindowWidth() / 2.0f;
	bossY = CP_System_GetWindowHeight() / 2.0f;

	bossHealth = boss_health_max;
	bossActive = 1;
	bossPhase = 1;
	bossState = 0;

	bossDashTimer = boss_dash_cooldown;
	bossLockTimer = 0.0f;
	bossdashRemaining = 0.0f;

	bossDashX = 0.0f;
	bossDashY = 0.0f;

	bossSummonTimer = boss_summon_interval;
}

//boss movement and dash
void boss_update(CP_Vector playerPos)
{
	if (bossActive == 0)
	{
		return;
	}

	float dt = CP_System_GetDt();

	float damage = bullet_check_if_hit(bossX, bossY, boss_size / 2.0f);
	
	if (damage > 0.0f)
	{
		boss_take_damage(damage);
	}

	if (bossActive == 0)
	{
		return;
	}

	if (bossPhase == 2)
	{
		bossSummonTimer = bossSummonTimer - dt;

		if (bossSummonTimer <= 0.0f)
		{
			Swarm_SpawnBasic(bossX - 100.0f, bossY);
			Swarm_SpawnBasic(bossX + 100.0f, bossY);
			Swarm_SpawnBasic(bossX, bossY + 100.0f);
			bossSummonTimer = boss_summon_interval;

		}
	}

	if (bossState == 0)
	{
		float dx = playerPos.x - bossX;
		float dy = playerPos.y - bossY;
		float distance = sqrtf((dx * dx) + (dy * dy));

		if (distance > 0.01f)
		{
			bossX = bossX + ((dx / distance) * boss_speed * dt);
			bossY = bossY + ((dy / distance) * boss_speed * dt);
		}

		bossDashTimer = bossDashTimer - dt;

		if (bossDashTimer <= 0.0f)
		{
			bossState = 1;
			bossLockTimer = boss_lock_on_time;

			bossDashX = playerPos.x;
			bossDashY = playerPos.y;
		}
	}
	else if (bossState == 1)
	{
		bossLockTimer = bossLockTimer - dt;

		if (bossLockTimer <= 0.0f)
		{
			float dx = bossDashX - bossX;
			float dy = bossDashY - bossY;
			float distance = sqrtf((dx * dx) + (dy * dy));

			if (distance > 0.01f)
			{
				bossDashX = dx / distance;
				bossDashY = dy / distance;

				bossdashRemaining = boss_dash_distance;
				bossState = 2;
			}
			else
			{
				bossState = 0;
				bossDashTimer = boss_dash_cooldown;
			}
		}
	}
	else if (bossState == 2)
	{
		float dash_step = boss_dash_speed * dt;

		if (dash_step > bossdashRemaining)
		{
			dash_step = bossdashRemaining;
		}

		bossX = bossX + bossDashX * dash_step;
		bossY = bossY + bossDashY * dash_step;

		bossdashRemaining = bossdashRemaining - dash_step;

		if (bossdashRemaining <= 0.0f)
		{
			bossState = 0;
			bossDashTimer = boss_dash_cooldown;
		}
	}
}

void boss_draw(void)
{
	if (bossActive == 0)
	{
		return;
	}

	CP_Settings_NoStroke();

	if (bossState == 1)
	{
		CP_Settings_Fill(CP_Color_Create(255, 200, 50, 255));
	}
	else if (bossState == 2)
	{
		CP_Settings_Fill(CP_Color_Create(255, 50, 50, 255));
	}
	else
	{
		CP_Settings_Fill(CP_Color_Create(180, 50, 100, 255));
	}

	CP_Graphics_DrawCircle(bossX, bossY, boss_size);
}

void boss_take_damage(float damage)
{
	if (bossActive == 0 || damage <= 0.0f)
	{
		return;
	}

	bossHealth = bossHealth - damage;

	if (bossHealth <= 0.0f)
	{
		bossHealth = 0.0f;
		bossActive = 0;
		return;
	}

	if (bossHealth <= boss_health_max / 2.0f)
	{
		bossPhase = 2;
	}
}

void boss_take_AOE_damage(float x, float y, float radius, float damage)
{
	if (bossActive == 0)
	{
		return;
	}

	float dx = bossX - x;
	float dy = bossY - y;
	float distance = sqrtf((dx * dx) + (dy * dy));

	if (distance <= radius + boss_size / 2.0f)
	{
		boss_take_damage(damage);
	}
}

float boss_get_collision_damage(float x, float y, float radius)
{
	if (bossActive == 0)
	{
		return 0.0f;
	}

	float dx = bossX - x;
	float dy = bossY - y;
	float distanceSquared = (dx * dx) + (dy * dy);
	float radius_sum = (boss_size / 2.0f) + radius;

	if (distanceSquared <= radius_sum * radius_sum)
	{
		if (bossState == 2)
		{
			return boss_dash_collision_damage;
		}

		else
		{
			return boss_collision_damage;
		}
	}
	return 0.0f;
}

int boss_get_target(float x, float y, float range, CP_Vector* target)
{
	if (bossActive == 0 || target == NULL)
	{
		return 0;
	}

	float dx = bossX - x;
	float dy = bossY - y;
	float distance = sqrtf((dx * dx) + (dy * dy));

	if (distance <= range)
	{
		*target = CP_Vector_Set(bossX, bossY);
		return 1;
	}

	else
	{
		return 0;
	}
}

void boss_reset(void)
{
	bossActive = 0;
}