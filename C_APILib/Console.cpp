/*
	Main console implementation
*/

#include "Console.h"

#ifdef NDEBUG
#ifndef CONSOLE_CLASS_MACROS
#define CONSOLE_CLASS_MACROS
#endif
#endif

/*
// This makes a good performance increase, however the behavior of these
// is changed (from a function to a macro), so I allow turning them off
// You could call this "customized super-inlining"
// Also take note that with these macros, the Peek() 'functions' operate
// exactly the same as the Poke() family.
#ifndef CONSOLE_CLASS_MACROS
#pragma message ("NOT using class function macros for Console class library")
#endif

#ifdef CONSOLE_CLASS_MACROS
#pragma message ("Using class function macros for Console class library")
#include "C_Macros.h"
#endif CONSOLE_CLASS_MACROS
*/


#define MEMPTR ConsMemHeap
#include "C_Thunks.h"


Console::Console (HWND hWnd, int Width, int Height)
{
	cHWND = hWnd;
	Cout = NULL;
	Cin = NULL;
    PipeIn = NULL;
    PipeOut = NULL;
    PipeEcho = false;
    LineHistoryMax = 10;

	InitShObject (&ConsoleLock, FALSE);

	// create our private memory heap first thing. well, second thing.
	ConsMemHeap = new C_Memory (0);

	// Default options
    RTInsert = true;
    CurrentFlag = 0;
	ConsoleText = NULL;
	ConsoleXSize = 0;
	ConsoleYSize = 0;

	WinX1 = 0; WinX2 = 0;
	WinY1 = 0; WinY2 = 0;

	CurrentXPos = 0;
	CurrentYPos = 0;

	ScrollCount = 0;

	// Idle stage defaults
    // dammit we should read these from configuration settings!
	Stage1Idle = 20;
	Stage2Idle = 200;
	Stage2IdleDelay = 10000; // 10 seconds

	ScrollIdle = 10;
	ScrollIdleFreq = 5; // every 5 iterations, Sleep (10);

	SetMaxWriteBuffer (16384);  // 16K max for WriteTextf() ... if you need more, go ahead
	SetAutoRefresh (TRUE);
	SetTabSize (8);
	SetCursor (TRUE);

	SetResizeLock      (FALSE);
	SetExpandCtrlChars (FALSE);
	SetStripCtrlChars  (FALSE);
	SetStripHighAscii  (FALSE);

	// standard DOS-ish colors
	SetForeground (RGB (192, 192, 192)); // dos white
	SetBackground (RGB (  0,   0,   0)); // black
    SetUnderline (false);

	// should probably do some sorta error flag
	if (ResizeXY (Width, Height) == FALSE)
    {
		MessageBox (NULL, "Internal error allocating RAM for console text!", "Oh no ...", MB_OK);
        throw ("NO ....");
    }

	// input init
	KeyQ = NULL;
	KeyQBegin = 0;
	KeyQEnd = 0;
	KeyQLength = 0;
	ResizeQ (1024);  // I really hope this is PLENTY of type ahead!

    CTRLExceptions = TRUE;

	LastInputEvent = GetTickCount();

	SetWindow (0, 0, GetMaxConsX(), GetMaxConsY());

    InputSignal = CreateEvent (NULL, TRUE, FALSE, NULL);

    return;
}


Console::~Console ()
{
    DestroyShObject (&ConsoleLock);

	for (int i = 0; i < GetMaxConsY(); i++)
	{
		free (ConsoleText[i]);
		ConsoleText[i] = NULL;
	}

	free (KeyQ);
	KeyQ = NULL;

	free (ConsoleText);
	ConsoleText = NULL;

	// delete memory heap
	delete ConsMemHeap;
}


BOOL Console::AttachOutput (C_Output *CO)
{
	if (CO == NULL)
		return FALSE;


    Cout = CO;
	Cout->SetHWND (cHWND);

	if (Cout->AttachConsole (this) == FALSE)
		return (FALSE);

	RedrawAll ();

	return (TRUE);
}


BOOL Console::AttachInput (C_Input *CI)
{
	if (CI == NULL)
		return FALSE;

	Cin = CI;
	Cin->SetHWND (cHWND);

	if (Cin->AttachConsole (this) == FALSE)
		return (FALSE);

	return (TRUE);
}


C_Output *Console::GetOutput (void)
{
	C_Output *ReturnVal;

	LockConsole();
	ReturnVal = Cout;
	UnlockConsole();

	return (ReturnVal);
}


C_Input  *Console::GetInput  (void)
{
	C_Input *ReturnVal;

	LockConsole();
	ReturnVal = Cin;
	UnlockConsole();

	return (ReturnVal);
}

// returns TRUE if processed, FALSE if not
// Call this in your WndProc, and if the message isn't processed by us (returned TRUE)
// then YOU should process it or call DefWindowProc (), because it doesn't pertain
// to The Console
//#define DBG_WNDPROC
BOOL Console::WndProc (UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL ReturnVal = FALSE;

	// else ... hand messages down to input and output

	if (Cout != NULL)
		if (Cout->WndProc (message, wParam, lParam))
			ReturnVal = TRUE;

	if (Cin != NULL)
		if (Cin->WndProc (message, wParam, lParam))
			ReturnVal = TRUE;
	
	return (ReturnVal);
}



