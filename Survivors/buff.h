#pragma once
/*
passive damage buff: every 10 enemies the player kills, all drone damage goes up by 10%.
each buff is 10% of the CURRENT damage ex(100 -> 110 -> 121 -> 133.1)
Kills and buffs carry over from swarm 1 to swarm 2 and the boss fight.
They only reset when a new game starts.
*/

//kills needed for each buff
#define BUFF_KILLS_PER_BUFF 10

//each buff multiplies damage by this (1.10 = +10% of the current damage)
#define BUFF_DAMAGE_STEP 1.10f

//reset kills to 0 and damage back to normal (x1.0)
//called once in Game_Init
void Buff_Init(void);

//counts one kill. every BUFF_KILLS_PER_BUFF kills, damage goes up by 10%
//called in swarm.c wherver an enemy dies (next to Coins_drop)
void Buff_AddKill(void);

//returns damage with the buff applied (dmg x current multiplier)
//called in game.c when a bullet is fired and when a rocket explodes
float Buff_ScaleDamage(float damage);

//returns the current multiplier: 1.0 at the start, 1.1 after 10 kills, 1.21 after 20...
float Buff_GetMultiplier(void);

// Draws "DMG +21% NEXT BUFF: 7 kills" in the bottom left-corner.
//caleed in Game_update after HUD_Draw
//UI: text, size, pos and colour are in the function stored in buff.c
void Buff_Draw(void);