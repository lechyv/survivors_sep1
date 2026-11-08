/*
UI GUIDE (for Weejie, delete after reading):
Search this file for "UI:" to find every part that controls how the shop and drones look.
Changing anything marked UI: only changes looks, not how the shop works.
Gameplay numbers (cost, fire rate, damage, the 6-drone cap) are NOT UI, ask Elvis first.
  - drone colours ............ Shop_SetDroneColor
  - drone sizes .............. shopDroneSize
  - drones on the map ........ Shop_DrawDrones
  - "B: SHOP" hint ........... Shop_Draw, top part
  - shop menu screen ......... Shop_Draw, rest of the function
  - button size .............. SHOP_BUTTON_WIDTH / SHOP_BUTTON_HEIGHT
  - button look (border, text) is drawn by menu_draw_button in mainmenu.c (shared with the
    main menu and pause menu, so changing it changes those too)
Colours are CP_Color_Create(red, green, blue, alpha), each 0 to 255 (alpha 255 = solid).
Screen positions like "height * 0.18f" mean 18% of the way down the window,
so the menu still lines up if the window size changes.
*/
#include <stdio.h>          // snprintf
#include <math.h>           // cosf, sinf
#include "cprocessing.h"
#include "shop.h"
#include "coin-drop.h"      // Coins_Spend, Coins_GetCount
#include "Swarm.h"          // Swarm_GetNearest
#include "mainmenu.h"       // menu_draw_button
#include "utils.h"          // IsAreaClicked
#include "pause.h"          // Pause_IsPaused


// player pos and firing functions from game.c
extern float playerX;
extern float playerY;
void bullet_fire(float fromX, float fromY, float targetX, float targetY, float damage);
void rocket_fire(float fromX, float fromY, float targetX, float targetY);
int combat_get_target(float x, float y, float range, CP_Vector* target);

#define DRONE_DISTANCE 60.0f    //orbit radius around the player (same as game.c)
#define DRONE_ORBIT_SPEED 1.5f  //radians per second (same as game.c)
#define DRONE_RANGE 350.0f      //how far a drone can see an enemy (same as game.c)
#define TWO_PI 6.28318531f

// UI related constants
#define SHOP_BUTTON_WIDTH  640.0f
#define SHOP_BUTTON_HEIGHT 60.0f

//stats of each drone type, indexed by DRONE_BASIC/ DRONE_MACHINE_GUN / DRONE_ROCKET
// fire rate, dmg and size copied from the drone
const int shopDroneCost[DRONE_TYPE_COUNT] = { 5, 10, 15 }; // cost of each drone
const float shopDroneFireRate[DRONE_TYPE_COUNT] = { 1.5f, 6.0f, 0.3f }; // seconds between shots]
const float shopDroneDamage[DRONE_TYPE_COUNT] = { 10.0f, 5.0f, 0.0f }; // damage per shot

//UI
const float shopDroneSize[DRONE_TYPE_COUNT] = { 16.0f, 14.0f, 18.0f }; // radius of each drone

//squad data: slot 0 to shopdronecount - 1 are in use
int shopDroneType[SHOP_MAX_DRONES];
float shopDroneX[SHOP_MAX_DRONES];
float shopDroneY[SHOP_MAX_DRONES];
float shopDroneFireTimer[SHOP_MAX_DRONES]; //seconds until this drone can fire again
int shopDroneCount;
float shopOrbitAngle; //angle of drone 0, the others are spread evenly after it]

int shopOpen;             //1 = shop menu is showing
const char* shopMessage;  //feedback line under the buttons ("" = nothing). UI: the texts are set in Shop_BuyDrone

// UI: name shown on the shop button (button label is "NAME - COST") 
const char* Shop_DroneName(int type)
{
	if (type == DRONE_MACHINE_GUN)
	{
		return "MACHINE GUN";
	}
	else if (type == DRONE_ROCKET)
	{
		return "ROCKET";
	}
	return "BASIC";
}

// UI: colour of each drone type, used for the drones on the map AND the dots in the shop menu. (same colours as the old drones in game.c) 

void Shop_SetDroneColor(int type)
{
	if (type == DRONE_MACHINE_GUN)
	{
		CP_Settings_Fill(CP_Color_Create(255, 180, 50, 255));   //orange
	}
	else if (type == DRONE_ROCKET)
	{
		CP_Settings_Fill(CP_Color_Create(100, 160, 255, 255));  //blue
	}
	else
	{
		CP_Settings_Fill(CP_Color_Create(80, 220, 170, 244));   //green
	}
}

