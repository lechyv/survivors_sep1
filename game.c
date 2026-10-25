//---------------------------------------------------------
// file:    game.c
// authors: Javier, Brandon, Elvis, Wee Jie
// brief:   All gameplay for Survivor, in sections by owner:
//            Brandon - projectiles, effects
//            Javier  - player, enemies, waves, drone movement
//            Brandon - drone firing, boss, combat
//            Elvis   - economy, shop logic, buffs, drone prices
//            Wee Jie - HUD and screens, game flow
//          Flow: WAVE 1 -> SHOP -> WAVE 2 -> SHOP -> FINAL BOSS -> VICTORY
//          Only edit your own section to avoid merge conflicts.
//---------------------------------------------------------
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include "cprocessing.h"
#include "game.h"
#include "mainmenu.h"
#include "utils.h"

//=========================================================
// PROJECTILES
// owner: Brandon
//=========================================================
typedef struct Projectile
{
	int       active;
	CP_Vector pos;
	CP_Vector vel;
	float     damage;
	float     splashRadius;   // > 0 for rockets
	float     radius;
	float     age;
	CP_Color  color;
} Projectile;

void Projectile_Spawn(Projectile list[], int max, const Projectile* p);
void Projectile_UpdateAll(Projectile list[], int max, float dt);
void Projectile_DrawAll(const Projectile list[], int max);
void Projectile_ClearAll(Projectile list[], int max);

#define MAX_LIFETIME 2.5f
#define TRAIL_LENGTH 14.0f

void Projectile_Spawn(Projectile list[], int max, const Projectile* p)
{
	for (int i = 0; i < max; ++i)
	{
		if (!list[i].active)
		{
			list[i] = *p;
			list[i].active = 1;
			list[i].age = 0.0f;
			return;
		}
	}
	// Pool full: drop the shot. Raise MAX_PROJECTILES if this happens a lot.
}

void Projectile_UpdateAll(Projectile list[], int max, float dt)
{
	for (int i = 0; i < max; ++i)
	{
		Projectile* p = &list[i];
		if (!p->active)
			continue;

		p->pos = CP_Vector_Add(p->pos, CP_Vector_Scale(p->vel, dt));
		p->age += dt;
		if (p->age > MAX_LIFETIME || Util_IsOutsideWindow(p->pos, 40.0f))
			p->active = 0;
	}
}

void Projectile_DrawAll(const Projectile list[], int max)
{
	for (int i = 0; i < max; ++i)
	{
		const Projectile* p = &list[i];
		if (!p->active)
			continue;

		CP_Vector tail = CP_Vector_Subtract(p->pos, CP_Vector_Scale(Util_SafeNormalize(p->vel), TRAIL_LENGTH));
		CP_Settings_Stroke(Util_WithAlpha(p->color, 0.5f));
		CP_Settings_StrokeWeight(2.0f);
		CP_Graphics_DrawLine(tail.x, tail.y, p->pos.x, p->pos.y);

		CP_Settings_NoStroke();
		CP_Settings_Fill(Util_Color(255, 230, 150, 255));
		CP_Graphics_DrawCircle(p->pos.x, p->pos.y, p->radius * 2.0f);
	}
}

void Projectile_ClearAll(Projectile list[], int max)
{
	for (int i = 0; i < max; ++i)
		list[i].active = 0;
}

//=========================================================
// HIT EFFECTS
// owner: Brandon
//=========================================================
typedef struct Effect
{
	int       active;
	CP_Vector pos;
	float     maxRadius;
	float     age;
	float     life;
	CP_Color  color;
} Effect;

void Fx_Spawn(Effect list[], int max, CP_Vector pos, float maxRadius, float life, CP_Color color);
void Fx_UpdateAll(Effect list[], int max, float dt);
void Fx_DrawAll(const Effect list[], int max);
void Fx_ClearAll(Effect list[], int max);

void Fx_Spawn(Effect list[], int max, CP_Vector pos, float maxRadius, float life, CP_Color color)
{
	for (int i = 0; i < max; ++i)
	{
		if (!list[i].active)
		{
			list[i].active = 1;
			list[i].pos = pos;
			list[i].maxRadius = maxRadius;
			list[i].age = 0.0f;
			list[i].life = life;
			list[i].color = color;
			return;
		}
	}
}

void Fx_UpdateAll(Effect list[], int max, float dt)
{
	for (int i = 0; i < max; ++i)
	{
		if (!list[i].active)
			continue;
		list[i].age += dt;
		if (list[i].age >= list[i].life)
			list[i].active = 0;
	}
}

void Fx_DrawAll(const Effect list[], int max)
{
	CP_Settings_NoFill();
	CP_Settings_StrokeWeight(2.0f);
	for (int i = 0; i < max; ++i)
	{
		const Effect* e = &list[i];
		if (!e->active)
			continue;
		float t = e->age / e->life;
		CP_Settings_Stroke(Util_WithAlpha(e->color, 1.0f - t));
		CP_Graphics_DrawCircle(e->pos.x, e->pos.y, e->maxRadius * t * 2.0f);
	}
}

void Fx_ClearAll(Effect list[], int max)
{
	for (int i = 0; i < max; ++i)
		list[i].active = 0;
}

//=========================================================
// PLAYER (movement, dash, health)
// owner: Javier
//=========================================================
#define PLAYER_RADIUS      16.0f
#define PLAYER_MAX_HEALTH  100.0f

typedef struct Player
{
	CP_Vector pos;
	CP_Vector facing;        // last movement direction, used as dash direction
	CP_Vector dashDir;
	float     health;
	float     dashTimer;     // > 0 while dashing
	float     dashCooldown;  // > 0 while dash is recharging
	float     hurtTimer;     // > 0 while invulnerable after a hit
} Player;

void  Player_Init(Player* p, CP_Vector start);
void  Player_Update(Player* p, float dt);
void  Player_Draw(const Player* p);

void  Player_TakeDamage(Player* p, float amount);
void  Player_Heal(Player* p, float amount);

int   Player_IsDead(const Player* p);
int   Player_IsDashing(const Player* p);
int   Player_IsInvulnerable(const Player* p);
float Player_DashReady01(const Player* p);   // 0 = just used, 1 = ready

#define MOVE_SPEED       250.0f
#define DASH_SPEED       950.0f
#define DASH_DURATION    0.14f
#define DASH_COOLDOWN    1.10f
#define HURT_INVULN_TIME 0.60f

static CP_Vector ReadMoveInput(void)
{
	CP_Vector input = CP_Vector_Zero();
	if (CP_Input_KeyDown(KEY_W) || CP_Input_KeyDown(KEY_UP))    input.y -= 1.0f;
	if (CP_Input_KeyDown(KEY_S) || CP_Input_KeyDown(KEY_DOWN))  input.y += 1.0f;
	if (CP_Input_KeyDown(KEY_A) || CP_Input_KeyDown(KEY_LEFT))  input.x -= 1.0f;
	if (CP_Input_KeyDown(KEY_D) || CP_Input_KeyDown(KEY_RIGHT)) input.x += 1.0f;
	return Util_SafeNormalize(input);
}

void Player_Init(Player* p, CP_Vector start)
{
	p->pos = start;
	p->facing = CP_Vector_Set(1.0f, 0.0f);
	p->dashDir = p->facing;
	p->health = PLAYER_MAX_HEALTH;
	p->dashTimer = 0.0f;
	p->dashCooldown = 0.0f;
	p->hurtTimer = 0.0f;
}

