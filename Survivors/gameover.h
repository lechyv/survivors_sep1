#pragma once

void GameOver_Init(void);       // call in Game_Init: a new game is never "game over"
void GameOver_Start(void);      // call when the player dies
int  GameOver_IsActive(void);   // returns 1 while the game over screen is showing

// Call LAST in Game_Update: draws the game over screen if active
//   coins:        coins collected this run
//   timeSurvived: seconds the player stayed alive
void GameOver_Draw(int coins, float timeSurvived);
