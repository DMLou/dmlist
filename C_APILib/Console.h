/*it's not scrolling correctly because yer using validaterect() and so the main
thread ends up drawing it from within the message loop. cha CHING
*/

#ifndef CONSOLE_H
#define CONSOLE_H

#pragma warning (disable: 4786 4251)

// All coordinate value pairs are X,Y. Take note of this because many terminals
// use Y,X (yuck!)

#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>

#include <vector>
#include <string>

#include "C_Linkage.h"
#include "C_Types.h"
#include "C_Memory.h"
#include "C_ShObj.h"
#include "C_Pipe.h"


#if !defined(NDEBUG) || defined(DEBUG)
// Put debug options in here
//#define CSAFEPOKE
#endif

#ifdef CSAFEPOKE
#pragma message ("Using safe Poke() and Peek() functions")
#endif


// a trick i found somewhere. can't remember where exactly.
// only works for integral types.
#ifndef SWAP
#define SWAP(a,b)  (a ^= b, b ^= a, a ^= b)
#endif
// ie, a xor b, b xor a, a xor b


// make sure coords are in-bounds and in the correct order (ie, X2 should always be >= X1)
#define FIX_CONS_COORDS(X1,Y1,X2,Y2) (FitInConsBounds ((X1),(Y1)),FitInConsBounds ((X2),(Y2)),CorrectOrder ((X1),(Y1),(X2),(Y2)))
#define FIX_WIN_COORDS(X1,Y1,X2,Y2)	 (FitInWinBounds ((X1), (Y1)),FitInWinBounds ((X2), (Y2)),CorrectOrder ((X1), (Y1), (X2), (Y2))


class CAPILINK Console;
class CAPILINK C_Input;
class CAPILINK C_Output;
class CAPILINK C_Program;


#define CF_KEY_BACKSPACE  8
#define CF_KEY_TAB        9
#define CF_KEY_ENTER     13
#define CF_KEY_ENTER2    10
#define CF_KEY_BELL       7


CAPILINK BOOL CopyConsole (Console *Dest, Console *Src, BOOL CopyKeyQ);


// for fixing X/Y parameters so as to avoid crashes
CAPILINK void CorrectOrder (int *X1, int *Y1, int *X2, int *Y2);
CAPILINK void FitInBounds (int *x, int *y, int BoundX1, int BoundY1, int BoundX2, int BoundY2);
CAPILINK BOOL IsInBounds (int x, int y, int BoundX1, int BoundY1, int BoundX2, int BoundY2);


// convert colors from one format to another
// will return a "foreground" color. +10 for "background"
// if *Bold is true, then you should set the Bold bit when sending over an ANSI color
CAPILINK void Convert24toANSI (COLORREF Orig, int *Result, BOOL *Bold);


// will return a 24-bit RGB value
CAPILINK void ConvertANSIto24 (int Color, BOOL Bold, COLORREF *result);



class CAPILINK C_Input
{
public:
	C_Input ();
	virtual ~C_Input ();

	virtual BOOL AttachConsole (Console *C);

	BOOL GetDetachFlag (void)   { return (Detached); }
	void SetDetachFlag (BOOL d) { Detached = d;      }

	// call Console::AddKeyToQ() from WndProc() or UpdateInput()
	virtual BOOL WndProc(UINT message, WPARAM wParam, LPARAM lParam)
	{ return (FALSE); }

	// called every time Console::KeyPressed() is called.
	virtual void  UpdateInput (void)
	{ return;         }

	Console *GetConsole (void)  { return (Cons); }

	void SetHWND (HWND h)    { cHWND = h;      }
	HWND GetHWND (void)      { return (cHWND); }

	// always SetNewPID *first*
	BOOL  GetHasNewPID (void)   { return (HasNewPID); }
	void  SetHasNewPID (BOOL h) { HasNewPID = h;      }
	DWORD GetNewPID (void)      { return (NewPID);    }
	void  SetNewPID (DWORD p)   { NewPID = p;         }

protected:
	Console *Cons;
	HWND cHWND;

