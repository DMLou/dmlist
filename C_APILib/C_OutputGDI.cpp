/*****************************************************************************

  C_OutputGDI

  C_Output class derivative for Win32 GDI

*****************************************************************************/


#include "C_OutputGDI.h"
#include <algorithm>


C_OutputGDI::C_OutputGDI ()
: C_Output ()
{
    FontNormal = NULL;
    FontUnderline = NULL;

    CursorBlinkRate = 250;

    SetCurrentDC (0);
    SetBeep (MB_OK);
    SetBackFill (0);  // Default font does not need background erasing ("Backfill")
    cHWND = NULL;

    OutputMode = C_DIRECT;
    DefaultOutputMode = C_DIRECT;

    FeatureLineOut = FALSE;
    SetVersionString ("Consframe Win32 GDI Output Driver");

    strcpy (FName, "");

    HandleHighlight = true;
    EnableHighlight = false;
    HighlightType = 0;
    HLx1 = 0;
    HLy1 = 0;
    HLx2 = 0;
    HLy2 = 0;

    Plus1FudgeX = false;
    Plus1FudgeY = false;

    WText = NULL;
    WTextLength = 0;
    AText = NULL;
    ATextLength = 0;

    FontNormal = NULL;
    FontUnderline = NULL;

    // Default to ANSI text drawing, only switch to Unicode if they ask for it
    UseUnicode = false;
    return;
}


C_OutputGDI::~C_OutputGDI ()
{
    if (FontNormal != NULL)
        DeleteObject (FontNormal);

    if (FontUnderline != NULL)
        DeleteObject (FontUnderline);

    if (WText != NULL)
        free (WText);

    if (AText != NULL)
        free (AText);

    return;
}


BOOL C_OutputGDI::AttachConsole (Console *C)
{
    BOOL Success = FALSE;

    if (!(Success = C_Output::AttachConsole (C)))
        return (FALSE);
    else
    {	// TO DO: move to a more appropriate place?
        if (strlen(FName) == 0)
            SetFont ("Courier New", 16);

        FitConsoleInParent ();

        return (TRUE);
    }
}


// This function is called by Console::ResizeXY () to 'ask' if the new
// X/Y size is OK. For the GDI output driver, we've simply imposed
// some leniant restrictions. Some real restrictions for other drivers
// might be based on physical limitations ... ie, DOS text modes can
// not have just any random pairing of X,Y values, so they would return
// a FALSE if the X,Y were invalid.
BOOL C_OutputGDI::ResizeQuery (int XSize, int YSize)
{
    // check if the base class allows it
    if (!this->C_Output::ResizeQuery (XSize, YSize))
        return (FALSE);

    // otherwise we're fine with the new X, Y size
    return (TRUE);
}


// We handle clipboard pasting in a separate thread, because otherwise if you have a lot to
// paste, the primary thread (which handles moving the window around, quitting, etc) can't 
// do anything.
DWORD WINAPI ClipboardPaste (void *COutGDI)
{
    static bool Pasting = false;
    C_Output *GDI  = (C_Output *) COutGDI;
    Console          *Cons = GDI->GetConsole();

    if (Pasting)  // none of this 'let's start up 50 paste threads' :)
        ExitThread (0);

    if (!IsClipboardFormatAvailable(CF_TEXT))
        ExitThread (0);

    if (OpenClipboard (GDI->GetHWND()))
    {
        HANDLE Clip;
        char *Data;
        char *ClipData;
        DWORD StrLen;

        Pasting = true;

        Clip = GetClipboardData (CF_TEXT);

        // dammit, can't I use something besides GlobalLock() ?!

        ClipData = (char *) GlobalLock (Clip);
        Data = (char *) malloc (strlen (ClipData) + 1);
        strcpy (Data, ClipData);
        GlobalUnlock (Clip);
        CloseClipboard ();

        // The clipboard data is a bunch of text, separated by CR/LF
        // What we do is insert the data that's in the clipboard into the keyboard
        // input queue for the Console we've been given. We interpret CR as '13'
        // (Enter) and skip over LF. 
        StrLen = strlen(Data);
        for (DWORD i = 0; i < StrLen; i++)
        {
            char Paste;

            Paste = Data[i];

            if (Paste == 10) // 'LF'
            {
                continue;
            }

            if (Paste == 13) // 'CF'
            {
                //Sleep (10);
            }

            //PostMessage (GDI->GetHWND(), WM_CHAR, Paste, 0);
            try
            {
                while (Cons->IsQFull())
                    Sleep (10);

                Cons->AddKeyToQ (Paste);
            }
            catch (...)
            {
                break;
            }

            //Sleep (0);
        }

        free (Data);

        Pasting = false;
    }

    ExitThread (0);
    return (0);
}


