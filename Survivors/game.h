
#pragma once

#define world_width 2500.0f
#define world_height 2500.0f

void Game_Init(void);

void Game_Update(void);

void Game_Exit(void);

float bullet_check_if_hit(float x, float y, float radius);