	// if set to TRUE you should dispose of this instance but NOT the PID
	// you thought it was attached to!
	BOOL Detached;
	// when you do an 'attach,' the driver ends up getting attach to another
	// PID. this is to notify the keeper of the driver of this event. when the
	// 'keeper' has gone through the appropriate housekeeping for this event,
	// it should set HasNewPID to FALSE and NewPID to -1 (0xffffffff)
	BOOL  HasNewPID;
	DWORD NewPID;

	// can only update input from 1 thread at a time
	ShObj UpdateInp;
};


class CAPILINK C_Output
{
public:
	C_Output ();
	virtual ~C_Output ();

	virtual BOOL AttachConsole (Console *C);

	virtual BOOL WndProc(UINT message, WPARAM wParam, LPARAM lParam)
	{ return (FALSE); }

	BOOL GetDetachFlag (void)   { return (Detached); }
	void SetDetachFlag (BOOL d) { Detached = d;      }
	
	// These should be implemented by any derivatives ("drivers")

	// Every call to WriteCharE() calls this
	virtual void  StdOutChar    (char c)
	{ return; }

	// Same as above, but sends multiple characters.
	virtual void  StdOutLine    (const char *S)
	{ return; }

	virtual void  SetXYPos      (int X, int Y)
	{ return; }

	virtual void  RefreshXY     (int X1, int Y1, int X2, int Y2)
	{ return; }

	virtual void  ClearScr      (void)
	{ return; }

	virtual void  ScrollRegion  (signed int Xdir, signed int Ydir, int X1, int Y1, int X2, int Y2)
	{ return; }

	virtual void  ResizeNotify  (void)
	{ return; }

	// these two will be given absolute coordinates
	virtual void  SaveCursPos (void)
	{ return; }

	virtual void  RestoreCursPos (void)
	{ return; }

	virtual int   GetOutputMode (void)   
	{ return (OutputMode); }

	virtual void  SetOutputMode (int m)  
	{ if (m == C_DIRECT) OutputMode = m; else OutputMode = C_LINE; }

	// pass NULL to receive how many bytes to allocated for result
	DWORD GetVersionString (char *Result);

	BOOL SupportsColor        (void)  { return (FeatureColor);      }
	BOOL SupportsDirectMode   (void)  { return (FeatureDirectOut);  }
	BOOL SupportsLineMode     (void)  { return (FeatureLineOut);    }
	int  GetDefaultOutputMode (void)  { return (DefaultOutputMode); } 

	// the Console will 'ask' if the new X/Y size is ok. For example, a simple text output
	// driver would say gNO to anything but 80x25
	virtual BOOL  ResizeQuery (int XSize, int YSize);

	// WriteText ("\a"); will call Beep(), as \a is the control char for a beep
	virtual void  SetBeep    (int BV)   { BeepVal = BV; }
	virtual void  Beep       (void)     { return; }

	Console *GetConsole (void)  { return (Cons); }

	void SetHWND (HWND h)  { cHWND = h;      }
	HWND GetHWND (void)    { return (cHWND); }

	// always SetNewPID *first*
	BOOL  GetHasNewPID (void)   { return (HasNewPID); }
	void  SetHasNewPID (BOOL h) { HasNewPID = h;      }
	DWORD GetNewPID (void)      { return (NewPID);    }
	void  SetNewPID (DWORD p)   { NewPID = p;         }

protected:
	void SetVersionString (char *VersionStr);

	char  VerString[2048];
	DWORD VerLength;

	Console *Cons;
	HWND cHWND;

	int   SavedXPos;
	int   SavedYPos;

	BOOL  FeatureColor;
	BOOL  FeatureDirectOut;
	BOOL  FeatureLineOut;
	int   DefaultOutputMode;

