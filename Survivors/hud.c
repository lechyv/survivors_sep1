#include <stdio.h>          
#include "cprocessing.h"
#include "hud.h"

#define hud_x       10.0f    // top-left corner of the panel
#define hud_y       10.0f
#define hud_width   260.0f
#define hud_height  120.0f
#define bar_width   236.0f   // width of the HP and dash bars
#define bar_height  10.0f
#define text_size   14.0f

// Draws a bar that is that goes from 0.0 to 1.0 (0% - 100%)
static void HUD_Draw_Bar(float x, float y, float percent, CP_Color color)
{
	if (percent < 0.0f) percent = 0.0f;
	if (percent > 1.0f) percent = 1.0f;

	CP_Settings_NoStroke();
	CP_Settings_RectMode(CP_POSITION_CORNER);

	// dark background
	CP_Settings_Fill(CP_Color_Create(40, 40, 40, 255));
	CP_Graphics_DrawRect(x, y, bar_width, bar_height);

	// coloured fill
	CP_Settings_Fill(color);
	CP_Graphics_DrawRect(x, y, bar_width * percent, bar_height);
}

// Draws white text with its top-left corner at (x, y)
static void HUD_Draw_Text(const char* text, float x, float y)
{
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextSize(text_size);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_TOP);
	CP_Font_DrawText(text, x, y);
}

void HUD_Draw(float playerHealth, float playerMaxHealth, int coins,
	float dashCooldownLeft, float dashCooldownMax)
{
	char text[32];
	float left = hud_x + 12.0f;   // left edge of everything inside the panel

	// Background
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 160));   // see-through black
	CP_Settings_Stroke(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_StrokeWeight(2.0f);
	CP_Graphics_DrawRect(hud_x, hud_y, hud_width, hud_height);

	// Player HP
	snprintf(text, sizeof(text), "HP %d/%d", (int)playerHealth, (int)playerMaxHealth);
	HUD_Draw_Text(text, left, hud_y + 12.0f);
	HUD_Draw_Bar(left, hud_y + 32.0f, playerHealth / playerMaxHealth,
		CP_Color_Create(220, 50, 50, 255));   // red

	// coins
	snprintf(text, sizeof(text), "COINS %d", coins);
	HUD_Draw_Text(text, left, hud_y + 54.0f);

	// dash cooldown
	if (dashCooldownLeft <= 0.0f)
	{
		snprintf(text, sizeof(text), "DASH READY");
	}
	else
	{
		snprintf(text, sizeof(text), "DASH %.1fs", (double)dashCooldownLeft);
	}
	HUD_Draw_Text(text, left, hud_y + 78.0f);

	// bar fills up as the dash recharges (full = ready)
	float dashPercent = 1.0f - dashCooldownLeft / dashCooldownMax;
	HUD_Draw_Bar(left, hud_y + 98.0f, dashPercent,
		CP_Color_Create(80, 200, 255, 255));  // light blue
}