// NewXSize and NewYSize are cardinal (start @ 1)
// ie, "NewYSize" of 150 creates ConsoleText[0..149]
// use this to resize the console to a new X and Y size
//
// if expanding the size, data inside the new region will be init'd to have
//         spaces and the default text formatting
// if shrinking, data outside the new region will be discarded, forever lost
// return value is success or failure of reallocating (or initially allocating)
// memory.
BOOL Console::ResizeXY  (int NewXSize, int NewYSize, bool DoClearScr)
{
	int x = 0;
	int y = 0;

	if (GetResizeLock() == TRUE)  
		return (FALSE);

    // First off, max width for anyone is 132. Minimum is 10.
    // naw forget about the max of 132
    if (NewXSize < 10)//  ||  NewXSize > 132)
        return (FALSE);

    // Second off, max height for anyone is 60. Minimum is 10.
    // Nah forget about the max of 60
    if (NewYSize < 10)//  ||  NewYSize > 60)
        return (FALSE);

    // Third off, the output driver has to approve it as well.
	if (Cout)
		if (!(Cout->ResizeQuery (NewXSize, NewYSize)) )
			return (FALSE); // if the output driver doesn't like it, then it's a no-go.

    // Lock console during surgery!
	LockConsole();

	// Free the old console
	for (y = 0; y < ConsoleYSize; y++)
	{
		free (ConsoleText[y]);
		ConsoleText[y] = NULL;
	}

	free (ConsoleText);
	ConsoleText = NULL;

	// Allocate the new console
	ConsoleText = (cText **) malloc (NewYSize * sizeof (cText *));

	for (y = 0; y < NewYSize; y++)
	{
		ConsoleText[y] = (cText *) malloc (NewXSize * sizeof (cText));
	}

	// Blank it all out
	for (y = 0; y < NewYSize; y++)
	{	
		for (x = 0; x < NewXSize; x++)
		{
			ConsoleText[y][x].Char    = ' ';
			ConsoleText[y][x].Format  = TextFormat;
			ConsoleText[y][x].Refresh = true;
		}
	}

	// Finally, set the size variables to the correct values
	ConsoleXSize = NewXSize;
	ConsoleYSize = NewYSize;

	WinX1 = 0; 
	WinY1 = 0;
	WinX2 = ConsoleXSize - 1;
	WinY2 = ConsoleYSize - 1;
	CurrentXPos = 0;
	CurrentYPos = 0;

	// since we clear the screen, basically, we might as well ... officially clear the screen :)
	if (Cout && DoClearScr) 
		Cout->ClearScr();

	UnlockConsole();

	if (Cout) 
		Cout->ResizeNotify ();

	return (TRUE);
}


void Console::SetWindow (int X1, int Y1, int X2, int Y2)
{
	FitInConsBounds (&X1, &Y1);
	FitInConsBounds (&X2, &Y2);
	CorrectOrder (&X1, &Y1, &X2, &Y2);

	LockConsole();

	GoToXY (0,0);
	WinX1 = X1;
	WinY1 = Y1;
	WinX2 = X2;
	WinY2 = Y2;

	//if (Cout) RefreshAll ();

	UnlockConsole();
}


// send absolute Console coords
void Console::ClearXY (int X1, int Y1, int X2, int Y2)
{
	int x;
	int y;

	FitInConsBounds (&X1, &Y1);
	FitInConsBounds (&X2, &Y2);
	CorrectOrder (&X1, &Y1, &X2, &Y2);

	LockConsole();

    for (y = Y1; y <= Y2; y++)
	{
	    for (x = X1; x <= X2; x++)
		{
			PokeRefresh (x,y) = true;
			PokeChar (x,y)    = ' ';
			PokeFormat (x,y)  = TextFormat;
		}
	}

	UnlockConsole();

	if (AutoRefresh) Refresh ();
}


// within the current window
void Console::GoToXY (int X, int Y)  
{
	LockConsole();
	
	FitInWinBounds (&X, &Y);

	// set new position
	PokeRefresh (GetAbsXPos(), GetAbsYPos()) = true;
	CurrentXPos = X; 
	CurrentYPos = Y; 
	PokeRefresh (GetAbsXPos(), GetAbsYPos()) = true;

	if (Cout)
		Cout->SetXYPos (X, Y);

	UnlockConsole();
}


void Console::SaveCursorState (void)
{ 
	SavedX1   = GetWinX1();
	SavedY1   = GetWinY1();
	SavedX2   = GetWinX2();
	SavedY2   = GetWinY2();
	SavedXPos = GetXPos(); 
	SavedYPos = GetYPos();

	Cout->SaveCursPos();
}


void Console::RestoreCursorState (void)
{ 
	SetWindow (SavedX1, SavedY1, SavedX2, SavedY2);
	GoToXY (SavedXPos, SavedYPos);

	Cout->RestoreCursPos();
}


// send absolute console coordinates
void Console::RedrawXY (int X1, int Y1, int X2, int Y2)
{
	LockConsole();

	FitInConsBounds (&X1, &Y1);
	FitInConsBounds (&X2, &Y2);
	CorrectOrder (&X1, &Y1, &X2, &Y2);

    for (int y = Y1; y <= Y2; y++)
	    for (int x = X1; x <= X2; x++)
			PokeRefresh (x,y) = true;

	UnlockConsole();

	Cout->RefreshXY (X1, Y1, X2, Y2);
}


