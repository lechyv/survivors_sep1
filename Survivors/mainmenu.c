#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "utils.h"

/* ---------- Main menu (Wee Jie) ----------
   Click START or press ENTER to play.
   Click EXIT or press ESC to quit. */

int menuDifficulty = difficulty_normal;   // still read by game.c, always NORMAL for now

// Font used for all menu text (loaded once in Main_Menu_Init)
CP_Font menuFont;

// Returns 1 if the mouse is inside the box centred at (x, y)
int menu_mouse_over(float x, float y, float w, float h)
{
	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	if (mouseX >= x - w / 2.0f && mouseX <= x + w / 2.0f &&
		mouseY >= y - h / 2.0f && mouseY <= y + h / 2.0f)
	{
		return 1;
	}
	return 0;
}

// Returns 1 on the frame the box is clicked
int menu_clicked(float x, float y, float w, float h)
{
	if (CP_Input_MouseTriggered(MOUSE_BUTTON_LEFT) && menu_mouse_over(x, y, w, h) == 1)
	{
		return 1;
	}
	return 0;
}

// Draws a button centred at (x, y). Lighter when the mouse is over it.
void menu_draw_button(float x, float y, float w, float h, const char* label,
	CP_Color color, CP_Color hoverColor)
{
	if (menu_mouse_over(x, y, w, h) == 1)
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
	menuFont = CP_Font_Load("Assets/Exo2-Regular.ttf");   // change to your .ttf file name
	CP_Font_Set(menuFont);
}

void Main_Menu_Update(void)
{
	float centerX = (float)CP_System_GetWindowWidth() / 2.0f;
	float height = (float)CP_System_GetWindowHeight();

	float startY = height * 0.50f;
	float exitY = height * 0.62f;

	/* ----- start and exit ----- */
	if (CP_Input_KeyTriggered(KEY_ENTER) || menu_clicked(centerX, startY, 300.0f, 60.0f) == 1)
	{
		CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
		return;
	}
	if (CP_Input_KeyTriggered(KEY_ESCAPE) || menu_clicked(centerX, exitY, 300.0f, 60.0f) == 1)
	{
		CP_Engine_Terminate();
		return;
	}

	/* ----- drawing ----- */
	CP_Graphics_ClearBackground(CP_Color_Create(92, 170, 250, 255));   // sky blue

	// title
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

	// controls
	CP_Settings_Fill(CP_Color_Create(40, 20, 80, 255));
	CP_Settings_TextSize(24.0f);
	CP_Font_DrawText("WASD: Nove    SPACE: Dash    Q: Back to menu    Esc: Exit game", centerX, height * 0.90f);
}

void Main_Menu_Exit(void)
{

}
