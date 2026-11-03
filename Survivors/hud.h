#pragma once

// Draws the HUD panel in the top-left corner.
// Call it LAST in Game_Update so it appears on top of everything.
//   dashCooldownLeft: seconds until dash is ready (0 = ready)
//   dashCooldownMax:  full cooldown length in seconds
void HUD_Draw(float playerHealth, float playerMaxHealth, int coins,
	float dashCooldownLeft, float dashCooldownMax, float runTime);