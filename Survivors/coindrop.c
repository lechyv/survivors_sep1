//Done by Elvis
#include <stdio.h> // for printf
#include "cprocessing.h"
#include "coin-drop.h"


#define MAX_COINS 128 //Max coins on ground at once
#define COIN_DIAMETER 14.0f //size of coin (pixel)
#define COIN_PICKUP_DELAY 0.5f //seconds before a coin can be picked up 
#define COIN_TEXT_SIZE 40.0f //size of the "Coins: N" text
#define COIN_TEXT_X 20.0f//position of the counter x
#define COIN_TEXT_Y 20.0f//position of the counter y

/* coin data */
int coinActive[MAX_COINS]; //1 = on grond, 0 = free slot
float coinX[MAX_COINS]; //x position of coin
float coinY[MAX_COINS]; //y position of coin
float coinAge[MAX_COINS]; //seconds since coin was dropped

int coinCount; //how many coins player has collected

CP_Font coinFont; //font for the coin counter (loaded only once)
int coinFontLoaded = 0; //flag to check if font is loaded

void Coins_Init(void)
{
	//clear every slot so no old coins are left from last game
	int i = 0;
	while (i < MAX_COINS) {
		coinActive[i] = 0;
		i += 1;
	}
	coinCount = 0;

	if (!coinFontLoaded) {
		coinFont = CP_Font_Load("assets/PressStart2P-Regular.ttf"); //to be edited by WeeJie for coinFont
		coinFontLoaded = 1;
	}
}

void Coins_Drop(float x, float y)
{
	// look for the first free slot; stop as soon as one is used
	int placed = 0;
	int i = 0;
	while (i < MAX_COINS && !placed)
	{
		if (coinActive[i] == 0)
		{
			coinActive[i] = 1;
			coinX[i] = x;
			coinY[i] = y;
			coinAge[i] = 0.0f;
			placed = 1;
		}
		i += 1;
	}
	// if every slot is in use, the coin is simply not dropped
}

void Coins_Update(float playerX, float playerY, float playerRadius)
{
	float dt = CP_System_GetDt();

	// player and coin touch when the distance between their centers is at most the sum of their radii (compared squared, so no sqrt)
	float touchDistance = playerRadius + COIN_DIAMETER / 2.0f;

	int i = 0;
	while (i < MAX_COINS)
	{
		if (coinActive[i] == 1)
		{
			coinAge[i] += dt;

			float dx = coinX[i] - playerX;
			float dy = coinY[i] - playerY;

			if (coinAge[i] >= COIN_PICKUP_DELAY && dx * dx + dy * dy <= touchDistance * touchDistance)
			{
				coinActive[i] = 0;   // remove coin from the ground
				coinCount += 1;      // and add it to the player's coins
			}
		}
		i += 1;
	}
}


void Coins_Draw(void)
{
	// gold coins with no outline (same style as the bullets)
	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 215, 0, 255));

	int i = 0;
	while (i < MAX_COINS)
	{
		if (coinActive[i] == 1)
		{
			CP_Graphics_DrawCircle(coinX[i], coinY[i], COIN_DIAMETER);
		}
		i += 1;
	}
}

void Coins_DrawCounter(void)
{
	// build the text "Coins: N" in a char array
	char text[32];
	snprintf(text, sizeof(text), "Coins: %d", coinCount);

	CP_Font_Set(coinFont);
	CP_Settings_TextSize(COIN_TEXT_SIZE);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_TOP);
	CP_Settings_Fill(CP_Color_Create(255, 215, 0, 255));
	CP_Font_DrawText(text, COIN_TEXT_X, COIN_TEXT_Y);
}

int Coins_GetCount(void)
{
	return coinCount;
}
