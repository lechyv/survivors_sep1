#include <stdio.h>
#include "cprocessing.h"
#include "buff.h"

//Gameplay related codes below

int buffKills = 0;				//enemies killed this game
float buffMultiplier = 1.0f;	//1.0 = normal damage, 1.1 = +10%, 1.21 = +21%...

void Buff_Init(void)
{
	buffKills = 0;
	buffMultiplier = 1.0f;
}

void Buff_AddKill(void) {
	buffKills += 1;
	if (buffKills % BUFF_KILLS_PER_BUFF == 0) {
		buffMultiplier = buffMultiplier * BUFF_DAMAGE_STEP;
	}
}

float Buff_ScaleDamage(float damage) {
	return damage * buffMultiplier;
}

float Buff_GetMultiplier(void) {
	return buffMultiplier;
}

//HUD getters (used by hud.c to show kills and buffs in the top-left panel)

int Buff_GetKills(void) {
	return buffKills;
}

int Buff_GetKillsToNextBuff(void) {
	return BUFF_KILLS_PER_BUFF - (buffKills % BUFF_KILLS_PER_BUFF);
}

//UI related codes below

void Buff_Draw(void)
{
	char text[64];
	int percent = (int)((buffMultiplier - 1.0f) * 100.0f + 0.5f);
	int killsToNext = BUFF_KILLS_PER_BUFF - (buffKills % BUFF_KILLS_PER_BUFF);

	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 120, 60, 255));
	CP_Settings_TextSize(20.0f);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_BOTTOM);
	CP_Font_DrawText(text, 20.0f, (float)CP_System_GetWindowHeight() - 20.0f);
}