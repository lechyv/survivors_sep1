#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"

/* ---------- Player state ---------- */
float playerX, playerY;     // arrow position
float speed;                // normal speed of movement
float orangeArrow;          // arrow rotation angle (degrees)

float dashSpeed;            // speed while dashing
float dashDistance;         // total distance of one dash
float dashRemaining;        // distance left in the current dash (0 = not dashing)
float dashCooldown;         // seconds between dashes
float dashCooldownTimer;    // seconds left until the next dash is allowed

/* ---------- Player controls ---------- */

// Space: start a dash if off cooldown and not already dashing
void Player_HandleDashInput(void)
{
	if (CP_Input_KeyTriggered(KEY_SPACE) && dashRemaining <= 0.0f && dashCooldownTimer <= 0.0f)
	{
		dashRemaining = dashDistance;
		dashCooldownTimer = dashCooldown;
	}
}

// Moves the arrow forward in the direction it is facing during a dash
void Player_Dash(void)
{ //step is a variable that tracks how much distance remains on your dash
  //and makes sure any value you set for dashspeed does not overshoot. 
  //since once you preess spacebar, dashremaining = dashdistance,
  //it constantly gets updated frame by frame and decreases its pixels left
  //because step is assigned to dashspeed to frame by frame smooth out the dash effect,
  // it may overshoot when dashremaing is let's say 10pixels left and dashspeed is 20pixels.
  // this would of course cause the last instance of dash frame to exceed the distance by 10 pixels.
  // hence this is where step is assigned dash remaining, so the last frame instance is 10 pixels, instead of 20pixels.

	float step = dashSpeed;
	if (step > dashRemaining)
	{
		step = dashRemaining;   
	}

	if (orangeArrow == 0.0f)
	{
		playerX += step;
	}
	else if (orangeArrow == 90.0f)
	{
		playerY += step;
	}
	else if (orangeArrow == 180.0f)
	{
		playerX -= step;
	}
	else if (orangeArrow == -90.0f)
	{
		playerY -= step;
	}

	dashRemaining -= step;
}

// WASD: normal movement and facing direction
void Player_Move(void)
{
	if (CP_Input_KeyDown(KEY_W))
	{
		playerY -= speed;
		orangeArrow = -90.0f;
	}
	else if (CP_Input_KeyDown(KEY_S))
	{
		playerY += speed;
		orangeArrow = 90.0f;
	}
	else if (CP_Input_KeyDown(KEY_A))
	{
		playerX -= speed;
		orangeArrow = 180.0f;
	}
	else if (CP_Input_KeyDown(KEY_D))
	{
		playerX += speed;
		orangeArrow = 0.0f;
	}
}

//  player function
 void Player_Update(void)
{
	// tick the dash cooldown down
	if (dashCooldownTimer > 0.0f)
	{
		dashCooldownTimer -= CP_System_GetDt();
	}

	Player_HandleDashInput();

	if (dashRemaining > 0.0f)
	{
		Player_Dash();      // WASD is ignored while dashing
	}
	else
	{
		Player_Move();
	}
}

// Draws an orange arrow
void Player_Draw(void)
{
	CP_Settings_Fill(CP_Color_Create(255, 165, 0, 255));
	CP_Graphics_DrawTriangleAdvanced(playerX + 30, playerY, playerX - 15, playerY - 25, playerX - 15, playerY + 25, orangeArrow);
}

/* ---------- Game state ---------- */

void Game_Init(void)
{
	// arrow position on screen (spawn point)
	playerX = CP_System_GetWindowWidth() / 3.0f;
	playerY = CP_System_GetWindowHeight() / 6.0f;

	// speed of player
	speed = 5.0f;

	// arrow starts facing right
	orangeArrow = 0.0f;

	// dash settings
	dashSpeed = 20.0f;
	dashDistance = 150.0f;
	dashRemaining = 0.0f;
	dashCooldown = 3.0f;
	dashCooldownTimer = 0.0f;
}

void Game_Update(void)
{
	// black background
	CP_Graphics_ClearBackground(CP_Color_Create(0, 0, 0, 255));

	Player_Update();
	Player_Draw();

	//program termination
	if (CP_Input_KeyTriggered(KEY_Q))
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}
}

void Game_Exit(void)
{

}
