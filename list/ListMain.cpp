#define _WIN32_WINDOWS 0x0410
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include "List.h"
#include "CLinesDB.h"
#include "resource.h"
//#include "Preferences.h"
#include "PreferencesV2.h"
#include "AboutBox.h"
#include "Settings.h"
#include "VarList.h"
#include "ShlObj.h"
#include "Shlwapi.h"
#include "NoticeBox.h"
#include "Pipe.h"


#define MEMORYALIGN 16


// Only use aligned new/delete for release builds
#ifdef NDEBUG
void *operator new (size_t size) throw (bad_alloc)
{
    void *ret;

    ret = _aligned_malloc (size, MEMORYALIGN);

    if (ret == NULL)
        guard_throw (bad_alloc());

    return (ret);
}

void operator delete (void *block)
{
    return (_aligned_free (block));
}
#endif


void TurnOnTimer (HWND Hwnd)
{
    guard
    {
        SetTimer (Hwnd, 1, 100, NULL);
        return;
    } unguard;
}


void TurnOffTimer (HWND Hwnd)
{
    guard
    {
        KillTimer (Hwnd, 1);
        return;
    } unguard;
}


bool HaveMMX = false;
bool HaveSSE = false;
bool HaveSSE2 = false;
int CPUCount;


void StreamCopyMMX (void *Dst, void *Src, uint32 Bytes)
{
    guard
    {
            __asm
        {
	        mov edi, Dst
	        mov esi, Src
	        mov edx, Bytes
            mov ecx, edx
	        shr ecx, 6
	        jz  MMXCopyLoop2_8a

        // First loop: 64 bytes per iteration
        MMXCopyLoop1_64:

            // Read
            movq mm0, [esi]
            movq mm1, [esi + 8]
            movq mm2, [esi + 16]
            movq mm3, [esi + 24]
            movq mm4, [esi + 32]
            movq mm5, [esi + 40]
            movq mm6, [esi + 48]
            movq mm7, [esi + 56]

            // Write
            movq [edi], mm0
            movq [edi + 8], mm1
            movq [edi + 16], mm2
            movq [edi + 24], mm3
            movq [edi + 32], mm4
            movq [edi + 40], mm5
            movq [edi + 48], mm6
            movq [edi + 56], mm7

            // Loop
            add esi, 64
            add edi, 64

            dec ecx
            jnz MMXCopyLoop1_64

        // Second loop: 8 bytes per iteration
        MMXCopyLoop2_8a:
	        mov ecx, edx
	        and ecx, 63
	        shr ecx, 3
	        cmp ecx, 0
	        jz MMXCopyLoop3_1a

        MMXCopyLoop2_8b:
            movq mm0, [esi]
            movq [edi], mm0

	        add esi, 8
	        add edi, 8

	        dec ecx
	        jnz MMXCopyLoop2_8b

        // Third loop: 1 byte per iteration
        MMXCopyLoop3_1a:
	        mov ecx, edx
	        and ecx, 7
	        cmp ecx, 0
	        jz MMXCopyLoopDone

        MMXCopyLoop3_1b:
	        mov ah, [esi]
	        mov [edi], ah

	        inc esi
	        inc edi

	        dec ecx
	        jnz MMXCopyLoop3_1b

        MMXCopyLoopDone:
            emms
        }

        return;
    } unguard;
}


void StreamCopySSE (void *Dst, void *Src, uint32 Bytes)
{
    guard
    {
        __asm
        {
	        mov edi, Dst
	        mov esi, Src
	        mov edx, Bytes
            mov ecx, edx
	        shr ecx, 6
	        jz  SSECopyLoop2_8a

        // First loop: 64 bytes per iteration
        SSECopyLoop1_64:

	        prefetchnta [esi+256]
	        prefetchnta [esi+288]

            // Read
            movq mm0, [esi]
            movq mm1, [esi + 8]
            movq mm2, [esi + 16]
            movq mm3, [esi + 24]
            movq mm4, [esi + 32]
            movq mm5, [esi + 40]
            movq mm6, [esi + 48]
            movq mm7, [esi + 56]

            // Write
            movntq [edi], mm0
            movntq [edi + 8], mm1
            movntq [edi + 16], mm2
            movntq [edi + 24], mm3
            movntq [edi + 32], mm4
            movntq [edi + 40], mm5
            movntq [edi + 48], mm6
            movntq [edi + 56], mm7

            // Loop
            add esi, 64
            add edi, 64

            dec ecx
            jnz SSECopyLoop1_64

        // Second loop: 8 bytes per iteration
        SSECopyLoop2_8a:
	        mov ecx, edx
	        and ecx, 63
	        shr ecx, 3
	        cmp ecx, 0
	        jz SSECopyLoop3_1a

        SSECopyLoop2_8b:
            movq mm0, [esi]
            movntq [edi], mm0

	        add esi, 8
	        add edi, 8

	        dec ecx
	        jnz SSECopyLoop2_8b

        // Third loop: 1 byte per iteration
        SSECopyLoop3_1a:
	        mov ecx, edx
	        and ecx, 7
	        cmp ecx, 0
	        jz SSECopyLoopDone

        SSECopyLoop3_1b:
	        mov ah, [esi]
	        mov [edi], ah

	        inc esi
	        inc edi

	        dec ecx
	        jnz SSECopyLoop3_1b

        SSECopyLoopDone:
            emms
        }

        return;
    } unguard;
}


void StreamCopySSE2 (void *Dst, void *Src, uint32 Bytes)
{
    guard
    {
        __asm
        {
	        mov edi, Dst
	        mov esi, Src
	        mov edx, Bytes
            mov ecx, edx
	        shr ecx, 7
	        jz  SSE2CopyLoop2_16a

            // Decide between unaligned or aligned version (aligned version is *way* faster, btw)
            test edi, 15
            jnz SSE2UnalignedCopyLoop1_128
            test esi, 15
            jnz SSE2UnalignedCopyLoop1_128
            jmp SSE2AlignedCopyLoop1_128

        // First loop: 128 bytes per iteration, UNALIGNED VERSION
        SSE2UnalignedCopyLoop1_128:

	        prefetchnta [esi+256]

            // Read
            movups xmm0, [esi]
            movups xmm1, [esi + 16]
            movups xmm2, [esi + 32]
            movups xmm3, [esi + 48]
            movups xmm4, [esi + 64]
            movups xmm5, [esi + 80]
            movups xmm6, [esi + 96]
            movups xmm7, [esi + 112]

            // Write
            movups [edi], xmm0
            movups [edi + 16], xmm1
            movups [edi + 32], xmm2
            movups [edi + 48], xmm3
            movups [edi + 64], xmm4
            movups [edi + 80], xmm5
            movups [edi + 96], xmm6
            movups [edi + 112], xmm7

            // Loop
            add esi, 128
            add edi, 128
            sub ecx, 1
            jnz SSE2UnalignedCopyLoop1_128
            jmp SSE2CopyLoop2_16a

        // First loop: 128 bytes per iteration, ALIGNED VERSION
        SSE2AlignedCopyLoop1_128:

	        prefetchnta [esi+256]

            // Read
            movaps xmm0, [esi]
            movaps xmm1, [esi + 16]
            movaps xmm2, [esi + 32]
            movaps xmm3, [esi + 48]
            movaps xmm4, [esi + 64]
            movaps xmm5, [esi + 80]
            movaps xmm6, [esi + 96]
            movaps xmm7, [esi + 112]

            // Write
            movaps [edi], xmm0
            movaps [edi + 16], xmm1
            movaps [edi + 32], xmm2
            movaps [edi + 48], xmm3
            movaps [edi + 64], xmm4
            movaps [edi + 80], xmm5
            movaps [edi + 96], xmm6
            movaps [edi + 112], xmm7

            // Loop
            add esi, 128
            add edi, 128
            sub ecx, 1
            jnz SSE2AlignedCopyLoop1_128

        // Second loop: 16 bytes per iteration
        SSE2CopyLoop2_16a:
	        mov ecx, edx
	        and ecx, 127
	        shr ecx, 4
	        jz SSE2CopyLoop3_1a

        SSE2CopyLoop2_16b:
            movups xmm0, [esi]
            movups [edi], xmm0

	        add esi, 16
	        add edi, 16
	        sub ecx, 1
	        jnz SSE2CopyLoop2_16b

        // Third loop: 1 byte per iteration
        SSE2CopyLoop3_1a:
	        mov ecx, edx
	        and ecx, 15
	        jz SSE2CopyLoopDone

        SSE2CopyLoop3_1b:
	        mov ah, [esi]
	        mov [edi], ah

	        add esi, 1
	        add edi, 1
	        sub ecx, 1
	        jnz SSE2CopyLoop3_1b

        SSE2CopyLoopDone:

        }

        return;
    } unguard;
}


