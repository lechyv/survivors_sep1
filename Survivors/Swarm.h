#pragma once
#include "cprocessing.h"

/* difficulty: 1 = easy, 2 = medium, 3 = hard
   stage:      1 = base enemies only, 2 = base + fast + tank */
void Swarm_Init(int difficulty, int stage);

/* Spawns, moves, shoots and draws everything in the swarm. Call once per frame. */
void Swarm_Update(CP_Vector player_pos);

/* Total damage dealt to the player this frame by tank projectiles touching the circle
   at (x, y). Touching projectiles are removed. Pass the result to combat_damage_player. */
float Swarm_ProjectilesHitPlayer(float x, float y, float radius);

int Swarm_KillEnemiesTouching(float x, float y, float radius, float* killed_x, float* killed_y, int max_killed);
/* int Swarm_KillEnemiesTouching added for coin-drop.c
Kills every enemy touching the circle at (x, y) with the given radius.
   The position of each killed enemy is written into killed_x[] / killed_y[]
   (at most max_killed enemies per call).
   Returns how many enemies were killed
*/

int Swarm_GetNearest(float x, float y, float range, CP_Vector* target);
int Swarm_NumOfEnemiesTouching(float x, float y, float radius);
