#include "cprocessing.h"
#include "pause.h"
#include "mainmenu.h"  
#include "utils.h"      // for IsAreaClicked

static int isPaused = 0;   // 1 = paused, 0 = playing

void Pause_Init(void)
{
	isPaused = 0;
}

int Pause_IsPaused(void)
{
	return isPaused;
}

// ESC or P to change between pause and not paused
void Pause_HandleInput(void)
{
	if (CP_Input_KeyTriggered(KEY_ESCAPE) || CP_Input_KeyTriggered(KEY_P))
	{
		if (isPaused == 1)
		{
			isPaused = 0;
		}
		else
		{
			isPaused = 1;
		}
	}
}

void Pause_Draw(void)
{
	if (isPaused == 0)
	{
		return;   // not paused: draw nothing
	}

	float width = (float)CP_System_GetWindowWidth();
	float height = (float)CP_System_GetWindowHeight();
	float centerX = width / 2.0f;

	float resumeY = height * 0.45f;
	float menuY = height * 0.57f;
	float exitY = height * 0.69f;

	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	// ----- button clicks -----
	if (IsAreaClicked(centerX, resumeY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		isPaused = 0;
		return;
	}
	if (IsAreaClicked(centerX, menuY, 300.0f, 60.0f, mouseX, mouseY) == 1)
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
		return;
	}

	// ----- dark see-through layer over the whole game -----
	CP_Settings_NoStroke(); //Don't throw stroke around the shape
	CP_Settings_RectMode(CP_POSITION_CORNER); //set Rectangle drawing mode to the corner
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 180)); // Set Opacity to 160 so there'll be a see-through layer
	CP_Graphics_DrawRect(0.0f, 0.0f, width, height);

	// ----- title -----
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextSize(48.0f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_DrawText("PAUSED", centerX, height * 0.28f);

	// ----- buttons -----
	menu_draw_button(centerX, resumeY, 300.0f, 60.0f, "RESUME", //Call the same function that was used to draw the buttons in main menu.
		CP_Color_Create(40, 160, 80, 255), CP_Color_Create(70, 200, 110, 255)); //Lighter hue of colour onHover
	menu_draw_button(centerX, menuY, 300.0f, 60.0f, "MAIN MENU",
		CP_Color_Create(110, 70, 200, 255), CP_Color_Create(150, 110, 230, 255)); //Lighter hue of colour onHover

	// ----- hint -----
	CP_Settings_Fill(CP_Color_Create(200, 200, 200, 255));
	CP_Settings_TextSize(14.0f);
	CP_Font_DrawText("ESC / P: RESUME", centerX, height * 0.70f);
}