// Our message hook.
BOOL C_OutputGDI::WndProc (UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL ReturnVal = FALSE;
    POINTS Point;
    bool DrawHighlight = false;
    DWORD ThreadId;

    switch (message)
    {
        PAINTSTRUCT ps;
        RECT Update;
        HDC OldDC;

    case WM_SIZE:
        DrawHighlight = false;
        EnableHighlight = false;
        break;

    case WM_PASTE:
        CreateThread (NULL, 4096, ClipboardPaste, (void *)this, 0, &ThreadId);
        break;

    // Repaint console frame after minimize/maximize/restore
    case WM_SHOWWINDOW:
        FitConsoleInParent ();
        Cons->RedrawAll ();
        ReturnVal = TRUE;
        break;

    // Repaint region
    case WM_PAINT:
        int fxsize;
        int fysize;

        if (!HandleHighlight)
            break;

        if (!GetUpdateRect (cHWND, &Update, FALSE))
        {	// update rectangle is blank
            ReturnVal = TRUE;
            break;
        }

        Cons->LockConsole();
        OldDC = GetCurrentDC (); // save old DC
        SetCurrentDC (BeginPaint (cHWND, &ps));


        // HACK: Doesn't seem to work quite right unless we redraw everything :(
        //       Oh well, GDI will clip it for us, although we are generating more work for ourself than is necessary
        /*
        fxsize = GetFXSize();
        fysize = GetFYSize();
        Cons->RedrawXY (Update.left / fxsize, Update.top / fysize,
        (Update.right + fxsize - 1) / fxsize , (Update.bottom + fysize - 1) / fysize);
        */

        Cons->RedrawAll ();

        EndPaint (cHWND, &ps);
        SetCurrentDC (OldDC); // restore old DC
        //ValidateRect (cHWND, &Update);
        Cons->UnlockConsole();

        ReturnVal = TRUE;
        break;

    case WM_MOUSEMOVE:
        if (!HandleHighlight)
            break;

        if (wParam & MK_LBUTTON)
        {
            Cons->LockConsole();

            Cons->FitInConsBounds (&HLx1, &HLy1);
            Cons->FitInConsBounds (&HLx2, &HLy2);

            RefreshHighlight (HighlightType, HLx1, HLy1, HLx2, HLy2, false, true);

            Point = MAKEPOINTS (lParam);
            HLx2 = Point.x / GetFXSize ();
            HLy2 = Point.y / GetFYSize ();

            Cons->FitInConsBounds (&HLx1, &HLy1);
            Cons->FitInConsBounds (&HLx2, &HLy2);

            RefreshHighlight (HighlightType, HLx1, HLy1, HLx2, HLy2, false, true);

            if (EnableHighlight)
                DrawHighlight = true;

            Cons->UnlockConsole();
        }

        break;

    case WM_LBUTTONDOWN:
        if (!HandleHighlight)
            break;

        SetCapture (GetHWND());
        EnableHighlight = true;

        if ((wParam & MK_SHIFT) || (wParam & MK_CONTROL))
            HighlightType = 1;
        else
            HighlightType = 0;

        Cons->FitInConsBounds (&HLx1, &HLy1);
        Cons->FitInConsBounds (&HLx2, &HLy2);

        RefreshHighlight (HighlightType, HLx1, HLy1, HLx2, HLy2, false, true);

        Point = MAKEPOINTS (lParam);
        HLx1 = HLx2 = Point.x / GetFXSize ();
        HLy1 = HLy2 = Point.y / GetFYSize ();

        Cons->FitInConsBounds (&HLx1, &HLy1);
        Cons->FitInConsBounds (&HLx2, &HLy2);

        RefreshHighlight (HighlightType, HLx1, HLy1, HLx2, HLy2, false, true);

        DrawHighlight = true;
        break;

    case WM_LBUTTONUP:
        Cons->FitInConsBounds (&HLx1, &HLy1);
        Cons->FitInConsBounds (&HLx2, &HLy2);

        RefreshHighlight (HighlightType, HLx1, HLy1, HLx2, HLy2, false, true);

        DrawHighlight = true;
        ReleaseCapture ();
        break;

    case WM_RBUTTONUP:
        if (!EnableHighlight) 
        {   // right click w/ no highlight region = paste
            // but only if they're holding down the shift or control key
            if ((wParam & MK_SHIFT) || (wParam & MK_CONTROL))
                CreateThread (NULL, 4096, ClipboardPaste, (void *)this, 0, &ThreadId);
        }
        else
        {   // right click w/ highlighted region = copy
            CopyHighlightedRegion ();
            ReleaseCapture ();
            EnableHighlight = false;
            DrawHighlight = true;
        }

        break;

    // most keyboard input invalidates the highlighted region
    case WM_CHAR:
        if (EnableHighlight)
        {
            if (wParam == 10  ||  wParam == 13  ||  wParam == 3)
                CopyHighlightedRegion ();

            ReleaseCapture ();
            EnableHighlight = false;
            DrawHighlight = true;
        }

        break;
    }

    if (DrawHighlight)
        InvalidateRect (Cons->GetHWND(), NULL, FALSE);

    return (ReturnVal);
}


