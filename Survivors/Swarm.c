#include <math.h>
#include "Swarm.h"
#include "coin-drop.h" //coin-drop function
#include <stddef.h>
#include "game.h" // for world width and height (random spawn locations)ddddd
#include "diff.h"
#include "buff.h"

#define pi 3.14159265f

#define max_enemies 200
#define max_enemy_projectiles 64

// tank projectiles 
#define tank_fire_interval        3.5f     //projectile 
#define enemy_projectile_speed    250.0f   
#define enemy_projectile_diameter 12.0f
#define enemy_projectile_damage   10.0f
#define enemy_projectile_lifetime 8.0f
#define health_bar_height 4.0f   
#define health_bar_gap    6.0f

/*enum associates each number element/index with a name.
This gets updated when a new enemy type is introduced e.g. BOSS.
Enemy_Type_Count stays at the end to "total"-up the enemy types.*/
typedef enum
{
    Enemy_Base, //0
    Enemy_Fast, //1
    Enemy_Tank, //2
    Enemy_Type_Count //3 (total enemy count Base + Fast + Tank)
} Enemy_Type;

//Constructing a type Enemy_Stats for differing enemy types stats. All enemies have these common stats throughout.
typedef struct
{
    float speed;             
    float size;              
    float max_health;        //drone bullet does 10hp
    float spawn_multiplier;  // spawn interval 
} Enemy_Stats; //each type Enemy_Stats has these parameters.

//declaring an array named enemy_stats with a type of Enemy_Stats
static const Enemy_Stats enemy_stats[Enemy_Type_Count] = 
{
   // (speed, size,  hp,  spawn_multiplier) 
    { 120.0f, 24.0f, 30.0f, 1.0f },   //base
    { 220.0f, 18.0f, 10.0f, 3.0f },   //fast
    {  70.0f, 36.0f, 90.0f, 8.0f }    //tank
};

//Characteristics of each of the 200 enemies in the array
static CP_Vector enemy_pos[max_enemies]; //x,y coordiantes
static int   enemy_active[max_enemies];//dead or alive or has yet to spawn
static int   enemy_type[max_enemies]; //base?fast?tank?
static float enemy_health[max_enemies];
static float enemy_angle[max_enemies];       //rotation face for fast triangle enemy types
static float enemy_fire_timer[max_enemies];  //shows how long left till the enemy can fire again (uses #define tank_fire_interval        3.5f to find out)

//Projectile characteristics

//Coordinate of the projectile
static CP_Vector projectile_pos[max_enemy_projectiles];
//How many pixels it travels in a straight line from the initial x,y coordinate it was fired from
static CP_Vector projectile_vel[max_enemy_projectiles];
//How long till the projectile fizzles out? Uses #define enemy_projectile_lifetime 8.0f to help.
static float projectile_age[max_enemy_projectiles];
//does this position array currently holds a "live" projectile? Contributes to MAX projectiles on screen.
static int   projectile_active[max_enemy_projectiles]; 

/*THE SINGLE MOST IMPORTNAT PLACEHOLDER VARIABLE
Sets the current stage to 1.
Condition Stage 1 only has base enemies.
Condition Stage 2 has fast and tanks (Maybe stage 3 erases prev enemies to spwan only boss)
Swarm_Init overwrites this however*/
static int   current_stage = 1;
int swarm_auto_spawn = 1;
//how many seconds to wait between spawns of a certain type
static float spawn_interval[Enemy_Type_Count];
/*how long it has been since an enemy of a certain type last spawned. Each frame dt is added to it. 
When it reaches spawn_interval[Enemy_Type_Count], an enemy of that type spawns and the timer resets to 0.*/
static float spawn_timer[Enemy_Type_Count];

float bullet_check_if_hit(float x, float y, float radius);

/* ------------------------------------------------------------------ */
/* Spawn intervals                                                     */
/* ------------------------------------------------------------------ */

/* Sets every type's spawn interval from the base enemy's interval.
   Change a type's spawn_multiplier above to make it rarer or more common.
   Set_Spawn_Interval uses argument from Set_Difficulty for the entire game*/
static void Set_Spawn_Intervals(float base_interval)
{
    for (int t = 0; t < Enemy_Type_Count; t++)
    {
        spawn_interval[t] = base_interval * enemy_stats[t].spawn_multiplier; //this goes to Swarm_Update
    }
}

