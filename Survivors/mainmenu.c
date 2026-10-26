

#include "cprocessing.h"
#include "utils.h"
#include "game.h"
#include "Swarm.h"

CP_Font myFont;
float playX, playY, quitX, quitY, sizeX, sizeY;
int selectedDifficulty = 2;

void Main_Menu_Init(void)
{
	playX = CP_System_GetWindowWidth() / 2.0f;
	playY = CP_System_GetWindowHeight() / 4.0f;
	quitX = CP_System_GetWindowWidth() / 2.0f;
	quitY = CP_System_GetWindowHeight() / 1.5f;
	sizeX = CP_System_GetWindowWidth() / 5.0f;
	sizeY = CP_System_GetWindowHeight() / 5.0f;
	
	//Text
	myFont = CP_Font_Load("Assets/Exo2-Regular.ttf");
	CP_Font_Set(myFont);
	CP_Settings_TextSize(100.0f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
}

void Main_Menu_Update(void)
{
	//set background to gray
	CP_Graphics_ClearBackground(CP_Color_Create(192, 192, 192, 255));

	CP_Settings_Fill(CP_Color_Create(150, 0, 0, 150));
	CP_Settings_RectMode(CP_POSITION_CENTER);

	//play button
	CP_Graphics_DrawRect(playX, playY, sizeX, sizeY);

	//exit button
	CP_Graphics_DrawRect(quitX, quitY, sizeX, sizeY);

	//text
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 255));
	CP_Font_DrawText("Play", playX, playY);
	CP_Font_DrawText("Exit", quitX, quitY);



	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();


	if (IsAreaClicked(playX, playY, sizeX, sizeY, mouseX, mouseY))
	{
		CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
	}

	if (IsAreaClicked(quitX, quitY, sizeX, sizeY, mouseX, mouseY))
	{
		CP_Engine_Terminate();
	}

}

void Main_Menu_Exit(void)
{
	CP_Font_Free(myFont);
}