void C_OutputGDI::ResizeNotify (void) 
{
//	FitConsoleInParent (); 
    return;
}


void C_OutputGDI::CopyHighlightedRegion (void)
{
    char *Data;
    HGLOBAL GlobalMem;
    int DataSize;
    int ByteSize;
    int RegionCount;
    RECT Regions[3];
    int Cursor;
    int i;

    if (!OpenClipboard (GetHWND()))
        return;

    // Remove the data from the clipboard
    EmptyClipboard ();

    // Get a buffer for out data
    GetHighlightRegions (HighlightType, HLx1, HLy1, HLx2, HLy2, &RegionCount, &Regions[0],
        &Regions[1], &Regions[2]);

    DataSize = 0;
    for (i = 0; i < RegionCount; i++)
        DataSize += ((Regions[i].right - Regions[i].left) + 1) * ((Regions[i].bottom - Regions[i].top) + 1);

    // If box-highlight, we want a \r\n at the end of every line
    // And we know that RegionCount == 1, so only Regions[0] is applicable.
    if (HighlightType == 0)
        DataSize += 2 * (Regions[0].bottom - Regions[0].top + 1);

    ByteSize = (DataSize + 1) * sizeof (char);
    Data = (char *) malloc (ByteSize);

    // Copy data
    memset (Data, 0, (DataSize + 1) * sizeof (char));
    Cursor = 0;

    Cons->LockConsole();

    for (i = 0; i < RegionCount; i++)
    {
        int x, y;

        for (y = Regions[i].top; y <= Regions[i].bottom; y++)
        {
            for (x = Regions[i].left; x <= Regions[i].right; x++)
            {
                Data[Cursor] = (char) Cons->PeekChar (x, y);
                Cursor++;
            }

            // For box-highlight, add \n at the end of each line except the last
            if (HighlightType == 0)
            {
                // Only append carriage returns if we're going to copy more than 1 line
                if (!(RegionCount == 1  &&  (Regions[0].bottom == Regions[0].top)))
                {
                    Data[Cursor] = '\r';
                    Cursor++;
                    Data[Cursor] = '\n';
                    Cursor++;
                }
            }
        }
    }

    Cons->UnlockConsole();

    // Allocate global memory
    GlobalMem = GlobalAlloc (GMEM_MOVEABLE, ByteSize);
    if (GlobalMem != NULL)
    {
        char *GlobalPointer;

        GlobalPointer = (char *) GlobalLock (GlobalMem);
        if (GlobalPointer != NULL)
        {
            memcpy (GlobalPointer, Data, ByteSize);
            GlobalUnlock (GlobalMem);
            SetClipboardData (CF_TEXT, GlobalMem);
        }
    }

    free (Data);

    CloseClipboard ();
    return;
}