/*Set_Difficulty takes from Swarm_Init an int value of difficulty, and
feeds an argument into Set_Spawn_Interval depending on difficulty*/
static void Set_Difficulty(int difficulty)
{
    switch (difficulty)
    {//number next to Set_Spawn-Interval is the base_interval
    case 1:  Set_Spawn_Intervals(1.5f); break;   //easy
    case 2:  Set_Spawn_Intervals(0.8f); break;   //medium (could change to normal as well)
    case 3:  Set_Spawn_Intervals(0.3f); break;   //hard
    default: Set_Spawn_Intervals(1.0f); break;   //normal
    }
}

// Which types are allowed to spawn in the current stage 
static int Type_Is_Enabled(int type)
{
    if (type == Enemy_Base)
    {
        return 1; //base enemy only
    }
    return current_stage >= 2;   //fast and tank enemies onnly spawn at stage 2
}

/* ------------------------------------------------------------------ 
   Spawning                                                            
   ------------------------------------------------------------------ */

//Creates randomized enemy spawn locations at the edge of the map/screen
static CP_Vector Get_Spawn_Location(void)
{
    float w = world_width;
    float h = world_height;

    switch (CP_Random_RangeInt(0, 3))
    {
    case 0:  return CP_Vector_Set(CP_Random_RangeFloat(0, w), 0);   // top    x,y coordinate, 0 cause edge of screen
    case 1:  return CP_Vector_Set(CP_Random_RangeFloat(0, w), h);   // bottom comparing width
    case 2:  return CP_Vector_Set(0, CP_Random_RangeFloat(0, h));   // left   
    default: return CP_Vector_Set(w, CP_Random_RangeFloat(0, h));   // right  
    }
}

static void Spawn_Enemy(int type)
{
    for (int i = 0; i < max_enemies; i++)
    {
        if (enemy_active[i] == 0)
        {
            enemy_pos[i] = Get_Spawn_Location();
            enemy_active[i] = 1;
            enemy_type[i] = type;
            enemy_health[i] = enemy_stats[type].max_health;
            enemy_angle[i] = 0.0f;
            enemy_fire_timer[i] = tank_fire_interval;
            return;
        }
    }
}

//Spawn enemies for phase 2 of boss fight - Brandon

//Function to spawn a basic enemy at a given position, used by the boss's phase 2 summon - Brandon
void Swarm_SpawnBasic(float x, float y)
{
    for (int i = 0; i < max_enemies; i++)   //For loop to loop through all enemy indexes to find an inactive enemy
    {
        if (enemy_active[i] == 0)   //If statement to check if the enemy index is available
        {
            enemy_pos[i] = CP_Vector_Set(x, y); //Sets the enemy's starting position to the given coordinates
            enemy_active[i] = 1;    //Sets the enemy to active
            enemy_type[i] = Enemy_Base; //Sets the enemy's type to a basic enemy
            enemy_health[i] = enemy_stats[Enemy_Base].max_health;   //Sets the enemy's health to the basic enemy's maximum health
            enemy_angle[i] = 0.0f;  //Resets the enemy's angle to 0
            return; //Exits the function after spawning one enemy

        }
    }
}

//Function to spawn a tank enemy at a given position, used by the boss's phase 2 summon - Brandon
void Swarm_SpawnTank(float x, float y)
{
    for (int i = 0; i < max_enemies; i++)   //For loop to loop through all enemy indexes to find an inactive enemy
    {
        if (enemy_active[i] == 0)   //If statement to check if the enemy index is available
        {
            enemy_pos[i] = CP_Vector_Set(x, y); //Sets the enemy's starting position to the given coordinates
            enemy_active[i] = 1;    //Sets the enemy to active
            enemy_type[i] = Enemy_Tank; //Sets the enemy's type to a tank enemy
            enemy_health[i] = enemy_stats[Enemy_Tank].max_health;   //Sets the enemy's health to the tank enemy's maximum health
            enemy_angle[i] = 0.0f;  //Resets the enemy's angle to 0
            enemy_fire_timer[i] = tank_fire_interval;   //Sets the time until the tank can fire its first shot
            return; //Exits the function after spawning one enemy

        }
    }
}


/* ------------------------------------------------------------------ 
   Tank projectiles                                                    
   ------------------------------------------------------------------ */

/* Fires one slow projectile toward where the player is right now AT THIS CURRENT INSTANCE */
static void Fire_Enemy_Projectile(CP_Vector from, CP_Vector target)
{
    CP_Vector dir = CP_Vector_Normalize(CP_Vector_Subtract(target, from));

    for (int i = 0; i < max_enemy_projectiles; i++)
    {
        if (projectile_active[i] == 0)
        {
            projectile_active[i] = 1;
            projectile_pos[i] = from;
            projectile_vel[i] = CP_Vector_Scale(dir, enemy_projectile_speed);
            projectile_age[i] = 0.0f;
            return;
        }
    }
}

