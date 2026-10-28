#pragma once
#include "cprocessing.h"


void Swarm_Init(int difficulty);          /* 1 = easy, 2 = medium, 3 = hard */
void Swarm_Update(CP_Vector playerPos);


int Swarm_KillEnemiesTouching(float x, float y, float radius, float* killedX, float* killedY, int maxKilled); 
/* int Swarm_KillEnemiesTouching added for coin-drop.c
Kills every enemy touching the circle at (x, y) with the given radius.
   The position of each killed enemy is written into killedX[] / killedY[]
   (at most maxKilled enemies per call).
   Returns how many enemies were killed
*/

int Swarm_GetNearest(float x, float y, float range, CP_Vector* target);

