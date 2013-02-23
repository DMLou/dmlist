/*****************************************************************************

  C_InputKbd

  Provides input services from the windows message loop.

*****************************************************************************/

#ifndef CONSOLE_INPUT_KEYBOARD_H
#define CONSOLE_INPUT_KEYBOARD_H

#include "C_Linkage.h"
#include "Console.h"

class CAPILINK C_InputKbd : public C_Input
{
public:
	C_InputKbd ();
	~C_InputKbd ();

	// Windows message based input
	virtual BOOL WndProc(UINT message, WPARAM wParam, LPARAM lParam);

protected:

};


#endif