void  Console::EraseCursor (BOOL Refresh)
{   
	// don't bother if the cursor isn't even turned on
	if (!(this->GetCursor()))
		return;

	LockConsole();

	PokeRefresh (GetAbsXPos(), GetAbsYPos()) = true;

	if (Refresh)
	{
		int OldCursor;

		OldCursor = this->GetCursor();
		this->SetCursor (0);
		RedrawXY (GetAbsXPos(), GetAbsYPos(), GetAbsXPos(), GetAbsYPos());
		this->SetCursor (OldCursor);
	}

	UnlockConsole();
}


// I use this->GetCursor and this->SetCursor to distinguish between our class' version
// and the Win32 version
// and send absolute Console coords, and XDir/YDir are positive or negative direction
void Console::ScrollRegion (signed int XDir, signed int YDir, int X1, int Y1, int X2, int Y2)
{
	int xScroll = 0;
	int yScroll = 0;
	int OldCursor = 0;
    int MaxConsY; // cache this var
    int MaxConsX; // and this one too

	// help control CPU usage ... every Nth scroll, give up our time slice.
	ScrollCount++;
	if (ScrollIdleFreq != 0  &&  (ScrollCount % ScrollIdleFreq) == 0)
		Sleep (ScrollIdle);

	// make sure what's on the screen is current!
	OldCursor = this->GetCursor ();
	this->SetCursor (0);
	EraseCursor (FALSE);
	Refresh ();
	this->SetCursor (OldCursor);

	LockConsole();

    MaxConsX = GetMaxConsX();
    MaxConsY = GetMaxConsY();

	FitInConsBounds (&X1, &Y1);
	FitInConsBounds (&X2, &Y2);
	CorrectOrder (&X1, &Y1, &X2, &Y2);

	// If we're doing the scroll which is done 99.99% of the type, then we can do a
	// simple, quick little hack. Which scroll is that? Why, the entire screen up one
	// line. Simple. We simply shift all the pointers in ConsoleText
	if (X1 == 0  &&  Y1 == 0  &&  X2 == MaxConsX  &&  Y2 == MaxConsY &&
		XDir == 0  && YDir == -1)
	{
        cText *Line = NULL;
		cText *Temp = NULL;
        cText RefMe;
		int x;

		// save the top line pointer, shift everything up one, then put the top pointer
		// in the bottom position, and simply ...
		Temp = ConsoleText[0];
        for (x = 1; x <= MaxConsY; x++)
        {
            ConsoleText[x - 1] = ConsoleText[x];
        }

		ConsoleText[MaxConsY] = Temp;
 
		// reset the whole console to 'refresh = true'. 
        // then reset the new bottom line
        // the new bottom line is actually the old "top" line
        // cache data we want to copy into each character slot

        // bottom line
        RefMe.Char = ' ';
        RefMe.Format = TextFormat;
        RefMe.Refresh = true;

        Line = ConsoleText[MaxConsY];
        for (x = 0; x <= MaxConsX; x++)
            Line[x] = RefMe;
	}
	else
	{	// this algorithm should handle *any* scrolling
		// First off, set the entire region's Refresh flag to TRUE
    	for (int y = Y1; y < Y2; y++)
	    	for (int x = X1; x <= X2; x++)
                ConsoleText[y][x].Refresh = true;
				//PokeRefresh (x,y) = true;
		// do we really need that? i don't think so ... but i'll leave it here just in case

		int YMin, YMax, YInc;
		int XMin, XMax, XInc;

		if (YDir > 0) // scrolling down
		{
			YInc = -1;
			YMin = Y2;
			YMax = Y1;
		}
		else          // scrolling up
		{
			YInc = +1;
			YMin = Y1;
			YMax = Y2;
		}

		if (XDir > 0) // scrolling right
		{
			XInc = -1;
			XMin = X2;
			XMax = X1;
		}
		else		  // scrolling left
		{
			XInc = +1;
			XMin = X1;
			XMax = X2;
		}

		for (yScroll = YMin; yScroll != (YMax + YInc); yScroll += YInc)
		{
			for (xScroll = XMin; xScroll != (XMax + XInc); xScroll += XInc)
			{
				int NewX;
				int NewY;

				NewX = xScroll + XDir;
				NewY = yScroll + YDir;

				if ( // clip to within given region
					(NewX >= X1) && (NewX <= X2) && (NewY >= Y1) && (NewY <= Y2)
				)
				{
					// only move and set refresh flags for chars which CHANGE
					if (   !memcmp ( &(PokeFormat(NewX,NewY)), &(PokeFormat(xScroll,yScroll)), sizeof (PokeFormat(0,0)) ) 
						&& (PokeChar(NewX,NewY) == PokeChar(xScroll,yScroll)) )
					{
						// do nothing. do NOT re-enable the next line, because we will be clearing
						// the refresh flag for chars that have NOT YET BEEN UPDATED! this would be
						// bad.
						
						// PokeRefresh (NewX, NewY) = FALSE;
					}
					else
					{
						// else we should be absoultely sure to refresh on next draw
						PokeRefresh (NewX, NewY) = true;
					}
	
					PokeChar    (NewX, NewY) = PokeChar   (xScroll, yScroll);
					PokeFormat  (NewX, NewY) = PokeFormat (xScroll, yScroll);
				}

				// Init the edges that are now "blank" to spaces and Refresh as TRUE
				if ( ((xScroll < (X1 + XDir)) || (xScroll > (X2 + XDir))) ||
					((yScroll < (Y1 + YDir)) || (yScroll > (Y2 + YDir))) )
				{
					PokeRefresh (xScroll, yScroll) = true;
					PokeChar    (xScroll, yScroll) = ' ';
					PokeFormat  (xScroll, yScroll) = TextFormat;
				} // if
			} // for x
		} // for y
	} // generic algo
	
	// Tell the output driver to do its scroll. It usually has a better
	// method than just calling RedrawAll ()
	if (AutoRefresh)
	{
		Cout->ScrollRegion (XDir, YDir, X1, Y1, X2, Y2);
	}

	UnlockConsole();
}


