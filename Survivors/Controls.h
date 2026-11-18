#pragma once

//Controls screen (a game state): shown after the player picks a difficulty.
//ENTER / SPACE / clicking START begins the game. ESC goes back to difficulty select.
//UI: the whole layout (text, colours, positions) is in Controls_Update in controls.c
void Controls_Init(void);
void Controls_Update(void);
void Controls_Exit(void);