void Player_Update(Player* p, float dt)
{
	CP_Vector input = ReadMoveInput();
	CP_Vector velocity;

	p->dashCooldown = Util_MaxF(0.0f, p->dashCooldown - dt);
	p->hurtTimer = Util_MaxF(0.0f, p->hurtTimer - dt);

	if (input.x != 0.0f || input.y != 0.0f)
		p->facing = input;

	if (!Player_IsDashing(p) && p->dashCooldown <= 0.0f &&
		(CP_Input_KeyTriggered(KEY_SPACE) || CP_Input_KeyTriggered(KEY_LEFT_SHIFT)))
	{
		p->dashDir = p->facing;
		p->dashTimer = DASH_DURATION;
		p->dashCooldown = DASH_COOLDOWN;
	}

	if (Player_IsDashing(p))
	{
		p->dashTimer -= dt;
		velocity = CP_Vector_Scale(p->dashDir, DASH_SPEED);
	}
	else
	{
		velocity = CP_Vector_Scale(input, MOVE_SPEED);
	}

	p->pos = Util_ClampToWindow(CP_Vector_Add(p->pos, CP_Vector_Scale(velocity, dt)), PLAYER_RADIUS);
}

void Player_Draw(const Player* p)
{
	CP_Color body = Player_IsDashing(p) ? Util_Color(150, 255, 220, 255) : Util_Color(32, 160, 130, 255);

	// Faint ring around the player, like the prototype
	CP_Settings_NoFill();
	CP_Settings_Stroke(Util_Color(32, 160, 130, 90));
	CP_Settings_StrokeWeight(2.0f);
	CP_Graphics_DrawCircle(p->pos.x, p->pos.y, (PLAYER_RADIUS + 20.0f) * 2.0f);

	// Blink while invulnerable after being hit
	if (p->hurtTimer > 0.0f && ((int)(p->hurtTimer * 20.0f) % 2) == 0)
		return;

	CP_Settings_NoStroke();
	CP_Settings_Fill(body);
	CP_Graphics_DrawCircle(p->pos.x, p->pos.y, PLAYER_RADIUS * 2.0f);
}

void Player_TakeDamage(Player* p, float amount)
{
	if (Player_IsInvulnerable(p) || Player_IsDead(p))
		return;
	p->health = Util_MaxF(0.0f, p->health - amount);
	p->hurtTimer = HURT_INVULN_TIME;
}

void Player_Heal(Player* p, float amount)
{
	p->health = Util_MinF(PLAYER_MAX_HEALTH, p->health + amount);
}

int Player_IsDead(const Player* p)         { return p->health <= 0.0f; }
int Player_IsDashing(const Player* p)      { return p->dashTimer > 0.0f; }
int Player_IsInvulnerable(const Player* p) { return Player_IsDashing(p) || p->hurtTimer > 0.0f; }

float Player_DashReady01(const Player* p)
{
	return 1.0f - CP_Math_ClampFloat(p->dashCooldown / DASH_COOLDOWN, 0.0f, 1.0f);
}

//=========================================================
// ENEMY SWARM
// owner: Javier
//=========================================================
#define ENEMY_RADIUS 11.0f

typedef struct Enemy
{
	int       active;
	CP_Vector pos;
	CP_Vector facing;
	float     health;
	float     speed;
	float     contactDamage;
	int       coinReward;
} Enemy;

// Spawns into the first free slot. Returns 1 on success, 0 if the pool is full.
// Difficulty multipliers are applied inside; the extra multipliers are per wave.
int  Enemy_Spawn(Enemy enemies[], int max, CP_Vector pos, float healthMult, float speedMult);

void Enemy_UpdateAll(Enemy enemies[], int max, CP_Vector target, float dt);
void Enemy_DrawAll(const Enemy enemies[], int max);
int  Enemy_ActiveCount(const Enemy enemies[], int max);
void Enemy_ClearAll(Enemy enemies[], int max);

#define ENEMY_BASE_HEALTH         20.0f
#define ENEMY_BASE_SPEED          110.0f
#define ENEMY_BASE_CONTACT_DAMAGE 10.0f
#define ENEMY_BASE_COIN_REWARD    2
#define SEPARATION_PUSH     0.5f

int Enemy_Spawn(Enemy enemies[], int max, CP_Vector pos, float healthMult, float speedMult)
{
	const DifficultySettings* d = Difficulty_Settings();
	for (int i = 0; i < max; ++i)
	{
		Enemy* e = &enemies[i];
		if (e->active)
			continue;

		e->active = 1;
		e->pos = pos;
		e->facing = CP_Vector_Set(1.0f, 0.0f);
		e->health = ENEMY_BASE_HEALTH * healthMult * d->enemyHealth;
		e->speed = ENEMY_BASE_SPEED * speedMult * d->enemySpeed;
		e->contactDamage = ENEMY_BASE_CONTACT_DAMAGE * d->contactDamage;
		e->coinReward = ENEMY_BASE_COIN_REWARD;
		return 1;
	}
	return 0;
}

// Pushes overlapping enemies apart. O(n^2) is fine for a few hundred enemies.
static void Separate(Enemy enemies[], int max)
{
	const float minDist = ENEMY_RADIUS * 2.0f;
	for (int i = 0; i < max; ++i)
	{
		if (!enemies[i].active)
			continue;
		for (int j = i + 1; j < max; ++j)
		{
			if (!enemies[j].active)
				continue;

			CP_Vector delta = CP_Vector_Subtract(enemies[j].pos, enemies[i].pos);
			float dist = CP_Vector_Length(delta);
			if (dist >= minDist || dist < 0.0001f)
				continue;

			CP_Vector push = CP_Vector_Scale(delta, (minDist - dist) / dist * SEPARATION_PUSH);
			enemies[i].pos = CP_Vector_Subtract(enemies[i].pos, push);
			enemies[j].pos = CP_Vector_Add(enemies[j].pos, push);
		}
	}
}

void Enemy_UpdateAll(Enemy enemies[], int max, CP_Vector target, float dt)
{
	for (int i = 0; i < max; ++i)
	{
		Enemy* e = &enemies[i];
		if (!e->active)
			continue;

		CP_Vector dir = Util_SafeNormalize(CP_Vector_Subtract(target, e->pos));
		if (dir.x != 0.0f || dir.y != 0.0f)
			e->facing = dir;
		e->pos = CP_Vector_Add(e->pos, CP_Vector_Scale(dir, e->speed * dt));
	}
	Separate(enemies, max);
}

void Enemy_DrawAll(const Enemy enemies[], int max)
{
	CP_Color color = Util_Color(230, 100, 50, 255);
	for (int i = 0; i < max; ++i)
	{
		if (enemies[i].active)
			Util_DrawArrow(enemies[i].pos, enemies[i].facing, ENEMY_RADIUS, color);
	}
}

int Enemy_ActiveCount(const Enemy enemies[], int max)
{
	int count = 0;
	for (int i = 0; i < max; ++i)
		count += enemies[i].active ? 1 : 0;
	return count;
}

void Enemy_ClearAll(Enemy enemies[], int max)
{
	for (int i = 0; i < max; ++i)
		enemies[i].active = 0;
}

//=========================================================
// WAVES
// owner: Javier
//=========================================================
typedef struct WaveManager
{
	int   wave;          // 1 or 2
	int   total;         // enemies in this wave
	int   remaining;     // enemies still to spawn
	float spawnTimer;
	float interval;
	float healthMult;
	float speedMult;
} WaveManager;

void      Wave_Start(WaveManager* w, int wave);
void      Wave_Update(WaveManager* w, Enemy enemies[], int max, float dt);
int       Wave_SpawningDone(const WaveManager* w);
CP_Vector Wave_RandomEdgePoint(void);

//                                 wave 1  wave 2
static const int   BASE_COUNT[]    = { 30,    60    };
static const float BASE_INTERVAL[] = { 2.00f, 0.90f };   // seconds between groups
static const float HEALTH_MULT[]   = { 1.00f, 1.60f };
static const float SPEED_MULT[]    = { 1.00f, 1.15f };