// absolute Console coords. mainly used internally
BOOL Console::HasRegionChanged (int X1, int Y1, int X2, int Y2)
{
	BOOL Changed;

	LockConsole();

	FitInConsBounds (&X1, &Y1);
	FitInConsBounds (&X2, &Y2);
	CorrectOrder (&X1, &Y1, &X2, &Y2);

	Changed = FALSE;
	for (int y = Y1; y <= Y2; y++)
	{
		for (int x = X1; x <= X2; x++)
		{
			if (PeekRefresh (x,y))
			{
				Changed = TRUE;
				break;
			}
		}
		if (Changed) break;
	}

	UnlockConsole();

	return (Changed);
}


// Checks to see if the X and Y positions are out of bounds, and if
// necessary scrolls the console. Used by WriteChar()
// uses logical screen coords within current 'window'
// the 'unlock' parameter decides whether or not we will unlock before
// scrolling and then lock afterward
void Console::CheckTextScroll (BOOL Refresh, BOOL Unlock)
{
	// X wrapping
	if (GetXPos() > GetMaxX())
	{
		CurrentXPos = GetMinX();
		CurrentYPos++;
	}

	if (GetXPos() < GetMinX())    // a backspace could cause an invalid X value
	{
		CurrentXPos = GetMaxX();
		if (GetYPos() != GetMinY())
			CurrentYPos--;
	}

	if (GetYPos() < GetMinY())  // too many backspace could cause an invalid Y value
	{
		CurrentYPos = GetMinY();
	}

	// Y scrolling
	if (GetYPos() > GetMaxY())
	{
		CurrentYPos = GetMaxY();

        if (Unlock) UnlockConsole ();
		ScrollRegion (0, -1, GetWinX1(), GetWinY1(), GetWinX2(), GetWinY2());
		if (Unlock) LockConsole ();

		if (Refresh) 
		{
			RefreshXY (GetWinX1(), GetWinY2(), GetWinX2(), GetWinY2());
		}
	}
}


void Console::WriteCharE (char c, BOOL Refresh)
{
	if (Refresh)
		if (Cout) Cout->StdOutChar (c);

    // only check every new line
    /*
    if (c == '\n' || c == '\r')
        DoControlExceptions ();
    */

	if (StripHighAscii  &&  c >= 128)
        c &= 0x7f;

    if (!strchr ("\n\r\t\a\b", c))
    {
	    if (StripCtrlChars  &&  c <= 31)
    		c = ' ';

    	if (ExpandCtrlChars &&  c <= 31)
    	{
		    WriteCharE ('^', FALSE);
		    c += 64;
	    }
    }

	switch (c)
	{
		// new line
		case '\n' :  CurrentXPos = GetMinX (); 
			         CurrentYPos++; 
					 break;

		// reset line
		case '\r' :  CurrentXPos = GetMinX (); 
					 break;

		// expand tab char
		case '\t' :  {	// for some reason using the TabExpanded deal REALLY messes up telnet ... scratching my head
						int j = (1 + (CurrentXPos / TabSize)) * TabSize;

						for (int i = CurrentXPos; i < j; i++)
						{
							WriteCharE (' ', FALSE);

							// if we wrap around, stop da tab expansion
							if (i == GetMaxX()) 
								break;
						}
					 }
					 break;

		// beep
		case '\a' :  Cout->Beep();
					 Refresh = FALSE;
					 break;

		// backspace
		case '\b' :  CurrentXPos--; 
					 break;

		default   :  {
						int AbsX = GetAbsXPos ();
						int AbsY = GetAbsYPos ();

						PokeChar    (AbsX, AbsY) = c;
						PokeFormat  (AbsX, AbsY) = TextFormat;
						PokeRefresh (AbsX, AbsY) = true;
						CurrentXPos++;
					 }
					 break;
	}

	CheckTextScroll (Refresh, TRUE);
	if (Refresh) 
        this->Refresh ();
}


void Console::WriteChar (char c)
{
	WriteCharE (c, AutoRefresh);
}


void __cdecl Console::WriteTextf (const char *S, ...)
{
	va_list marker;
	char *buffer;
	
	buffer = (char *) malloc (MaxWriteBuffer);

	va_start (marker, S);
	_vsnprintf (buffer, MaxWriteBuffer-1, S, marker);
	va_end (marker);

	WriteText (buffer);

	free (buffer);
	return;
}


void Console::WriteText (const char *S)
{
	int SLen = strlen (S);
    int i;

    LockConsole ();

    if (PipeOut != NULL)
        PipeOut->WriteString ((char *)S);

    if (PipeOut == NULL  ||  PipeEcho == true)
    {
	    if (Cout)
		    Cout->StdOutLine (S);

        for (i = 0; i < SLen; i++)
            WriteCharE (S[i], FALSE);
    }

    UnlockConsole ();

    if (AutoRefresh) 
		Refresh (); 

	return;
}