void StreamCopy (void *Dst, void *Src, uint32 Bytes)
{
    guard
    {
        if (Bytes == 0)
            return;

        if (HaveSSE2)
            StreamCopySSE2 (Dst, Src, Bytes);
        else
        if (HaveSSE)
            StreamCopySSE (Dst, Src, Bytes);
        else
        if (HaveMMX)
            StreamCopyMMX (Dst, Src, Bytes);
        else
            memcpy (Dst, Src, Bytes);

        return;
    } unguard;
}


void ApplyAlwaysOnTop (HWND Hwnd, bool OnTop)
{
    guard
    {
        SetWindowPos (Hwnd, OnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 
            0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);

        return;
    } unguard;
}


bool MouseXYtoLineColumn (ListThreadContext *LC, int MX, int MY, uint64 &LineResult, uint32 &ColumnResult)
{
    /*
    int X;
    int Y;
    uint64 Line = 0;
    uint32 Col = 0;

    if (LC == NULL  ||  LC->FileContext == NULL  ||  LC->FileContext->NormalLinesDB == NULL)
        return (false);

    //X = GET_X_LPARAM (LParam);
    //Y = GET_Y_LPARAM (LParam);
    X = MX / OutputGDI->GetFXSize ();
    Y = MY / OutputGDI->GetFYSize ();

    if (ScreenXYtoLineCol (LC, X, Y, Line, Col))
    {
        LineResult = Line;
        ColumnResult = Col;
        return (true);
    }

    return (false);
    */
    return false;
}


