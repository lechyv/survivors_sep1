#pragma once
/* when a swarm timer runs out, leftover enemies are cleared.
player gets a short break to pick coins up from the ground.
a message shows that swarm is over and counts down to the next swarm.
UI: message is drawn in Wave_Draw in wave.c
func are called by game.c
*/

//how long the break between swarms last in seconds
#define WAVE_BREAK_TIME 5.0f

//cancels any breaks
//called in game_init and when M skips the boss
void Wave_Init(void);

//starts the break after swarm number swarmThatEnded ( 1 or 2).
//called once in Game_End_Swarm in game.c
void Wave_StartBreak(int swarmThatEnded);

//returns 1 while the break is running, else 0
int Wave_IsOnBreak(void);


//Counts the break down. Returns 1 on the frame the break finishes, 0 otherwise.
//Called in Game_Update while the game is running
int Wave_UpdateBreak(void);



//Draws the "SWARM 1 CLEARED!" message and countdown while the break is running.
//Draws nothing otherwise. Called in Game_Update after HUD_Draw, before Shop_Draw
//UI: the whole message layout (text, sizes, positions, colours) is in this function in wave.c
void Wave_Draw(void);