CIKey Console::GetChar (void)
{
	CIKey c;

    if (PipeIn != NULL  &&  PipeIn->IsCharAvail())
    {
        PipeIn->ReadChar ((char *)&c);
        return (c);
    }

ihategoto:
    SleepUntilInput ();
    while (!KeyPressed())
    {
   		// save CPU time if the user's been idle
	    if (GetIdleTime() > Stage2IdleDelay)
   			Sleep (Stage2Idle);
	    else
   			Sleep (Stage1Idle);
   	}

    // Save a function call if not enabled
    if (CTRLExceptions)
    {
        DoControlExceptions ();

        // goto isn't really necessary, but cleaner in this case
        // yes, i could change the fucking code to not use goto but it's not worth my effort
        if (!KeyPressed())
            goto ihategoto;
    }

	GetKeyFromQ (&c);
	
	return (c);
}


// GetChar + echo
CIKey Console::GetCharE (void)
{
	CIKey c;

	c = GetChar ();
	WriteChar (c);

	return (c);
}


// Resizing the key queue will flush the contents
BOOL Console::ResizeQ (int QSize)
{
	LockConsole ();

    UnsignalInput ();

	KeyQBegin = 0;
	KeyQEnd = 0;
	KeyQLength = QSize;

	KeyQ = (CIKey *) realloc (KeyQ, QSize * sizeof (CIKey));

	UnlockConsole();

	if (KeyQ == NULL) return (FALSE);

	return (TRUE);
}


BOOL Console::IsQEmpty ()
{
	BOOL Result;

	LockConsole ();

	Result = (KeyQEnd == KeyQBegin) || (KeyQ == NULL);

    if (Result)
        UnsignalInput ();

	UnlockConsole ();

	return (Result);
}


// full, as in no space left
BOOL Console::IsQFull ()
{
	BOOL Result;

	LockConsole ();

	int QEnd = KeyQEnd;
	int QBeg = KeyQBegin;

	QEnd++;
	if (QEnd == KeyQLength)
		QEnd = 0;

	if (QEnd == QBeg)
    {
		Result = TRUE;
        SignalInput ();
    }
	else
		Result = FALSE;

	UnlockConsole ();

	return (Result);
}


// KeyQ is full if KeyQBegin == (KeyQEnd + 1); with KeyQEnd wrapping around to 0
// when it equals KeyQLength. Ie, if KeyQEnd is 29 and KeyQLength is 30, adding
// 1 to KeyQEnd should wrap to 0. Conversely, subracting 1 from KeyQEnd when it
// is 0 should set it to (KeyQLength - 1). This applies to KeyQBegin as well,
// except that the KeyQ is full when KeyQEnd = (KeyQBegin - 1);
BOOL Console::AddKeyToQ (CIKey k)
{
	LockConsole ();

	if (IsQFull()) 
	{
		UnlockConsole ();
		return (FALSE);
	}

	KeyQ[KeyQEnd] = k;
	KeyQEnd++;
	if (KeyQEnd == KeyQLength) 
        KeyQEnd = 0;

	ResetIdleTime ();

    SignalInput ();
	UnlockConsole ();

	return (TRUE);
}


BOOL Console::AddManyKeysToQ (CIKey *keys, int Count)
{
    int i;

    if (Count == -1)
        Count = strlen ((const char *)keys);

    LockConsole ();

    for (i = 0; i < Count; i++)
        AddKeyToQ (keys[i]);

    UnlockConsole ();
    return (TRUE);
}



// like ungetch()
BOOL Console::AddKeyToFrontOfQ (CIKey k)
{
	if (IsQFull()) return (FALSE);

	LockConsole ();

	if (KeyQBegin == 0) 
		KeyQBegin = KeyQLength;

	KeyQBegin--;
	KeyQ[KeyQBegin] = k;

	ResetIdleTime ();

	UnlockConsole ();
    SignalInput ();

	return (TRUE);
}


BOOL Console::PeekAtKeyQ (CIKey *k)
{
    BOOL ReturnVal = TRUE;

    LockConsole();

    if (IsQEmpty())
    {
        ReturnVal = FALSE;
    }
    else
    {
        *k = KeyQ[KeyQBegin];
    }

    UnlockConsole();

    return (ReturnVal);
}


// result stored in *k, BOOL returns FALSE if the
// queue is empty, and *k will be unmodified.
BOOL Console::GetKeyFromQ (CIKey *k)
{
	LockConsole ();

	if (IsQEmpty ()) 
    {
        UnsignalInput ();
        UnlockConsole();
        return (FALSE);
    }

	*k = KeyQ[KeyQBegin];
	KeyQBegin++;
	if (KeyQBegin == KeyQLength)
    {
        KeyQBegin = 0;
        UnsignalInput ();
    }

	UnlockConsole ();

	return (TRUE);
}


// can't call GetChar, because that's who calls us
void Console::DoControlExceptions (void)
{
    if (!CTRLExceptions)
        return;

    if (!IsQEmpty())  // a key is in the input queue!!!
    {
        CIKey c;

        PeekAtKeyQ (&c);  // snag that key
        switch (c)
        {
            case ('S' - 'A' + 1) : // program pause
                GetKeyFromQ (&c); // remove CTRL+S from queue
                // wait for another key press
                while (!KeyPressed())
                {
                    Sleep (50);
                }
                GetKeyFromQ (&c); // remove that key as well
                break;

            case (1) :          // program attach
                GetKeyFromQ (&c);  // remove CTLR+A from queue
                // wait for next key press
                while (!KeyPressed())
                {
                    Sleep (50);
                }
                GetKeyFromQ (&c); // remove this next key
                // now what do they want to do?
                // not that "CTRL+A, A" and "CTRL+A, CTRL+A" both do the same thing, etc
                switch (tolower(c))
                {
                    case 1:
                    case 'a':   // switch to their next console
                        SetCurrentFlag (CON_ACTION_NEXT);
                        break;

                    case 26:
                    case 'z':   // switch to their previous console
                        SetCurrentFlag (CON_ACTION_PREV);
                        break;

                    case 24:
                    case 'x':   // detach and log out
                        SetCurrentFlag (CON_ACTION_QUIT);
                        break;

                    case 14:
                    case 'n':   // new login process, then attach to it
                        SetCurrentFlag (CON_ACTION_NEWP);
                        break;

                    default:
                        WriteText ("\a");
                        break;
                }
                break;
        }
    }

    return;
}


