#include <stdio.h>      // snprintf for the countdown text
#include "cprocessing.h"
#include "wave.h"

//gameplay
int waveOnBreak = 0; //1 while the break is running
float waveBreakTimer = 0.0f; //seconds left in the break
int waveEndedSwarm = 0; // which swarm just ended ( 1 or 2), used for the message

void Wave_Init(void) {
	waveOnBreak = 0;
	waveBreakTimer = 0.0f;
	waveEndedSwarm = 0;
}
void Wave_StartBreak(int swarmThatEnded) {
	waveOnBreak = 1;
	waveBreakTimer = WAVE_BREAK_TIME;
	waveEndedSwarm = swarmThatEnded;
}

int Wave_IsOnBreak(void) {
	return waveOnBreak;
}

int Wave_UpdateBreak(void) {
	if (waveOnBreak == 0) {
		return 0;
	}
	waveBreakTimer -= CP_System_GetDt();
	if (waveBreakTimer <= 0.0f)
	{
		Wave_Init();
		return 1;
	}
	return 0;
}

//Send display message 
void Wave_Draw(void) {
	if (waveOnBreak == 0) {
		return;
	}
	float centerX = (float)CP_System_GetWindowWidth() / 2.0f;
	float height = (float)CP_System_GetWindowHeight();
	int secondsLeft = (int)waveBreakTimer + 1;

	char title[32];
	char countdown[64];

	/* UI: the words on screen. Keep the %d, it becomes the swarm number / seconds left */
	snprintf(title, sizeof(title), "SWARM %d CLEARED!", waveEndedSwarm);
	if (waveEndedSwarm == 1)
	{
		snprintf(countdown, sizeof(countdown), "SWARM 2 STARTS IN %d", secondsLeft);
	}
	else
	{
		snprintf(countdown, sizeof(countdown), "BOSS ARRIVES IN %d", secondsLeft);
	}
	//UI: dark see - through band behind the text * /
	CP_Settings_NoStroke();
	CP_Settings_RectMode(CP_POSITION_CENTER);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 150));                           //UI: band colour (alpha 150 = see-through)
	CP_Graphics_DrawRect(centerX, height * 0.30f, (float)CP_System_GetWindowWidth(), 200.0f); //UI: band 30% down, full width, 200 px tall

	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE); //UI: x, y below is the centre of the text

	/* UI: big title */
	CP_Settings_Fill(CP_Color_Create(255, 215, 0, 255));   //UI: title colour (gold, same as the coins)
	CP_Settings_TextSize(48.0f);                            //UI: title size
	CP_Font_DrawText(title, centerX, height * 0.30f - 50.0f);

	/* UI: countdown line */
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255)); //UI: countdown colour (white)
	CP_Settings_TextSize(24.0f);                            //UI: countdown size
	CP_Font_DrawText(countdown, centerX, height * 0.30f + 10.0f);

	/* UI: hint line */
	CP_Settings_Fill(CP_Color_Create(200, 200, 220, 255)); //UI: hint colour (light grey)
	CP_Settings_TextSize(18.0f);                            //UI: hint size
	CP_Font_DrawText("PICK UP YOUR COINS - PRESS B FOR THE SHOP", centerX, height * 0.30f + 55.0f);
}