	// either C_DIRECT (RefreshXY) or C_LINE (StdOut)
	int   OutputMode;

	int   BeepVal;
	DWORD BeepFreq;
	DWORD BeepDur;

	// if set to TRUE you should dispose of this instance but NOT the PID
	// you thought it was attached to!
	BOOL  Detached;

	// when you do an 'attach,' the driver ends up getting attach to another
	// PID. this is to notify the keeper of the driver of this event. when the
	// 'keeper' has gone through the appropriate housekeeping for this event,
	// it should set HasNewPID to FALSE and NewPID to -1 (0xffffffff)
	BOOL  HasNewPID;
	DWORD NewPID;
};


// 
class CAPILINK Console
{
public:
	// X/Y Size should be cardinal values (ie, they represent 1..80, 1..25)
	Console  (HWND hWnd, int Width, int Height);  
	virtual ~Console (void);

	DWORD GetMemUsage (void)  { return (!IsBadReadPtr (ConsMemHeap, sizeof (C_Memory))
		                                ? ConsMemHeap->GetTotalHeapSize()
										: 0); }

	BOOL AttachOutput (C_Output *CO);
	BOOL AttachInput  (C_Input *CI);

	C_Output *GetOutput (void);
	C_Input  *GetInput  (void);

	HWND GetHWND (void)        { return (cHWND);           }
	BOOL SetHWND (HWND h)      { cHWND = h; return (TRUE); }

	// Call this from your main WndProc function. It will return TRUE if the message
	// was processed by Console, C_Output, or C_Input, or FALSE if the
	// message was not processed. The goal is to make it safe to call this first,
	// and if it succeeds, to exit out of the program's main WndProc. This does not
	// stop you from processing the messages yourself, again. Also, this is only
	// necessary for Wind0ze, of course.
	BOOL WndProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	void  WriteCharE           (char c, BOOL Refresh);  // this refreshes based on BOOL Refresh
	void  WriteChar            (char c);                // this refreshes based on AutoRefresh
public:
	void  WriteText            (const char *S);         // uses WriteChar
	void  __cdecl WriteTextf   (const char *S, ...);    // uses WriteText

    C_Pipe *GetOutPipe (void)      { return (PipeOut); }
    void    SetOutPipe (C_Pipe *P) { PipeOut = P;      }
    C_Pipe *GetInPipe  (void)      { return (PipeIn);  }
    void    SetInPipe  (C_Pipe *P) { PipeIn = P;       }

    // You can have pipe echoing either on or off
    bool GetOutPipeEcho (void)   { return (PipeEcho); }
    void SetOutPipeEcho (bool P) { PipeEcho = P;      }

	// Both return a pointer to the char *S that's passed in. This is so that you can
	// do scanf() type reading by doing the following:
	//		sscanf (ReadTextE(buffer, length, Echo), "%s %u", string, &uint);
	char *ReadText             (char *S, DWORD MaxLen, BOOL NewLine = TRUE, BOOL History = TRUE);
	char *ReadTextE            (char *S, DWORD MaxLen, BOOL Echo, BOOL Newline = FALSE, BOOL History = FALSE);  // to echo or not to echo?
    void  ClearHistory         (void)    { LockConsole (); LineHistory.clear(); UnlockConsole (); }
    DWORD GetHistoryMax        (void)    { return (LineHistoryMax); }
    void  SetHistoryMax        (DWORD H) { LineHistoryMax = H;      }
    bool  GetInsertMode        (void)    { return (RTInsert);       }
    void  SetInsertMode        (bool I)  { RTInsert = I;            }

	CIKey GetChar   (void);
	CIKey GetCharE  (void);
	BOOL  UnGetChar (CIKey k)  { return (AddKeyToFrontOfQ(k)); }