DWORD Console::GetIdleTime (void)
{
	DWORD TickCount = GetTickCount ();
	DWORD ReturnVal;

	if (TickCount > LastInputEvent)
	{
		ReturnVal = TickCount - LastInputEvent;
	}
	else
	{	
		ReturnVal = LastInputEvent - TickCount;
	}

	return (ReturnVal);
}


void  Console::ResetIdleTime   (void)
{ 
	LastInputEvent = GetTickCount();
}

	
char *Console::ReadText (char *S, DWORD MaxLen, BOOL NewLine, BOOL History)
{
	return (ReadTextE (S, MaxLen, TRUE, NewLine, History));
}


// Enter finishes the line you're typing in
char *Console::ReadTextE (char *S, DWORD MaxLen, BOOL Echo, BOOL NewLine, BOOL History)
{
    int CX, CY;
    char c;
    int BufPos;
    bool Continue = true;
    int CurHistory = LineHistory.size();

    BufPos = 0;
    ReadBuffer = "";
    ReadMisc = "";
    CX = GetXPos ();
    CY = GetYPos ();
    while (Continue)
    {
        int i = 0;
        bool Redraw = false;

        c = GetChar ();
        switch (c)
        {
            case '\r':
            case '\n':
                Continue = false;
                break;

            case 9: // CTRL+I = tab. simply add *spaces*
                for (i = 0; i < GetTabSize(); i++)
                {
                    AddKeyToQ (' ');
                }
                break;

            case 8: // backspace
                if (BufPos != 0)
                {
                    BufPos--;
                    ReadBuffer.erase (BufPos, 1);

                    if (Echo) 
                        WriteText ("\b");

                    Redraw = true;
                }
                break;

            case 127: // delete
                if (BufPos != ReadBuffer.size())
                {
                    ReadBuffer.erase (BufPos, 1);
                    Redraw = true;
                }
                break;

            case 27: // esc sequence?
                c = GetChar ();
                switch (c)
                {
                    case 27: // escape key
                        if (Echo)
                            WriteTextf ("\\%s", NewLine ? "\n" : "");

                        strcpy (S, "");
                        return (S);

                    case '[':
                        c = GetChar ();
                        if (c == 'O') 
                            c = GetChar ();  // some clients send an unnecessary O

                        switch (c)
                        {
                            case 'D' : // left arrow
                                if (BufPos != 0)
                                {
                                    BufPos--;
                                    if (Echo) WriteText ("\b");
                                }
                                break;

                            case 'C' : // right arrow
                                if (BufPos != ReadBuffer.size())
                                {
                                    if (Echo) WriteTextf ("%c", ReadBuffer[BufPos]);
                                    BufPos++;
                                }
                                break;

                            case 'A' : // up arrow, previous history
                                if (LineHistory.size() == 0)  // no history? oh well
                                    continue;

                                if (CurHistory == 0  ||  CurHistory > LineHistory.size())
                                    CurHistory = LineHistory.size();

                                CurHistory--;

                                // erase what's displayed
                                if (Echo)
                                {
                                    ReadMisc.resize (0);
                                    ReadMisc.insert (ReadMisc.begin(), BufPos, '\b'); // backspace all the way to the beginning
                                    ReadMisc.insert (ReadMisc.end(), ReadBuffer.size(), ' '); // erase it all
                                    ReadMisc.insert (ReadMisc.end(), ReadBuffer.size(), '\b'); // backspace all the way to the beginning again
                                }

                                // set input buffer to that history element
                                ReadBuffer = LineHistory[CurHistory];
                                ReadMisc.insert (ReadMisc.size(), ReadBuffer);
                                WriteText (ReadMisc.c_str());
                                BufPos = ReadBuffer.size();
                                break;

                            case 'B' : // down arrow, next history
                                if (LineHistory.size() == 0)  // no history? oh well
                                    continue;

                                CurHistory++;

                                if (CurHistory >= LineHistory.size())
                                    CurHistory = 0;
                                
                                // erase what's displayed
                                if (Echo)
                                {
                                    ReadMisc.resize (0);
                                    ReadMisc.insert (ReadMisc.begin(), BufPos, '\b'); // backspace all the way to the beginning
                                    ReadMisc.insert (ReadMisc.end(), ReadBuffer.size(), ' '); // erase it all
                                    ReadMisc.insert (ReadMisc.end(), ReadBuffer.size(), '\b'); // backspace all the way to the beginning again
                                }

                                // set input buffer to that history element
                                ReadBuffer = LineHistory[CurHistory];
                                ReadMisc.insert (ReadMisc.size(), ReadBuffer);
                                WriteText (ReadMisc.c_str());
                                BufPos = ReadBuffer.size();
                                break;

                            case 'L' : // insert key
                                RTInsert = !RTInsert;
                                break;

                            case 'H' : // home
                                ReadMisc.resize (0);
                                ReadMisc.insert ((int)0, BufPos, '\b'); // backspace all the way to the end
                                BufPos = 0;

                                if (Echo) 
                                    WriteText (ReadMisc.c_str());

                                break;

                            default:
                                ///**/WriteTextf (" ^%u ", c); // for diagnostics and finding out what keys are what values
                                break;

                        }
                }
                break;

            default:
                if (RTInsert)
                {
                    ReadBuffer.insert (BufPos, 1, c);
                    if (Echo) WriteTextf ("%c", c);
                    ///**/WriteTextf (" %u ", c); // for diagnostics and finding out what keys are what values
                    BufPos++;
                    if (Echo) if (BufPos != ReadBuffer.size()) Redraw = true; // redraw if not at end of string
                }
                else
                {   // overwrite
                    ReadBuffer.erase (BufPos, 1);
                    ReadBuffer.insert (BufPos, 1, c);
                    BufPos++;
                    if (Echo) WriteTextf ("%c", c);
                }
                break;
        }

        if (Redraw && Echo)
        {
            ReadMisc.resize (0);
            ReadMisc.insert (ReadMisc.begin(), ReadBuffer.size() - BufPos, ' ');  // go all the way to the end
            ReadMisc.insert (ReadMisc.size(), " \b"); // erase any char that would have been there
            ReadMisc.insert (ReadMisc.end(), ReadBuffer.size(), '\b'); // backspace all the way
            ReadMisc.insert (ReadMisc.size(), ReadBuffer);           // print contents of buffer
            ReadMisc.insert (ReadMisc.end(), ReadBuffer.size() - BufPos, '\b'); // go to where the cursor is

            WriteText (ReadMisc.c_str());
        }

        // If length is over a certain amount, end input. This can ensure no memory killing I suppose
        if (ReadBuffer.size() > 4096)
            Continue = false;
    }

    strncpy (S, ReadBuffer.c_str(), min (MaxLen + 1, ReadBuffer.size() + 1));

    if (Echo && NewLine)
        WriteText ("\n");

    if (History  &&  ReadBuffer.size() != 0)  // don't add strings where user "just pushed enter"
    {
        LineHistory.push_back (S);  // remember, length of Buffer may > length put into S. so put what really gets used
        while (LineHistory.size() > LineHistoryMax)     // max history elements?
            LineHistory.erase (LineHistory.begin());  // then kill the first one (repeat until under max)
    }

    return (S);
}


