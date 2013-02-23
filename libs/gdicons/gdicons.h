/*****************************************************************************

  GDI Console I/O class

*****************************************************************************/


#ifndef GDICONS_H
#define GDICONS_H


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 


#ifndef CAPIDLL_IMP
//#define CAPIDLL_IMP
#endif


#include "../../C_APILib/Console.h"
#include "../../C_APILib/C_OutputGDI.h"
#include "../../C_APILib/C_InputKbd.h"


extern LRESULT CALLBACK GDIConsWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
typedef bool (*GDIConsWindowProcType) (void *Context, LRESULT &LResult, HWND HWnd, UINT Message, WPARAM WParam, LPARAM LParam);


class GDICons : public Console
{
public:
    GDICons (HWND Hwnd, int Width, int Height, char *FontName = "Courier New", int FontSize = 16);
    GDICons (char *WindowTitle, int Width, int Height, int X, int Y, char *FontName = "Courier New", int FontSize = 16,
             HINSTANCE HInstance = GetModuleHandle (NULL), 
             HICON Icon = NULL, 
             HICON IconSm = NULL,
             HMENU Menu = NULL,
             bool Show = true);

    ~GDICons ();

    void SetWindowTitle (char *WindowTitle) 
    { 
        SetWindowText (GetHWND(), WindowTitle); 
        if (Title != NULL) free (Title);
        Title = (char *) malloc (strlen(WindowTitle) + 1);
        strcpy (Title, WindowTitle);
        return;
    }

    C_OutputGDI *GetGDIOutput (void)  { return (GDIOut); }
    C_InputKbd  *GetKbdInput  (void)  { return (KbdIn);  }

    static WNDPROC GetDefaultWNDPROC (void);
    void   DoMessageLoop (void);

    void SetWindowProc (GDIConsWindowProcType NewWindowProc)
    {
        ExtWindowProc = NewWindowProc;
        return;
    }

    void SetWindowProcContext (void *NewContext)
    {
        WPContext = NewContext;
        return;
    }

    void *GetWindowProcContext (void)
    {
        return (WPContext);
    }

    GDIConsWindowProcType GetWindowProc (void)
    {
        return (ExtWindowProc);
    }

private:
    C_OutputGDI *GDIOut;
    C_InputKbd *KbdIn;
    char *Title;
    GDIConsWindowProcType ExtWindowProc;
    void *WPContext;
};


#endif // GDICons
