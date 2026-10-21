

#include <stdlib.h>
#include <time.h>
#include "cprocessing.h"
#include "mainmenu.h"
#include "utils.h"
#include "game.h"

float p1X, p1Y;   // green position
float p2X, p2Y;   // red position
float size;       // circle diameter
float speed;      // speed of movement
float defaultGreen, defaultRed;
int controlled;


void Game_Init(void)
{

	//green position
	p1X = CP_System_GetWindowWidth() / 3.0f;
	p1Y = CP_System_GetWindowHeight() /6.0f;

	//red position
	p2X = CP_System_GetWindowWidth() / 1.5f;
	p2Y = CP_System_GetWindowHeight() / 2.0f;

	//size and speed of both green and red
	size = 70.0f;
	speed = 5.0f;

	//random direction of circles
	srand(time(NULL));
	float defaultGreen = (rand() % 360) + 1;  // Random float from 1 - 360
	float defaultRed = (rand() % 360) + 1;  // Random float from 1 - 360

	controlled = 1;

}


void Game_Update(void)
{
	CP_Graphics_ClearBackground(CP_Color_Create(192, 192, 192, 255));

	//Used for detection of circle nearing the boundary
	float width = (float)CP_System_GetWindowWidth();
	float height = (float)CP_System_GetWindowHeight();
	float radius = size / 2.0f;

	float mouseX = CP_Input_GetMouseX();
	float mouseY = CP_Input_GetMouseY();

	

	if (CP_Input_MouseClicked()) 
	{
		if (IsCircleClicked(p1X, p1Y, size, mouseX, mouseY))
		{
			controlled = 1;
		}
		if (IsCircleClicked(p2X, p2Y, size, mouseX, mouseY)) 
		{
			controlled = 2;
		}
	}

	if (controlled == 1)
	{
		//Green circle will be the layer on top of red circle
		// red circle
		CP_Settings_Fill(CP_Color_Create(220, 50, 50, 255));
		CP_Graphics_DrawCircle(p2X, p2Y, size);
		//red triangle
		CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
		CP_Graphics_DrawTriangleAdvanced(p2X + 30, p2Y, p2X - 15, p2Y - 25, p2X - 15, p2Y + 25, defaultRed);

		// green circle
		CP_Settings_Fill(CP_Color_Create(50, 180, 50, 255));
		CP_Graphics_DrawCircle(p1X, p1Y, size);
		//green triangle
		CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
		CP_Graphics_DrawTriangleAdvanced(p1X + 30, p1Y, p1X - 15, p1Y - 25, p1X - 15, p1Y + 25, defaultGreen);

		// For red "AI" to move on its own
		int canRight = (p2X + radius + speed <= width);    // not hitting the right border
		int canDown = (p2Y + radius + speed <= height);   // not hitting the bottom border
		int canLeft = (p2X - radius - speed >= 0);        // not hitting the left border
		int canUp = (p2Y - radius - speed >= 0);        // not hitting the top border

		// how far red is from green, sideways and up/down
		float distX = p2X - p1X;
		float distY = p2Y - p1Y;
		float range = size * 2.0f;   // how close counts as "near"

		// red will try to avoid collision with green
		if (distX * distX + distY * distY <= range * range)
		{
			if (p1X > p2X)
			{
				canRight = 0;   // green is to the right
			}
			else
			{
				canLeft = 0;    // green is to the left
			}

			if (p1Y > p2Y)
			{
				canDown = 0;    // green is below
			}
			else
			{
				canUp = 0;      // green is above
			}
		}

		if (defaultRed == 0 && canRight)          // facing right
		{
			p2X += speed;
		}
		else if (defaultRed == 90 && canDown)     // facing down
		{
			p2Y += speed;
		}
		else if (defaultRed == 180 && canLeft)    // facing left
		{
			p2X -= speed;
		}
		else if (defaultRed == -90 && canUp)      // facing up
		{
			p2Y -= speed;
		}
		else
		{
			//Turn to safe direction
			if (canRight)
			{
				defaultRed = 0;
			}
			else if (canDown)
			{
				defaultRed = 90;
			}
			else if (canLeft)
			{
				defaultRed = 180;
			}
			else if (canUp)
			{
				defaultRed = -90;
			}
		}

		// move green circle
		if (CP_Input_KeyDown(KEY_W))
		{
			p1Y -= speed;
			defaultGreen = -90;
		}
		else if (CP_Input_KeyDown(KEY_S))
		{
			p1Y += speed;
			defaultGreen = 90;
		}
		else if (CP_Input_KeyDown(KEY_A))
		{
			p1X -= speed;
			defaultGreen = 180;
		}
		else if (CP_Input_KeyDown(KEY_D))
		{
			p1X += speed;
			defaultGreen = 0;
		}
	}


	else if (controlled == 2)
	{
		//Red circle will be the layer on top of green circle
		// green circle
		CP_Settings_Fill(CP_Color_Create(50, 180, 50, 255));
		CP_Graphics_DrawCircle(p1X, p1Y, size);
		//green triangle
		CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
		CP_Graphics_DrawTriangleAdvanced(p1X + 30, p1Y, p1X - 15, p1Y - 25, p1X - 15, p1Y + 25, defaultGreen);

		// red circle
		CP_Settings_Fill(CP_Color_Create(220, 50, 50, 255));
		CP_Graphics_DrawCircle(p2X, p2Y, size);
		//red triangle
		CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
		CP_Graphics_DrawTriangleAdvanced(p2X + 30, p2Y, p2X - 15, p2Y - 25, p2X - 15, p2Y + 25, defaultRed);

		// For Green "AI" To move on its own
		int canRight = (p1X + radius + speed <= width);    // not hitting the right border
		int canDown = (p1Y + radius + speed <= height);   // not hitting the bottom border
		int canLeft = (p1X - radius - speed >= 0);        // not hitting the left border
		int canUp = (p1Y - radius - speed >= 0);        // not hitting the top border

		// how far green is from red, sideways and up/down
		float distX = p1X - p2X;
		float distY = p1Y - p2Y;
		float range = size * 2.0f;   // how close counts as "near"

		// green will try to avoid collision with red
		if (distX * distX + distY * distY <= range * range)
		{
			if (p2X > p1X)
			{
				canRight = 0;   // red is to the right
			}
			else
			{
				canLeft = 0;    // red is to the left
			}

			if (p2Y > p1Y)
			{
				canDown = 0;    // red is below
			}
			else
			{
				canUp = 0;      // red is above
			}
		}

		if (defaultGreen == 0 && canRight)          // facing right
		{
			p1X += speed;
		}
		else if (defaultGreen == 90 && canDown)     // facing down
		{
			p1Y += speed;
		}
		else if (defaultGreen == 180 && canLeft)    // facing left
		{
			p1X -= speed;
		}
		else if (defaultGreen == -90 && canUp)      // facing up
		{
			p1Y -= speed;
		}
		else
		{
			// Turn to next safest directsion
			if (canRight)
			{
				defaultGreen = 0;
			}
			else if (canDown)
			{
				defaultGreen = 90;
			}
			else if (canLeft)
			{
				defaultGreen = 180;
			}
			else if (canUp)
			{
				defaultGreen = -90;
			}
		}

		// move red circle
		if (CP_Input_KeyDown(KEY_W))
		{
			p2Y -= speed;
			defaultRed = -90;
		}
		else if (CP_Input_KeyDown(KEY_S))
		{
			p2Y += speed;
			defaultRed = 90;
		}
		else if (CP_Input_KeyDown(KEY_A))
		{
			p2X -= speed;
			defaultRed = 180;
		}
		else if (CP_Input_KeyDown(KEY_D))
		{
			p2X += speed;
			defaultRed = 0;
		}
	}
	
	if (CP_Input_KeyTriggered(KEY_Q)) {
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}
}

void Game_Exit(void)
{

}