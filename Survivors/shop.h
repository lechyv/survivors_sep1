#pragma once
//For Wee Jie: Delete this after reading and editing
// Shop_DrawDrones and Shop_Draw are for their colours, size and positions in shop.c
//please amend them however u see fit, ty :) 

 

//drone type (to be used with Shop_BuyDrone)
#define DRONE_BASIC 0
#define DRONE_MACHINE_GUN 1
#define DRONE_ROCKET 2
#define DRONE_TYPE_COUNT 3

//max number of drones player can have in a game
#define SHOP_MAX_DRONES 6

//Gives player one basic drone. Called once in Game_Init
void Shop_Init(void);

//"B" opens/closes shop. ESC also closes it.
//Called in Game_Update while game is not paused and not over
void Shop_HandleInput(void);

//Returns 1 while the game is frozen (basically stops game and opens shop), 0 otherwise
int Shop_IsOpen(void);

//Spend drone cost and add drone to squad. 
//Return 1 if bought. 0 if not bought or max squad
int Shop_BuyDrone(int type);

//Return how many drones player has
int Shop_GetDroneCount(void);

//Move every drone around player and fire at nearest enemy
//Called in Game_Update while game is running
void Shop_UpdateDrones(void);

//Draw every drone in the squad
//UI: drone colours are in Shop_SetDroneColor, size in shopDroneSize (shop.c)
void Shop_DrawDrones(void);

//Draw the shop menu and handle button clicks while it is open.
//When closed, only draw a small "B: SHOP" hint.
//Call before Pause_Draw() in Game_Draw()
//UI: the whole shop screen layout (title, buttons, text, colours) is in this function in shop.c
void Shop_Draw(void);