#define GROUP_SIZE   3
#define EDGE_OFFSET  20.0f
#define GROUP_SPREAD 30.0f

void Wave_Start(WaveManager* w, int wave)
{
	const DifficultySettings* d = Difficulty_Settings();
	int index = wave - 1;
	if (index < 0) index = 0;
	if (index >= SWARM_WAVE_COUNT) index = SWARM_WAVE_COUNT - 1;

	w->wave = index + 1;
	w->total = (int)((float)BASE_COUNT[index] * d->enemyCount + 0.5f);
	w->remaining = w->total;
	w->interval = BASE_INTERVAL[index] * d->spawnInterval;
	w->spawnTimer = 1.0f;   // short grace period before the first group
	w->healthMult = HEALTH_MULT[index];
	w->speedMult = SPEED_MULT[index];
}

CP_Vector Wave_RandomEdgePoint(void)
{
	float w = (float)WINDOW_WIDTH;
	float h = (float)WINDOW_HEIGHT;
	switch (CP_Random_RangeInt(0, 3))
	{
	case 0:  return CP_Vector_Set(CP_Random_RangeFloat(0.0f, w), -EDGE_OFFSET);     // top
	case 1:  return CP_Vector_Set(CP_Random_RangeFloat(0.0f, w), h + EDGE_OFFSET);  // bottom
	case 2:  return CP_Vector_Set(-EDGE_OFFSET, CP_Random_RangeFloat(0.0f, h));     // left
	default: return CP_Vector_Set(w + EDGE_OFFSET, CP_Random_RangeFloat(0.0f, h));  // right
	}
}

void Wave_Update(WaveManager* w, Enemy enemies[], int max, float dt)
{
	if (Wave_SpawningDone(w))
		return;

	w->spawnTimer -= dt;
	if (w->spawnTimer > 0.0f)
		return;
	w->spawnTimer += w->interval;

	CP_Vector origin = Wave_RandomEdgePoint();
	for (int i = 0; i < GROUP_SIZE && w->remaining > 0; ++i)
	{
		CP_Vector offset = CP_Vector_Set(CP_Random_RangeFloat(-GROUP_SPREAD, GROUP_SPREAD),
			CP_Random_RangeFloat(-GROUP_SPREAD, GROUP_SPREAD));
		if (!Enemy_Spawn(enemies, max, CP_Vector_Add(origin, offset), w->healthMult, w->speedMult))
			return;   // pool full: try again next interval
		w->remaining--;
	}
}

int Wave_SpawningDone(const WaveManager* w)
{
	return w->remaining <= 0;
}

//=========================================================
// DRONES (movement: Javier, firing: Brandon, prices: Elvis)
// owner: Javier + Brandon + Elvis
//=========================================================
typedef enum DroneType
{
	DRONE_BASIC,
	DRONE_MACHINE_GUN,
	DRONE_ROCKET,
	DRONE_TYPE_COUNT
} DroneType;

typedef struct DroneStats
{
	const char* name;
	int         cost;             // shop price in coins
	float       fireRate;         // shots per second
	float       damage;
	float       projectileSpeed;
	float       range;
	float       splashRadius;     // 0 = single target
	float       projectileRadius;
	CP_Color    color;
} DroneStats;

typedef struct Drone
{
	int       active;
	DroneType type;
	CP_Vector pos;
	CP_Vector aim;
	float     cooldown;
} Drone;

const DroneStats* Drone_GetStats(DroneType type);

void Drone_Init(Drone* d, DroneType type, CP_Vector pos);

// Javier: keeps drone 'index' of 'count' on an evenly spaced, rotating orbit
void Drone_UpdateMovement(Drone* d, CP_Vector center, int index, int count, float time, float dt);

// Brandon: returns 1 and fills 'out' when the drone fires this frame
int  Drone_TryFire(Drone* d, int hasTarget, CP_Vector target, float damageMult,
	float fireRateMult, float dt, Projectile* out);

void Drone_Draw(const Drone* d);

#define ORBIT_RADIUS      60.0f
#define ORBIT_SPEED       1.2f    // radians per second
#define FOLLOW_SHARPNESS  10.0f   // higher = drones snap to their slot faster
#define DRONE_SIZE        10.0f

// Balancing table. Elvis owns prices, Brandon owns combat numbers.
static const DroneStats STATS[DRONE_TYPE_COUNT] =
{
	// name                 cost rate   dmg    speed   range   splash  projR  color
	{ "BASIC DRONE",        20,  2.0f,  10.0f, 650.0f, 380.0f,  0.0f,  4.0f, { { 80, 220, 170, 255 } } },
	{ "MACHINE GUN DRONE",  45,  6.0f,   3.5f, 800.0f, 320.0f,  0.0f,  3.0f, { { 120, 200, 255, 255 } } },
	{ "ROCKET DRONE",       70,  0.6f,  35.0f, 420.0f, 450.0f, 70.0f,  6.0f, { { 255, 170, 80, 255 } } },
};

const DroneStats* Drone_GetStats(DroneType type)
{
	if (type < DRONE_BASIC || type >= DRONE_TYPE_COUNT)
		type = DRONE_BASIC;
	return &STATS[type];
}

void Drone_Init(Drone* d, DroneType type, CP_Vector pos)
{
	d->active = 1;
	d->type = type;
	d->pos = pos;
	d->aim = CP_Vector_Set(1.0f, 0.0f);
	d->cooldown = 0.0f;
}

//---------------------------------------------------------
// Movement (Javier)
//---------------------------------------------------------
void Drone_UpdateMovement(Drone* d, CP_Vector center, int index, int count, float time, float dt)
{
	if (count < 1)
		count = 1;

	float angle = time * ORBIT_SPEED + (float)index * TAU_F / (float)count;
	CP_Vector slot = CP_Vector_Add(center, CP_Vector_Set(cosf(angle) * ORBIT_RADIUS, sinf(angle) * ORBIT_RADIUS));

	// Frame-rate independent smoothing toward the orbit slot
	float t = 1.0f - expf(-FOLLOW_SHARPNESS * dt);
	d->pos.x = CP_Math_LerpFloat(d->pos.x, slot.x, t);
	d->pos.y = CP_Math_LerpFloat(d->pos.y, slot.y, t);
}

//---------------------------------------------------------
// Firing (Brandon)
//---------------------------------------------------------
int Drone_TryFire(Drone* d, int hasTarget, CP_Vector target, float damageMult,
	float fireRateMult, float dt, Projectile* out)
{
	const DroneStats* s = Drone_GetStats(d->type);
	d->cooldown = Util_MaxF(0.0f, d->cooldown - dt);

	if (!hasTarget)
		return 0;

	CP_Vector toTarget = CP_Vector_Subtract(target, d->pos);
	if (CP_Vector_Length(toTarget) > s->range)
		return 0;

	d->aim = Util_SafeNormalize(toTarget);
	if (d->cooldown > 0.0f)
		return 0;

	d->cooldown = 1.0f / (s->fireRate * fireRateMult);

	out->active = 1;
	out->pos = d->pos;
	out->vel = CP_Vector_Scale(d->aim, s->projectileSpeed);
	out->damage = s->damage * damageMult;
	out->splashRadius = s->splashRadius;
	out->radius = s->projectileRadius;
	out->age = 0.0f;
	out->color = s->color;
	return 1;
}

void Drone_Draw(const Drone* d)
{
	Util_DrawArrow(d->pos, d->aim, DRONE_SIZE, Drone_GetStats(d->type)->color);
}

//=========================================================
// FINAL BOSS
// owner: Brandon
//=========================================================
#define BOSS_RADIUS 48.0f

typedef enum BossPhase
{
	BOSS_CHASE,
	BOSS_WINDUP,
	BOSS_CHARGE,
	BOSS_RECOVER
} BossPhase;

