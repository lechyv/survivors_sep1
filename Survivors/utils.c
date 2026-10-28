

#include <math.h>
#include "cprocessing.h"
#include "utils.h"

int IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y)
{
    float half_w = area_width / 2.0f;
    float half_h = area_height / 2.0f;

    if (CP_Input_MouseClicked() &&
        click_x >= area_center_x - half_w &&
        click_x <= area_center_x + half_w &&
        click_y >= area_center_y - half_h &&
        click_y <= area_center_y + half_h)
    {
        return 1;
    }
    else {
        return 0;
    }
}


int IsCircleClicked(float circle_center_x, float circle_center_y, float diameter, float click_x, float click_y)
{
    float radius = diameter / 2.0f;
    float distX = click_x - circle_center_x;
    float distY = click_y - circle_center_y;


    if (distX * distX + distY * distY <= radius * radius) 
    {
        return 1;
    }
    return 0;


}

int IsAreaHovered(float area_center_x, float area_center_y, float area_width, float area_height)
{
    float mouse_x = CP_Input_GetMouseX();
    float mouse_y = CP_Input_GetMouseY();
    float half_w = area_width / 2.0f;
    float half_h = area_height / 2.0f;

    if (mouse_x >= area_center_x - half_w &&
        mouse_x <= area_center_x + half_w &&
        mouse_y >= area_center_y - half_h &&
        mouse_y <= area_center_y + half_h)
    {
        return 1;
    }
    else {
        return 0;
    }
}
