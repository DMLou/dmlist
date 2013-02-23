#ifndef CONSOLE_OUTPUT_GDI_H
#define CONSOLE_OUTPUT_GDI_H


#include "C_Linkage.h"
#include "Console.h"


class CAPILINK C_OutputGDI : public C_Output
{
public:
	C_OutputGDI ();
	~C_OutputGDI ();

	BOOL AttachConsole (Console *C);
	BOOL WndProc (UINT message, WPARAM wParam, LPARAM lParam);

	void RefreshXY     (int X1, int Y1, int X2, int Y2); // uses ordinal (ie, 0..79) values
	void ScrollRegion (int XDir, int YDir, int X1, int Y1, int X2, int Y2);

	void ResizeNotify (void);

	BOOL ResizeQuery (int XSize, int YSize);

    // Draws the cursor.
    void DrawCursor (void);
    int  GetCursorBlinkRate (void)  { return (CursorBlinkRate); }
    void SetCursorBlinkRate (int B) { CursorBlinkRate = B;      }

	void  SetSimpleCursor (int c)       { SimpleCursor = c;        }
	int   GetSimpleCursor (void)        { return (SimpleCursor);   }
	// Set to FALSE for faster redraw. Some fonts or font sizes will need it,
	// some won't.
	void  SetBackFill (BOOL B)          { EraseBack = B;           }
	BOOL  GetBackFill (void)            { return (EraseBack);      }

	void  ToggleBackFill ()             { EraseBack = (EraseBack == FALSE);     }

	// Font Management. Output implementation
	void  SetFont       (char *FontName, int FontSize);
	int   GetFXSize     (void)   { return (FontXSize); }
	int   GetFYSize     (void)   { return (FontYSize); }
	int   GetFSize      (void)   { return (FontSize);  }
	char *GetFontName   (void)   { return (FName);     }
	void  FitConsoleInParent (void);  // sizes the window (cHWND) to fit the console
	int   GetXPixSize        (void);  // size of console, in pixels
	int   GetYPixSize        (void);  // same

	void  SetCurrentDC  (HDC h)  { CurrentDC = h;      }
	HDC   GetCurrentDC  (void)   { return (CurrentDC); }

    void SetPlus1FudgeX (bool NewVal) { Plus1FudgeX = NewVal; return; }
    bool GetPlus1FudgeX (void)        { return (Plus1FudgeX);         }
    void SetPlus1FudgeY (bool NewVal) { Plus1FudgeY = NewVal; return; }
    bool GetPlus1FudgeY (void)        { return (Plus1FudgeY);         }

    void CopyHighlightedRegion (void);

    bool IsARegionHighlighted (void)
    {
        return (EnableHighlight);
    }

    void DoClipboardPaste (void)
    {
        SendMessage (GetHWND(), WM_PASTE, 0, 0);
        return;
    }

    void DoClipboardCopy (void)
    {
        if (EnableHighlight)
            SendMessage (GetHWND(), WM_RBUTTONUP, 0, 0);

        return;
    }

    bool GetHandleHighlight (void)
    {
        return (HandleHighlight);
    }

    void SetHandleHighlight (bool NewHH)
    {
        HandleHighlight = NewHH;

        if (NewHH == false)
        {
            ReleaseCapture ();
            EnableHighlight = false;
            InvalidateRect (Cons->GetHWND(), NULL, FALSE);
        }

        return;
    }

    bool IsUsingUnicode (void)
    {
        return (UseUnicode);
    }

    void SwitchToUnicode (void)
    {
        UseUnicode = true;
        return;
    }

    void SwitchToANSI (void)
    {
        UseUnicode = false;
        return;
    }

protected:
    void RefreshHighlight (int HighlightType, int x1, int y1, int x2, int y2, bool SetColors = true, bool SetRefresh = false);
    void GetHighlightRegions (int HighlightType, int x1, int y1, int x2, int y2,
                              int *CountResult,
                              RECT *Region1,
                              RECT *Region2,
                              RECT *Region3);

    void DrawTextGDIA (HDC hdc, const char *Text, int TextLength, cFormat Format, int X, int Y);
    void DrawTextGDIW (HDC hdc, const cChar *Text, int TextLength, cFormat Format, int X, int Y);

    bool UseUnicode;      // We use TextOutW in WinNT/2K/XP, and TextOutA in 95/98/ME

	int   FontXSize;      // Font X Size
	int   FontYSize;      // Font Y Size
	int   FontSize;       // equal to int FontSize from SetFont()
	char  FName[33];
    int   CursorBlinkRate; // like, every 250ms or 500ms

	HDC CurrentDC;        // If 0, use GetDC(), otherwise use what's in here.
	                      // WndProc() will process WM_PAINT and set this var
	                      // to what's returned by BeginPaint(), and then reset
	                      // it to 0.

	TEXTMETRIC FontMetrics;

    // Font objects
	HFONT FontNormal;
    HFONT FontUnderline;

	BOOL EraseBack;

	BOOL  FlashWin;
	int   SimpleCursor;  // CURSOR_NEVER, EVERY_WRITE, or EVERY_CHAR (see above)

    // Highlight region
    bool HandleHighlight;
    bool EnableHighlight;
    int  HighlightType; // 0 = box, 1 = "wrapping text"
    int  HLx1;
    int  HLy1;
    int  HLx2;
    int  HLy2;

    // When doing FitConsoleInParent, actually resize to Height+1
    // Useful for when doing a maximize window
    bool Plus1FudgeX;
    bool Plus1FudgeY;

    // Used in RefreshXY
    int WTextLength;
    cChar *WText;
    int ATextLength;
    char  *AText;

    void ResizeWText (int NewSize)
    {
        if (NewSize > WTextLength)
        {
            WText = (cChar *) realloc (WText, NewSize);
            WTextLength = NewSize;
        }

        return;
    }

    void ResizeAText (int NewSize)
    {
        if (NewSize > ATextLength)
        {
            AText = (char *) realloc (AText, NewSize);
            ATextLength = NewSize;
        }

        return;
    }
};



#endif