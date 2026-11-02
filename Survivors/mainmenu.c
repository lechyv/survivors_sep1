#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "utils.h" 


int menuDifficulty = difficulty_normal;   
CP_Font menuFont;

void menu_draw_button(float x, float y, float w, float h, const char* label,
	CP_Color color, CP_Color hoverColor)
{
	if (IsAreaHovered(x, y, w, h) == 1)
	{
		CP_Settings_Fill(hoverColor);
	}
	else
	{
		CP_Settings_Fill(color);
	}

	CP_Settings_Stroke(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_StrokeWeight(3.0f);
	CP_Settings_RectMode(CP_POSITION_CENTER);
	CP_Graphics_DrawRect(x, y, w, h);

	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextSize(h * 0.55f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_DrawText(label, x, y);
}

void Main_Menu_Init(void)
{
	CP_System_SetWindowTitle("Survivor");
	menuFont = CP_Font_Load("Assets/press-start.ttf");	
	CP_Font_Set(menuFont);
}

void Main_Menu_Update(void)
{
	float centerX = (float)CP_System_GetWindowWidth() / 2.0f;
	float height = (float)CP_System_GetWindowHeight();

	float startY = height * 0.50f;
	float exitY = height * 0.62f;

	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	/* ----- start and exit ----- */
	if (CP_Input_KeyTriggered(KEY_ENTER) || IsAreaClicked(centerX, startY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
		return;
	}
	if (CP_Input_KeyTriggered(KEY_ESCAPE) || IsAreaClicked(centerX, exitY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_Terminate();
		return;
	}

	CP_Graphics_ClearBackground(CP_Color_Create(92, 170, 250, 255));   // sky blue

	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextSize(100.0f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_DrawText("SURVIVOR", centerX, height * 0.22f);

	// green START, red EXIT
	menu_draw_button(centerX, startY, 300.0f, 60.0f, "START",
		CP_Color_Create(40, 160, 80, 255), CP_Color_Create(70, 200, 110, 255));
	menu_draw_button(centerX, exitY, 300.0f, 60.0f, "EXIT",
		CP_Color_Create(190, 50, 50, 255), CP_Color_Create(230, 80, 80, 255));

	// Instructions for controls
	CP_Settings_Fill(CP_Color_Create(40, 20, 80, 255));
	CP_Settings_TextSize(24.0f);
	CP_Font_DrawText("WASD: Move    SPACE: Dash     Esc/P: Pause", centerX, height * 0.90f);
}

void Main_Menu_Exit(void)
{

}
