#include "cprocessing.h"
#include "diff.h"
#include "mainmenu.h"   // menuDifficulty, difficulty_easy/normal/hard, menu_draw_button
#include "utils.h"      // IsAreaClicked
#include "game.h"

#define DIFF_EASY_MULTIPLIER 0.75f
#define DIFF_NORMAL_MULTIPLIER 1.0f
#define DIFF_HARD_MULTIPLIER 1.25f

//UI buttons and spacing
#define DIFF_BUTTON_WIDTH 220.0f
#define DIFF_BUTTON_HEIGHT 50.0f
#define DIFF_BUTTON_GAP 30.0f

float Diff_GetMultiplier(void) {
	if (menuDifficulty == difficulty_easy) {
		return DIFF_EASY_MULTIPLIER;
	}
	else if (menuDifficulty == difficulty_hard) {
		return DIFF_HARD_MULTIPLIER;
	}
	else {
		return DIFF_NORMAL_MULTIPLIER; // default to normal if something goes wrong
	}
}

float Diff_ScaleDamageToPlayer(float damage){
	return damage * Diff_GetMultiplier();
}

float Diff_ScaleDamageToEnemy(float damage) {
	return damage / Diff_GetMultiplier();
}

int Diff_GetSwarmLevel(void) {
	if (menuDifficulty == difficulty_easy) {
		return 1;
	}
	else if (menuDifficulty == difficulty_hard) {
		return 3;
	}
	else {
		return 2; // default to normal if something goes wrong
	}
}
//UI: text on each difficulty button
const char* Diff_Name(int level) {
	if (level == difficulty_easy)
	{
		return "EASY";
	}
	else if (level == difficulty_hard)
	{
		return "HARD";
	}
	else {
		return "NORMAL";
	}
}
//UI: colour of each difficulty button when the mouse is over it (green, gold, red)
	CP_Color Diff_SelectedColor(int level)
	{
	if (level == difficulty_easy) 
	{
		return CP_Color_Create(40,160, 80, 255); 
	}
	else if (level == difficulty_hard) 
	{
		return CP_Color_Create(190, 50, 50, 255); 
	}
	else 
	{
		return CP_Color_Create(200, 140, 20, 255); 
	}
	} 
	void Diff_Select_Init(void) {
		return;
	}

void Diff_Select_Update(void)
{
	float centerX = (float)CP_System_GetWindowWidth() / 2.0f;
	float height = (float)CP_System_GetWindowHeight();
	float buttonY = height * 0.50f;
	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	CP_Graphics_ClearBackground(CP_Color_Create(0, 0, 0, 255)); //UI: Black background temp

	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255)); //UI: title color (white)
	CP_Settings_TextSize(40.0f);                           //UI: title size
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE); //UI: x, y below is the centre of the text
	CP_Font_DrawText("SELECT DIFFICULTY", centerX, height * 0.35f); //UI: title 30% down the screen

	//UI small hint at the bottom
	CP_Settings_Fill(CP_Color_Create(150,150,150,255));
	CP_Settings_TextSize(18.0f);
	CP_Font_DrawText("ESC: BACK TO MENU", centerX, height * 0.90f);

	int level = difficulty_easy;
	while (level <= difficulty_hard) {
		float buttonX = centerX + (float)(level - difficulty_normal) * (DIFF_BUTTON_WIDTH + DIFF_BUTTON_GAP); //UI: easy left, normal middle, hard right)
		/* UI: grey button, turns green / gold / red when the mouse is over it */
		menu_draw_button(buttonX, buttonY, DIFF_BUTTON_WIDTH, DIFF_BUTTON_HEIGHT, Diff_Name(level),
			CP_Color_Create(70, 70, 90, 255), Diff_SelectedColor(level));

		/* clicking a button sets the difficulty and starts the game */
		if (IsAreaClicked(buttonX, buttonY, DIFF_BUTTON_WIDTH, DIFF_BUTTON_HEIGHT, mouseX, mouseY) == 1)
		{
			menuDifficulty = level;
			CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
		}
		level += 1;
	}
		//ESC goes back to main menu
		if (CP_Input_KeyTriggered(KEY_ESCAPE)) {
			CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
		}
}

	void Diff_Select_Exit(void) {
		return;
	}
	