typedef struct Boss
{
	int       active;
	CP_Vector pos;
	CP_Vector facing;
	CP_Vector chargeDir;
	float     health;
	float     maxHealth;
	float     contactDamage;
	float     timer;
	BossPhase phase;
	int       cycle;
} Boss;

void Boss_Reset(Boss* b);
void Boss_Spawn(Boss* b, CP_Vector pos);
void Boss_Update(Boss* b, CP_Vector playerPos, Enemy enemies[], int maxEnemies, float dt);
void Boss_Draw(const Boss* b);
int  Boss_IsEnraged(const Boss* b);

#define BOSS_BASE_HEALTH         1500.0f
#define BOSS_BASE_CONTACT_DAMAGE 25.0f
#define CHASE_SPEED         85.0f
#define CHASE_SPEED_ENRAGED 120.0f
#define CHARGE_SPEED        700.0f
#define CHARGE_SPEED_ENRAGED 850.0f
#define CHASE_TIME          2.8f
#define CHASE_TIME_ENRAGED  1.8f
#define WINDUP_TIME         0.8f
#define WINDUP_TIME_ENRAGED 0.5f
#define CHARGE_TIME         0.55f
#define RECOVER_TIME        0.7f
#define SUMMON_COUNT        5
#define SUMMON_COUNT_ENRAGED 8
#define SUMMON_RING_RADIUS  90.0f
#define MINION_HEALTH_MULT  1.3f

void Boss_Reset(Boss* b)
{
	b->active = 0;
	b->health = 0.0f;
	b->maxHealth = 1.0f;
}

void Boss_Spawn(Boss* b, CP_Vector pos)
{
	const DifficultySettings* d = Difficulty_Settings();
	b->active = 1;
	b->pos = pos;
	b->facing = CP_Vector_Set(0.0f, 1.0f);
	b->chargeDir = b->facing;
	b->maxHealth = BOSS_BASE_HEALTH * d->bossHealth;
	b->health = b->maxHealth;
	b->contactDamage = BOSS_BASE_CONTACT_DAMAGE * d->contactDamage;
	b->phase = BOSS_CHASE;
	b->timer = CHASE_TIME;
	b->cycle = 0;
}

int Boss_IsEnraged(const Boss* b)
{
	return b->health < b->maxHealth * 0.5f;
}

static void Summon(const Boss* b, Enemy enemies[], int maxEnemies)
{
	int count = Boss_IsEnraged(b) ? SUMMON_COUNT_ENRAGED : SUMMON_COUNT;
	for (int i = 0; i < count; ++i)
	{
		float angle = (float)i * TAU_F / (float)count;
		CP_Vector offset = CP_Vector_Set(cosf(angle) * SUMMON_RING_RADIUS, sinf(angle) * SUMMON_RING_RADIUS);
		Enemy_Spawn(enemies, maxEnemies, CP_Vector_Add(b->pos, offset), MINION_HEALTH_MULT, 1.0f);
	}
}

void Boss_Update(Boss* b, CP_Vector playerPos, Enemy enemies[], int maxEnemies, float dt)
{
	if (!b->active)
		return;

	int enraged = Boss_IsEnraged(b);
	CP_Vector toPlayer = Util_SafeNormalize(CP_Vector_Subtract(playerPos, b->pos));
	b->timer -= dt;

	switch (b->phase)
	{
	case BOSS_CHASE:
		if (toPlayer.x != 0.0f || toPlayer.y != 0.0f)
			b->facing = toPlayer;
		b->pos = CP_Vector_Add(b->pos, CP_Vector_Scale(b->facing, (enraged ? CHASE_SPEED_ENRAGED : CHASE_SPEED) * dt));
		if (b->timer <= 0.0f)
		{
			b->phase = BOSS_WINDUP;
			b->timer = enraged ? WINDUP_TIME_ENRAGED : WINDUP_TIME;
		}
		break;

	case BOSS_WINDUP:
		// Keeps tracking the player while telegraphing, then locks direction
		if (toPlayer.x != 0.0f || toPlayer.y != 0.0f)
			b->facing = toPlayer;
		if (b->timer <= 0.0f)
		{
			b->chargeDir = b->facing;
			b->phase = BOSS_CHARGE;
			b->timer = CHARGE_TIME;
		}
		break;

	case BOSS_CHARGE:
	{
		CP_Vector next = CP_Vector_Add(b->pos, CP_Vector_Scale(b->chargeDir, (enraged ? CHARGE_SPEED_ENRAGED : CHARGE_SPEED) * dt));
		CP_Vector clamped = Util_ClampToWindow(next, BOSS_RADIUS);
		int hitWall = clamped.x != next.x || clamped.y != next.y;
		b->pos = clamped;
		if (b->timer <= 0.0f || hitWall)
		{
			b->phase = BOSS_RECOVER;
			b->timer = RECOVER_TIME;
		}
		break;
	}

	case BOSS_RECOVER:
	default:
		if (b->timer <= 0.0f)
		{
			b->cycle++;
			if (b->cycle % 2 == 0)
				Summon(b, enemies, maxEnemies);
			b->phase = BOSS_CHASE;
			b->timer = enraged ? CHASE_TIME_ENRAGED : CHASE_TIME;
		}
		break;
	}

	b->pos = Util_ClampToWindow(b->pos, BOSS_RADIUS);
}

void Boss_Draw(const Boss* b)
{
	if (!b->active)
		return;

	CP_Color color = Boss_IsEnraged(b) ? Util_Color(255, 60, 60, 255) : Util_Color(235, 95, 45, 255);

	if (b->phase == BOSS_WINDUP)
	{
		// Telegraph: line showing where the charge will go, and a flashing body
		CP_Vector end = CP_Vector_Add(b->pos, CP_Vector_Scale(b->facing, 400.0f));
		CP_Settings_Stroke(Util_Color(255, 80, 80, 140));
		CP_Settings_StrokeWeight(3.0f);
		CP_Graphics_DrawLine(b->pos.x, b->pos.y, end.x, end.y);

		if (((int)(b->timer * 12.0f) % 2) == 0)
			color = Util_Color(255, 255, 255, 255);
	}

	Util_DrawArrow(b->pos, b->facing, BOSS_RADIUS * 1.3f, color);
}

//=========================================================
// COMBAT (collision, damage, targeting)
// owner: Brandon
//=========================================================
// Nearest enemy or boss within range of 'from'. Returns 1 and sets outTarget if found.
int  Combat_FindTarget(CP_Vector from, float range, const Enemy enemies[], int maxEnemies,
	const Boss* boss, CP_Vector* outTarget);

// Projectiles damage enemies/boss. Rockets deal splash damage.
void Combat_ResolveProjectiles(Projectile projectiles[], int maxProjectiles,
	Enemy enemies[], int maxEnemies, Boss* boss, Effect fx[], int maxFx);

// Enemies and the boss damage the player by touching (no enemy projectiles)
void Combat_ResolveContact(Player* player, const Enemy enemies[], int maxEnemies, const Boss* boss);

int Combat_FindTarget(CP_Vector from, float range, const Enemy enemies[], int maxEnemies,
	const Boss* boss, CP_Vector* outTarget)
{
	float bestDistSq = range * range;
	int found = 0;

	for (int i = 0; i < maxEnemies; ++i)
	{
		if (!enemies[i].active || enemies[i].health <= 0.0f)
			continue;
		float dx = enemies[i].pos.x - from.x;
		float dy = enemies[i].pos.y - from.y;
		float distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			*outTarget = enemies[i].pos;
			found = 1;
		}
	}

	if (boss->active && boss->health > 0.0f)
	{
		float dx = boss->pos.x - from.x;
		float dy = boss->pos.y - from.y;
		// Measure to the boss's edge so drones engage it as soon as it is in reach
		float edge = Util_MaxF(0.0f, CP_Vector_Length(CP_Vector_Set(dx, dy)) - BOSS_RADIUS);
		if (edge * edge < bestDistSq)
		{
			*outTarget = boss->pos;
			found = 1;
		}
	}
	return found;
}