#ifdef GDISHOWREFRESH
#pragma message ("GDI output driver will draw boxes around refresh points")
#define SHOWREFRESH RECT blah; \
		blah.top = Y*GetFYSize(); \
		blah.left = (X*GetFXSize()) - (SPos*GetFXSize()); \
		blah.bottom = (Y*GetFYSize())+GetFYSize(); \
		blah.right = X*GetFXSize(); \
		FrameRect (hdc,&blah,(HBRUSH)GetStockObject (WHITE_BRUSH));
#else
#define SHOWREFRESH
#endif

/*
#define FLUSH() { \
	if (!(SPos == 0 || (SXOfs >= CWidth))) \
	{ \
		SetTextColor (hdc, CurFG); \
		SetBkColor (hdc, CurBG); \
		TextOut (hdc, SXOfs * GetFXSize(), Y * GetFYSize(), BText, SPos); \
		SHOWREFRESH; \
	} \
	SPos = 0; \
	SXOfs = X; \
	CurFG = Cons->PeekFormat (X, Y).Foreground; \
	CurBG = Cons->PeekFormat (X, Y).Background; }
*/


#define FLUSH(F)  \
    { \
        if (!(SPos == 0  ||  (SXOfs >= CWidth))) \
        { \
            DrawTextGDI##F (hdc, F##Text, SPos, CurFmt, SXOfs, Y); \
        } \
        SPos = 0; \
        SXOfs = X; \
        CurFmt = Cons->PeekFormat (min(X, Cons->GetMaxConsX()), Y); \
    }


// send absolute console coordinates
void C_OutputGDI::RefreshXY (int X1, int Y1, int X2, int Y2)
{
    HDC hdc;
    HDC CurHDC;
    RECT rt;
    HGDIOBJ OldFont;
    int AbsX;
    int AbsY;
    DWORD CWidth;
    DWORD CHeight;
    DWORD SPos;
    DWORD SXOfs;
    BOOL Flush;
    cFormat CurFmt;

    Cons->LockConsole ();

    // If we have a highlight region, invert its fore/back
    if (EnableHighlight)
        RefreshHighlight (HighlightType, HLx1, HLy1, HLx2, HLy2, true, false);

    Cons->FitInConsBounds (&X1, &Y1);
    Cons->FitInConsBounds (&X2, &Y2);
    CorrectOrder (&X1, &Y1, &X2, &Y2);

    AbsX = Cons->GetAbsXPos ();
    AbsY = Cons->GetAbsYPos ();
    Cons->PokeRefresh (AbsX, AbsY) = true;

    GetClientRect (cHWND, &rt);

    // Use the set HDC, or if none then get one
    CurHDC = GetCurrentDC ();

    if (CurHDC == 0)
        hdc = GetDC (cHWND);
    else
        hdc = GetCurrentDC ();

    CWidth = Cons->GetConsWidth();
    CHeight = Cons->GetConsHeight();
    Flush = FALSE;
    SPos = 0;  // 'place to put NEXT char' ... so, SPos == Length-1
    SXOfs = 0; // starting console X position

    if (UseUnicode)
        ResizeWText ((CWidth + 2) * sizeof(cChar));
    else
        ResizeAText ((CWidth + 2) * sizeof(char));

    // This is where we buffer our TextOut() calls
    for (int Y = Y1; Y <= Y2; Y++)
    {
        // another diag tool ... force everything to refresh
        /*
        for (int xr = X1; xr <= X2; xr++)
        Cons->PokeRefresh (xr, Y) = TRUE;
        */

        // if this row hasn't changed, don't even bother with it
        if (!Cons->HasRegionChanged (X1, Y, X2, Y))
            continue;

        /* // just for diagnostics
        Cons->PokeChar (0, Y) = Cons->PokeChar (0, Y) + 1;
        Cons->PokeRefresh (0, Y) = TRUE;
        */

        CurFmt = Cons->PeekFormat (X1, Y);

        SPos = 0;
        SXOfs = X1;

        int X;
        for (X = X1; X <= X2; X++)
        {	
            cFormat PeekFmt;

            if (!Cons->PeekRefresh(X,Y))
            {	// we increment SXOfs because FLUSH() sets the X offset to the *current*
                // X position, but we want it to start on the *next* X position
                if (UseUnicode)
                    FLUSH(W)
                else
                FLUSH(A);

                SXOfs++;
                continue;
            }

            PeekFmt = Cons->PeekFormat (X, Y);
            if (!CFORMATEQ (CurFmt, PeekFmt))
            {
                if (UseUnicode)
                    FLUSH(W)
                else
                FLUSH(A);
            }

            if (UseUnicode)
                WText[SPos] = Cons->PeekChar(X, Y);
            else
                AText[SPos] = (char) Cons->PeekChar(X, Y);

            SPos++;

            Cons->PokeRefresh (X,Y) = FALSE;
        }

        // we gotta flush for every row
        if (UseUnicode)
            FLUSH(W)
        else
        FLUSH(A);
    }

#undef FLUSH

    DrawCursor ();
    Cons->PokeRefresh (AbsX, AbsY) = true;

    if (CurHDC == 0)
        ReleaseDC (cHWND, hdc);

    // If we have a highlight region, invert its fore/back
    if (EnableHighlight)
        RefreshHighlight (HighlightType, HLx1, HLy1, HLx2, HLy2, true, false);

    Cons->UnlockConsole ();
    return;
}

