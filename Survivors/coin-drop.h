//Done by Elvis
#pragma once
/*
Clears all coin data and resets the coin counter to 0.  Called once in Game_Init
*/
void Coins_Init(void);

/*
Drops coin at (x,y) where the enemy died. If no free coin slot, coin no drop
Calls swarm.c (move enemies) when a drone bullet brings an enemy to 0 hp.
*/
void Coins_Drop(float x, float y);

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

//deduct coins from player after purchase
// return 1 if coins were spent, 0 if player cannot afford
int Coins_Spend(int amount);