static void ApplySplash(CP_Vector center, float radius, float damage,
	Enemy enemies[], int maxEnemies, Boss* boss)
{
	for (int i = 0; i < maxEnemies; ++i)
	{
		if (enemies[i].active && Util_CirclesOverlap(center, radius, enemies[i].pos, ENEMY_RADIUS))
			enemies[i].health -= damage;
	}
	if (boss->active && Util_CirclesOverlap(center, radius, boss->pos, BOSS_RADIUS))
		boss->health -= damage;
}

void Combat_ResolveProjectiles(Projectile projectiles[], int maxProjectiles,
	Enemy enemies[], int maxEnemies, Boss* boss, Effect fx[], int maxFx)
{
	for (int p = 0; p < maxProjectiles; ++p)
	{
		Projectile* shot = &projectiles[p];
		if (!shot->active)
			continue;

		Enemy* hitEnemy = NULL;
		for (int i = 0; i < maxEnemies; ++i)
		{
			Enemy* e = &enemies[i];
			if (e->active && e->health > 0.0f && Util_CirclesOverlap(shot->pos, shot->radius, e->pos, ENEMY_RADIUS))
			{
				hitEnemy = e;
				break;
			}
		}
		int hitBoss = boss->active && boss->health > 0.0f &&
			Util_CirclesOverlap(shot->pos, shot->radius, boss->pos, BOSS_RADIUS);

		if (hitEnemy == NULL && !hitBoss)
			continue;

		shot->active = 0;

		if (shot->splashRadius > 0.0f)
		{
			ApplySplash(shot->pos, shot->splashRadius, shot->damage, enemies, maxEnemies, boss);
			Fx_Spawn(fx, maxFx, shot->pos, shot->splashRadius, 0.30f, Util_Color(255, 170, 80, 255));
		}
		else
		{
			if (hitEnemy != NULL)
				hitEnemy->health -= shot->damage;
			else
				boss->health -= shot->damage;
			Fx_Spawn(fx, maxFx, shot->pos, 8.0f, 0.15f, Util_Color(255, 230, 150, 255));
		}
	}
}

void Combat_ResolveContact(Player* player, const Enemy enemies[], int maxEnemies, const Boss* boss)
{
	// Player_TakeDamage ignores hits while dashing or during hit invulnerability
	for (int i = 0; i < maxEnemies; ++i)
	{
		if (enemies[i].active && Util_CirclesOverlap(player->pos, PLAYER_RADIUS, enemies[i].pos, ENEMY_RADIUS))
			Player_TakeDamage(player, enemies[i].contactDamage);
	}
	if (boss->active && Util_CirclesOverlap(player->pos, PLAYER_RADIUS, boss->pos, BOSS_RADIUS))
		Player_TakeDamage(player, boss->contactDamage);
}

//=========================================================
// ECONOMY (coins, shop logic, drone cap, passive buffs)
// owner: Elvis
//=========================================================
#define KILLS_PER_BUFF  20
#define BUFF_STEP       0.10f    // +10% per buff
#define HEAL_COST       15
#define HEAL_AMOUNT     30.0f

typedef enum ShopResult
{
	SHOP_OK,
	SHOP_NOT_ENOUGH_COINS,
	SHOP_DRONE_CAP_REACHED,
	SHOP_ALREADY_FULL_HEALTH
} ShopResult;

typedef struct Economy
{
	int   coins;
	int   kills;
	int   buffLevel;
	float damageMult;
	float fireRateMult;
	float messageTimer;
	char  message[64];
} Economy;

void       Economy_Init(Economy* e);
void       Economy_Update(Economy* e, float dt);
void       Economy_RegisterKill(Economy* e, int baseReward);

ShopResult Economy_CanBuyDrone(const Economy* e, DroneType type, int droneCount);
ShopResult Economy_BuyDrone(Economy* e, DroneType type, int droneCount);   // caller adds the drone on SHOP_OK
ShopResult Economy_BuyHeal(Economy* e, Player* player);

const char* Economy_ResultText(ShopResult result);

#define BUFF_MESSAGE_TIME 2.5f

void Economy_Init(Economy* e)
{
	e->coins = 0;
	e->kills = 0;
	e->buffLevel = 0;
	e->damageMult = 1.0f;
	e->fireRateMult = 1.0f;
	e->messageTimer = 0.0f;
	e->message[0] = '\0';
}

void Economy_Update(Economy* e, float dt)
{
	if (e->messageTimer > 0.0f)
		e->messageTimer -= dt;
}

// Passive buffs alternate between drone damage and drone fire rate
static void ApplyPassiveBuff(Economy* e)
{
	e->buffLevel++;
	if (e->buffLevel % 2 == 1)
	{
		e->damageMult += BUFF_STEP;
		snprintf(e->message, sizeof(e->message), "PASSIVE BUFF: +10%% DRONE DAMAGE");
	}
	else
	{
		e->fireRateMult += BUFF_STEP;
		snprintf(e->message, sizeof(e->message), "PASSIVE BUFF: +10%% DRONE FIRE RATE");
	}
	e->messageTimer = BUFF_MESSAGE_TIME;
}

void Economy_RegisterKill(Economy* e, int baseReward)
{
	int reward = (int)((float)baseReward * Difficulty_Settings()->coinReward + 0.5f);
	if (reward < 1)
		reward = 1;

	e->coins += reward;
	e->kills++;
	if (e->kills % KILLS_PER_BUFF == 0)
		ApplyPassiveBuff(e);
}

ShopResult Economy_CanBuyDrone(const Economy* e, DroneType type, int droneCount)
{
	if (droneCount >= MAX_DRONES)
		return SHOP_DRONE_CAP_REACHED;
	if (e->coins < Drone_GetStats(type)->cost)
		return SHOP_NOT_ENOUGH_COINS;
	return SHOP_OK;
}

ShopResult Economy_BuyDrone(Economy* e, DroneType type, int droneCount)
{
	ShopResult result = Economy_CanBuyDrone(e, type, droneCount);
	if (result == SHOP_OK)
		e->coins -= Drone_GetStats(type)->cost;
	return result;
}

ShopResult Economy_BuyHeal(Economy* e, Player* player)
{
	if (player->health >= PLAYER_MAX_HEALTH)
		return SHOP_ALREADY_FULL_HEALTH;
	if (e->coins < HEAL_COST)
		return SHOP_NOT_ENOUGH_COINS;
	e->coins -= HEAL_COST;
	Player_Heal(player, HEAL_AMOUNT);
	return SHOP_OK;
}

const char* Economy_ResultText(ShopResult result)
{
	switch (result)
	{
	case SHOP_OK:                  return "PURCHASED!";
	case SHOP_NOT_ENOUGH_COINS:    return "NOT ENOUGH COINS";
	case SHOP_DRONE_CAP_REACHED:   return "DRONE CAP REACHED (6/6)";
	case SHOP_ALREADY_FULL_HEALTH: return "ALREADY AT FULL HEALTH";
	default:                       return "";
	}
}

//=========================================================
// HUD AND SCREENS (HUD, shop, pause, game over, victory)
// owner: Wee Jie
//=========================================================
typedef enum UiAction
{
	UI_NONE,
	UI_BUY_BASIC,
	UI_BUY_MACHINE_GUN,
	UI_BUY_ROCKET,
	UI_BUY_HEAL,
	UI_CONTINUE,
	UI_RESUME,
	UI_RESTART,
	UI_MAIN_MENU,
	UI_EXIT
} UiAction;

void     UI_DrawHUD(const Player* player, const Economy* economy, int droneCount,
	const char* waveLabel, const Boss* boss);
