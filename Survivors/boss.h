#pragma once
#include "cprocessing.h"

void boss_init();
void boss_update(CP_Vector player_pos);
void boss_draw(void);
void boss_take_damage(float damage);
int boss_get_target(float x, float y, float range, CP_Vector* target);
void boss_take_AOE_damage(float x, float y, float radius, float damage);
float boss_get_collision_damage(float x, float y, float radius);
void boss_reset(void);
#define boss_health_max 750.0f