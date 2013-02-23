#include "NoticeBox.h"
#include <windows.h>
#include "resource.h"
#include "List.h"
#pragma warning(disable:4312)

typedef struct
{
    bool Initial;
    bool Result;
    char *Text;
    char *Title;
} NoticeContext;


INT_PTR CALLBACK NoticeBoxProc (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    guard
    {
        NoticeContext *NC = NULL;

        NC = (NoticeContext *) GetWindowLongPtr (Hwnd, GWLP_USERDATA);

        switch (Msg)
        {
            case WM_INITDIALOG:
                SetWindowLongPtr (Hwnd, GWLP_USERDATA, (LONG_PTR)LParam);
                NC = (NoticeContext *) GetWindowLongPtr (Hwnd, GWLP_USERDATA);
                SendDlgItemMessage (Hwnd, IDC_DISABLEME, BM_SETCHECK, NC->Initial ? BST_CHECKED : BST_UNCHECKED, 0);
                SendDlgItemMessage (Hwnd, IDC_TEXTBODY, WM_SETTEXT, 0, (LPARAM)NC->Text); 
                SetWindowText (Hwnd, NC->Title);
                CenterWindow (Hwnd);
                return (FALSE);

            case WM_COMMAND:
                switch (LOWORD(WParam))
                {
                    case IDCANCEL:
                    case ID_CANCEL:
                    case IDOK:
                        NC->Result = (BST_CHECKED == SendDlgItemMessage (Hwnd, IDC_DISABLEME, BM_GETCHECK, 0, 0));
                        EndDialog (Hwnd, 1); // end value of 1 = success!
                        return (1);
                }
        }

        return (FALSE);
    } unguard;
}


bool NoticeBox (HWND Parent,
                const char *Text,
                const char *Title,
                bool InitialState,
                bool *ResultantState)
{
    guard
    {
        NoticeContext NC;
        INT_PTR Res;

        NC.Initial = InitialState;
        NC.Text = (char *)Text;
        NC.Title = (char *)Title;
        Res = DialogBoxParam (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_NOTICE), Parent, NoticeBoxProc, (LPARAM)&NC);

        if (Res == 0  ||  Res == -1)
            return (false);

        if (ResultantState != NULL)
            *ResultantState = NC.Result;

        return (true);
    } unguard;
}