bool WindowProc (void *Context, LRESULT &LResult, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    guard
    {
        SCROLLINFO ScrollInfo;
        ListThreadContext *LTC;
        C_OutputGDI *OutputGDI;
        HMENU Menu;
        short ZDelta;

        LTC = (ListThreadContext *) Context;

        if (LTC == NULL)
            return (false);

        OutputGDI = ((C_OutputGDI *)LTC->Screen->GetOutput());

        Menu = GetMenu (Hwnd);

        switch (Msg)
        {
            case WM_MOUSEMOVE:
                /*
                if (LTC == NULL || LTC->FileContext == NULL || LTC->FileContext->NormalLinesDB == NULL)
                    break;
                    */

                // and the point of this is ... ? well we were going to do something but ended up not doing it ... maybe for v1.1
                break;

            case WM_REALLY_QUIT:
                TurnOffTimer (Hwnd);
                break;

            case WM_UPDATE_DISPLAY:
                AddSimpleCommand (LTC, LCMD_UPDATE_TEXT);
                AddSimpleCommand (LTC, LCMD_UPDATE_INFO);
                return (0);

            case WM_ENTERMENULOOP:
                HMENU HParsers;
                int i;

                CheckMenuItem (Menu, IDM_DISPLAY_HELP, MF_BYCOMMAND | (LTC->DisplayHelp ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_HEXMODE, MF_BYCOMMAND | (LTC->FileContext->HexMode ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_FILEINFORMATION, MF_BYCOMMAND | (LTC->DisplayInfo ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_WRAPLINES, MF_BYCOMMAND | (LTC->FileContext->WrapText ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_TAILING, MF_BYCOMMAND | (LTC->FileContext->Tailing ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_ASCIIJUNKFILTER, MF_BYCOMMAND | (LTC->JunkFilter ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_ALWAYSONTOP, MF_BYCOMMAND | (LTC->AlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_TRANSPARENCY, MF_BYCOMMAND | ((LTC->SettingsV2.GetValUint32("Transparent") == 1) ? MF_CHECKED : MF_UNCHECKED));

                CheckMenuItem (Menu, IDM_VIEW_HEXMODE_ENABLED, MF_BYCOMMAND | (LTC->FileContext->HexMode ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_HEXMODE_BYTES,  MF_BYCOMMAND | ((LTC->SettingsV2.GetValUint32 ("HexWordSizeLog2") == 0) ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_HEXMODE_WORDS,  MF_BYCOMMAND | ((LTC->SettingsV2.GetValUint32 ("HexWordSizeLog2") == 1) ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_HEXMODE_DWORDS, MF_BYCOMMAND | ((LTC->SettingsV2.GetValUint32 ("HexWordSizeLog2") == 2) ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_VIEW_HEXMODE_QWORDS, MF_BYCOMMAND | ((LTC->SettingsV2.GetValUint32 ("HexWordSizeLog2") == 3) ? MF_CHECKED : MF_UNCHECKED));

                CheckMenuItem (Menu, IDM_SEARCH_MATCHCASE, MF_BYCOMMAND | (LTC->FileContext->SearchCaseSensitive ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_SEARCH_LITERALSEARCH, MF_BYCOMMAND | ((LTC->FileContext->SearchType == SEARCHTYPE_LITERAL) ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_SEARCH_BOOLEANSEARCH, MF_BYCOMMAND | ((LTC->FileContext->SearchType == SEARCHTYPE_BOOLEAN) ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_SEARCH_REGEXSEARCH, MF_BYCOMMAND | ((LTC->FileContext->SearchType == SEARCHTYPE_REGEX) ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_SEARCH_ANIMATEDSEARCHES, MF_BYCOMMAND | (LTC->SettingsV2.GetValUint32("AnimatedSearch") ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_SEARCH_SEARCHDOWN, MF_BYCOMMAND | ((LTC->FileContext->SearchDirection == +1) ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem (Menu, IDM_SEARCH_SEARCHUP, MF_BYCOMMAND | ((LTC->FileContext->SearchDirection == -1) ? MF_CHECKED : MF_UNCHECKED));


                EnableMenuItem (Menu, IDM_EDIT_COPY, MF_BYCOMMAND | (OutputGDI->IsARegionHighlighted() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
                EnableMenuItem (Menu, IDM_EDIT_PASTE, MF_BYCOMMAND | (IsClipboardFormatAvailable (CF_TEXT) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

                // Parsers
                HParsers = GetSubMenu (Menu, 4);

                // Remove all the Parsers menu items
                while (RemoveMenu (HParsers, 0, MF_BYPOSITION))
                    ;

                // Add "Auto detect" menu item, and set the checkmark
                InsertMenu (HParsers, 0, MF_BYPOSITION | MF_STRING, PARSER_MENUBASE - 2, "Auto detect");
                CheckMenuItem (Menu, PARSER_MENUBASE - 2, MF_BYCOMMAND | (LTC->SettingsV2.GetValUint32("DetectParsers") ? MF_CHECKED : MF_UNCHECKED));
                InsertMenu (HParsers, 1, MF_BYPOSITION | MF_SEPARATOR, PARSER_MENUBASE - 1, NULL);

                // Add all the registered parsers!
                for (i = 0; i < GetParserCount(); i++)
                {
                    char Name[1000];

                    sprintf (Name, "%s\t&%d", GetParserName(i).c_str(), i + 1);
                    InsertMenu (HParsers, i + 2, MF_BYPOSITION | MF_STRING, PARSER_MENUBASE + i, Name);

                    if (LTC->FileContext->ParserIndice == i)
                        CheckMenuItem (Menu, PARSER_MENUBASE + i, MF_CHECKED);
                    else
                        CheckMenuItem (Menu, PARSER_MENUBASE + i, MF_UNCHECKED);
                }

                // If a search is in progress then we can't allow any of the search command thingies
                // But do enable the "cancel search"
                if (LTC->FileContext->DoingSearch)
                {
                    EnableMenuItem (Menu, IDM_SEARCH_CANCELSEARCH, MF_ENABLED);
                    EnableMenuItem (Menu, IDM_SEARCH_FIND, MF_DISABLED | MF_GRAYED);
                    EnableMenuItem (Menu, IDM_SEARCH_FINDNEXT, MF_DISABLED | MF_GRAYED);
                    EnableMenuItem (Menu, IDM_SEARCH_MATCHCASE, MF_DISABLED | MF_GRAYED);
                    EnableMenuItem (Menu, IDM_SEARCH_RESETSEARCH, MF_DISABLED | MF_GRAYED);
                    EnableMenuItem (Menu, IDM_SEARCH_LITERALSEARCH, MF_DISABLED | MF_GRAYED);
                    EnableMenuItem (Menu, IDM_SEARCH_BOOLEANSEARCH, MF_DISABLED | MF_GRAYED);
                    EnableMenuItem (Menu, IDM_SEARCH_REGEXSEARCH, MF_DISABLED | MF_GRAYED);
                }
                else
                {
                    EnableMenuItem (Menu, IDM_SEARCH_CANCELSEARCH, MF_DISABLED | MF_GRAYED);
                    EnableMenuItem (Menu, IDM_SEARCH_FIND, MF_ENABLED);
                    EnableMenuItem (Menu, IDM_SEARCH_FINDNEXT, LTC->FileContext->HaveDoneSearchBefore ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));
                    EnableMenuItem (Menu, IDM_SEARCH_MATCHCASE, MF_ENABLED);
                    EnableMenuItem (Menu, IDM_SEARCH_RESETSEARCH, MF_ENABLED);
                    EnableMenuItem (Menu, IDM_SEARCH_LITERALSEARCH, MF_ENABLED);
                    EnableMenuItem (Menu, IDM_SEARCH_BOOLEANSEARCH, MF_ENABLED);
                    EnableMenuItem (Menu, IDM_SEARCH_REGEXSEARCH, MF_ENABLED);
                }

                LResult = 0;
                return (true);

            case WM_DROPFILES:
                {
                    HDROP Drop;
                    char *FileName;
                    int Length;

                    Drop = (HDROP) WParam;
                    Length = DragQueryFile (Drop, 0, NULL, 0);
                    FileName = (char *) malloc ((1 + Length) * sizeof(char)); // we must use malloc() because the handler for LCMD_OPEN_FILE uses free()
                    DragQueryFile (Drop, 0, FileName, Length + 1);
                    AddCommand (LTC, LCMD_OPEN_FILE, 0, 0, 0, 0, (void *)FileName);
                    DragFinish (Drop);
                }

                LResult = 0;
                return (true);

            case WM_SIZING:
                /*
                static RECT OldRect;
                if (!LTC->SizingSignal)
                {
                    RECT *Rect;

                    Rect = (LPRECT) LParam;
                    *Rect = OldRect;
                    PostMessage (Hwnd, Msg, WParam, LParam);
                    LResult = TRUE;
                    return (true);
                }
                else
                */
                {
                    int FXSize;
                    int FYSize;
                    int WinWidth;
                    int WinHeight;
                    int TextWidth;
                    int TextHeight;
                    int DiffWidth;
                    int DiffHeight;
                    int FWSize;
                    RECT Client;
                    RECT Window;
                    RECT *Rect;

                    ApplyAlwaysOnTop (LTC->Screen->GetHWND(), LTC->AlwaysOnTop);

                    FXSize = OutputGDI->GetFXSize();
                    FYSize = OutputGDI->GetFYSize();

                    Rect = (LPRECT) LParam;

                    //
                    WinWidth = Rect->right - Rect->left + 1;
                    WinHeight = Rect->bottom - Rect->top + 1;

                    // Find difference between client and window sizes
                    GetClientRect (LTC->Screen->GetHWND(), &Client);
                    GetWindowRect (LTC->Screen->GetHWND(), &Window);

                    DiffWidth = (Window.right - Client.right) + (Client.left - Window.left);
                    DiffHeight = (Window.bottom - Client.bottom) + (Client.top - Window.top);

                    // Adjust so we are calculating the size of the *client* area
                    WinWidth -= DiffWidth;
                    WinHeight -= DiffHeight;

                    // How many characters are we looking at, width x height?
                    TextWidth = WinWidth / FXSize;
                    TextHeight = WinHeight / FYSize;

                    // Enforce minimum width/height
                    TextWidth = max (80, TextWidth);
                    TextHeight = max (10, TextHeight);

                    // Calculate client size
                    WinWidth = TextWidth * FXSize;
                    WinHeight = TextHeight * FYSize;

                    // Adjust for client -> window size
                    WinWidth += DiffWidth;
                    WinHeight += DiffHeight;

                    // Set size properly depending on which side they're dragging
                    FWSize = WParam;

                    switch (FWSize)
                    {
                        case WMSZ_BOTTOM:
                            Rect->bottom = Rect->top + WinHeight;
                            break;

                        case WMSZ_BOTTOMLEFT:
                            Rect->bottom = Rect->top + WinHeight;
                            Rect->left = Rect->right - WinWidth;
                            break;

                        case WMSZ_BOTTOMRIGHT:
                            Rect->bottom = Rect->top + WinHeight;
                            Rect->right = Rect->left + WinWidth;
                            break;

                        case WMSZ_LEFT:
                            Rect->left = Rect->right - WinWidth;
                            break;

                        case WMSZ_RIGHT:
                            Rect->right = Rect->left + WinWidth;
                            break;

                        case WMSZ_TOP:
                            Rect->top = Rect->bottom - WinHeight;
                            break;

                        case WMSZ_TOPLEFT:
                            Rect->top = Rect->bottom - WinHeight;
                            Rect->left = Rect->right - WinWidth;
                            break;

                        case WMSZ_TOPRIGHT:
                            Rect->top = Rect->bottom - WinHeight;
                            Rect->right = Rect->left + WinWidth;
                            break;
                    }
                    
                    //OldRect = *Rect;
                    LResult = TRUE;
                    return (true);
                }

                return (true);

            case WM_SIZE:
                {
                    int TextWidth;
                    int TextHeight;
                    int FXSize;
                    int FYSize;
                    bool Maximizing = false;
                    RECT ClientRect;

                    ApplyAlwaysOnTop (LTC->Screen->GetHWND(), LTC->AlwaysOnTop);

                    switch (WParam)
                    {
                        case SIZE_MAXHIDE:
                            LResult = 1;
                            return (true);

                        case SIZE_MAXSHOW:
                            LResult = 1;
                            return (true);

                        case SIZE_MINIMIZED:
                            LResult = 1;
                            return (true);

                        case SIZE_MAXIMIZED:
                            Maximizing = true;

                        case SIZE_RESTORED:
                            bool FudgeX;
                            bool FudgeY;
                            ListResizeInfo *ResizeInfo;

                            ResizeInfo = new ListResizeInfo;

                            FudgeX = false;
                            FudgeY = false;

                            FXSize = OutputGDI->GetFXSize();
                            FYSize = OutputGDI->GetFYSize();

                            TextWidth = (LOWORD(LParam) + (FXSize - 1)) / FXSize;
                            TextHeight = (HIWORD(LParam) + (FYSize - 1)) / FYSize;

                            // Fix it if we are maximizing and the last line would be chopped off
                            if (WParam == SIZE_MAXIMIZED)
                            {
                                sint32 WinHeight;
                                sint32 WinWidth;
                                sint32 ReqHeight;
                                sint32 ReqWidth;

                                GetClientRect (Hwnd, &ClientRect);
                                WinHeight = ClientRect.bottom - ClientRect.top;
                                WinWidth = ClientRect.right - ClientRect.left;
                                ReqHeight = TextHeight * FYSize;              
                                ReqWidth = TextWidth * FXSize;

                                if (ReqHeight - WinHeight > 2)
                                {
                                    TextHeight--;
                                    FudgeY = true;
                                }

                                if (ReqWidth - WinWidth > 1)
                                {
                                    TextWidth--;
                                    FudgeY = true;
                                }
                            }

                            ResizeInfo->NewWidth = TextWidth;
                            ResizeInfo->NewHeight = TextHeight;
                            ResizeInfo->FudgeX = FudgeX;
                            ResizeInfo->FudgeY = FudgeY;
                            ResizeInfo->Maximizing = Maximizing;

                            // All this crazy stuff is done to make absolutely sure this message
                            // is inserted into the queue!
                            LTC->CmdQueue.Lock ();
                            bool Enabled = LTC->CmdQueue.IsEnabled ();
                            int Count = 0;

                            if (!Enabled)
                            {
                                while (!LTC->CmdQueue.IsEnabled())
                                {
                                    Count++;
                                    LTC->CmdQueue.Enable ();
                                }
                            }


                            LTC->CmdQueue.ReplaceCommandUrgentAsync (LCMD_RESIZEXY, CCommand (
                                LCMD_RESIZEXY, 0, 0, 0, 0, ResizeInfo));

                            if (!Enabled)
                            {
                                while (Count > 0)
                                {
                                    LTC->CmdQueue.Disable ();
                                    Count--;
                                }
                            }

                            LTC->CmdQueue.Unlock ();
                            LResult = 1;
                            return (true);
                    }
                }

                return (false);

            case WM_TIMER:
                switch (WParam)
                {
                    case 1:
                        //try
                        {
                            bool Changed;

                            OutputGDI->DrawCursor ();

                            if (LTC->FileContext->File != NULL)
                            {
                                Changed = HasFileChanged (LTC->FileContext->File);
                                
                                if (Changed)
                                    AddSimpleCommand (LTC, LCMD_UPDATE_INFO);

                                // If tailing is enabled then check the file's size to see if it has changed OR if they are asking us to force updates
                                if (LTC->FileContext->Tailing  &&  
                                    LTC->SizeCheckInterval < (GetTickCount() - LTC->SizeCheckTimestamp)  && 
                                    (Changed  ||
                                    LTC->SettingsV2.GetValUint32 ("TailingForceUpdate") == 1))
                                {
                                    AddSimpleCommand (LTC, LCMD_FILE_CHANGED);
                                    LTC->SizeCheckTimestamp = GetTickCount ();
                                }
                            }
                        }

                        //catch (...)
                        {

                        }

                        TurnOnTimer (Hwnd);
                        break;
                }
                break;

            case WM_COMMAND:
                int ID;
                int Event;

                ID = LOWORD (WParam);
                Event = HIWORD (WParam);

                switch (ID)
                {
                    case IDM_FILE_EXIT:
                        TurnOffTimer (Hwnd);
                        AddSimpleCommand (LTC, LCMD_QUIT);
                        break;

                    case IDM_FILE_EDIT:
                        AddSimpleCommand (LTC, LCMD_EDIT);
                        break;

                    case IDM_FILE_OPEN:
                        AddSimpleCommand (LTC, LCMD_INPUT_OPEN);
                        break;

                    case IDM_FILE_OPEN_NEW:
                        AddSimpleCommand (LTC, LCMD_INPUT_OPEN_NEW);
                        break;

                    case IDM_FILE_RESET:
                        AddSimpleCommand (LTC, LCMD_FILE_RESET);
                        break;

                    case IDM_DISPLAY_HELP:
                        if (LTC->DisplayInfo)
                            AddSimpleCommand (LTC, LCMD_TOGGLE_INFO_DISPLAY);

                        AddSimpleCommand (LTC, LCMD_HELP_DISPLAY_TOGGLE);
                        AddSimpleCommand (LTC, LCMD_UPDATE_INFO);
                        AddSimpleCommand (LTC, LCMD_UPDATE_TEXT);
                        break;

                    case IDM_HELP_ABOUT:
                        DialogBoxParam (GetModuleHandle (NULL), MAKEINTRESOURCE (IDD_ABOUT), Hwnd, AboutBoxDialog, NULL);
                        break;

                    case IDM_FILE_PREFERENCESV2:
                        TurnOffTimer (Hwnd);
                        DoPreferences (Hwnd, LTC, "");
                        TurnOnTimer (Hwnd);
                        break;

                    case IDM_EDIT_COPY:
                        OutputGDI->DoClipboardCopy();
                        break;

                    case IDM_EDIT_PASTE:
                        OutputGDI->DoClipboardPaste();
                        break;

                    case IDM_VIEW_FILEINFORMATION:
                        if (LTC->DisplayHelp)
                            AddSimpleCommand (LTC, LCMD_HELP_DISPLAY_TOGGLE);

                        AddSimpleCommand (LTC, LCMD_TOGGLE_INFO_DISPLAY);
                        AddSimpleCommand (LTC, LCMD_UPDATE_INFO);
                        AddSimpleCommand (LTC, LCMD_UPDATE_TEXT);
                        break;

                    case IDM_VIEW_ASCIIJUNKFILTER:
                        AddSimpleCommand (LTC, LCMD_TOGGLE_JUNK_FILTER);
                        break;

                    case IDM_VIEW_HEXMODE_ENABLED:
                        AddSimpleCommand (LTC, LCMD_TOGGLE_HEX_DISPLAY);
                        break;
                    
                    case IDM_VIEW_HEXMODE_BYTES:
                        AddCommand (LTC, LCMD_SET_HEX_WORDSIZELOG2, 0, 0, 0, 0, NULL);
                        break;

                    case IDM_VIEW_HEXMODE_WORDS:
                        AddCommand (LTC, LCMD_SET_HEX_WORDSIZELOG2, 1, 0, 0, 0, NULL);
                        break;

                    case IDM_VIEW_HEXMODE_DWORDS:
                        AddCommand (LTC, LCMD_SET_HEX_WORDSIZELOG2, 2, 0, 0, 0, NULL);
                        break;

                    case IDM_VIEW_HEXMODE_QWORDS:
                        AddCommand (LTC, LCMD_SET_HEX_WORDSIZELOG2, 3, 0, 0, 0, NULL);
                        break;

                    case IDM_VIEW_TAILING:
                        AddSimpleCommand (LTC, LCMD_TOGGLE_TAILING);
                        break;

                    case IDM_VIEW_WRAPLINES:
                        AddSimpleCommand (LTC, LCMD_TOGGLE_LINE_WRAPPING);
                        break;

                    case IDM_VIEW_ALWAYSONTOP:
                        AddCommand (LTC, LCMD_SET_ALWAYS_ON_TOP, LTC->AlwaysOnTop ? 0 : 1, 0, 0, 0, NULL);
                        break;

                    case IDM_VIEW_TRANSPARENCY:
                        AddSimpleCommand (LTC, LCMD_TOGGLE_TRANSPARENCY);
                        break;

                    case IDM_SEARCH_FIND:
                        LTC->Screen->AddKeyToQ (6); // CTRL+F
                        break;

                    case IDM_SEARCH_LITERALSEARCH:
                        AddCommand (LTC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_LITERAL, 0, 0, 0, NULL);
                        break;

                    case IDM_SEARCH_BOOLEANSEARCH:
                        AddCommand (LTC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_BOOLEAN, 0, 0, 0, NULL);
                        break;

                    case IDM_SEARCH_REGEXSEARCH:  
                        AddCommand (LTC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_REGEX, 0, 0, 0, NULL);
                        break;

                    case IDM_SEARCH_MATCHCASE:
                        AddSimpleCommand (LTC, LCMD_SEARCH_TOGGLE_MATCH_CASE);
                        break;

                    case IDM_SEARCH_FINDNEXT:
                        AddSimpleCommand (LTC, LCMD_SEARCH_FIND_NEXT);
                        break;

                    case IDM_SEARCH_RESETSEARCH:
                        AddSimpleCommand (LTC, LCMD_SEARCH_RESET);
                        break;

                    case IDM_SEARCH_SETCURRENTLINE:
                        if (!LTC->FileContext->DoingSearch)
                        {
                            if (LTC->FileContext->HexMode)
                            {
                                if (!HexLineToNormalLine (LTC, LTC->FileContext->CurrentLine, LTC->FileContext->NextLineToSearch))
                                    LTC->FileContext->NextLineToSearch = 0;
                            }
                            else
                            {
                                LTC->FileContext->NextLineToSearch = LTC->FileContext->CurrentLine;
                            }
                        }

                        AddCommand (LTC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                        AddSimpleCommand (LTC, LCMD_UPDATE_INFO);
                        break;

                    case IDM_SEARCH_CANCELSEARCH:
                        AddSimpleCommand (LTC, LCMD_SEARCH_CANCEL);
                        break;

                    case IDM_SEARCH_ANIMATEDSEARCHES:
                        AddSimpleCommand (LTC, LCMD_SEARCH_TOGGLE_ANIMATED);
                        break;

                    case IDM_SEARCH_SEARCHDOWN:
                        AddCommand (LTC, LCMD_SEARCH_SET_DIRECTION, 1, 0, 0, 0, NULL);
                        break;

                    case IDM_SEARCH_SEARCHUP:
                        AddCommand (LTC, LCMD_SEARCH_SET_DIRECTION, 0, 0, 0, 0, NULL);
                        break;

                    case PARSER_MENUBASE - 2:
                        {
                            bool Value;

                            Value = LTC->SettingsV2.GetValUint32 ("DetectParsers");
                            Value = !Value;
                            LTC->SettingsV2.SetValUint32 ("DetectParsers", Value ? 1 : 0);
                        }
                        break;

                    // There are only 9 parsers that can be installed at a time, btw
                #define DO_PARSER_MENU(num) \
                    case (PARSER_MENUBASE + (num)): \
                        AddCommand (LTC, LCMD_SET_PARSER, (num), 0, 0, 0, NULL); \
                        break;

                    DO_PARSER_MENU(0);
                    DO_PARSER_MENU(1);
                    DO_PARSER_MENU(2);
                    DO_PARSER_MENU(3);
                    DO_PARSER_MENU(4);
                    DO_PARSER_MENU(5);
                    DO_PARSER_MENU(6);
                    DO_PARSER_MENU(7);
                    DO_PARSER_MENU(8);
                }

                LResult = 0;
                return (true);

            case WM_SYSCOMMAND:
                switch (WParam)
                {
                    case SC_CLOSE:
                        TurnOffTimer (Hwnd);
                        AddSimpleCommand (LTC, LCMD_QUIT);
                        LResult = 0;
                        return (true);
                }

                return (false);

            case WM_MOUSEWHEEL:
                ULONG WheelLines;

                WheelLines = 3;

                if (0 == SystemParametersInfo (SPI_GETWHEELSCROLLLINES, 0, &WheelLines, 0))
                    WheelLines = 3; // SDK doesn't really define what happens to pvParam (i.e., &WheelLines) if this call fails

                if (WheelLines == 0) // also, I assume a return value of 0 to mean "I have no clue!" and I assume 3
                    WheelLines = 3;

                ZDelta = (short) HIWORD (WParam);
                ZDelta /= 120 / (signed)WheelLines;

                // If shift is being held down, scroll one page per ZDelta/120
                if ((LOWORD(WParam) & MK_SHIFT)  ||  (LOWORD(WParam) & MK_RBUTTON))
                {
                    ZDelta /= (signed)WheelLines;
                    //ZDelta *= LTC->Screen->GetMaxY() - 1;

                    for (i = 0; i < abs(ZDelta); i++)
                    {
                        if (ZDelta > 0)
                        {   // Simulate pressing the page-up key
                            AddSimpleCommand (LTC, LCMD_INPUT_PAGE_UP);
                        }
                        else
                        {   // Simulate presing the page-down key
                            AddSimpleCommand (LTC, LCMD_INPUT_PAGE_DOWN);
                        }
                    }
                }
                else
                for (i = 0; i < abs(ZDelta); i++)
                {
                    if (ZDelta > 0)
                    {   // Simulate pressing the up arrow key
                        AddSimpleCommand (LTC, LCMD_INPUT_LINE_UP);
                    }
                    else
                    {   // Simulate pressing the down arrow key
                        AddSimpleCommand (LTC, LCMD_INPUT_LINE_DOWN);
                    }
                }

                LResult = 0;
                return (true);

            case WM_HSCROLL:
                ZeroMemory (&ScrollInfo, sizeof (ScrollInfo));
                ScrollInfo.cbSize = sizeof (ScrollInfo);
                
                switch (LOWORD(WParam))
                {
                    case SB_LINELEFT:
                        // Simulate pressing the left arrow key
                        AddSimpleCommand (LTC, LCMD_INPUT_COLUMN_LEFT);
                        break;

                    case SB_LINERIGHT:
                        // Simulate pressing the right arrow key
                        AddSimpleCommand (LTC, LCMD_INPUT_COLUMN_RIGHT);
                        break;

                    case SB_PAGELEFT:
                        AddSimpleCommand (LTC, LCMD_INPUT_PAGE_LEFT);
                        break;

                    case SB_PAGERIGHT:
                        AddSimpleCommand (LTC, LCMD_INPUT_PAGE_RIGHT);
                        break;

                    case SB_THUMBTRACK:
                        ScrollInfo.fMask = SIF_TRACKPOS;
                        GetScrollInfo (LTC->Screen->GetHWND(), SB_HORZ, &ScrollInfo);

                        if (ScrollInfo.nTrackPos == 0)
                            AddCommand (LTC, LCMD_SET_COLUMN, 0, 0, 0, 0, NULL);
                        else
                            AddCommand (LTC, LCMD_SET_COLUMN, ScrollInfo.nTrackPos - 1, 0, 0, 0, NULL);

                        LTC->WaitForScan = false;
                        break;
                }

                LResult = 0;
                return (true);

            case WM_VSCROLL:
                ZeroMemory (&ScrollInfo, sizeof (ScrollInfo));
                ScrollInfo.cbSize = sizeof (ScrollInfo);

                switch (LOWORD(WParam))
                {
                    case SB_BOTTOM:
                        // Simulate pressing the End key
                        AddSimpleCommand (LTC, LCMD_INPUT_GO_END);
                        break;

                    case SB_LINEDOWN:
                        // Simulate pressing the down arrow key
                        AddSimpleCommand (LTC, LCMD_INPUT_LINE_DOWN);
                        break;

                    case SB_LINEUP:
                        // Simulate pressing the up arrow key
                        AddSimpleCommand (LTC, LCMD_INPUT_LINE_UP);
                        break;

                    case SB_PAGEUP:
                        // Simulate pressing the Page Up key
                        AddSimpleCommand (LTC, LCMD_INPUT_PAGE_UP);
                        break;

                    case SB_PAGEDOWN:
                        // Simulate pressing the Page Down key
                        AddSimpleCommand (LTC, LCMD_INPUT_PAGE_DOWN);
                        break;

                    case SB_THUMBTRACK:
                        ScrollInfo.fMask = SIF_TRACKPOS;
                        GetScrollInfo (LTC->Screen->GetHWND(), SB_VERT, &ScrollInfo);

                        if (ScrollInfo.nTrackPos == 0)
                            LTC->CmdQueue.SendCommandUrgentAsync (CCommand (LCMD_SET_LINE, 0, 0, 0, 0, NULL));
                        else
                            LTC->CmdQueue.SendCommandUrgentAsync (CCommand (LCMD_SET_LINE, ScrollInfo.nTrackPos - 1, 0, 0, 0, NULL)); // Note: Win32 GUI seems to be limiting us to not allow full 64-bit line numbers

                        LTC->WaitForScan = false;
                        break;
                }

                LResult = 0;
                return (true);
        }

        return (false);
    } unguard;
}


typedef BOOL (WINAPI *SLWAFunction) (HWND, COLORREF, BYTE, DWORD);


bool SetWindowAlphaValues (HWND Hwnd, bool Enable, uint8 Alpha)
{
    guard
    {
        HMODULE User32DLL;
        DWORD ExStyle;
        SLWAFunction _SetLayeredWindowAttributes_;

        User32DLL = LoadLibrary ("user32.dll");
        if (User32DLL == NULL)
            return (false);

        _SetLayeredWindowAttributes_ = (SLWAFunction) GetProcAddress (User32DLL, "SetLayeredWindowAttributes");
        if (_SetLayeredWindowAttributes_ == NULL)
        {
            FreeLibrary (User32DLL);
            return (false);
        }

        ExStyle = GetWindowLong (Hwnd, GWL_EXSTYLE);
        if (Enable)
            ExStyle |= WS_EX_LAYERED;
        else
            ExStyle &= ~WS_EX_LAYERED;

        SetWindowLong (Hwnd, GWL_EXSTYLE, ExStyle);
        _SetLayeredWindowAttributes_ (Hwnd, 0, Enable ? Alpha : 255, LWA_ALPHA);
        FreeLibrary (User32DLL);
        return (true);
    } unguard;
}


bool IsWindowAlphaAllowed (void)
{
    guard
    {
        HMODULE User32DLL;
        SLWAFunction _SetLayeredWindowAttributes_;

        User32DLL = LoadLibrary ("user32.dll");
        if (User32DLL == NULL)
            return (false);

        _SetLayeredWindowAttributes_ = (SLWAFunction) GetProcAddress (User32DLL, "SetLayeredWindowAttributes");
        if (_SetLayeredWindowAttributes_ == NULL)
        {
            FreeLibrary (User32DLL);
            return (false);
        }

        FreeLibrary (User32DLL);
        return (true);
    } unguard;
}


void CenterWindow (HWND Hwnd)
{
    guard
    {
            RECT ParentRect;
        RECT OurRect;
        HWND Parent;
        int NewX;
        int NewY;

        Parent = GetParent (Hwnd);

        if (Parent == NULL)
            Parent = GetDesktopWindow ();

        GetWindowRect (Parent, &ParentRect);
        GetClientRect (Hwnd, &OurRect);

        NewX = (ParentRect.left + ParentRect.right - OurRect.right) / 2;
        NewY = (ParentRect.top + ParentRect.bottom - OurRect.bottom) / 2;

        SetWindowPos (Hwnd, 0, NewX, NewY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        return;
    } unguard;
}


#define PACKVERSION(major,minor) MAKELONG(minor,major)
DWORD GetDllVersion(LPCTSTR lpszDllName)
{
    guard
    {
        HINSTANCE hinstDll;
        DWORD dwVersion = 0;

        hinstDll = LoadLibrary (lpszDllName);
    	
        if (hinstDll != NULL)
        {
            DLLGETVERSIONPROC pDllGetVersion;

            pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

            /*
            Because some DLLs might not implement this function, you
            must test for it explicitly. Depending on the particular 
            DLL, the lack of a DllGetVersion function can be a useful
            indicator of the version.
            */

            if (pDllGetVersion)
            {
                DLLVERSIONINFO dvi;
                HRESULT hr;

                ZeroMemory (&dvi, sizeof(dvi));
                dvi.cbSize = sizeof(dvi);

                hr = (*pDllGetVersion)(&dvi);

                if (SUCCEEDED (hr))
                {
                    dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
                }
            }
            
            FreeLibrary (hinstDll);
        }

        return (dwVersion);
    } unguard;
}


bool IsOKPath (LPITEMIDLIST IIDList)
{
    guard
    {
        char Path[MAX_PATH * 10];

        if (!SHGetPathFromIDListA (IIDList, Path))
            return (false);

        if (strlen (Path) == 0)
            return (false);

        return (true);
    } unguard;
}


int CALLBACK ChooseDirCallback (HWND Hwnd, UINT Msg, LPARAM LParam, LPARAM lpData)
{
    guard
    {
        switch (Msg)
        {
            case BFFM_INITIALIZED:
                if (lpData != NULL)
                {   // Set initial directory
                    SendMessage (Hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);
                }

                CenterWindow (Hwnd);

                break;

            case BFFM_SELCHANGED:
                if (!IsOKPath ((LPITEMIDLIST) LParam))
                    SendMessage (Hwnd, BFFM_ENABLEOK, 0, 0);
                else
                    SendMessage (Hwnd, BFFM_ENABLEOK, 0, 1);

                break;

            case BFFM_VALIDATEFAILED:
                return (1);
        }

        return (0);
    } unguard;
}


void PrintPackedVer (char *Dst, DWORD PackedVer)
{
    guard
    {
        sprintf (Dst, "%u.%u", (PackedVer & 0xffff0000) >> 16, PackedVer & 0xffff);
        return;
    } unguard;
}


bool ChooseDirectory (HWND ParentHwnd, string &Result, char *InitialDir, string WindowName)
{
    guard
    {
        BROWSEINFO BInfo;
        char DisplayName[MAX_PATH * 10];
        char Path[MAX_PATH * 10];
        LPITEMIDLIST IIDList;
        IMalloc *Malloc;
        bool ReturnVal;
        DWORD Shell32Ver;
        
        Shell32Ver = GetDllVersion ("shell32.dll");
        SHGetMalloc (&Malloc);
        CoInitialize (NULL);

        BInfo.hwndOwner = ParentHwnd;
        BInfo.pidlRoot = NULL;
        BInfo.pszDisplayName = DisplayName;
        BInfo.lpszTitle = WindowName.c_str();

        BInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
        BInfo.lpfn = NULL;
        BInfo.lpfn = NULL;

        if (Shell32Ver >= PACKVERSION (5,0))
        {
            BInfo.ulFlags |= BIF_EDITBOX | BIF_VALIDATE;
            BInfo.lpfn = ChooseDirCallback;
            BInfo.lParam = (LPARAM)InitialDir;
            BInfo.ulFlags |= BIF_USENEWUI | BIF_SHAREABLE;
        }

        BInfo.iImage = NULL;

        IIDList = SHBrowseForFolder (&BInfo);
        if (IIDList == NULL)
            ReturnVal = false;
        else
        {
            if (SHGetPathFromIDListA (IIDList, Path))
                Result = Path;

            Malloc->Free (IIDList);
            ReturnVal = true;
        }

        Malloc->Release ();
        return (ReturnVal);
    } unguard;
}


void RunShell (HWND Hwnd, const char *Command)
{
    guard
    {
        ShellExecute (Hwnd, "open", Command, NULL, ".", SW_SHOWNORMAL);
        return;
    } unguard;
}


UINT_PTR CALLBACK ChooseFileCallback (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    guard
    {
        LPOFNOTIFY Notify;

        switch (Msg)
        {
            case WM_INITDIALOG:
                CenterWindow (GetParent (Hwnd));
                return (0);

            case WM_NOTIFY:
                Notify = (LPOFNOTIFY) LParam;

                switch (Notify->hdr.code)
                {
                    case CDN_INITDONE:
                        CenterWindow (GetParent (Hwnd));
                        return (0);
                }

                return (0);
        }

        return (0);
    } unguard;
}


bool ChooseFile (HWND ParentHwnd, string &Result, string WindowName, const char *FileSpec)
{
    guard
    {
        string Return;
        OPENFILENAME OFN;
        BOOL APIResult;
        char FileName[2048];
        
        ZeroMemory (FileName, sizeof (FileName));
        ZeroMemory (&OFN, sizeof (OFN));

        if (GetDllVersion ("shell32.dll") >= PACKVERSION (5,0))
            OFN.lStructSize = sizeof (OFN);
        else
            OFN.lStructSize = OPENFILENAME_SIZE_VERSION_400; //sizeof (OFN); // maintain Win9x/NT4 compatibility

        OFN.hwndOwner = ParentHwnd;
        OFN.hInstance = NULL;

        if (FileSpec == "") 
            OFN.lpstrFilter = "Text Files\0*.TXT\0Log Files\0*.LOG\0All Files\0*.*\0";
        else
            OFN.lpstrFilter = FileSpec;

        OFN.lpstrCustomFilter = NULL;
        OFN.nMaxCustFilter = 0;
        OFN.nFilterIndex = 3;
        OFN.lpstrFile = FileName;
        OFN.nMaxFile = sizeof (FileName);
        OFN.lpstrInitialDir = NULL;
        OFN.lpstrTitle = WindowName.c_str();

        OFN.Flags = OFN_ENABLEHOOK |
                    OFN_HIDEREADONLY | 
                    OFN_ENABLESIZING | 
                    OFN_EXPLORER | 
                    OFN_FILEMUSTEXIST | 
                    OFN_LONGNAMES | 
                    OFN_DONTADDTORECENT |
                    OFN_PATHMUSTEXIST;

        OFN.nFileOffset = 0;
        OFN.nFileExtension = 0;
        OFN.lpstrDefExt = NULL;
        OFN.lpfnHook = ChooseFileCallback;

        APIResult = GetOpenFileName (&OFN);
     
        if (APIResult == 0)
        {
            /*
            char *ErrorName;
            DWORD Result = CommDlgExtendedError ();

            switch (Result)
            {
    #define DOERROR(msg) \
            case msg: \
                ErrorName = #msg; \
                break;
                    DOERROR(CDERR_DIALOGFAILURE)
                    DOERROR(CDERR_FINDRESFAILURE)
                    DOERROR(CDERR_NOHINSTANCE)
                    DOERROR(CDERR_INITIALIZATION)
                    DOERROR(CDERR_NOHOOK)
                    DOERROR(CDERR_LOCKRESFAILURE)
                    DOERROR(CDERR_NOTEMPLATE)
                    DOERROR(CDERR_LOADRESFAILURE)
                    DOERROR(CDERR_STRUCTSIZE)
                    DOERROR(CDERR_LOADSTRFAILURE)
                    DOERROR(FNERR_BUFFERTOOSMALL)
                    DOERROR(CDERR_MEMALLOCFAILURE)
                    DOERROR(FNERR_INVALIDFILENAME)
                    DOERROR(CDERR_MEMLOCKFAILURE)
                    DOERROR(FNERR_SUBCLASSFAILURE)
            }

            MessageBox (NULL, "Couldn't GetOpenFileName", ErrorName, MB_OK);
            */
            return (false);
        }

        Result = string (OFN.lpstrFile);

        if (Result.find(' ') != string::npos)
            Result = string("\"") + Result + string("\"");

        return (true);
    } unguard;
}


// Takes a string and breaks it into its individual components, including quotation atomization
// i.e. the string:
//     one big fish
// is parsed into a vector of three strings: [0]="one" [1]="big" and [2]="fish"
// however, the string:
//     one "giant fish"
// is parsed into a vector of two strings: [0]="one" [1]="giant fish"
vector<string> ParseFileList (const char *CommandLine)
{
    guard
    {
        vector<string> List;
        char *CmdLineCopy;
        char *p;
        char *begin;
        char *end;

        CmdLineCopy = new char[1 + strlen(CommandLine)];
        strcpy (CmdLineCopy, CommandLine);
        begin = CmdLineCopy;
        end = CmdLineCopy + strlen(CmdLineCopy);
        p = begin;

        while (p < end)
        {
            char s;
            char *q;

            // Skip over whitespace
            while (isspace(*p))
                p++;
            
            // Normally the filenames are delimited with a space character
            s = ' '; 
            if (*p == '"')
            {
                s = '"'; // But quotation marks supercede that
                p++;
            }

            // Find the next occurence of the delimiter (space or quote)
            q = strchr (p, s); 

            if (q == NULL) // No delimiter found ... we will just use the entire string starting from p
            {
                List.push_back (string(p));
                p = end; // force break out of loop
            }
            else
            {   // Found a filename.
                *q = '\0';
                List.push_back (string(p));
                p = q + 1;
            }
        }

        delete CmdLineCopy;
        return (List);
    } unguard;
}


string MakeUniqueFileName (void)
{
    const char table[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    string Return ("_$lxp");
    int i;

    for (i = 0; i < 16; i++)
        Return.push_back (table[(rand() % 36)]);

    Return.append (".tmp");
    return (Return);
}


template<class IntT>
int FirstBit (IntT mask)
{
    int i;

    for (i = 0; i < (sizeof(IntT) * 8); ++i)
        if (mask & (1 << i))
            return (i);

    return (-1);
}


void UnifyThreadAffinities (HANDLE *threadHandles, int count)
{
    guard
    {
        int i;
        DWORD_PTR pmask;
        DWORD_PTR smask;
        DWORD_PTR tmask;
        int bit;

        if (!GetProcessAffinityMask (GetCurrentProcess(), &pmask, &smask))
            return;

        bit = FirstBit (pmask);

        if (bit == -1) // no bits set in pmask? frownie faces :(
            return;

        tmask = 1 << bit;

        for (i = 0; i < count; ++i)
            SetThreadAffinityMask (threadHandles[i], tmask);
    } unguard;

    return;
}


void RelaxThreadAffinities (HANDLE *threadHandles, int count)
{
    guard
    {
        DWORD_PTR pmask;
        DWORD_PTR smask;
        int i;

        if (!GetProcessAffinityMask (GetCurrentProcess(), &pmask, &smask))
            return;

        for (i = 0; i < count; ++i)
            SetThreadAffinityMask (threadHandles[i], pmask);
    } unguard;

    return;
}


int ListStart (HINSTANCE HInstance, HINSTANCE HPrevInstance, LPSTR CmdLine, int ShowCmd)
{
    guard
    {
        GDICons *Screen = NULL;
        DWORD ThreadID;
        HANDLE ThreadHandle = INVALID_HANDLE_VALUE;
        DWORD WinStyle;
        ListStartContext LSC;
        ListThreadContext LC;
        string FileName;
        vector<string> FileList;
        OptionList GlobalOptions;
        string GlobalOptionsString; // GlobalOptions, in string form
        vector<string>::iterator vsit;
        Pipe *Redirector = NULL;
        int i;

        LC.SettingsV2.SetRegistryKey (LISTREGROOTKEY, LISTREGSUBKEY);
        LC.SettingsV2.SetDefaultsList (&GlobalDefaults);
        LC.SettingsV2.LoadVarsFromReg ();

        Screen = new GDICons (LISTTITLE " " LISTVERSION, 
                              LC.SettingsV2.GetValUint32("Width"), 
                              LC.SettingsV2.GetValUint32("Height"),
                              atoi(LC.SettingsV2.GetValString("X").c_str()),
                              atoi(LC.SettingsV2.GetValString("Y").c_str()),
                              (char *)LC.SettingsV2.GetValString("FontName").c_str(),
                              LC.SettingsV2.GetValUint32("FontSize"),
                              HInstance, 
                              LoadIcon (HInstance, MAKEINTRESOURCE(IDI_LISTICON)),
                              NULL,
                              LoadMenu (HInstance, MAKEINTRESOURCE(IDR_LISTMENU)),
                              false);

        LoadAccelerators (HInstance, MAKEINTRESOURCE(IDR_LISTACC));
        WinStyle = GetWindowLong (Screen->GetHWND(), GWL_STYLE);
        WinStyle |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_VSCROLL | WS_HSCROLL | WS_THICKFRAME;
        SetWindowLong (Screen->GetHWND(), GWL_STYLE, WinStyle);
        SetWindowPos (Screen->GetHWND(), NULL, 0, 0, 0, 0,  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        Screen->SetWindowProc (WindowProc);
        Screen->SetBackground (LC.SettingsV2.GetValUint32("TextColor.Background")); //0x00ffffff);
        Screen->SetForeground (LC.SettingsV2.GetValUint32("TextColor.Foreground")); //0x00000000);
        Screen->Clear ();
        Screen->RedrawAll ();

        // Parse into the various file names given -- we will want to open each one in a new window
        FileList = ParseFileList (CmdLine);
        FileName = string("");

        // Find the first string that doesn't start with ? and use that (otherwise we
        // will use the blank "" string which forces an Open dialog box)
        for (vsit = FileList.begin(); vsit != FileList.end(); ++vsit)
        {
            if (vsit->length() > 0  &&  (*vsit)[0] != '?')
            {
                FileName = *vsit;
                FileList.erase (vsit); // remove this so we don't double-parse it later
                break;
            }
        }

        // Find if any "files" in the list start with ? and parse them for global options
        // that will apply to every file that is opened
        for (vsit = FileList.begin(); vsit != FileList.end(); ++vsit)
        {
            if ((*vsit)[0] == '?') // starts immediately with a '?' ... yeah!
            {
                OptionList olist;

                olist = ParseForOptions (vsit->begin(), vsit->end()); // get the options specified here
                copy (olist.begin(), olist.end(), back_inserter(GlobalOptions)); // and add them to the global options
            }
        }

        // Make a string out of these global options so we can pass them to the other windows as well
        GlobalOptionsString = OptionListToString (GlobalOptions);

        // Give the thread the global options
        LSC.GlobalOptions = &GlobalOptions;
        LSC.ShowCmd = ShowCmd;

        // Enable drag 'n drop
        DragAcceptFiles (Screen->GetHWND(), TRUE);

        // If we didn't find a filename ... then read from standard in!
        // the Pipe class... 
        if (FileName.length() == 0)
        {
            HANDLE StdInCopy = INVALID_HANDLE_VALUE;

            if (!DuplicateHandle (GetCurrentProcess(),
                                  GetStdHandle (STD_INPUT_HANDLE), 
                                  GetCurrentProcess(),
                                  &StdInCopy,
                                  0,
                                  TRUE,
                                  DUPLICATE_SAME_ACCESS))
            {
                MessageBox (NULL, "Could not duplicate stdin handle", "ListXP Pipe Error", MB_OK | MB_ICONERROR);
                return (0);
            }

            FileName = MakeUniqueFileName ();
            Redirector = new Pipe (StdInCopy, FileName);
            Redirector->RunThread ();
            GlobalOptions.push_back (make_pair (string("tailing"), string ("on")));
            GlobalOptions.push_back (make_pair (string("_DisableTailWarning"), string ("1"))); // if they do this then they're smart enough to not be bothered by the warning dialog
        }

        // grab base filename
        LSC.CommandLine = (char *) FileName.c_str(); // this will be the "command line" for our base-case window
                                                     // ListThread will parse out any specific ?-based options
        
        // Enter open/close/open/... loop
        do
        {
            HANDLE Handles[2];

            UseNewFileName = false;

            LSC.Screen = Screen;
            LSC.LTC = &LC;
            LSC.DoneInit = false;

            ThreadHandle = CreateThread (NULL, 0, ListThread, (LPVOID)&LSC, CREATE_SUSPENDED, &ThreadID);
            Handles[0] = GetCurrentThread ();
            Handles[1] = ThreadHandle;
            UnifyThreadAffinities (Handles, 2); // makes all the threads run on the same processor

            ResumeThread (ThreadHandle);

            // We use LSC.DoneInit to serialize these next sections of code
            // First, we have set LSC.DoneInit to false. So, we wait until
            // the thread we just created changes that value. Then we continue.
            while (LSC.DoneInit == false)
                Sleep (1);

            // Invariant: LSC.DoneInit is equal to true
            // At this point, our child thread is waiting for us to change the
            // value from true to false.
            Screen->SetWindowProcContext ((void *)LSC.LTC);
            //ShowWindow (Screen->GetHWND(), ShowCmd);

            // Open up all the other files (ignore global options entries)
            for (i = 1; i < FileList.size(); i++)
            {
                if (FileList[i].length() > 0 && FileList[i][0] != '?')
                {
                    string filename;

                    filename = FileList[i];

                    if (GlobalOptionsString.length() > 0)
                        filename += string (" ") + GlobalOptionsString;

                    OpenInNewWindow (Screen->GetHWND(), filename);
                }
            }

            // Set up blinking cursor timer event
            TurnOnTimer (Screen->GetHWND());

            // So we set LSC.DoneInit to false now. The child thread is waiting for this;
            // it will actually reset it to true, just to preserve the meaning of the
            // variable in tact.
            LSC.DoneInit = false;

            while (LSC.DoneInit == false)
                Sleep (1);

            //Screen->GetGDIOutput()->FitConsoleInParent();

            // Turn back on multi-CPU shit
            RelaxThreadAffinities (Handles, 2);

            // Message loop
            while (true)
            {
                MSG msg;
                BOOL ReturnVal;

                ReturnVal = GetMessage (&msg, NULL, 0, 0);

                if (msg.message == WM_REALLY_QUIT)
                {
                    break;
                }
                else
                if (ReturnVal == -1)
                {
                    break;
                }
                else
                if (ReturnVal == 0) // got WM_QUIT
                {
                    break;
                }
                else
                {
                    TranslateMessage(&msg);
	                DispatchMessage(&msg);
                }
            }

            if (UseNewFileName)
            {
                LSC.CommandLine = (char *) NewFileName.c_str();
            }

        } while (UseNewFileName);

        if (Redirector != NULL)
        {
            string filename;

            filename = Redirector->GetFileName ();
            delete Redirector;
            DeleteFile (filename.c_str());
        }

        TurnOffTimer (Screen->GetHWND ());
        Screen->SetWindowProcContext (NULL);
        CloseHandle (ThreadHandle);
        delete Screen;
        return (0);
    } unguard;
}


// This function checks to see if a processor has SSE or Extended 3DNow!
// If it has neither, false is returned. Otherwise, true is returned.
bool CanUseStreamingCopy (void)
{
    uint32 Result = 0; // Once we are done executing our __asm block, if Result==0,
                       // then we return false. Otherwise we return true.
    uint32 MMX = 0;
    uint32 SSE = 0;
    uint32 SSE2 = 0;

    // Step 1. Verify we can use the CPUID instruction
	__try 
    {
		__asm xor    eax, eax
		__asm xor    ebx, ebx
		__asm xor    ecx, ecx
		__asm xor    edx, edx
		__asm cpuid
	}

	__except (EXCEPTION_EXECUTE_HANDLER) 
    {
		return (false); // No, we can not use CPUID, and therefore we have neither SSE or 3DNow!Ext
                        // So we must put away our boots of ghetto nard stomping and go home :(
	}

    // Step 2. Check if CPUID supports function 1 (signature/std features)
    __asm
    {
        xor     eax, eax
        cpuid
        test    eax, eax          // largest function supported = 0?
        jz      SChkDone

        //
        xor     eax, eax          // eax = 0
        inc     eax               // eax = 1
        cpuid        

        // Check for MMX support (bit 23 of edx)
        // This is requisite for either SSE or 3DNow!Ext
        test    edx, 0x00800000
        jz      SChkDone          // no MMX support
        mov     MMX, 1            // arr matey

        // Ok so we have MMX. Do we have SSE?
        test    edx, 0x02000000
        jz      SChkNoSSE         // no SSE, so check for 3DNow!Ext
        mov     SSE, 1
        mov     Result, 1

        // Check for SSE2 while we're at it
        test    edx, 0x04000000
        jz      SChkDone
        mov     SSE2, 1
        jmp     SChkDone

    SChkNoSSE:
        // Check for CPUID "extended" (AMD) functions
        mov     eax, 0x80000000
        cpuid
        cmp     eax, 0x80000000
        jbe     SChkDone          // doesn't support ext. cpuid functions, so obviously no 3DNow!Ext

        // Get the extended features info
        mov     eax, 0x80000001
        cpuid

        // Check for 3DNow!Ext
        test    edx, 0x40000000
        jz      SChkDone
        mov     Result, 1
        jmp     SChkDone

    SChkDone:

    }

    HaveMMX = bool(MMX);
    HaveSSE = bool(SSE);
    HaveSSE2 = bool(SSE2);

    // If we have SSE/SSE2, double check we have OS support
    if (HaveSSE || HaveSSE2)
    {
        _try
        {
            __asm xorps xmm0, xmm0
        }

        _except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
            {
                HaveSSE = false;
                HaveSSE2 = false;
            }
        }
    }

    if (Result == 0)
        return (false);

    return (true);
}


void SetLastEXEPath (void)
{
    guard
    {
        HKEY Key;
        LONG Result;
        char String[4096];
        char EXEName[4096];

        GetModuleFileName (GetModuleHandle (NULL), EXEName, sizeof (EXEName));
        sprintf (String, "%s", EXEName);

        Result = RegCreateKeyEx (LISTREGROOTKEY, LISTREGSUBKEY, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &Key, NULL);

        if (Result == ERROR_SUCCESS)
        {
            RegSetValueEx (Key, "LastEXEPath", 0, REG_SZ, (const BYTE *)String, strlen(String) + 1);
            RegCloseKey (Key);
        }

        return;
    } unguard;
}


string NewFileName = "";
volatile bool UseNewFileName = false;


uint32 GetCPUCount (void)
{
    guard
    {
        uint32 CPUCount;

    #ifdef WIN32
        SYSTEM_INFO SystemInfo;

        GetSystemInfo (&SystemInfo);
        CPUCount = SystemInfo.dwNumberOfProcessors;
    #else
        ?? linux ??
    #endif

        return (CPUCount);
    } unguard;
}


int __stdcall WinMain (HINSTANCE HInstance, HINSTANCE HPrevInstance, LPSTR CmdLine, int ShowCmd)
{
    string CommandLine;
    vector<string> FileList;

    try // this try block can ONLY throw a std::exception
    {
        try // the catch handlers for this will turn any non-std::exception into a std::exception
        {
            // Can we use the streaming store, or should we use memcpy?
            // This also fills in the HaveMMX/SSE/SSE2 variables
            CanUseStreamingCopy();

            // Get CPU count
            CPUCount = GetCPUCount ();

            // Set the EXE path so the shell DLL can find us
            SetLastEXEPath ();

            //
            NewFileName = string (CmdLine);
            UseNewFileName = true;

            while (UseNewFileName)
            {
                char *NewCmdLine;

                if (NewFileName.length() == 0)
                {
                    bool Result;

                    Result = ChooseFile (GetDesktopWindow(), NewFileName, "Open (ListXP)");

                    if (!Result)
                        break;
                }

                UseNewFileName = false;
                NewCmdLine = new char[NewFileName.length() + 1];
                strcpy (NewCmdLine, NewFileName.c_str());
                ListStart (HInstance, HPrevInstance, NewCmdLine, ShowCmd);        
                delete NewCmdLine;
            }
        }

        catch (exception &ex)
        {
            throw ex;
        }

        catch (...)
        {
            throw exception ("(unknown exception type)");
        }
    }

#ifdef NDEBUG
    catch (exception &ex)
    {
        MessageBox (NULL, ex.what(), "Unhandled C++ Exception", MB_OK);
        ExitProcess (1);
    }
#else
    catch (...)
    {
        throw;
    }
#endif    

    return (0);
}