void C_OutputGDI::GetHighlightRegions (int HighlightType, int x1, int y1, int x2, int y2,
                                       int *CountResult,
                                       RECT *Region1,
                                       RECT *Region2,
                                       RECT *Region3)
{
    int x;

    if (HighlightType == 0)
    {   // rectangular highlight
        *CountResult = 1;
        Region1->left = min (x1, x2);
        Region1->top = min (y1, y2);
        Region1->right = max (x1, x2);
        Region1->bottom = max (y1, y2);
    }
    else
    {
        if (y1 == y2) // only one line?
        {
            *CountResult = 1;
            Region1->left = min (x1, x2);
            Region1->top = min (y1, y2);
            Region1->right = max (x1, x2);
            Region1->bottom = max (y1, y2);
        }
        else
        {
            int topx, topy;
            int botx, boty;
            RECT *ThirdLine;

            if (HLy1 > HLy2)
            {
                topx = HLx2;
                topy = HLy2;
                botx = HLx1;
                boty = HLy1;
            }
            else
            {
                topx = HLx1;
                topy = HLy1;
                botx = HLx2;
                boty = HLy2;
            }

            ThirdLine = Region2;
            *CountResult = 2;

            // Top line
            for (x = topx; x <= Cons->GetMaxConsX(); x++)
            {
                Region1->left = topx;
                Region1->right = Cons->GetMaxConsX();
                Region1->top = topy;
                Region1->bottom = topy;
            }

            // Middle box
            if ((boty - topy) > 1)
            {
                Region2->left = 0;
                Region2->right = Cons->GetMaxConsX();
                Region2->top = topy + 1;
                Region2->bottom = boty - 1;
                ThirdLine = Region3;
                *CountResult = 3;
            }

            // Bottom line
            for (x = 0; x <= botx; x++)
            {
                ThirdLine->left = 0;
                ThirdLine->right = botx;
                ThirdLine->top = boty;
                ThirdLine->bottom = boty;
            }
        }
    }
    return;
}


void C_OutputGDI::RefreshHighlight (int HighlightType, int x1, int y1, int x2, int y2, 
                                    bool SetColors, bool SetRefresh)
{
    int RegionCount;
    RECT Regions[3];
    int i;

    GetHighlightRegions (HighlightType, x1, y1, x2, y2, &RegionCount, &Regions[0], &Regions[1], &Regions[2]);

    for (i = 0; i < RegionCount; i++)
    {
        int x, y;

        for (y = Regions[i].top; y <= Regions[i].bottom; y++)
        {
            for (x = Regions[i].left; x <= Regions[i].right; x++)
            {
                if (SetColors)
                {
                    Cons->PokeFormat (x, y).Foreground = 0xffffff - Cons->PeekFormat (x, y).Foreground;
                    Cons->PokeFormat (x, y).Background = 0xffffff - Cons->PeekFormat (x, y).Background;
                }

                if (SetRefresh)
                    Cons->PokeRefresh (x, y) = true;
            }
        }
    }

    return;
}