void     UI_DrawBanner(const char* text, float timeLeft);

UiAction UI_DrawShop(const Economy* economy, const Player* player, int droneCount,
	const char* continueLabel, const char* statusText);
UiAction UI_DrawPause(void);
UiAction UI_DrawEndScreen(int won, const Economy* economy, float runSeconds);

#define CX ((float)WINDOW_WIDTH * 0.5f)
#define CY ((float)WINDOW_HEIGHT * 0.5f)

static CP_Color White(void)  { return Util_Color(255, 255, 255, 255); }
static CP_Color Muted(void)  { return Util_Color(200, 190, 230, 255); }

static void DrawPanel(float x, float y, float w, float h)
{
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(Util_Color(40, 20, 80, 220));
	CP_Settings_Stroke(Util_Color(200, 110, 230, 255));
	CP_Settings_StrokeWeight(2.0f);
	CP_Graphics_DrawRectAdvanced(x, y, w, h, 0.0f, 10.0f);
}

static void DrawBar(float x, float y, float w, float h, float fill01, CP_Color color)
{
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_NoStroke();
	CP_Settings_Fill(Util_Color(30, 30, 30, 255));
	CP_Graphics_DrawRect(x, y, w, h);
	CP_Settings_Fill(color);
	CP_Graphics_DrawRect(x, y, w * CP_Math_ClampFloat(fill01, 0.0f, 1.0f), h);
}

//---------------------------------------------------------
// HUD: health, currency, drone count, kill count (+ wave, dash, boss)
//---------------------------------------------------------
void UI_DrawHUD(const Player* player, const Economy* economy, int droneCount,
	const char* waveLabel, const Boss* boss)
{
	char text[64];
	const float x = 20.0f;

	DrawPanel(10.0f, 10.0f, 270.0f, 190.0f);

	snprintf(text, sizeof(text), "HP %d / %d", (int)(player->health + 0.5f), (int)PLAYER_MAX_HEALTH);
	Util_DrawTextLeft(text, x, 30.0f, 20.0f, White());
	DrawBar(x, 42.0f, 250.0f, 12.0f, player->health / PLAYER_MAX_HEALTH, Util_Color(80, 220, 120, 255));

	snprintf(text, sizeof(text), "COINS: %d", economy->coins);
	Util_DrawTextLeft(text, x, 76.0f, 20.0f, Util_Color(255, 210, 80, 255));

	snprintf(text, sizeof(text), "KILLS: %d", economy->kills);
	Util_DrawTextLeft(text, x, 100.0f, 20.0f, White());

	snprintf(text, sizeof(text), "DRONES: %d / %d", droneCount, MAX_DRONES);
	Util_DrawTextLeft(text, x, 124.0f, 20.0f, White());

	Util_DrawTextLeft(waveLabel, x, 148.0f, 20.0f, Muted());

	Util_DrawTextLeft("DASH", x, 176.0f, 16.0f, Muted());
	DrawBar(x + 50.0f, 170.0f, 200.0f, 10.0f, Player_DashReady01(player),
		Player_DashReady01(player) >= 1.0f ? Util_Color(150, 255, 220, 255) : Util_Color(90, 120, 110, 255));

	// Difficulty + buff multipliers, top right
	snprintf(text, sizeof(text), "%s  |  DMG x%.1f  RATE x%.1f",
		Difficulty_Name(Difficulty_Get()), (double)economy->damageMult, (double)economy->fireRateMult);
	Util_DrawTextRight(text, (float)WINDOW_WIDTH - 20.0f, 25.0f, 18.0f, Muted());
	Util_DrawTextRight("[B] OPEN DRONE SHOP", (float)WINDOW_WIDTH - 20.0f, 50.0f, 18.0f, Util_Color(255, 210, 80, 255));

	if (economy->messageTimer > 0.0f)
		Util_DrawTextCentered(economy->message, CX, 120.0f, 26.0f,
			Util_WithAlpha(Util_Color(255, 230, 120, 255), Util_MinF(1.0f, economy->messageTimer)));

	if (boss->active)
	{
		Util_DrawTextCentered("THE SWARM QUEEN", CX, 30.0f, 22.0f, White());
		DrawBar(CX - 250.0f, 45.0f, 500.0f, 16.0f, boss->health / boss->maxHealth,
			Boss_IsEnraged(boss) ? Util_Color(255, 60, 60, 255) : Util_Color(235, 95, 45, 255));
	}
}

void UI_DrawBanner(const char* text, float timeLeft)
{
	if (timeLeft <= 0.0f)
		return;
	Util_DrawTextCentered(text, CX, CY - 120.0f, 56.0f, Util_WithAlpha(White(), Util_MinF(1.0f, timeLeft)));
}

//---------------------------------------------------------
// Shop (between waves)
//---------------------------------------------------------
UiAction UI_DrawShop(const Economy* economy, const Player* player, int droneCount,
	const char* continueLabel, const char* statusText)
{
	static const UiAction DRONE_ACTIONS[DRONE_TYPE_COUNT] = { UI_BUY_BASIC, UI_BUY_MACHINE_GUN, UI_BUY_ROCKET };
	UiAction action = UI_NONE;
	char text[96];

	UI_DrawDimOverlay();
	DrawPanel(CX - 330.0f, 70.0f, 660.0f, 580.0f);

	Util_DrawTextCentered("DRONE SHOP", CX, 110.0f, 44.0f, White());
	snprintf(text, sizeof(text), "COINS: %d     DRONES: %d / %d     HP: %d",
		economy->coins, droneCount, MAX_DRONES, (int)(player->health + 0.5f));
	Util_DrawTextCentered(text, CX, 155.0f, 22.0f, Util_Color(255, 210, 80, 255));

	for (int i = 0; i < DRONE_TYPE_COUNT; ++i)
	{
		const DroneStats* s = Drone_GetStats((DroneType)i);
		int canBuy = Economy_CanBuyDrone(economy, (DroneType)i, droneCount) == SHOP_OK;
		float y = 215.0f + (float)i * 75.0f;

		snprintf(text, sizeof(text), "[%d] %s  -  %d COINS", i + 1, s->name, s->cost);
		if (UI_Button(CX, y, 560.0f, 48.0f, text, canBuy, 0))
			action = DRONE_ACTIONS[i];

		snprintf(text, sizeof(text), "%.1f shots/s   %.0f dmg%s", (double)s->fireRate, (double)s->damage,
			s->splashRadius > 0.0f ? "   splash damage" : "");
		Util_DrawTextCentered(text, CX, y + 32.0f, 15.0f, Muted());
	}

	snprintf(text, sizeof(text), "[4] REPAIR +%d HP  -  %d COINS", (int)HEAL_AMOUNT, HEAL_COST);
	if (UI_Button(CX, 440.0f, 560.0f, 48.0f, text,
		economy->coins >= HEAL_COST && player->health < PLAYER_MAX_HEALTH, 0))
		action = UI_BUY_HEAL;

	Util_DrawTextCentered(statusText, CX, 500.0f, 20.0f, Util_Color(255, 230, 120, 255));

	if (UI_Button(CX, 580.0f, 460.0f, 56.0f, continueLabel, 1, 1))
		action = UI_CONTINUE;

	return action;
}

//---------------------------------------------------------
// Pause
//---------------------------------------------------------
UiAction UI_DrawPause(void)
{
	UiAction action = UI_NONE;
	UI_DrawDimOverlay();
	DrawPanel(CX - 220.0f, CY - 200.0f, 440.0f, 400.0f);

	Util_DrawTextCentered("PAUSED", CX, CY - 140.0f, 48.0f, White());
	if (UI_Button(CX, CY - 50.0f, 300.0f, 54.0f, "[ESC] RESUME", 1, 1)) action = UI_RESUME;
	if (UI_Button(CX, CY + 20.0f, 300.0f, 54.0f, "[R] RESTART", 1, 0))   action = UI_RESTART;
	if (UI_Button(CX, CY + 90.0f, 300.0f, 54.0f, "[M] MAIN MENU", 1, 0)) action = UI_MAIN_MENU;
	Util_DrawTextCentered("Your drones fight. You dodge.", CX, CY + 160.0f, 16.0f, Muted());
	return action;
}

