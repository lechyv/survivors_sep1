#pragma once

void Pause_Init(void);          // call in Game_Init: game always starts unpaused
void Pause_HandleInput(void);   // call FIRST in Game_Update: ESC or P toggles pause
int  Pause_IsPaused(void);      // returns 1 while the game is paused
void Pause_Draw(void);          // call LAST in Game_Update: draws the pause menu if paused