static void Update_Projectiles(float dt)
{
    float w = world_width;
    float h = world_height;

    for (int i = 0; i < max_enemy_projectiles; i++)
    {
        //checks if projectile slot is not in use or unalive. (Only "alive" projectiles needs updating below)
        if (projectile_active[i] == 0) continue; 

        /*CP_Vector_Scale gives the projectile velocity multiplied by the current system time to get the 
        total distance travelled in this frame.
        Projectile_pos is the current projectile position. 
        CP_Vector_Add adds together the updated distance travelled by CP_Vector_Scale to the current position
        to give the current new projectile position*/
        projectile_pos[i] = CP_Vector_Add(projectile_pos[i], CP_Vector_Scale(projectile_vel[i], dt));

        //projectile_age[i] += dt; tracks the "life" of the projectile; how long left till it fades or disappears.
        projectile_age[i] += dt;

        //Unalives the projectile when it has reached the end of its life cycle, or reaches somewhere beyond the edge of screen.
        if (projectile_age[i] > enemy_projectile_lifetime ||
            projectile_pos[i].x < -20.0f || projectile_pos[i].y < -20.0f ||
            projectile_pos[i].x > w + 20.0f || projectile_pos[i].y > h + 20.0f)
        {
            projectile_active[i] = 0;
        }
    }
}

float Swarm_ProjectilesHitPlayer(float x, float y, float radius)
{
    float touch_distance = radius + enemy_projectile_diameter / 2.0f;
    float damage = 0.0f;

    for (int i = 0; i < max_enemy_projectiles; i++)
    {
        //Again, checks for all "alive" projectiles only.
        if (projectile_active[i] == 0) continue;

        float dx = projectile_pos[i].x - x;
        float dy = projectile_pos[i].y - y;
        if ((dx * dx + dy * dy) <= (touch_distance * touch_distance))
        {
            //projectile is assigned "unalive" state
            projectile_active[i] = 0;
            damage += enemy_projectile_damage;
        }
    }
    return damage;
}

/* ------------------------------------------------------------------ 
   Behaviour of enemies                                                         
   ------------------------------------------------------------------ */

/* Every enemy: take bullet damage, then chase the player at its own speed.
   Tanks also shoot projectiles. */
static void Move_Enemies(CP_Vector player_pos, float dt)
{
    for (int i = 0; i < max_enemies; i++)
    {
        if (enemy_active[i] == 0) continue;

        int type = enemy_type[i];

        // drone bullets: subtract damage, die at 0 hp 
        float damage = bullet_check_if_hit(enemy_pos[i].x, enemy_pos[i].y, enemy_stats[type].size / 2.0f);
        if (damage > 0.0f)
        {
            damage = Diff_ScaleDamageToEnemy(damage);//difficulty: easy = less hp, hard = more hp
            enemy_health[i] -= damage;
            if (enemy_health[i] <= 0.0f)
            {
				Coins_Drop(enemy_pos[i].x, enemy_pos[i].y); //coin-drop, drop a coin where the enemy died)
                Buff_AddKill();//count kill, add buff respectively
                enemy_active[i] = 0; //enemy is dead
                continue;
            }
        }

        //chase 
        CP_Vector dir = CP_Vector_Normalize(CP_Vector_Subtract(player_pos, enemy_pos[i]));
        enemy_pos[i] = CP_Vector_Add(enemy_pos[i], CP_Vector_Scale(dir, enemy_stats[type].speed * dt));
        enemy_angle[i] = atan2f(dir.y, dir.x) * 180.0f / pi;

        // tank only: occasionally shoot at the player's current location 
        if (type == Enemy_Tank)
        {
            enemy_fire_timer[i] -= dt;
            if (enemy_fire_timer[i] <= 0.0f)
            {
                Fire_Enemy_Projectile(enemy_pos[i], player_pos);
                enemy_fire_timer[i] = tank_fire_interval;
            }
        }
    }
}

/* ------------------------------------------------------------------ 
   Spawning Enemies through Draw function (could be pngs later)                                                            
   ------------------------------------------------------------------ */