//---------------------------------------------------------
// Game over / victory
//---------------------------------------------------------
UiAction UI_DrawEndScreen(int won, const Economy* economy, float runSeconds)
{
	UiAction action = UI_NONE;
	char text[96];

	UI_DrawDimOverlay();
	DrawPanel(CX - 300.0f, CY - 230.0f, 600.0f, 460.0f);

	Util_DrawTextCentered(won ? "VICTORY!" : "GAME OVER", CX, CY - 170.0f, 60.0f,
		won ? Util_Color(120, 255, 180, 255) : Util_Color(255, 90, 90, 255));
	Util_DrawTextCentered(won ? "The SWARM has been defeated." : "The SWARM overwhelmed you.",
		CX, CY - 115.0f, 22.0f, White());

	snprintf(text, sizeof(text), "KILLS: %d     COINS: %d     TIME: %d:%02d     %s",
		economy->kills, economy->coins, (int)runSeconds / 60, (int)runSeconds % 60,
		Difficulty_Name(Difficulty_Get()));
	Util_DrawTextCentered(text, CX, CY - 60.0f, 20.0f, Muted());

	if (UI_Button(CX, CY + 20.0f, 320.0f, 54.0f, "[R] RESTART", 1, 1))     action = UI_RESTART;
	if (UI_Button(CX, CY + 90.0f, 320.0f, 54.0f, "[M] MAIN MENU", 1, 0))   action = UI_MAIN_MENU;
	if (UI_Button(CX, CY + 160.0f, 320.0f, 54.0f, "[ESC] EXIT GAME", 1, 0)) action = UI_EXIT;
	return action;
}

//=========================================================
// GAME FLOW (ties every section together)
// owner: Wee Jie
//=========================================================
#define BANNER_TIME 2.0f
#define MAX_DT      0.05f   // avoids huge jumps after a hitch or window drag

typedef enum GamePhase
{
	PHASE_PLAYING,
	PHASE_SHOP,
	PHASE_PAUSED,
	PHASE_GAME_OVER,
	PHASE_VICTORY
} GamePhase;

// World state. Fixed-size pools, no dynamic memory.
static Player      g_player;
static Enemy       g_enemies[MAX_ENEMIES];
static Drone       g_drones[MAX_DRONES];
static int         g_droneCount;
static Projectile  g_projectiles[MAX_PROJECTILES];
static Effect      g_effects[MAX_EFFECTS];
static Boss        g_boss;
static WaveManager g_waves;
static Economy     g_economy;

static GamePhase   g_phase;
static int         g_nextIsBoss;
static float       g_worldTime;
static float       g_runTime;
static float       g_bannerTimer;
static char        g_bannerText[64];
static char        g_waveLabel[32];
static char        g_shopStatus[64];
static int         g_shopMidWave;   // 1 = shop opened with B during a wave

//---------------------------------------------------------
// Helpers
//---------------------------------------------------------
static void ShowBanner(const char* text)
{
	snprintf(g_bannerText, sizeof(g_bannerText), "%s", text);
	g_bannerTimer = BANNER_TIME;
}

static void AddDrone(DroneType type)
{
	if (g_droneCount >= MAX_DRONES)
		return;
	Drone_Init(&g_drones[g_droneCount], type, g_player.pos);
	g_droneCount++;
}

static void StartWave(int wave)
{
	char text[32];
	Wave_Start(&g_waves, wave);
	snprintf(g_waveLabel, sizeof(g_waveLabel), "WAVE %d / %d", g_waves.wave, SWARM_WAVE_COUNT);
	snprintf(text, sizeof(text), "WAVE %d", g_waves.wave);
	ShowBanner(text);
}

static void StartBoss(void)
{
	Boss_Spawn(&g_boss, CP_Vector_Set((float)WINDOW_WIDTH * 0.5f, BOSS_RADIUS + 20.0f));
	snprintf(g_waveLabel, sizeof(g_waveLabel), "FINAL BOSS");
	ShowBanner("FINAL BOSS");
}

// B during a wave: shop opens and the game pauses until you go back
static void OpenShopMidWave(void)
{
	g_shopMidWave = 1;
	snprintf(g_shopStatus, sizeof(g_shopStatus), "GAME PAUSED WHILE SHOPPING");
	g_phase = PHASE_SHOP;
}

// Between waves: shop opens automatically after a wave is cleared
static void EnterShop(void)
{
	g_shopMidWave = 0;
	g_nextIsBoss = g_waves.wave >= SWARM_WAVE_COUNT;
	snprintf(g_shopStatus, sizeof(g_shopStatus), "WAVE %d CLEARED!", g_waves.wave);
	Projectile_ClearAll(g_projectiles, MAX_PROJECTILES);
	g_phase = PHASE_SHOP;
}

static void LeaveShop(void)
{
	if (g_shopMidWave)
	{
		g_shopMidWave = 0;
		g_phase = PHASE_PLAYING;
		return;
	}

	if (g_nextIsBoss)
		StartBoss();
	else
		StartWave(g_waves.wave + 1);
	g_phase = PHASE_PLAYING;
}

static void Restart(void)
{
	CP_Engine_SetNextGameStateForced(Game_Init, Game_Update, Game_Exit);
}

static void GoToMenu(void)
{
	CP_Engine_SetNextGameState(Menu_Init, Menu_Update, Menu_Exit);
}

// Removes dead g_enemies and pays out coins immediately (no coin pickups)
static void CollectKills(void)
{
	for (int i = 0; i < MAX_ENEMIES; ++i)
	{
		Enemy* e = &g_enemies[i];
		if (e->active && e->health <= 0.0f)
		{
			Economy_RegisterKill(&g_economy, e->coinReward);
			Fx_Spawn(g_effects, MAX_EFFECTS, e->pos, 18.0f, 0.25f, Util_Color(230, 100, 50, 255));
			e->active = 0;
		}
	}
}

//---------------------------------------------------------
// Simulation
//---------------------------------------------------------
static void UpdateDrones(float dt)
{
	for (int i = 0; i < g_droneCount; ++i)
	{
		Drone* d = &g_drones[i];
		CP_Vector target = CP_Vector_Zero();
		Projectile shot;

		Drone_UpdateMovement(d, g_player.pos, i, g_droneCount, g_worldTime, dt);

		int hasTarget = Combat_FindTarget(d->pos, Drone_GetStats(d->type)->range,
			g_enemies, MAX_ENEMIES, &g_boss, &target);
		if (Drone_TryFire(d, hasTarget, target, g_economy.damageMult, g_economy.fireRateMult, dt, &shot))
			Projectile_Spawn(g_projectiles, MAX_PROJECTILES, &shot);
	}
}

static void CheckProgress(void)
{
	if (Player_IsDead(&g_player))
	{
		g_phase = PHASE_GAME_OVER;
		return;
	}

	if (g_boss.active)
	{
		if (g_boss.health <= 0.0f)
		{
			Fx_Spawn(g_effects, MAX_EFFECTS, g_boss.pos, 220.0f, 0.8f, Util_Color(255, 200, 120, 255));
			g_boss.active = 0;
			Enemy_ClearAll(g_enemies, MAX_ENEMIES);
			g_phase = PHASE_VICTORY;
		}
		return;
	}

	if (Wave_SpawningDone(&g_waves) && Enemy_ActiveCount(g_enemies, MAX_ENEMIES) == 0)
		EnterShop();
}