    virtual bool KeyPressed (void)
    {   // if there is no input pipe, OR if there is an input pipe but it's run out of data,
        // use the regular input driver
        LockConsole ();
        if (PipeIn == NULL  ||  (PipeIn != NULL && !PipeIn->IsCharAvail()))
        {
            Cin->UpdateInput ();
        }
        else
        {   // otherwise grab keyboard keys
            UnlockConsole ();
            return (true);
        }
        UnlockConsole ();
        return (!IsQEmpty());
    }
//	virtual BOOL KeyPressed (void)	    { LockConsole(); Cin->UpdateInput(); UnlockConsole(); return (!IsQEmpty()); }

	__forceinline void  SetMaxWriteBuffer (DWORD max) { MaxWriteBuffer = max;    }
	__forceinline DWORD GetMaxWriteBuffer ()          { return (MaxWriteBuffer); }

	__forceinline void  SetAutoRefresh (BOOL a)       { AutoRefresh = a;         }
	__forceinline BOOL  GetAutoRefresh (void)         { return (AutoRefresh);    }

	__forceinline void  SetCursor (int C)             { CursorType = C;          }
	__forceinline int   GetCursor (void)              { return (CursorType);     }

	__forceinline void  SetTabSize (int TabSz)        { TabSize = TabSz;  }
	__forceinline int   GetTabSize (void)             { return (TabSize); }

	// takes/returns a 32-bit xRGB value, ie, 0x00FFFFFF is white (first byte unused/reserved)
	// format is 0x00bbggrr  (blue, green, red)
	// see info in MSDN on "COLORREF" data type
	// these set the text color for subsequent calls to WriteText()
	__forceinline void     SetForeground (COLORREF C)  { TextFormat.Foreground = C;      }
	__forceinline COLORREF GetForeground (void)        { return (TextFormat.Foreground); }
	__forceinline void     SetBackground (COLORREF C)  { TextFormat.Background = C;      }
	__forceinline COLORREF GetBackground (void)        { return (TextFormat.Background); }
    __forceinline void     SetUnderline  (bool U)      { TextFormat.Underline = U;       }
    __forceinline bool     GetUnderline  (void)        { return (TextFormat.Underline);  }
    __forceinline void     SetTextFormat (cFormat F)   { TextFormat = F;                 }
	__forceinline cFormat  GetTextFormat (void)        { return (TextFormat);            }

	// for direct modifying.
	// note that if x and y are out of bounds, a crash will occur

#ifndef CSAFEPOKE
	__forceinline cText   &Poke        (int x, int y) { return (ConsoleText[y][x]); }
#else
	// This is too slow, but good for debugging
	cText   &Poke        (int x, int y)
	{
		cText *ReturnVal;

		LockShObject (&ConsoleLock);
		FitInConsBounds (&x, &y);
		ReturnVal = &(ConsoleText[y][x]);
		UnlockShObject (&ConsoleLock);

		return (*ReturnVal);
	}
#endif

	__forceinline cFormat &PokeFormat  (int x, int y) { return (Poke(x,y).Format);  }
	__forceinline cChar   &PokeChar    (int x, int y) { return (Poke(x,y).Char);    }
	__forceinline bool    &PokeRefresh (int x, int y) { return (Poke(x,y).Refresh); }

	__forceinline cText    Peek        (int x, int y) { return (Poke(x,y));         }
	__forceinline cFormat  PeekFormat  (int x, int y) { return (Poke(x,y).Format);  }
	__forceinline cChar    PeekChar    (int x, int y) { return (Poke(x,y).Char);    }
	__forceinline bool     PeekRefresh (int x, int y) { return (Poke(x,y).Refresh); }


