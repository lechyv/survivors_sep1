//---------------------------------------------------------
// file:    main.c
// authors: Elvis, Brandon, Wee Jie, Javier
// brief:   Entry point for Survivor, a top-down drone survival game
//          built on DigiPen's CProcessing library.
//
// documentation link:
// https://github.com/DigiPen-Faculty/CProcessing/wiki
//---------------------------------------------------------
#include "cprocessing.h"
#include "mainmenu.h"

int main(void)
{
	// Uncomment to see printf output while debugging
	// CP_System_ShowConsole(TRUE);

	CP_Engine_SetNextGameState(Menu_Init, Menu_Update, Menu_Exit);
	CP_Engine_Run();
	return 0;
}
