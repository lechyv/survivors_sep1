#include <stdio.h>          
#include "cprocessing.h"
#include "hud.h"
#include "boss.h"
#include "buff.h"           // kills, buffs and damage multiplier

#define hud_x       10.0f    // top-left corner of the panel
#define hud_y       10.0f
#define hud_width   260.0f
#define hud_height  220.0f   // taller to fit the kills and buff lines
#define bar_width   236.0f   // width of the HP and dash bars
#define bar_height  10.0f
#define text_size   14.0f

extern float bossHealth;
extern int bossActive;

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
	float dashCooldownLeft, float dashCooldownMax, float runTime, float swarmTimer)
{
	char text[32];
	float left = hud_x + 12.0f;   // left edge of everything inside the panel

	int seconds_left = (int)(swarmTimer + 0.999f);   // round up: 44.2s left shows as 45

	(void)runTime;   // not shown yet

	// Background
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 160));   // see-through black
	CP_Settings_Stroke(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_StrokeWeight(2.0f);
	CP_Graphics_DrawRect(hud_x, hud_y, hud_width, hud_height);

	//Game Run Time
	snprintf(text, sizeof(text), "TIME LEFT: %ds", seconds_left);
	HUD_Draw_Text(text, left, hud_y + 12.0f);

	// Player HP
	snprintf(text, sizeof(text), "HP %d/%d", (int)playerHealth, (int)playerMaxHealth);
	HUD_Draw_Text(text, left, hud_y + 42.0f);
	HUD_Draw_Bar(left, hud_y + 62.0f, playerHealth / playerMaxHealth,
		CP_Color_Create(220, 50, 50, 255));   // red

	// coins
	snprintf(text, sizeof(text), "COINS %d", coins);
	HUD_Draw_Text(text, left, hud_y + 84.0f);

	// dash cooldown
	if (dashCooldownLeft <= 0.0f)
	{
		snprintf(text, sizeof(text), "DASH READY");
	}
	else
	{
		snprintf(text, sizeof(text), "DASH %.1fs", (double)dashCooldownLeft);
	}
	HUD_Draw_Text(text, left, hud_y + 108.0f);

	// bar fills up as the dash recharges (full = ready)
	float dashPercent = 1.0f - dashCooldownLeft / dashCooldownMax;
	HUD_Draw_Bar(left, hud_y + 128.0f, dashPercent,
		CP_Color_Create(80, 200, 255, 255));  // light blue

	int buffPercent = (int)((Buff_GetMultiplier() - 1.0f) * 100.0f + 0.5f);

	snprintf(text, sizeof(text), "KILLS %d", Buff_GetKills());
	HUD_Draw_Text(text, left, hud_y + 150.0f);

	snprintf(text, sizeof(text), "DMG +%d%%", buffPercent);
	HUD_Draw_Text(text, left, hud_y + 172.0f);

	snprintf(text, sizeof(text), "KILLS TO BUFF:%d", Buff_GetKillsToNextBuff());
	HUD_Draw_Text(text, left, hud_y + 194.0f);

	
	if (bossActive == 1)	//If statement to check if the boss is active
	{
		HUD_Draw_Boss_Health_Bar(bossHealth);	//Draws the boss's health bar using the boss's current health
	}
}

//Function to draw the boss health bar at the top centre of the screen - Brandon
void HUD_Draw_Boss_Health_Bar(float bossHealth)
{
	float width = 500.0f;	//Width of the health bar in pixels
	float height = 20.0f;	//Height of the health bar in pixels
	float x = (CP_System_GetWindowWidth() - width) / 2.0f;	//X-coordinate of the bar's left edge, which centres the bar horizontally
	float y = 40.0f;	//Y-coordinate of the bar's top edge

	float health_percent = bossHealth / boss_health_max;	//Percentage of health the boss has left

	if (health_percent < 0.0f)	//If statement to check if the health percent is below 0
	{
		health_percent = 0.0f;	//Sets the health percent to 0 to prevent a negative bar width
	}

	if (health_percent > 1.0f)	//If statement to check if the health percent is above 1
	{
		health_percent = 1.0f;	//Sets the health percent to 1 to prevent the bar from going past its full width
	}

	CP_Settings_NoStroke();	//Prevents outlines from appearing around the bar
	CP_Settings_RectMode(CP_POSITION_CORNER);	//Sets the rectangle position to start from the top left corner

	CP_Settings_Fill(CP_Color_Create(40, 40, 40, 255));	//Sets the colour to dark grey for the bar's background
	CP_Graphics_DrawRect(x, y, width, height);	//Draws the full-width background of the bar

	CP_Settings_Fill(CP_Color_Create(220, 40, 40, 255));	//Sets the colour to red for the remaining health
	CP_Graphics_DrawRect(x, y, width * health_percent, height);	//Draws the red bar, shortened according to the boss's remaining health

	char text[64];	//Stores the text shown above the bar
	snprintf(text, sizeof(text), "FINAL BOSS %d / %d", (int)bossHealth, (int)boss_health_max);	//Writes the boss's current and maximum health into the text

	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));	//Sets the text colour to white
	CP_Settings_TextSize(18.0f);	//Sets the text size to 18
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_BOTTOM);	//Centres the text horizontally and aligns its bottom edge to the given Y-coordinate
	CP_Font_DrawText(text, x + width / 2.0f, y - 5.0f); //Draws the text centred above the bar, 5 pixels over its top edge
}
