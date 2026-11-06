#pragma once
//Diff: player picks easy/medium/hard on main menu
//Easy: 0.75x
//Medium: 1x
//Hard: 1.25x
//UI: Stored in Diff_MenuUpdate()
float Diff_GetMultiplier(void);

//scales damage dealt to player by enemies, tank shots and boss.
//called once per frame in Game_Update()
float Diff_ScaleDamageToPlayer(float damage);

//scales damage dealt by the drones to swarms and boss.
//apply the diff scaling to the damage dealt by the drones to swarms and boss. 
//Called once per frame in Game_Update()
float Diff_ScaleDamageToEnemy(float damage);

//returns diff number swarm_init expects: 1(easy), 2(medium), 3(hard)
//for swarm.c to set spawn rate
int Diff_GetSwarmLevel(void);


//difficulty screen ( a game state): black screen with EASY/NORMAL/HARD buttons.
//START oon the main menu goes here, clicking a button sets the difficulty and starts the game.
//ESC goes back to main menu.
//UI: the screen layout is in Diff_Select_Update in diff.c
void Diff_Select_Init(void);
void Diff_Select_Update(void);
void Diff_Select_Exit(void);