void C_OutputGDI::DrawCursor (void)
{
    HDC hdc;
    HBRUSH White;
    HBRUSH OldBrush;
    HPEN Pen;
    HPEN OldPen;
    HGDIOBJ OldFont;
    int CX;
    int CY;
    cChar Char;

    Cons->LockConsole ();

    if (GetCurrentDC() == 0)
        hdc = GetDC (cHWND);
    else
        hdc = GetCurrentDC ();

    // We want white brush, no pen
    //Pen = CreatePen (PS_SOLID, 0, RGB (255, 255, 255));
    Pen = CreatePen (PS_SOLID, 0, Cons->GetForeground());
    OldPen = (HPEN) SelectObject (hdc, (HGDIOBJ) Pen);

    White = CreateSolidBrush (Cons->GetForeground());
    OldBrush = (HBRUSH) SelectObject (hdc, (HGDIOBJ) White);

    // Set the font
    OldFont = SelectObject (hdc, FontNormal);

    CX = Cons->GetAbsXPos ();
    CY = Cons->GetAbsYPos ();

    // Draw the text that is here
    SetTextColor (hdc, Cons->PeekFormat (CX, CY).Foreground);
    SetBkColor (hdc, Cons->PeekFormat (CX, CY).Background);
    Char = Cons->PeekChar (CX, CY);
    TextOutW (hdc, CX * GetFXSize(), CY * GetFYSize(), &Char, 1);

    // Blink cursor
    if (Cons->GetCursor() && ((GetTickCount() / CursorBlinkRate) % 2))
    {
        //SetTextColor (hdc, RGB (255, 255, 255));
        //TextOut (hdc, CX * GetFXSize(), CY * GetFYSize(), "_", 1);
        Rectangle (hdc, CX * FontXSize, ((CY + 1) * FontYSize) - 1, ((CX + 1) * FontXSize) - 1, ((CY + 1) * FontYSize) - (FontYSize / 4));
    }

    SelectObject (hdc, OldFont);
    SelectObject (hdc, OldBrush);
    SelectObject (hdc, OldPen);
    DeleteObject (White);
    DeleteObject (Pen);

    if (GetCurrentDC() == 0)
        ReleaseDC (cHWND, hdc);

    Cons->UnlockConsole ();
    return;
}


void C_OutputGDI::DrawTextGDIA (HDC hdc, const char *Text, int TextLength, cFormat Format, int X, int Y)
{
    HFONT Font;
    HGDIOBJ OldFont;

    if (Format.Underline)
        Font = FontUnderline;
    else
        Font = FontNormal;

    OldFont = SelectObject (hdc, (HGDIOBJ)Font);
    SetTextColor (hdc, Format.Foreground);
    SetBkColor (hdc, Format.Background);
    TextOut (hdc, X * GetFXSize(), Y * GetFYSize(), Text, TextLength);
    SelectObject (hdc, OldFont);

    return;
}


void C_OutputGDI::DrawTextGDIW (HDC hdc, const cChar *Text, int TextLength, cFormat Format, int X, int Y)
{
    HFONT Font;
    HGDIOBJ OldFont;

    if (Format.Underline)
        Font = FontUnderline;
    else
        Font = FontNormal;

    OldFont = SelectObject (hdc, (HGDIOBJ)Font);
    SetTextColor (hdc, Format.Foreground);
    SetBkColor (hdc, Format.Background);
    TextOutW (hdc, X * GetFXSize(), Y * GetFYSize(), Text, TextLength);
    SelectObject (hdc, OldFont);

    return;
}