// spreads every drone evenly around the player, starting at shopOrbitAngle 
void Shop_PlaceDrones(void)
{
	int i = 0;
	while (i < shopDroneCount)
	{
		float angle = shopOrbitAngle + (TWO_PI * (float)i) / (float)shopDroneCount;
		shopDroneX[i] = playerX + cosf(angle) * DRONE_DISTANCE;
		shopDroneY[i] = playerY + sinf(angle) * DRONE_DISTANCE;
		i += 1;
	}
}



// adds a drone to the squad for free. Returns 1 if added, 0 if the squad is full 
int Shop_AddDrone(int type)
{
	if (shopDroneCount >= SHOP_MAX_DRONES)
	{
		return 0;
	}
	shopDroneType[shopDroneCount] = type;
	shopDroneFireTimer[shopDroneCount] = 0.0f; //ready to fire straight away
	shopDroneCount += 1;
	Shop_PlaceDrones(); //re-space the squad so the new drone shows up right away
	return 1;
}

void Shop_Init(void)
{
	shopDroneCount = 0;
	shopOrbitAngle = 0.0f;
	shopOpen = 0;
	shopMessage = "";
	Shop_AddDrone(DRONE_BASIC); //player starts with one basic drone
}

void Shop_HandleInput(void)
{
	if (CP_Input_KeyTriggered(KEY_B))
	{
		if (shopOpen == 1)
		{
			shopOpen = 0;
		}
		else
		{
			shopOpen = 1;
			shopMessage = "";
		}
	}
	else if (shopOpen == 1 && CP_Input_KeyTriggered(KEY_ESCAPE))
	{
		shopOpen = 0; // makes esc only close shop, wont open main menu ontop of shop
	}
}

int Shop_IsOpen(void)
{
	return shopOpen;
}

int Shop_BuyDrone(int type)
{
	if (type < 0 || type >= DRONE_TYPE_COUNT)
	{
		return 0;
	}
	if (shopDroneCount >= SHOP_MAX_DRONES)
	{
		shopMessage = "SQUAD FULL (6/6)";      //UI: text shown under the buttons
		return 0;
	}
	if (Coins_Spend(shopDroneCost[type]) == 0)
	{
		shopMessage = "NOT ENOUGH COINS";      //UI: text shown under the buttons
		return 0;
	}

	Shop_AddDrone(type);
	shopMessage = "DRONE ADDED TO SQUAD";      //UI: text shown under the buttons
	return 1;
}

int Shop_GetDroneCount(void)
{
	return shopDroneCount;
}

void Shop_UpdateDrones(void)
{
	float dt = CP_System_GetDt();

	shopOrbitAngle += DRONE_ORBIT_SPEED * dt;
	Shop_PlaceDrones();

	int i = 0;
	while (i < shopDroneCount)
	{
		shopDroneFireTimer[i] -= dt;

		if (shopDroneFireTimer[i] <= 0.0f)
		{
			CP_Vector target;
			if (combat_get_target(shopDroneX[i], shopDroneY[i], DRONE_RANGE, &target) == 1)
			{
				int type = shopDroneType[i];
				if (type == DRONE_ROCKET)
				{
					rocket_fire(shopDroneX[i], shopDroneY[i], target.x, target.y);
				}
				else
				{
					bullet_fire(shopDroneX[i], shopDroneY[i], target.x, target.y, shopDroneDamage[type]);
				}
				shopDroneFireTimer[i] = 1.0f / shopDroneFireRate[type];
			}
		}
		i += 1;
	}
}


// UI: draws each drone as a plain circle (no outline) in its type's colour and size.
// To use pictures instead, load them once in Shop_Init with CP_Image_Load and
// replace CP_Graphics_DrawCircle below with CP_Image_Draw at the same x, y 
void Shop_DrawDrones(void)
{
	CP_Settings_NoStroke(); //UI: no outline. Use CP_Settings_Stroke + CP_Settings_StrokeWeight for one

	int i = 0;
	while (i < shopDroneCount)
	{
		Shop_SetDroneColor(shopDroneType[i]);
		CP_Graphics_DrawCircle(shopDroneX[i], shopDroneY[i], shopDroneSize[shopDroneType[i]]);
		i += 1;
	}
}