static void Draw_Enemy(int i)
{
    float x = enemy_pos[i].x;
    float y = enemy_pos[i].y;
    float s = enemy_stats[enemy_type[i]].size;

    switch (enemy_type[i])
    {
    case Enemy_Fast:   // pink triangle pointing at the player 
        CP_Settings_NoStroke();
        CP_Settings_Fill(CP_Color_Create(255, 105, 180, 255));
        CP_Graphics_DrawTriangleAdvanced(x + s * 0.8f, y,
            x - s * 0.5f, y - s * 0.5f,
            x - s * 0.5f, y + s * 0.5f,
            enemy_angle[i]);
        break;

    case Enemy_Tank:   // maroon square with a light outline so it stays visible on black 
        CP_Settings_Stroke(CP_Color_Create(200, 120, 120, 255));
        CP_Settings_StrokeWeight(2.0f);
        CP_Settings_Fill(CP_Color_Create(128, 0, 0, 255));
        CP_Settings_RectMode(CP_POSITION_CENTER);
        CP_Graphics_DrawRect(x, y, s, s);
        CP_Settings_NoStroke();
        break;

    default:           // base: purple circle 
        CP_Settings_NoStroke();
        CP_Settings_Fill(CP_Color_Create(160, 60, 220, 255));
        CP_Graphics_DrawCircle(x, y, s);
        break;
    }
}

// Draws a small health bar just above enemy i
static void Draw_Enemy_Health_Bar(int i)
{
    float x = enemy_pos[i].x;
    float y = enemy_pos[i].y;
    float s = enemy_stats[enemy_type[i]].size;

    // how much health is left: 1.0 = full, 0.5 = half, 0.0 = empty
    float percent = enemy_health[i] / enemy_stats[enemy_type[i]].max_health;
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 1.0f) percent = 1.0f;

    float bar_width = s;                                                 // same width as the enemy
    float bar_left = x - bar_width / 2.0f;                               // centred on the enemy
    float bar_top = y - s / 2.0f - health_bar_gap - health_bar_height;   // above its "head"

    CP_Settings_NoStroke();
    CP_Settings_RectMode(CP_POSITION_CORNER);

    // dark background (missing health)
    CP_Settings_Fill(CP_Color_Create(40, 40, 40, 255));
    CP_Graphics_DrawRect(bar_left, bar_top, bar_width, health_bar_height);

    // green fill (health left)
    CP_Settings_Fill(CP_Color_Create(80, 220, 100, 255));
    CP_Graphics_DrawRect(bar_left, bar_top, bar_width * percent, health_bar_height);
}


static void Draw_Enemies(void)
{
    for (int i = 0; i < max_enemies; i++)
    {
        if (enemy_active[i] == 1)
        {
            Draw_Enemy(i);
            Draw_Enemy_Health_Bar(i); 
        }
    }
}

static void Draw_Projectiles(void)
{
    CP_Settings_NoStroke();
    CP_Settings_Fill(CP_Color_Create(255, 90, 40, 255));
    for (int i = 0; i < max_enemy_projectiles; i++)
    {
        if (projectile_active[i] == 1)
        {
            CP_Graphics_DrawCircle(projectile_pos[i].x, projectile_pos[i].y, enemy_projectile_diameter);
        }
    }
}

/* ------------------------------------------------------------------ 
   Swarm.C's main loop
   ------------------------------------------------------------------ */

void Swarm_Init(int difficulty, int stage)
{
    CP_Random_Seed(12345);   /* fixed seed: change the number for a different pattern */
    current_stage = stage;
    Set_Difficulty(difficulty);

    swarm_auto_spawn = 1;

    for (int t = 0; t < Enemy_Type_Count; t++) spawn_timer[t] = 0.0f;
    for (int i = 0; i < max_enemies; i++) enemy_active[i] = 0;
    for (int i = 0; i < max_enemy_projectiles; i++) projectile_active[i] = 0;
}

void Swarm_Update(CP_Vector player_pos)
{
    float dt = CP_System_GetDt();
    if (swarm_auto_spawn == 1)
    {
        /* each type has its own timer, so rarer types just have a longer interval */
        for (int t = 0; t < Enemy_Type_Count; t++)
        {
            if (!Type_Is_Enabled(t)) continue;

            spawn_timer[t] += dt;
            if (spawn_timer[t] >= spawn_interval[t])
            {
                Spawn_Enemy(t);
                spawn_timer[t] = 0.0f;
            }
        }
    }
    Move_Enemies(player_pos, dt);
    Update_Projectiles(dt);
}

void Swarm_Draw(void)
{
    Draw_Enemies();
    Draw_Projectiles();
}