void Console::TestPatternY (void)
{
	BOOL ref;

	GoToXY (0,0);
	ref = GetAutoRefresh();
	SetAutoRefresh (FALSE);
	for (int y = 0; y < ConsoleYSize; y++)
	{
		for (int x = 0; x < ConsoleXSize; x+=10)
		{
			PokeChar (x,y) = (y % 10) + '0' - 1;
		}
	}
	SetAutoRefresh (ref);
}


void Console::TestPatternX (void)
{
	BOOL ref;

	GoToXY (0,0);
	ref = GetAutoRefresh();
	SetAutoRefresh (FALSE);
	for (int y = 0; y < GetMaxConsY(); y++)
	{
		for (int x = 0; x < GetMaxConsX(); x++)
		{
			PokeChar (x,y) = (x % 10) + '0' - 1;
		}
	}
	SetAutoRefresh (ref);
}


void Console::TestPatternChars (void)
{
	char curChar = '\0';
	BOOL ref;

	GoToXY (0,0);
	ref = GetAutoRefresh ();
	SetAutoRefresh (FALSE);
	for (int y = 0; y < GetMaxConsY(); y++)
	{
		for (int x = 0; x < GetMaxConsX(); x++)
		{
			PokeChar (x,y) = curChar++;
		}
	}
	SetAutoRefresh (ref);
}


// non-Console class members can't use these macros
#undef GetConsWidth
#undef GetConsHeight
#undef GetWinX1
#undef GetWinX2
#undef GetWinY1
#undef GetWinY2
#undef Peek
#undef PeekChar
#undef PeekFormat
#undef PeekRefresh
#undef Poke
#undef PokeChar
#undef PokeFormat
#undef PokeRefresh
#undef GetXPos
#undef GetYPos
#undef GetAbsXPos
#undef GetAbsYPos


void CorrectOrder (int *X1, int *Y1, int *X2, int *Y2)
{	// SWAP macro defined in Console.h
	if (*X1 > *X2)  SWAP (*X1, *X2);
	if (*Y1 > *Y2)  SWAP (*Y1, *Y2);
}


void FitInBounds (int *x, int *y, int BoundX1, int BoundY1, int BoundX2, int BoundY2)
{
	if (*x < BoundX1) *x = BoundX1;
	if (*x > BoundX2) *x = BoundX2;
	if (*y < BoundY1) *y = BoundY1;
	if (*y > BoundY2) *y = BoundY2;
}


BOOL IsInBounds (int x, int y, int BoundX1, int BoundY1, int BoundX2, int BoundY2)
{
	return !(x < BoundX1  ||  x > BoundX2  ||  y < BoundY1  ||  y > BoundY2);
}


void Console::FitInWinBounds (int *x, int *y)
{
	FitInBounds (x, y, GetMinX(), GetMinY(), GetMaxX(), GetMaxY());
}


