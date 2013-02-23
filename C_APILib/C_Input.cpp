#include "Console.h"


C_Input::C_Input ()
{
	Cons = NULL;
	cHWND = NULL;
	Detached = FALSE;
	HasNewPID = FALSE;
	NewPID = ~0;

	InitShObject (&UpdateInp, FALSE);
}


C_Input::~C_Input ()
{
	DestroyShObject (&UpdateInp);
}


BOOL C_Input::AttachConsole (Console *C)
{
	if (C == NULL)
		return (FALSE);

	Cons = C;
	return (TRUE);
}
