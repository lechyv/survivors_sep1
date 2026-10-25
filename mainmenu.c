//---------------------------------------------------------
// file:    mainmenu.c
// owner:   Wee Jie (Game Flow and UI)
// brief:   Main menu CProcessing game state.
//          LEFT/RIGHT or click to pick difficulty, ENTER to start, ESC to exit.
//---------------------------------------------------------
#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "utils.h"

#define CX ((float)WINDOW_WIDTH * 0.5f)

static void StartGame(void)
{
	CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
}

void Menu_Init(void)
{
	CP_System_SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	CP_System_SetWindowTitle("Survivor");
	CP_System_SetFrameRate(TARGET_FPS);
}

void Menu_Update(void)
{
	int level = (int)Difficulty_Get();

	// Keyboard
	if (CP_Input_KeyTriggered(KEY_LEFT) || CP_Input_KeyTriggered(KEY_A))
		level = (level + DIFFICULTY_COUNT - 1) % DIFFICULTY_COUNT;
	if (CP_Input_KeyTriggered(KEY_RIGHT) || CP_Input_KeyTriggered(KEY_D))
		level = (level + 1) % DIFFICULTY_COUNT;
	Difficulty_Set((DifficultyLevel)level);

	if (CP_Input_KeyTriggered(KEY_ENTER) || CP_Input_KeyTriggered(KEY_SPACE))
	{
		StartGame();
		return;
	}
	if (CP_Input_KeyTriggered(KEY_ESCAPE))
	{
		CP_Engine_Terminate();
		return;
	}

	// Drawing
	CP_Graphics_ClearBackground(Util_Color(92, 170, 250, 255));

	Util_DrawTextCentered("SURVIVOR", CX + 5.0f, 145.0f, 120.0f, Util_Color(130, 90, 210, 255));
	Util_DrawTextCentered("SURVIVOR", CX, 140.0f, 120.0f, Util_Color(255, 255, 255, 255));
	Util_DrawTextCentered("You build the drones. The drones do the fighting.", CX, 225.0f, 24.0f,
		Util_Color(255, 255, 255, 255));

	Util_DrawTextCentered("DIFFICULTY", CX, 300.0f, 24.0f, Util_Color(40, 20, 80, 255));
	for (int i = 0; i < DIFFICULTY_COUNT; ++i)
	{
		float x = CX + (float)(i - 1) * 190.0f;
		if (UI_Button(x, 350.0f, 170.0f, 50.0f, Difficulty_Name((DifficultyLevel)i), 1, i == level))
			Difficulty_Set((DifficultyLevel)i);
	}

	if (UI_Button(CX, 450.0f, 320.0f, 60.0f, "[ENTER] START", 1, 1))
	{
		StartGame();
		return;
	}
	if (UI_Button(CX, 525.0f, 320.0f, 50.0f, "[ESC] EXIT", 1, 0))
	{
		CP_Engine_Terminate();
		return;
	}

	Util_DrawTextCentered("WASD / ARROWS: MOVE     SPACE / SHIFT: DASH     B: DRONE SHOP     ESC / P: PAUSE",
		CX, 610.0f, 18.0f, Util_Color(40, 20, 80, 255));
	Util_DrawTextCentered("MADE BY ELVIS, BRANDON, WEE JIE, JAVIER", CX, 680.0f, 16.0f,
		Util_Color(255, 255, 255, 255));
}

void Menu_Exit(void)
{
}
