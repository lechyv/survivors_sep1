#include <math.h>
#include "Swarm.h"

#define pi 3.14159265f

#define max_enemies 200
#define max_enemy_projectiles 64

// tank projectiles 
#define tank_fire_interval        3.5f     //projectile cd
#define enemy_projectile_speed    250.0f   
#define enemy_projectile_diameter 12.0f
#define enemy_projectile_damage   10.0f
#define enemy_projectile_lifetime 8.0f

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

//Constructing a type Enemy_Stats for differing enemy types.
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
    float w = (float)CP_System_GetWindowWidth();
    float h = (float)CP_System_GetWindowHeight();

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
    float w = (float)CP_System_GetWindowWidth();
    float h = (float)CP_System_GetWindowHeight();

    for (int i = 0; i < max_enemy_projectiles; i++)
    {
        if (projectile_active[i] == 0) continue;

        projectile_pos[i] = CP_Vector_Add(projectile_pos[i], CP_Vector_Scale(projectile_vel[i], dt));
        projectile_age[i] += dt;

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
        if (projectile_active[i] == 0) continue;

        float dx = projectile_pos[i].x - x;
        float dy = projectile_pos[i].y - y;
        if ((dx * dx + dy * dy) <= (touch_distance * touch_distance))
        {
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

        // drone bullets: subtract damage, die at 0 hp (TO BE DONE WHEN WE HAVE HP)
        float damage = bullet_check_if_hit(enemy_pos[i].x, enemy_pos[i].y, enemy_stats[type].size / 2.0f);
        if (damage > 0.0f)
        {
            enemy_health[i] -= damage;
            if (enemy_health[i] <= 0.0f)
            {
                enemy_active[i] = 0;
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

static void Draw_Enemies(void)
{
    for (int i = 0; i < max_enemies; i++)
    {
        if (enemy_active[i] == 1)
        {
            Draw_Enemy(i);
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

    for (int t = 0; t < Enemy_Type_Count; t++) spawn_timer[t] = 0.0f;
    for (int i = 0; i < max_enemies; i++) enemy_active[i] = 0;
    for (int i = 0; i < max_enemy_projectiles; i++) projectile_active[i] = 0;
}

void Swarm_Update(CP_Vector player_pos)
{
    float dt = CP_System_GetDt();

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

    Move_Enemies(player_pos, dt);
    Update_Projectiles(dt);

    Draw_Enemies();
    Draw_Projectiles();
}

int Swarm_GetNearest(float x, float y, float range, CP_Vector* target)
{
    int found = 0;
    float nearest_distance = range;

    for (int i = 0; i < max_enemies; i++)
    {
        if (enemy_active[i] == 0)
        {
            continue;
        }
        float distance = CP_Math_Distance(x, y, enemy_pos[i].x, enemy_pos[i].y);
        if (distance < nearest_distance)
        {
            nearest_distance = distance;
            *target = enemy_pos[i];
            found = 1;
        }
    }
    return found;
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

//counts the number of enemies that touch the player
int Swarm_NumOfEnemiesTouching(float x, float y, float radius)
{
    int count = 0;
    for (int i = 0; i < max_enemies; i++)
    {
        if (enemy_active[i] == 0)
        {
            continue;
        }
        float enemy_radius = enemy_stats[enemy_type[i]].size / 2.0f;
        float combined_radius = radius + enemy_radius;
        float dx = enemy_pos[i].x - x;
        float dy = enemy_pos[i].y - y;
        if ((dx * dx) + (dy * dy) <= (combined_radius * combined_radius))
        {
            count = count + 1;
        }
    }
    return count;
}