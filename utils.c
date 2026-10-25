//---------------------------------------------------------
// file:    utils.c
// authors: everyone (difficulty table: Elvis, buttons: Wee Jie)
// brief:   Shared math/drawing helpers, difficulty settings and UI buttons
//---------------------------------------------------------
#include "utils.h"

//---------------------------------------------------------
// Math and drawing helpers
//---------------------------------------------------------
float Util_MinF(float a, float b) { return a < b ? a : b; }
float Util_MaxF(float a, float b) { return a > b ? a : b; }

CP_Vector Util_SafeNormalize(CP_Vector v)
{
	if (v.x * v.x + v.y * v.y < 0.000001f)
		return CP_Vector_Zero();
	return CP_Vector_Normalize(v);
}

CP_Vector Util_ClampToWindow(CP_Vector p, float radius)
{
	p.x = CP_Math_ClampFloat(p.x, radius, (float)WINDOW_WIDTH - radius);
	p.y = CP_Math_ClampFloat(p.y, radius, (float)WINDOW_HEIGHT - radius);
	return p;
}

int Util_CirclesOverlap(CP_Vector a, float ra, CP_Vector b, float rb)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float r = ra + rb;
	return dx * dx + dy * dy <= r * r;
}

int Util_IsOutsideWindow(CP_Vector p, float margin)
{
	return p.x < -margin || p.y < -margin ||
		p.x > (float)WINDOW_WIDTH + margin || p.y > (float)WINDOW_HEIGHT + margin;
}

CP_Color Util_Color(int r, int g, int b, int a)
{
	return CP_Color_Create(r, g, b, a);
}

CP_Color Util_WithAlpha(CP_Color c, float alpha01)
{
	alpha01 = CP_Math_ClampFloat(alpha01, 0.0f, 1.0f);
	c.a = (unsigned char)((float)c.a * alpha01);
	return c;
}

void Util_DrawArrow(CP_Vector center, CP_Vector dir, float size, CP_Color color)
{
	CP_Vector d = Util_SafeNormalize(dir);
	if (d.x == 0.0f && d.y == 0.0f)
		d = CP_Vector_Set(1.0f, 0.0f);

	CP_Vector perp = CP_Vector_Set(-d.y, d.x);
	CP_Vector tip = CP_Vector_Add(center, CP_Vector_Scale(d, size));
	CP_Vector back = CP_Vector_Add(center, CP_Vector_Scale(d, -size * 0.7f));
	CP_Vector notch = CP_Vector_Add(center, CP_Vector_Scale(d, -size * 0.25f));
	CP_Vector left = CP_Vector_Add(back, CP_Vector_Scale(perp, size * 0.75f));
	CP_Vector right = CP_Vector_Add(back, CP_Vector_Scale(perp, -size * 0.75f));

	CP_Settings_NoStroke();
	CP_Settings_Fill(color);
	CP_Graphics_DrawTriangle(tip.x, tip.y, left.x, left.y, notch.x, notch.y);
	CP_Graphics_DrawTriangle(tip.x, tip.y, notch.x, notch.y, right.x, right.y);
}

//---------------------------------------------------------
// Pixel font text
// All text is drawn as small rectangles from a built-in 5x7 font,
// so it works with every CProcessing version (some course builds
// don't render CP_Font_DrawText). Lowercase is drawn as uppercase.
//---------------------------------------------------------
static const unsigned char FONT_5X7[64][7] =
{
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ' '
	{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 },   // '!'
	{ 0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00 },   // '"'
	{ 0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A },   // '#'
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   // '$'
	{ 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03 },   // '%'
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   // '&'
	{ 0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 },   // "'"
	{ 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 },   // '('
	{ 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 },   // ')'
	{ 0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00 },   // '*'
	{ 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 },   // '+'
	{ 0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08 },   // ','
	{ 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 },   // '-'
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C },   // '.'
	{ 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00 },   // '/'
	{ 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E },   // '0'
	{ 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },   // '1'
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },   // '2'
	{ 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E },   // '3'
	{ 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },   // '4'
	{ 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },   // '5'
	{ 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E },   // '6'
	{ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },   // '7'
	{ 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },   // '8'
	{ 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C },   // '9'
	{ 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 },   // ':'
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   // ';'
	{ 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 },   // '<'
	{ 0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00 },   // '='
	{ 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 },   // '>'
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   // '?'
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   // '@'
	{ 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 },   // 'A'
	{ 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E },   // 'B'
	{ 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E },   // 'C'
	{ 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E },   // 'D'
	{ 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F },   // 'E'
	{ 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 },   // 'F'
	{ 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F },   // 'G'
	{ 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 },   // 'H'
	{ 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E },   // 'I'
	{ 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C },   // 'J'
	{ 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 },   // 'K'
	{ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F },   // 'L'
	{ 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 },   // 'M'
	{ 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 },   // 'N'
	{ 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E },   // 'O'
	{ 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 },   // 'P'
	{ 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D },   // 'Q'
	{ 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 },   // 'R'
	{ 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E },   // 'S'
	{ 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },   // 'T'
	{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E },   // 'U'
	{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 },   // 'V'
	{ 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A },   // 'W'
	{ 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 },   // 'X'
	{ 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 },   // 'Y'
	{ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F },   // 'Z'
	{ 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E },   // '['
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   // 'backslash'
	{ 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E },   // ']'
	{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   // '^'
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F },   // '_'
};

