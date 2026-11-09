#include <stdio.h>
#include "cprocessing.h"
#include "win.h"
#include "game.h"
#include "mainmenu.h"
#include "utils.h"
#include "buff.h"

static int did_player_win = 0;

void win_init(void)
{
	did_player_win = 0;
}

void win_start(void)
{
	did_player_win = 1;
}

int win_active(void)
{
	return did_player_win;
}

void win_draw(int coins, float timeSurvived)
{
	if (did_player_win == 0)
	{
		return;
	}

	float width = (float)CP_System_GetWindowWidth();
	float height = (float)CP_System_GetWindowHeight();
	float centerX = width / 2.0f;

	float restartY = height * 0.55f;
	float menuY = height * 0.65f;
	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	if (CP_Input_KeyTriggered(KEY_R) || IsAreaClicked(centerX, restartY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_SetNextGameStateForced(Game_Init, Game_Update, Game_Exit);
		return;
	}

	if (CP_Input_KeyTriggered(KEY_Q) || IsAreaClicked(centerX, menuY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
		return;
	}

	CP_Settings_NoStroke();
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 180));
	CP_Graphics_DrawRect(0.0f, 0.0f, width, height);

	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_Fill(CP_Color_Create(255, 215, 0, 255));
	CP_Settings_TextSize(60.0f);
	CP_Font_DrawText("VICTORY!", centerX, height * 0.25f);

	char text[64];
	int minutes = (int)timeSurvived / 60;
	int seconds = (int)timeSurvived % 60;

	CP_Settings_TextSize(18.0f);

	snprintf(text, sizeof(text), "ENEMIES KILLED: %d", Buff_GetKills());
	CP_Font_DrawText(text, centerX, height * 0.35f);

	snprintf(text, sizeof(text), "COINS COLLECTED: %d", coins);
	CP_Font_DrawText(text, centerX, height * 0.45f);

	snprintf(text, sizeof(text), "TIME SURVIVED: %d:%02d", minutes, seconds);
	CP_Font_DrawText(text, centerX, height * 0.55f);

	menu_draw_button(centerX, restartY, 300.0f, 60.0f, "RESTART", CP_Color_Create(40, 160, 80, 255), CP_Color_Create(70, 200, 110, 255));
	menu_draw_button(centerX, menuY, 300.0f, 60.0f, "MAIN MENU", CP_Color_Create(110, 70, 200, 255), CP_Color_Create(150, 110, 230, 255));
}