	// Refresh    -- refreshes current window region
	// RefreshAll -- refreshes the entire console
	// RefreshXY  -- refreshes any XY region
	//
	// Redraw     -- redraws current window region
	// RedrawAll  -- redraws the entire console
	// RedrawXY   -- redraws any XY region
	//
	// Further explanation:
	//   RefreshXY () does all the real work. It goes from X1 to X2, Y1 to Y2, and redraws
	//   any character in the console who's Refresh flag is set to TRUE. The Redraw() functions
	//   simple reset all the respective XY region's Refresh flags to TRUE to force a redraw.
	//
	// Send absolute console coordinates to the XY functions
	__forceinline void  Refresh     (void)  
	    { Cout->RefreshXY (GetWinX1(), GetWinY1(), GetWinX2(), GetWinY2()); }

	__forceinline void  RefreshAll  (void)  
		{ Cout->RefreshXY (0, 0, GetMaxConsX(), GetMaxConsY()); }

	__forceinline void  RefreshXY   (int X1, int Y1, int X2, int Y2)
		{ Cout->RefreshXY (X1, Y1, X2, Y2); }

	__forceinline void  Redraw      (void)  
		{ RedrawXY  (GetWinX1(), GetWinY1(), GetWinX2(), GetWinY2()); }

	__forceinline void  RedrawAll   (void)  
		{ RedrawXY  (0, 0, GetMaxConsX(), GetMaxConsY()); }

	void  RedrawXY     (int X1, int Y1, int X2, int Y2);

	BOOL HasRegionChanged (int X1, int Y1, int X2, int Y2);

	void  ScrollRegion (signed int Xdir, signed int Ydir, int X1, int Y1, int X2, int Y2);

	void  EraseCursor  (BOOL Refresh);

	// takes absolute Console coords. use Clear() or ClearWin/ClearScr as sort of a CLS.
	// will clear the screen with the format characteristics in TextFormat
	void ClearXY  (int X1, int Y1, int X2, int Y2);

	// ^[[2J will clear the screen if ANSI parsing is going
	void ClearScr (void)  
	{ if (Cout) Cout->ClearScr(); ClearXY (0, 0, GetMaxConsX(), GetMaxConsY()); GoToXY (0,0); }

	void ClearWin (void)  { ClearXY (GetWinX1(), GetWinY1(), GetWinX2(), GetWinY2()); GoToXY (0,0); }

	// convenience function
	void Clear (void)  { ClearWin (); }

	// to set a new console size. Note that this seems to be kinda buggy right now, mainly
	// with *shrinking* the console.
	BOOL  ResizeXY   (int NewXSize, int NewYSize, bool DoClearScr = true);
	
	// this will cause any ResizeXY operation to return FALSE w/o doing anything
	void  SetResizeLock (BOOL rs)   { ResizeLock = rs;     }
	BOOL  GetResizeLock (void)      { return (ResizeLock); }

	// relative to currently defined window region
	void  GoToXY     (int X, int Y);

	void  SaveCursorState    (void);
	void  RestoreCursorState (void);

	// ordinal values, ie 0, 0, 79, 24
	// default window is 0, 0, GetConsMaxX(), GetConsMaxY()
	void  SetWindow  (int X1, int Y1, int X2, int Y2);

	// to set/get individual parts
	int   GetWinX1 (void)        { return (WinX1); }
	int   GetWinY1 (void)        { return (WinY1); }
	int   GetWinX2 (void)        { return (WinX2); }
	int   GetWinY2 (void)        { return (WinY2); }

	void  SetWinX1 (int X1)      { WinX1 = X1;     }
	void  SetWinY1 (int Y1)      { WinY1 = Y1;     }
	void  SetWinX2 (int X2)      { WinX2 = X2;     }
	void  SetWinY2 (int Y2)      { WinY2 = Y2;     }

	// Absolute values within the console
	int   GetConsWidth  (void)   { return (ConsoleXSize);        }  // cardinal (1..80)
	int   GetConsHeight (void)   { return (ConsoleYSize);        }
	int   GetMaxConsX   (void)   { return (ConsoleXSize-1);      }  // ordinal (0..79)
	int   GetMaxConsY   (void)   { return (ConsoleYSize-1);      }
	int   GetAbsXPos    (void)   { return (CurrentXPos + WinX1); }  // ordinal
	int   GetAbsYPos    (void)   { return (CurrentYPos + WinY1); }