typedef enum TextAlign { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } TextAlign;

static void DrawPixelText(const char* text, float x, float y, float size, CP_Color color, TextAlign align)
{
	// One font pixel is about a tenth of the requested size, rounded so
	// edges stay sharp. A glyph is 5x7 pixels with 1 pixel of spacing.
	float p = (float)(int)(size / 10.0f + 0.5f);
	if (p < 1.0f)
		p = 1.0f;

	int length = 0;
	while (text[length] != '\0')
		length++;
	if (length == 0)
		return;

	float width = (float)length * 6.0f * p - p;
	float left = align == ALIGN_CENTER ? x - width * 0.5f : align == ALIGN_RIGHT ? x - width : x;
	float top = y - 3.5f * p;   // y is the vertical middle of the text
	left = (float)(int)left;
	top = (float)(int)top;

	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_NoStroke();
	CP_Settings_Fill(color);

	for (int i = 0; i < length; ++i)
	{
		int c = (unsigned char)text[i];
		if (c >= 'a' && c <= 'z')
			c -= 'a' - 'A';
		if (c < 32 || c > 95)
			c = '?';
		const unsigned char* rows = FONT_5X7[c - 32];
		float gx = left + (float)i * 6.0f * p;

		for (int row = 0; row < 7; ++row)
		{
			// Draw each horizontal run of lit pixels as one rectangle
			int col = 0;
			while (col < 5)
			{
				if (!(rows[row] & (0x10 >> col)))
				{
					col++;
					continue;
				}
				int runStart = col;
				while (col < 5 && (rows[row] & (0x10 >> col)))
					col++;
				CP_Graphics_DrawRect(gx + (float)runStart * p, top + (float)row * p,
					(float)(col - runStart) * p, p);
			}
		}
	}
}

void Util_DrawTextCentered(const char* text, float x, float y, float size, CP_Color color)
{
	DrawPixelText(text, x, y, size, color, ALIGN_CENTER);
}

void Util_DrawTextLeft(const char* text, float x, float y, float size, CP_Color color)
{
	DrawPixelText(text, x, y, size, color, ALIGN_LEFT);
}

void Util_DrawTextRight(const char* text, float x, float y, float size, CP_Color color)
{
	DrawPixelText(text, x, y, size, color, ALIGN_RIGHT);
}

//---------------------------------------------------------
// Difficulty (Elvis) - Week 12 balancing happens in this table
//---------------------------------------------------------
static const DifficultySettings DIFFICULTY_TABLE[DIFFICULTY_COUNT] =
{
	//  name      hp     speed  count  spawn  coins  boss   touch
	{ "EASY",   0.75f, 0.85f, 0.75f, 1.25f, 1.25f, 0.75f, 0.75f },
	{ "NORMAL", 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ "HARD",   1.40f, 1.20f, 1.40f, 0.75f, 0.85f, 1.40f, 1.30f },
};

static DifficultyLevel currentLevel = DIFFICULTY_NORMAL;

void Difficulty_Set(DifficultyLevel level)
{
	if (level >= DIFFICULTY_EASY && level < DIFFICULTY_COUNT)
		currentLevel = level;
}

DifficultyLevel Difficulty_Get(void)
{
	return currentLevel;
}

const DifficultySettings* Difficulty_Settings(void)
{
	return &DIFFICULTY_TABLE[currentLevel];
}

const char* Difficulty_Name(DifficultyLevel level)
{
	if (level >= DIFFICULTY_EASY && level < DIFFICULTY_COUNT)
		return DIFFICULTY_TABLE[level].name;
	return "?";
}

//---------------------------------------------------------
// Shared UI (Wee Jie)
//---------------------------------------------------------
int UI_Button(float cx, float cy, float w, float h, const char* label, int enabled, int highlighted)
{
	float x = cx - w * 0.5f;
	float y = cy - h * 0.5f;
	float mx = CP_Input_GetMouseX();
	float my = CP_Input_GetMouseY();
	int hover = enabled && mx >= x && mx <= x + w && my >= y && my <= y + h;
	CP_Color white = Util_Color(255, 255, 255, 255);

	CP_Color bg = !enabled ? Util_Color(70, 60, 90, 255)
		: hover ? Util_Color(150, 110, 230, 255)
		: Util_Color(110, 70, 200, 255);

	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_Fill(bg);
	CP_Settings_Stroke(highlighted ? Util_Color(255, 230, 120, 255) : white);
	CP_Settings_StrokeWeight(highlighted ? 4.0f : 2.0f);
	CP_Graphics_DrawRectAdvanced(x, y, w, h, 0.0f, h * 0.3f);

	Util_DrawTextCentered(label, cx, cy, 22.0f, enabled ? white : Util_Color(150, 140, 170, 255));
	return hover && CP_Input_MouseTriggered(MOUSE_BUTTON_LEFT);
}

void UI_DrawDimOverlay(void)
{
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Settings_NoStroke();
	CP_Settings_Fill(Util_Color(0, 0, 0, 160));
	CP_Graphics_DrawRect(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
}
