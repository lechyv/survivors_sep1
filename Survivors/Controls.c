#include "cprocessing.h"
#include "controls.h"
#include "mainmenu.h"   // menu_draw_button
#include "diff.h"       // Diff_Select_Init/Update/Exit
#include "game.h"       // Game_Init/Update/Exit
#include "utils.h"      // IsAreaClicked

#define CONTROLS_ROW_COUNT 6
#define CONTROLS_FIRST_ROW_Y 0.30f   //UI: first row is 30% down the screen
#define CONTROLS_ROW_GAP 0.08f       //UI: each next row is 8% lower
#define CONTROLS_TEXT_GAP 40.0f      //UI: space between the key text and its description

//UI: left column = the key(s), right column = what they do. Add/remove rows here
//(and change CONTROLS_ROW_COUNT to match)
static const char* controlKeys[CONTROLS_ROW_COUNT] = {
	"W A S D / ARROWS",
	"SPACE",
	"B",
	"ESC / P",
	"MOUSE CLICK",
	"R"
};
static const char* controlActions[CONTROLS_ROW_COUNT] = {
	"MOVE",
	"DASH",
	"OPEN / CLOSE SHOP",
	"PAUSE / RESUME",
	"BUY / SELL IN SHOP",
	"RESTART (END SCREENS)"
};

void Controls_Init(void)
{
	return;
}

void Controls_Update(void)
{
	float centerX = (float)CP_System_GetWindowWidth() / 2.0f;
	float height = (float)CP_System_GetWindowHeight();
	float buttonY = height * 0.82f;
	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	// ----- start the game, or go back -----
	if (CP_Input_KeyTriggered(KEY_ENTER) || CP_Input_KeyTriggered(KEY_SPACE) ||
		IsAreaClicked(centerX, buttonY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
		return;
	}
	if (CP_Input_KeyTriggered(KEY_ESCAPE))
	{
		CP_Engine_SetNextGameState(Diff_Select_Init, Diff_Select_Update, Diff_Select_Exit);
		return;
	}

	CP_Graphics_ClearBackground(CP_Color_Create(0, 0, 0, 255));   //UI: black, same as the difficulty screen

	// ----- title -----
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextSize(48.0f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_DrawText("CONTROLS", centerX, height * 0.15f);

	// ----- one row per control -----
	CP_Settings_TextSize(24.0f);
	int i = 0;
	while (i < CONTROLS_ROW_COUNT)
	{
		float rowY = height * (CONTROLS_FIRST_ROW_Y + CONTROLS_ROW_GAP * (float)i);

		CP_Settings_Fill(CP_Color_Create(255, 215, 0, 255));   //UI: key colour (gold)
		CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_RIGHT, CP_TEXT_ALIGN_V_MIDDLE);
		CP_Font_DrawText(controlKeys[i], centerX - CONTROLS_TEXT_GAP, rowY);

		CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255)); //UI: action colour (white)
		CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_MIDDLE);
		CP_Font_DrawText(controlActions[i], centerX + CONTROLS_TEXT_GAP, rowY);

		i += 1;
	}

	// ----- note about the drones -----
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_Fill(CP_Color_Create(150, 150, 150, 255));
	CP_Settings_TextSize(16.0f);
	CP_Font_DrawText("YOUR DRONES FIRE AUTOMATICALLY. SURVIVE EVERY SWARM, THEN BEAT THE BOSS!", centerX, height * 0.75f);

	// ----- start button -----
	menu_draw_button(centerX, buttonY, 300.0f, 60.0f, "START",
		CP_Color_Create(40, 160, 80, 255), CP_Color_Create(70, 200, 110, 255));

	// ----- hint -----
	CP_Settings_Fill(CP_Color_Create(150, 150, 150, 255));
	CP_Settings_TextSize(18.0f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_DrawText("ENTER / SPACE: START    ESC: BACK", centerX, height * 0.92f);
}

void Controls_Exit(void)
{
	return;
}