static void UpdatePlaying(float dt)
{
	if (CP_Input_KeyTriggered(KEY_ESCAPE) || CP_Input_KeyTriggered(KEY_P))
	{
		g_phase = PHASE_PAUSED;
		return;
	}
	if (CP_Input_KeyTriggered(KEY_B))
	{
		OpenShopMidWave();
		return;
	}

	g_worldTime += dt;
	g_runTime += dt;
	g_bannerTimer -= dt;

	Player_Update(&g_player, dt);

	if (!g_boss.active)
		Wave_Update(&g_waves, g_enemies, MAX_ENEMIES, dt);
	Enemy_UpdateAll(g_enemies, MAX_ENEMIES, g_player.pos, dt);
	Boss_Update(&g_boss, g_player.pos, g_enemies, MAX_ENEMIES, dt);

	UpdateDrones(dt);
	Projectile_UpdateAll(g_projectiles, MAX_PROJECTILES, dt);

	Combat_ResolveProjectiles(g_projectiles, MAX_PROJECTILES, g_enemies, MAX_ENEMIES, &g_boss, g_effects, MAX_EFFECTS);
	CollectKills();
	Combat_ResolveContact(&g_player, g_enemies, MAX_ENEMIES, &g_boss);

	Fx_UpdateAll(g_effects, MAX_EFFECTS, dt);
	Economy_Update(&g_economy, dt);

	CheckProgress();
}

//---------------------------------------------------------
// Overlays (input + drawing for non-playing phases)
//---------------------------------------------------------
static void HandleShop(void)
{
	const char* continueLabel = g_shopMidWave ? "[ENTER / B] BACK TO THE FIGHT"
		: g_nextIsBoss ? "[ENTER] START FINAL BOSS" : "[ENTER] START NEXT WAVE";
	UiAction action = UI_DrawShop(&g_economy, &g_player, g_droneCount, continueLabel, g_shopStatus);

	if (CP_Input_KeyTriggered(KEY_1)) action = UI_BUY_BASIC;
	if (CP_Input_KeyTriggered(KEY_2)) action = UI_BUY_MACHINE_GUN;
	if (CP_Input_KeyTriggered(KEY_3)) action = UI_BUY_ROCKET;
	if (CP_Input_KeyTriggered(KEY_4)) action = UI_BUY_HEAL;
	if (CP_Input_KeyTriggered(KEY_ENTER)) action = UI_CONTINUE;
	if (g_shopMidWave && CP_Input_KeyTriggered(KEY_B)) action = UI_CONTINUE;

	ShopResult result = SHOP_OK;
	DroneType type = DRONE_BASIC;
	switch (action)
	{
	case UI_BUY_BASIC:
	case UI_BUY_MACHINE_GUN:
	case UI_BUY_ROCKET:
		type = action == UI_BUY_BASIC ? DRONE_BASIC : action == UI_BUY_MACHINE_GUN ? DRONE_MACHINE_GUN : DRONE_ROCKET;
		result = Economy_BuyDrone(&g_economy, type, g_droneCount);
		if (result == SHOP_OK)
			AddDrone(type);
		snprintf(g_shopStatus, sizeof(g_shopStatus), "%s", Economy_ResultText(result));
		break;
	case UI_BUY_HEAL:
		result = Economy_BuyHeal(&g_economy, &g_player);
		snprintf(g_shopStatus, sizeof(g_shopStatus), "%s", Economy_ResultText(result));
		break;
	case UI_CONTINUE:
		LeaveShop();
		break;
	default:
		break;
	}
}

static void HandlePause(void)
{
	UiAction action = UI_DrawPause();
	if (CP_Input_KeyTriggered(KEY_ESCAPE) || CP_Input_KeyTriggered(KEY_P)) action = UI_RESUME;
	if (CP_Input_KeyTriggered(KEY_R)) action = UI_RESTART;
	if (CP_Input_KeyTriggered(KEY_M)) action = UI_MAIN_MENU;

	if (action == UI_RESUME)    g_phase = PHASE_PLAYING;
	if (action == UI_RESTART)   Restart();
	if (action == UI_MAIN_MENU) GoToMenu();
}

static void HandleEndScreen(int won)
{
	UiAction action = UI_DrawEndScreen(won, &g_economy, g_runTime);
	if (CP_Input_KeyTriggered(KEY_R)) action = UI_RESTART;
	if (CP_Input_KeyTriggered(KEY_M)) action = UI_MAIN_MENU;
	if (CP_Input_KeyTriggered(KEY_ESCAPE)) action = UI_EXIT;

	if (action == UI_RESTART)   Restart();
	if (action == UI_MAIN_MENU) GoToMenu();
	if (action == UI_EXIT)      CP_Engine_Terminate();
}

//---------------------------------------------------------
// Rendering
//---------------------------------------------------------
static void DrawWorld(void)
{
	CP_Graphics_ClearBackground(Util_Color(20, 20, 20, 255));

	// Subtle grid so movement is readable on the fixed map
	CP_Settings_Stroke(Util_Color(35, 35, 40, 255));
	CP_Settings_StrokeWeight(1.0f);
	for (int x = 0; x <= WINDOW_WIDTH; x += 64)
		CP_Graphics_DrawLine((float)x, 0.0f, (float)x, (float)WINDOW_HEIGHT);
	for (int y = 0; y <= WINDOW_HEIGHT; y += 64)
		CP_Graphics_DrawLine(0.0f, (float)y, (float)WINDOW_WIDTH, (float)y);

	Fx_DrawAll(g_effects, MAX_EFFECTS);
	Enemy_DrawAll(g_enemies, MAX_ENEMIES);
	Boss_Draw(&g_boss);
	Projectile_DrawAll(g_projectiles, MAX_PROJECTILES);
	Player_Draw(&g_player);
	for (int i = 0; i < g_droneCount; ++i)
		Drone_Draw(&g_drones[i]);
}

//---------------------------------------------------------
// CProcessing game state functions
//---------------------------------------------------------
void Game_Init(void)
{
	CP_Vector center = CP_Vector_Set((float)WINDOW_WIDTH * 0.5f, (float)WINDOW_HEIGHT * 0.5f);

	Player_Init(&g_player, center);
	Enemy_ClearAll(g_enemies, MAX_ENEMIES);
	Projectile_ClearAll(g_projectiles, MAX_PROJECTILES);
	Fx_ClearAll(g_effects, MAX_EFFECTS);
	Boss_Reset(&g_boss);
	Economy_Init(&g_economy);

	g_droneCount = 0;
	AddDrone(DRONE_BASIC);   // the starter drone

	g_worldTime = 0.0f;
	g_runTime = 0.0f;
	g_nextIsBoss = 0;
	g_shopMidWave = 0;
	g_shopStatus[0] = '\0';

	StartWave(1);
	g_phase = PHASE_PLAYING;
}

void Game_Update(void)
{
	float dt = Util_MinF(CP_System_GetDt(), MAX_DT);

	// Remember the g_phase at the start of the frame so a key that changes
	// g_phase (e.g. ESC to pause) is not handled again by the new g_phase.
	GamePhase current = g_phase;

	if (current == PHASE_PLAYING)
		UpdatePlaying(dt);

	DrawWorld();
	UI_DrawHUD(&g_player, &g_economy, g_droneCount, g_waveLabel, &g_boss);

	switch (current)
	{
	case PHASE_PLAYING:   UI_DrawBanner(g_bannerText, g_bannerTimer); break;
	case PHASE_SHOP:      HandleShop();         break;
	case PHASE_PAUSED:    HandlePause();        break;
	case PHASE_GAME_OVER: HandleEndScreen(0);   break;
	case PHASE_VICTORY:   HandleEndScreen(1);   break;
	default: break;
	}
}

void Game_Exit(void)
{
}
