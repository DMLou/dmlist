#include "Console.h"

/*
Base C_Output class. To make a driver, inherit this class and implement RefreshXY
and ScrollRegion if you have a fast way to scroll (ie, through hardware).
*/


C_Output::C_Output ()
{
	Cons = NULL;
	cHWND = NULL;
	Detached = FALSE;
	HasNewPID = FALSE;
	NewPID = ~0;

	SetVersionString ("Consframe NULL Output Driver");

	SetBeep (0xFFFFFFFF);  // speaker "click"

	OutputMode = C_LINE;
	DefaultOutputMode = C_LINE;
	FeatureColor = TRUE;
	FeatureDirectOut = TRUE;
	FeatureLineOut   = TRUE;
}


C_Output::~C_Output ()
{
	return;
}


DWORD C_Output::GetVersionString (char *Result)
{
	if (Result)
	{
		strcpy (Result, VerString);
	}

	return (strlen(VerString) + 1);
}


void C_Output::SetVersionString (char *VersionStr)
{
	if (strlen(VersionStr) > (sizeof(VerString)+1))
		return;

	strcpy (VerString, VersionStr);
}


// Called by Console::AttachOutput
BOOL C_Output::AttachConsole (Console *C)
{
	if (C == NULL)
		return (FALSE);

	Cons = C;

	return (TRUE);
}


BOOL C_Output::ResizeQuery (int XSize, int YSize)
{
    // sure whatever you say boss.
	return (TRUE);
}