	// Logical, within current window. all ordinal (0..x) values
	int   GetXPos       (void)   { return (CurrentXPos);   }
	int   GetYPos       (void)   { return (CurrentYPos);   }
	int   GetMaxX       (void)   { return (WinX2 - WinX1); } 
	int   GetMaxY       (void)   { return (WinY2 - WinY1); } 
	int   GetMinX       (void)   { return (0);             } 
	int   GetMinY       (void)   { return (0);             }

	// BTW, I don't have "GetMinConsX/Y" functions. it would just do a return (0);

	// Misc. 'graceful error handling' functions. uses non-class functions declared below
	void FitInWinBounds (int *x, int *y);
	void FitInConsBounds (int *x, int *y);
	BOOL IsInWinBounds (int x, int y);
	BOOL IsInConsBounds (int x, int y);

	// Test patterns. For testing purposes mainly, of coursse
	void  TestPatternX      (void);
	void  TestPatternY      (void);
	void  TestPatternChars  (void);
	// void  TestPatterColors (void);

	// 'Low-level' input functions
	BOOL IsQEmpty ();
	BOOL IsQFull  ();
	BOOL ResizeQ          (int QSize);  // FYI this will flush the input queue
	int  GetQSize         (void)     { return (KeyQLength); }
	BOOL AddKeyToQ        (CIKey k);
    BOOL AddManyKeysToQ   (CIKey *keys, int Count);
	BOOL AddKeyToFrontOfQ (CIKey k);    // like unget()
	BOOL GetKeyFromQ      (CIKey *k);
    BOOL PeekAtKeyQ       (CIKey *k);   // returns first char in key queue, does not remove it

    void   SignalInput     (void)      { SetEvent (InputSignal); return;   } // sets wait object to say "yeah we've got input!"
    void   UnsignalInput   (void)      { ResetEvent (InputSignal); return; }
    HANDLE GetInputSignalObject (void) { return (InputSignal);             }
    void   SleepUntilInput (void)      { WaitForSingleObject (InputSignal, INFINITE); return; }

	DWORD  GetIdleTime     (void);
	void   ResetIdleTime   (void);

    // These handle control character handling for CTRL+C and CTRL+S
    // (quit program, pause program)
    BOOL GetConsoleExceptions (void)    { return (CTRLExceptions); }
    void SetConsoleExceptions (BOOL C)  { CTRLExceptions = C;      } // default is TRUE btw

    // called whenever GetChar is called, after KeyPressed() reports a key in the buffer
    // when CTRL+S is pressed, we wait until another key (any key, including CTRL+S)
    // to resume the program. if CTRL+C is pressed we do a   throw ("User Abort via CTRL+C");
    // if control key exception handling is disabled, we do nothing and simply return
    void DoControlExceptions (void);

    // Kernel checks each process' flag for stuff like "CON_ATTACH_NEXT" and then acts on them
    // 0 = no flag
    DWORD GetCurrentFlag (void)    { return (CurrentFlag); }
    void  SetCurrentFlag (DWORD f) { CurrentFlag = f;      }

	// if (char is > 127) then char -= 128;
	void SetStripHighAscii (BOOL a)  { StripHighAscii = a; }
	BOOL GetStripHighAscii (void)    { return (StripHighAscii); }

	// if (char < 32) then char = ' '
	void SetStripCtrlChars (BOOL c)  { StripCtrlChars = c; }
	BOOL GetStripCtrlChars (void)    { return (StripCtrlChars); }

	// if (char < 32) then print out ^[letter], i.e. if c==3 then "^C" would be printed
	void SetExpandCtrlChars(BOOL s)  { ExpandCtrlChars = s;      }
	BOOL GetExpandCtrlChars(void)    { return (ExpandCtrlChars); }