//Function to find the nearest active enemy within range and give its position as the target - Brandon
int Swarm_GetNearest(float x, float y, float range, CP_Vector* target)
{
    if (target == NULL || range < 0.0f) //If statement to check if no target variable was given or the range is negative
    {
        return 0;   //Returns 0 as there is nothing valid to search for
    }

    int found = 0;  //Checks if an enemy has been found (1) or not (0)
    float nearest_distance_squared = range * range; //Starts as the squared range, so only enemies within range can be picked

    for (int i = 0; i < max_enemies; i++)   //For loop to loop through all enemy indexes
    {
        if (enemy_active[i] == 0)   //If statement to check if the enemy is inactive
        {
            continue;   //Skips the inactive enemy and moves on to the next index
        }
        
        float dx = enemy_pos[i].x - x;  //Horizontal distance between the enemy and the shooter
        float dy = enemy_pos[i].y - y;  //Vertical distance between the enemy and the shooter


        float distance_squared = (dx * dx) + (dy * dy); //Squared distance between the enemy and the shooter

        if (distance_squared < nearest_distance_squared)    //If statement to check if this enemy is closer than the nearest one found so far
        {
            nearest_distance_squared = distance_squared;    //Updates the nearest distance to this enemy's distance
            *target = enemy_pos[i]; //Stores this enemy's position in the target variable
            found = 1;  //Sets found to 1 as an enemy is within range
        }
    }
    return found;   //Returns 1 if an enemy was found within range, or 0 if there are none
}

int Swarm_KillEnemiesTouching(float x, float y, float radius, float* killed_x, float* killed_y, int max_killed)
{
    int killed = 0;
    int i = 0;

    while (i < max_enemies)
    {
        if (enemy_active[i] == 1 && killed < max_killed)
        {
            /* size is a diameter, so half of it is this enemy's radius */
            float touch_distance = radius + enemy_stats[enemy_type[i]].size / 2.0f;
            float dx = enemy_pos[i].x - x;
            float dy = enemy_pos[i].y - y;
            if ((dx * dx + dy * dy) <= (touch_distance * touch_distance))
            {
                enemy_active[i] = 0; /* enemy is dead */
                killed_x[killed] = enemy_pos[i].x; /* remember where it died*/
                killed_y[killed] = enemy_pos[i].y;
                killed += 1;
            }
        }
        i += 1;
    }
    return killed;
}

//Function to count how many active enemies are touching the player - Brandon
int Swarm_NumOfEnemiesTouching(float x, float y, float radius)
{
    int count = 0;  //Total number of enemies touching the player
    for (int i = 0; i < max_enemies; i++)   //For loop to loop through all enemy indexes
    {
        if (enemy_active[i] == 0)   //If statement to check if the enemy is inactive
        {
            continue;   //Skips the inactive enemy and moves on to the next index
        }
        float enemy_radius = enemy_stats[enemy_type[i]].size / 2.0f;    //Radius of this enemy, based on its type
        float combined_radius = radius + enemy_radius;  //Combined radius of the enemy and the player
        float dx = enemy_pos[i].x - x;  //Horizontal distance between the enemy and the player
        float dy = enemy_pos[i].y - y;  //Vertical distance between the enemy and the player
        if ((dx * dx) + (dy * dy) <= (combined_radius * combined_radius))   //If statement to check if the enemy and the player are touching
        {
            count = count + 1;  //Adds this enemy to the total
        }
    }
    return count;   //Returns the number of enemies touching the player
}

//Function for AOE Damage of rocket drone
int Swarm_AOE_Damage(float x, float y, float radius, float damage)
{
    int killed = 0; //Total number of enemies killed by this explosion
    damage = Diff_ScaleDamageToEnemy(damage); //diff, scaled once, before the loop
    for (int i = 0; i < max_enemies; i++)   //For loop to loop through all enemy indexes
    {
        if (enemy_active[i] == 0)   //If statement to check if the enemy is inactive
        {
            continue;   //Skips the inactive enemy and moves on to the next index
        }

        float dx = enemy_pos[i].x - x;  //Horizontal distance between the enemy and the explosion
        float dy = enemy_pos[i].y - y;  //Vertical distance between the enemy and the explosion
        float max_hit_distance = radius + enemy_stats[enemy_type[i]].size / 2.0f;   //Furthest distance at which the explosion still reaches this enemy's edge

        if (((dx * dx) + (dy * dy)) <= (max_hit_distance * max_hit_distance))   //If statement to check if the enemy is within the explosion
        {
            enemy_health[i] = enemy_health[i] - damage; //Reduces the enemy's health by the damage received


            if (enemy_health[i] <= 0.0f)    //If statement to check if the enemy's health has reached 0
            {
                Coins_Drop(enemy_pos[i].x, enemy_pos[i].y);
                Buff_AddKill(); //buff: count the kill, every 10 kills = +10% damage
                enemy_active[i] = 0;
                killed++;   //Adds this enemy to the kill count
            }   
        }
    }
    return killed;  //Returns the number of enemies killed
}