static int CALLBACK BuildFontList (const ENUMLOGFONTEX *lpelfe,
                                   const NEWTEXTMETRICEX *lpntme,
                                   DWORD FontType,         
                                   LPARAM lParam)
{
    vector<LOGFONT> &fonts = *((vector<LOGFONT> *)lParam);

    if (((lpelfe->elfLogFont.lfPitchAndFamily & 0x3) == FIXED_PITCH))
        fonts.push_back (lpelfe->elfLogFont);

    return (1);
}


bool FindFont (HDC hdc, const char *Name, LOGFONT *logFontResult)
{
    vector<LOGFONT> allFonts;
    vector<LOGFONT> matchingFonts;
    LOGFONT logFont;
    int result;

    ZeroMemory (&logFont, sizeof(logFont));
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfFaceName[0] = '\0';
    logFont.lfPitchAndFamily = FIXED_PITCH | FF_DECORATIVE | FF_DONTCARE | FF_MODERN | FF_ROMAN | FF_SCRIPT | FF_SWISS;
    result = EnumFontFamiliesExA (hdc, &logFont, (FONTENUMPROC)BuildFontList, (LPARAM)&allFonts, 0);

    for (vector<LOGFONT>::iterator it = allFonts.begin(); it != allFonts.end(); ++it)
    {
        if (0 == stricmp (Name, it->lfFaceName))
            matchingFonts.push_back (*it);
    }

    // Do a pseudo sort ... prefer OEM, then ANSI, charsets
    for (vector<LOGFONT>::iterator it = matchingFonts.begin(); it != matchingFonts.end(); ++it)
    {
        if (it->lfCharSet == ANSI_CHARSET)
            swap (*matchingFonts.begin(), *it);
    }

    for (vector<LOGFONT>::iterator it = matchingFonts.begin(); it != matchingFonts.end(); ++it)
    {
        if (it->lfCharSet == OEM_CHARSET)
            swap (*matchingFonts.begin(), *it);
    }

    if (!matchingFonts.empty())
        *logFontResult = matchingFonts.front();

    return (!matchingFonts.empty());
}


HFONT MakeFont (HDC hdc, const char *Name, int Size, bool Underline)
{
    LOGFONT logFont;
    DWORD CharSet;

    CharSet = ANSI_CHARSET;

    if (FindFont (hdc, Name, &logFont))
        CharSet = logFont.lfCharSet;

    return (CreateFont (Size, 0, 0, 0, FW_NORMAL, 0,  Underline ? TRUE : 0, 0, CharSet,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, Name));
}


void C_OutputGDI::SetFont (char *FontName, int FontSize)
{
    HDC hdc;
    HDC CurHDC;
    HGDIOBJ OldFont = NULL;
    SIZE GTEPSize;

    DeleteObject (FontNormal);
    DeleteObject (FontUnderline);

    CurHDC = GetCurrentDC ();

    // Get font X/Y sizes
    if (CurHDC == 0)  // if the current DC has been set
        hdc = GetDC (cHWND);
    else
        hdc = GetCurrentDC ();

    FontNormal = MakeFont (hdc, FontName, FontSize, false);
    FontUnderline = MakeFont (hdc, FontName, FontSize, true);

    OldFont = SelectObject (hdc, FontNormal);
    SelectObject (hdc, FontNormal);
    GetTextMetrics (hdc, &FontMetrics);
    GetTextExtentPoint32 (hdc, "*", 1, &GTEPSize);
    SelectObject (hdc, OldFont);

    //FontXSize = FontMetrics.tmMaxCharWidth + FontMetrics.tmOverhang;
    FontXSize = GTEPSize.cx;
    FontYSize = FontMetrics.tmHeight;

    this->FontSize = FontYSize;

    strncpy (FName, FontName, sizeof (FName));

    if (CurHDC == 0)
        ReleaseDC (cHWND, hdc);

    return;
}


int  C_OutputGDI::GetXPixSize (void)
{
    return (Cons->GetConsWidth() * FontXSize);
}


int  C_OutputGDI::GetYPixSize (void)
{
    return (Cons->GetConsHeight() * FontYSize);
}