	void  SetStage1Idle (DWORD i)      { Stage1Idle = i;           }
	DWORD GetStage1Idle (void)         { return (Stage1Idle);      }
	void  SetStage2Idle (DWORD i)      { Stage2Idle = i;           }
	DWORD GetStage2Idle (void)         { return (Stage2Idle);      }
	void  SetStage2IdleDelay (DWORD i) { Stage2IdleDelay = i;      }
	DWORD GetStage2IdleDelay (void)    { return (Stage2IdleDelay); }
	void  SetScrollIdle (DWORD i)      { ScrollIdle = i;           }
	DWORD GetScrollIdle (void)         { return (ScrollIdle);      }
	void  SetScrollIdleFreq (DWORD i)  { ScrollIdleFreq = i;       }
	DWORD GetScrollIdleFreq (void)     { return (ScrollIdleFreq);  }

	void LockConsole (void)   { LockShObject (&ConsoleLock);   }
	void UnlockConsole (void) { UnlockShObject (&ConsoleLock); }

protected:
	void CheckTextScroll (BOOL Refresh, BOOL Unlock);  // used by WriteText ()

    HWND  cHWND;

	C_Memory *ConsMemHeap;

	ShObj ConsoleLock;

	C_Output *Cout;
	C_Input  *Cin;
    C_Pipe   *PipeIn;
    C_Pipe   *PipeOut;
    bool      PipeEcho;

    // Managed by ReadText
    bool RTInsert;
    std::vector<string> LineHistory;
    DWORD LineHistoryMax; // max lines in line history
    std::string ReadBuffer;
    std::string ReadMisc;
    
    DWORD CurrentFlag;

	BOOL StripHighAscii;  // print all chars as 7-bit
	BOOL StripCtrlChars;
	BOOL ExpandCtrlChars; // ie, print <31decimal as ^char, (ie, ^A, ^B ...)

	int   TabSize;     // default is 8
	BOOL  AutoRefresh; // TRUE or FALSE
	int   CursorType;  // TRUE or FALSE for now, but we may want to add stuff like cursor size

	BOOL ResizeLock;   // if TRUE, ResizeXY() will always return FALSE w/o doing jack

	// defines current clipping region
	int   WinX1;
	int   WinY1;
	int   WinX2;
	int   WinY2;

	int   SavedX1;
	int   SavedY1;
	int   SavedX2;
	int   SavedY2;
	int   SavedXPos;
	int   SavedYPos;

	// CurrentX/YPos are relative to the currently defined Window region
	// ordinal (0 ... 79)
	// To get absolute position, do CurrentXPos + WinX1, or CurrentYPos + WinY1
	int   CurrentXPos;
	int   CurrentYPos;

	// cardinal (1 ... 80)
	int   ConsoleXSize;
	int   ConsoleYSize;
	
	cText **ConsoleText; // The Console Data ... colors, text, etc.
	cFormat TextFormat;  // What WriteText() will use when writing next chars

	DWORD MaxWriteBuffer; // for WriteTextf ()

	// vars for 'low-level' functions
	// Key-pressed queue
	CIKey *KeyQ;
	signed int KeyQLength;
	signed int KeyQBegin;
	signed int KeyQEnd;

    BOOL CTRLExceptions;

	// used to determine idle time. as per GetTickCount()
	DWORD LastInputEvent;

	int ScrollCount;      // sometimes if you do a LOT of scrolling, it totally
	                      // bogs down the system. so every once in awhile we'll
	                      // do a Sleep(0) to be friendly to everyone else

	DWORD Stage1Idle;
	DWORD Stage2Idle;
	DWORD Stage2IdleDelay;

	DWORD ScrollIdle;
	DWORD ScrollIdleFreq;

    HANDLE InputSignal;
};




#endif // #ifndef CONSOLE_H
