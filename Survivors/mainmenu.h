#pragma once
#include "cprocessing.h"   // NEW: needed for CP_Color

void Main_Menu_Init(void);
void Main_Menu_Update(void);
void Main_Menu_Exit(void);

// Difficulty picked on the menu: 0 = easy, 1 = normal, 2 = hard
#define difficulty_easy 0
#define difficulty_normal 1
#define difficulty_hard 2

// "extern" means the variable lives in mainmenu.c, but any file that
// includes mainmenu.h (like game.c) can read it.
extern int menuDifficulty;

void menu_draw_button(float x, float y, float w, float h, const char* label,
	CP_Color color, CP_Color hoverColor);