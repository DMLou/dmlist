#include "gdicons.h"


LRESULT CALLBACK GDIConsWindowProc (HWND hWnd, UINT message, 
                                    WPARAM wParam, LPARAM lParam)
{
    GDICons *Cons;
    BOOL     DrvHandled = FALSE;

    Cons = (GDICons *) GetWindowLongPtr (hWnd, GWLP_USERDATA);

    if (message == WM_ERASEBKGND)
        return (DefWindowProc (hWnd, message, wParam, lParam));

    if (Cons != NULL)
    {
        DrvHandled = Cons->WndProc (message, wParam, lParam);

        if (Cons->GetWindowProc() != NULL)
        {
            bool Result;
            LRESULT LResult;

            Result = (Cons->GetWindowProc()) (Cons->GetWindowProcContext(), 
                LResult, hWnd, message, wParam, lParam);

            if (Result)
                return (LResult);
        }
    }

    switch (message)
    {
		case WM_ERASEBKGND:
			return (0);
			// fall through

		case WM_PAINT:
		case WM_SHOWWINDOW:
			if (DrvHandled)
				return (0);
			else
				return (1);

			break;
    }

    return (DefWindowProc (hWnd, message, wParam, lParam));
}


// To use with an existing window (it'll take over the client area)
GDICons::GDICons (HWND Hwnd, int Width, int Height, char *FontName, int FontSize)
: Console (Hwnd, Width, Height)
{
    ExtWindowProc = NULL;
    Title = NULL;
    GDIOut = NULL;
    KbdIn = NULL;
    WPContext = NULL;

    GDIOut = new C_OutputGDI;
    KbdIn = new C_InputKbd;

    GDIOut->SetFont (FontName, FontSize);
    AttachOutput (GDIOut);
    AttachInput (KbdIn);

    GDIOut->FitConsoleInParent ();
    RedrawAll ();

    return;
}


// To create a new window for the console
GDICons::GDICons (char *WindowTitle, int Width, int Height, int X, int Y, char *FontName, int FontSize,
                  HINSTANCE HInstance, 
                  HICON Icon, 
                  HICON IconSm,
                  HMENU Menu,
                  bool  Show)
: Console (NULL, Width, Height)
{
    HWND       Window;
    WNDCLASSEX WinClass;
    int WindowWidth;
    int WindowHeight;

    ExtWindowProc = NULL;
    GDIOut = NULL;
    KbdIn = NULL;
    WPContext = NULL;

    Title = NULL;
    Title = (char *) malloc (strlen (WindowTitle) + 1);
    strcpy (Title, WindowTitle);

    // Create our objects
    GDIOut = new C_OutputGDI;
    KbdIn = new C_InputKbd;
    GDIOut->SetFont (FontName, FontSize);
    WindowWidth = GDIOut->GetFXSize() * Width;
    WindowHeight = GDIOut->GetFYSize() * Height;

    // Set up and register window class
    WinClass.cbClsExtra = 0;
    WinClass.cbWndExtra = 0;
    WinClass.cbSize = sizeof (WinClass);
    WinClass.style = 0;
    WinClass.lpfnWndProc = GDIConsWindowProc;
    WinClass.cbClsExtra = 0;
    WinClass.cbWndExtra = 0;
    WinClass.hInstance = HInstance;
    WinClass.hIcon = Icon;
    WinClass.hIconSm = IconSm;
    WinClass.hCursor = LoadCursor (NULL, IDC_ARROW);
    WinClass.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
    WinClass.lpszMenuName = Title;
    WinClass.lpszClassName = Title;

    RegisterClassEx (&WinClass);

    // Create a window
    Window = CreateWindowEx
    (
        0,
        Title,
        Title,
        WS_DLGFRAME | WS_OVERLAPPEDWINDOW,
        X,
        Y,
        WindowWidth,
        WindowHeight,
        NULL,
        Menu,
        GetModuleHandle (NULL), //HInstance,
        NULL
    );

    if (!Window)
    {
        WriteTextf ("GDICons::GDICons() failed -> CreateWindowEx failed\n");
    }
    else
    {
        SetWindowLongPtr (Window, GWLP_USERDATA, (LONG_PTR)this);
        SetHWND          (Window);

        if (Show)
        {
            ShowWindow (Window, SW_SHOW | SW_SHOWNORMAL);
            UpdateWindow (Window);
        }

        AttachOutput (GDIOut);
        AttachInput (KbdIn);
        GDIOut->FitConsoleInParent ();
    }

    return;
}


GDICons::~GDICons ()
{
    if (GDIOut != NULL)
    {
        delete GDIOut;
        GDIOut = NULL;
        delete KbdIn;
        KbdIn = NULL;

        SetWindowLongPtr (GetHWND(), GWLP_USERDATA, 0);
        DestroyWindow (GetHWND());
        UnregisterClass (Title, GetModuleHandle(NULL));
    }

    if (Title != NULL)
        free (Title);

    return;
}

void GDICons::DoMessageLoop (void)
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
    {
      	TranslateMessage(&msg);
	    DispatchMessage(&msg);
    }

    return;
}