void C_OutputGDI::FitConsoleInParent (void)
{
    RECT rClient, rWin;
    int NewXSize, NewYSize;

    GetClientRect (cHWND, &rClient);
    GetWindowRect (cHWND, &rWin);

    // NewSize = Width of Console in pixels + Width of window border+menu
    NewXSize = GetXPixSize() + (rClient.left - rWin.left) + (rWin.right  - rClient.right);
    NewYSize = GetYPixSize() + (rClient.top  - rWin.top)  + (rWin.bottom - rClient.bottom);

    if (Plus1FudgeX)
        NewXSize += GetFXSize ();

    if (Plus1FudgeY)
        NewYSize += GetFYSize ();

    SetWindowPos (cHWND, HWND_NOTOPMOST, 0, 0, NewXSize, NewYSize, /*SWP_NOCOPYBITS |*/ SWP_NOMOVE);

    //Cons->RedrawAll ();
    return;
}


// Called by Console::ScrollRegion. Typically an output driver will have
// a 'better' way to scroll, via hardware or whatnot, other than a brute
// force 'redraw everything that's changed' method
void C_OutputGDI::ScrollRegion (int XDir, int YDir, int X1, int Y1, int X2, int Y2)
{
    // "Physical" scrolling
    RECT ScrollRegion;
    RECT UpdateRect;

    HDC CurHDC = 0;
    HDC hdc = 0;

    if (EnableHighlight)
    {
        //RefreshXY (X1, Y1, X2, Y2);
        return;
    }

    // Use the set HDC, or if none then get one
    CurHDC = GetCurrentDC ();

    if (CurHDC == 0)
        hdc = GetDC (cHWND);
    else
        hdc = GetCurrentDC ();

    ScrollRegion.left   = X1 * FontXSize;
    ScrollRegion.right  = (X2 * FontXSize) + FontXSize;
    ScrollRegion.top    = Y1 * FontYSize;
    ScrollRegion.bottom = (Y2 * FontYSize) + FontYSize;

    ScrollWindowEx (cHWND, XDir * FontXSize, YDir * FontYSize, &ScrollRegion, &ScrollRegion, NULL, &UpdateRect, 0);

    // change to text coordinates, then we'll set these refresh flags to TRUE way at
    // the end of the function
    UpdateRect.left  /= FontXSize;
    UpdateRect.top   /= FontYSize;

    UpdateRect.right /= FontXSize;
    UpdateRect.right--;

    UpdateRect.bottom /= FontYSize;
    //UpdateRect.bottom--;

    Cons->FitInConsBounds ((int *)&UpdateRect.left, (int *)&UpdateRect.top);
    Cons->FitInConsBounds ((int *)&UpdateRect.right, (int *)&UpdateRect.bottom);

    if (CurHDC == 0)
        ReleaseDC (cHWND, hdc);

    // We have effectively refreshed the scrolled area, so set those Refresh() flags
    // to FALSE, but TRUE for the "newly uncovered" area

    // what we will do is assign (x1, y1) - (x2, y2) to refresh as TRUE,
    // while (NewX1, NewY1) - (NewX2, NewY2) is FALSE
    int NewX1;
    int NewY1;
    int NewX2;
    int NewY2;
    int x,y;

    // set the false refresh area
    if (XDir > 0)
    {	// scrolling right
        NewX1 = X1 + XDir;
        NewX2 = X2;
    }
    else
    {	// scrolling left/not scrolling
        NewX1 = X1;
        NewX2 = X2 + XDir;
    }

    if (YDir > 0)
    {	// scrolling down
        NewY1 = Y1 + YDir;
        NewY2 = Y2;
    }
    else
    {	// scrolling up/not scrolling
        NewY1 = Y1;
        NewY2 = Y2 + YDir;
    }

    for (y = Y1; y <= Y2; y++)
        for (x = X1; x <= X2; x++)
            Cons->PokeRefresh (x,y) = TRUE;

    for (y = NewY1; y <= NewY2; y++)
        for (x = NewX1; x <= NewX2; x++)
            Cons->PokeRefresh (x,y) = FALSE;

    // Now, using the UpdateRect rectangle from way up, set THOSE to true.
    for (y = UpdateRect.top; y <= UpdateRect.bottom; y++)
        for (x = UpdateRect.left; x <= UpdateRect.right; x++)
            Cons->PokeRefresh (x,y) = TRUE;

    return;
}