BOOL Console::IsInWinBounds (int x, int y)
{
	return (IsInBounds (x,y, GetMinX(), GetMinY(), GetMaxX(), GetMaxY()));
}


void Console::FitInConsBounds (int *x, int *y)
{
	FitInBounds (x, y, 0, 0, GetMaxConsX(), GetMaxConsY());
}


BOOL Console::IsInConsBounds (int x, int y)
{
	return (IsInBounds (x, y, 0, 0, GetMaxConsX(), GetMaxConsY()));
}


void Convert24toANSI (COLORREF Orig, int *Result, BOOL *Bold)
{
	BYTE Red;
	BYTE Green;
	BYTE Blue;

	// First snag the individual color components
	Red   = GetRValue (Orig);
	Green = GetGValue (Orig);
	Blue  = GetBValue (Orig);

	// Now, the ANSI colors are just a 3-bit RGB value, but are backwards (BGR)
	// So, we figure if a color value is at least 128 (of 255), it counts for the bit value
	//              BGR
	// 30 Black   = 000
	// 31 Red     = 001
	// 32 Green   = 010
	// 33 Yellow  = 011
	// 34 Blue    = 100
	// 35 Magenta = 101
	// 36 Cyan    = 110
	// 37 White   = 111

	// if one is >192, then it's 'Bold'
	if ((Red > 192  ||  Green > 192  ||  Blue > 192)  ||
		(Red == 64  &&  Green == 64  && Blue == 64)) // account for grey
	{
		if (Bold) *Bold = TRUE;
	}
	else
    {
		if (Bold) *Bold = FALSE;
    }

	if (Red   >= 128) Red   = 1; else Red   = 0;
	if (Green >= 128) Green = 1; else Green = 0;
	if (Blue  >= 128) Blue  = 1; else Blue  = 0;

	if (Result) 
		*Result =  (30 + (Red + (Green << 1) + (Blue << 2)));
}


void ConvertANSIto24 (int Color, BOOL Bold, COLORREF *Result)
{
	if (Color > 37) Color -= 10;  // ANSI Background -> ANSI Foreground
	if (Color > 29) Color -= 30;

	if (Color > 7) 
	{
		if (Result) *Result = 0;
	}
	else
    {
		if (Result) *Result = (RGB ((64 * Bold) + 192 * (Color & 1), (64 * Bold) + 192 * (Color & 2), (64 * Bold) + 192 * (Color & 4)));
    }
}


BOOL CopyConsole (Console *Dest, Console *Src, BOOL CopyKeyQ)
{
	if (Dest == NULL  ||  Src == NULL)
		return (FALSE);

	//
	Dest->LockConsole();
	Src->LockConsole();
	//

	// copy over Console shizz0t
	if (Dest->GetConsWidth()  != Src->GetConsWidth()   ||
		Dest->GetConsHeight() != Src->GetConsHeight())
	{
		Dest->ResizeXY (Src->GetConsWidth(), Src->GetConsHeight());
	}

	if (Dest->GetWinX1() != Src->GetWinX1()  ||
		Dest->GetWinX2() != Src->GetWinX2()  ||
		Dest->GetWinY1() != Src->GetWinY1()  ||
		Dest->GetWinY2() != Src->GetWinY2() )
	{
		Dest->SetWindow (Src->GetWinX1(), Src->GetWinY1(), 
						 Src->GetWinX2(), Src->GetWinY2());
	}

	Dest->SetTabSize        (Src->GetTabSize());
	Dest->SetAutoRefresh    (Src->GetAutoRefresh());
	Dest->SetMaxWriteBuffer (Src->GetMaxWriteBuffer());
	Dest->SetResizeLock     (Src->GetResizeLock());
	Dest->SetForeground     (Src->GetTextFormat().Foreground);
	Dest->SetBackground     (Src->GetTextFormat().Background);
    Dest->SetUnderline      (Src->GetUnderline());
	Dest->SetCursor         (Src->GetCursor());

	// transfer screen data
	for (int y = 0; y < Src->GetConsHeight(); y++)
    {
	    for (int x = 0; x < Src->GetConsWidth(); x++)
		{
			if (Dest->PeekChar(x,y) != Src->PeekChar(x,y)   ||
                !CFORMATEQ (Dest->PeekFormat(x,y), Src->PeekFormat(x,y)))
                /*
				Dest->PeekFormat(x,y).Foreground != Src->PeekFormat(x,y).Foreground   ||
				Dest->PeekFormat(x,y).Background != Src->PeekFormat(x,y).Background)*/
			{
				Dest->PokeRefresh(x,y) = TRUE;
				Dest->PokeChar(x,y) = Src->PeekChar(x,y);
                Dest->PokeFormat(x,y) = Src->PeekFormat(x,y);
				//Dest->PokeFormat(x,y).Foreground = Src->PeekFormat(x,y).Foreground;
				//Dest->PokeFormat(x,y).Background = Src->PeekFormat(x,y).Background;
			}
			else
			{
				Dest->PokeRefresh(x,y) = FALSE;
			}
		}
	}

	// transfer keyboard input queue
	if (CopyKeyQ)
	{
		Dest->ResizeQ (Src->GetQSize ());
		while (Src->KeyPressed())
		{
			Dest->AddKeyToQ (Src->GetChar());
		}
	}

	Dest->GoToXY (Src->GetXPos(), Src->GetYPos());

	Src->UnlockConsole();
	Dest->UnlockConsole();

	return (TRUE);
}