void Shop_Draw(void)
{
	float width = (float)CP_System_GetWindowWidth();
	float height = (float)CP_System_GetWindowHeight();
	float centerX = width / 2.0f;
	char text[64];

	// UI: shop closed: just a small hint in the bottom-right corner 
	if (shopOpen == 0)
	{
		CP_Settings_NoStroke();
		CP_Settings_Fill(CP_Color_Create(255, 255, 255, 200));							//UI: hint colour (white, slightly see-through)
		CP_Settings_TextSize(16.0f);													//UI: hint text size
		CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_RIGHT, CP_TEXT_ALIGN_V_BOTTOM);		//UI: x, y below is the text's bottom-right corner
		CP_Font_DrawText("B: SHOP", width - 20.0f, height - 20.0f);						//UI: 20 px in from the right and bottom edges
		return;
	}

	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	// UI: dark see-through layer over the frozen game
	CP_Settings_NoStroke();
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 180));   //UI: black, alpha 180 = mostly dark. Lower alpha = see more of the game
	CP_Graphics_DrawRect(0.0f, 0.0f, width, height);   //covers the whole window

	// UI: title and coin / squad count 
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE); //UI: x, y below is the centre of the text
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));   //UI: title colour (white)
	CP_Settings_TextSize(48.0f);                             //UI: title size
	CP_Font_DrawText("SHOP", centerX, height * 0.18f);       //UI: title 18% down the screen

	snprintf(text, sizeof(text), "COINS: %d    DRONES: %d/%d", Coins_GetCount(), shopDroneCount, SHOP_MAX_DRONES); //UI: wording of the info line
	CP_Settings_Fill(CP_Color_Create(255, 215, 0, 255));     //UI: info line colour (gold, same as the coins)
	CP_Settings_TextSize(20.0f);                             //UI: info line size
	CP_Font_DrawText(text, centerX, height * 0.27f);         //UI: info line 27% down the screen

	// one button per drone type
	int type = 0;
	while (type < DRONE_TYPE_COUNT)
	{
		// UI: first button 40% down the screen, each next one 12% lower (40%, 52%, 64%).
		// Change 0.40f to move all buttons, 0.12f to change the gap between them 
		float buttonY = height * (0.40f + 0.12f * (float)type);

		// clicks are ignored while the pause menu is on top 
		if (Pause_IsPaused() == 0 &&
			IsAreaClicked(centerX, buttonY, SHOP_BUTTON_WIDTH, SHOP_BUTTON_HEIGHT, mouseX, mouseY) == 1)
		{
			Shop_BuyDrone(type);
		}

		snprintf(text, sizeof(text), "%s - %d", Shop_DroneName(type), shopDroneCost[type]); //UI: button label, e.g. "BASIC - 15"

		// UI: green if the player can buy it, grey if not.
		// menu_draw_button(x, y, width, height, label, normal colour, colour when the mouse is over it) */
		if (shopDroneCount < SHOP_MAX_DRONES && Coins_GetCount() >= shopDroneCost[type])
		{
			menu_draw_button(centerX, buttonY, SHOP_BUTTON_WIDTH, SHOP_BUTTON_HEIGHT, text,
				CP_Color_Create(40, 160, 80, 255), CP_Color_Create(70, 200, 110, 255));   //UI: green, lighter green on hover
		}
		else
		{
			menu_draw_button(centerX, buttonY, SHOP_BUTTON_WIDTH, SHOP_BUTTON_HEIGHT, text,
				CP_Color_Create(90, 90, 90, 255), CP_Color_Create(90, 90, 90, 255));      //UI: grey, no hover change (cannot buy)
		}

		// UI: drone colour dot on the left of the button, so players can tell which drone is which.
		//  40 px in from the button's left edge, 1.5x the drone's size on the map 
		CP_Settings_NoStroke();
		Shop_SetDroneColor(type);
		CP_Graphics_DrawCircle(centerX - SHOP_BUTTON_WIDTH / 2.0f + 40.0f, buttonY, shopDroneSize[type] * 1.5f);

		type += 1;
	}

	// UI: feedback and hint 
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));    //UI: feedback colour (white)
	CP_Settings_TextSize(20.0f);                              //UI: feedback size
	CP_Font_DrawText(shopMessage, centerX, height * 0.78f);   //UI: "NOT ENOUGH COINS" etc., 78% down the screen

	CP_Settings_Fill(CP_Color_Create(200, 200, 200, 255));    //UI: hint colour (light grey)
	CP_Settings_TextSize(14.0f);                              //UI: hint size
	CP_Font_DrawText("CLICK A DRONE TO BUY    B / ESC: CLOSE SHOP", centerX, height * 0.86f); //UI: hint 86% down the screen
}