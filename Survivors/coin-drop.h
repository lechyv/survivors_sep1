//Done by Elvis
#pragma once
/*
Clears all coin data and resets the coin counter to 0.  Called once in Game_Init
*/
void Coins_Init(void);
/*
Drops coin at (x,y) where the enemy died. If no free coin slot, coin no drop
*/
void Coins_Drop(float x, float y);
/*
Kills enemy that touch player, drops one coin
Called once per frame in Game_Update after player has moved
*/
void Coins_KillAndDrop(float playerX, float playerY, float playerRadius);
/*
Ages every coin and collect any coin that player touches
*/
void Coins_Update(float playerX, float playerY, float playerRadius);
/*
Draw every coin that is on the ground
*/
void Coins_Draw(void);
/*
Draws the coin counter ("Coins: N") in the top-left corner.
*/
void Coins_DrawCounter(void);
/*
Return how many coins player has collected
*/
int Coins_GetCount(void);
