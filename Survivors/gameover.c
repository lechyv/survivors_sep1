#include <stdio.h>          
#include "cprocessing.h"
#include "gameover.h"
#include "game.h"           
#include "mainmenu.h"       
#include "utils.h"         

static int isGameOver = 0;   // 1 = player is dead, 0 = still playing

void GameOver_Init(void)
{
	isGameOver = 0;
}

void GameOver_Start(void)
{
	isGameOver = 1;
}

int GameOver_IsActive(void)
{
	return isGameOver;
}

static void Restart(void)
{
	// "Forced" because we are already in the game state and want to run Game_Init again
	CP_Engine_SetNextGameStateForced(Game_Init, Game_Update, Game_Exit);
}

void GameOver_Draw(int coins, float timeSurvived)
{
	if (isGameOver == 0)
	{
		return;   // player still alive: draw nothing
	}

	float width = (float)CP_System_GetWindowWidth();
	float height = (float)CP_System_GetWindowHeight();
	float centerX = width / 2.0f;

	float restartY = height * 0.55f;
	float menuY = height * 0.67f;
	float exitY = height * 0.79f;

	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	// ----- keyboard and button clicks -----
	if (CP_Input_KeyTriggered(KEY_R) ||
		IsAreaClicked(centerX, restartY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		Restart();
		return;
	}
	if (IsAreaClicked(centerX, menuY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
		return;
	}
	if (IsAreaClicked(centerX, exitY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_Terminate();
		return;
	}

	// ----- dark red see-through layer over the whole game -----
	CP_Settings_NoStroke();
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(CP_Color_Create(60, 0, 0, 180));
	CP_Graphics_DrawRect(0.0f, 0.0f, width, height);

	// ----- title -----
	CP_Settings_Fill(CP_Color_Create(255, 80, 80, 255));
	CP_Settings_TextSize(56.0f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_DrawText("GAME OVER", centerX, height * 0.22f);

	// ----- stats -----
	char text[48];
	int minutes = (int)timeSurvived / 60;
	int seconds = (int)timeSurvived % 60;

	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextSize(18.0f);

	snprintf(text, sizeof(text), "COINS COLLECTED: %d", coins);
	CP_Font_DrawText(text, centerX, height * 0.35f);

	snprintf(text, sizeof(text), "TIME SURVIVED: %d:%02d", minutes, seconds);
	CP_Font_DrawText(text, centerX, height * 0.41f);

	// ----- buttons -----
	menu_draw_button(centerX, restartY, 300.0f, 60.0f, "RESTART",
		CP_Color_Create(40, 160, 80, 255), CP_Color_Create(70, 200, 110, 255));
	menu_draw_button(centerX, menuY, 300.0f, 60.0f, "MAIN MENU",
		CP_Color_Create(110, 70, 200, 255), CP_Color_Create(150, 110, 230, 255));
	menu_draw_button(centerX, exitY, 300.0f, 60.0f, "EXIT GAME",
		CP_Color_Create(190, 50, 50, 255), CP_Color_Create(230, 80, 80, 255));

	// ----- hint -----
	CP_Settings_Fill(CP_Color_Create(200, 200, 200, 255));
	CP_Settings_TextSize(14.0f);
	CP_Font_DrawText("R: RESTART    Q: MAIN MENU", centerX, height * 0.90f);
}
