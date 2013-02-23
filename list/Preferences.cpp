#include "Preferences.h"
#include "Settings.h"
#include "resource.h"
#include "commctrl.h"
#include "commdlg.h"


#if 0


// Takes options from LS and puts them into the dialog's controls
void SetDialogChoices (HWND Hwnd, ListSettings *LS)
{
    LONG Index;
    char NumString[32];

    ValidateSettings (LS);

    // Wrap text?
    SendMessage (GetDlgItem (Hwnd, IDC_WRAPTEXT), BM_SETCHECK, LS->GlobalWrapText ? BST_CHECKED : BST_UNCHECKED, 0);

    // Animated searches?
    SendMessage (GetDlgItem (Hwnd, IDC_ANIMATEDSEARCH), BM_SETCHECK, LS->AnimatedSearch ? BST_CHECKED : BST_UNCHECKED, 0);

    // Autodetect filters?
    SendMessage (GetDlgItem (Hwnd, IDC_DETECTFILTERS), BM_SETCHECK, LS->DetectFilters ? BST_CHECKED : BST_UNCHECKED, 0);

    // Tab size
    SetDlgItemInt (Hwnd, IDC_TABSIZE, LS->TabSize, FALSE);

    // Non-cached
    SendMessage (GetDlgItem (Hwnd, IDC_ALLOWNONCACHED), BM_SETCHECK, LS->AllowNonCached ? BST_CHECKED : BST_UNCHECKED, 0);
    SetDlgItemInt (Hwnd, IDC_NONCACHEDMINSIZE, LS->NonCachedMinSize / 1024, FALSE);

    // File cache size
    SetDlgItemInt (Hwnd, IDC_FILECACHESIZE, LS->FileCacheSize / 1024, FALSE);

    // Line cache size
    SetDlgItemInt (Hwnd, IDC_LINECACHESIZE, LS->LineCacheSize, FALSE);

    // Seek granularity
    SetDlgItemInt (Hwnd, IDC_SEEKGRANULARITY, LS->SeekGranularity, FALSE);

    // Font name
    Index = SendDlgItemMessage (Hwnd, IDC_FONTSELECT, CB_FINDSTRING, (WPARAM) -1, (LPARAM)LS->FontName.c_str());
    if (Index != CB_ERR)
        SendDlgItemMessage (Hwnd, IDC_FONTSELECT, CB_SETCURSEL, (WPARAM)Index, 0);

    // Font size
    sprintf (NumString, "%d", LS->FontSize);
    Index = SendDlgItemMessage (Hwnd, IDC_FONTSIZESELECT, CB_FINDSTRING, (WPARAM) -1, (LPARAM) NumString);
    if (Index != CB_ERR)
        SendDlgItemMessage (Hwnd, IDC_FONTSIZESELECT, CB_SETCURSEL, (WPARAM)Index, 0);

    // Colors
    SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 0, (LPARAM)LS->InfoColor.Background);
    SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 1, (LPARAM)LS->InfoColor.Foreground);
    SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 2, (LPARAM)LS->TextColor.Background);
    SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 3, (LPARAM)LS->TextColor.Foreground);

    // Transparency?
    SendMessage (GetDlgItem (Hwnd, IDC_TRANSCHECK), BM_SETCHECK, LS->Transparent ? BST_CHECKED : BST_UNCHECKED, 0);

    // Alpha level
    SetDlgItemInt (Hwnd, IDC_ALPHAEDIT, LS->AlphaLevel, FALSE);

    // Hive
    SendMessage (GetDlgItem (Hwnd, IDC_HIVEENABLED), BM_SETCHECK, LS->HiveEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SetDlgItemText (Hwnd, IDC_HIVEPATH, LS->HivePath.c_str());

    return;
}


// Takes the dialog control states and saves them into LS
// Returns true if you should "reset" the current loaded file (like if they toggle "word wrap'):
bool SaveDialogChoices (HWND Hwnd, ListSettings *LS)
{
    bool DoReset = false;
    bool NewWrapText;
    uint32 NewTabSize;
    char TextBuf[100];
    LONG Index;

    // Wrap text?
    if (BST_CHECKED == SendMessage (GetDlgItem (Hwnd, IDC_WRAPTEXT), BM_GETCHECK, 0, 0))
        NewWrapText = true;
    else
        NewWrapText = false;

    if (NewWrapText != LS->GlobalWrapText)
        DoReset = true;

    LS->GlobalWrapText = NewWrapText;
    // tag
    //LS->FileContext->WrapText = NewWrapText;

    // Animated search
    if (BST_CHECKED == SendMessage (GetDlgItem (Hwnd, IDC_ANIMATEDSEARCH), BM_GETCHECK, 0, 0))
        LS->AnimatedSearch = true;
    else
        LS->AnimatedSearch = false;

    // Autodetect filters
    if (BST_CHECKED == SendMessage (GetDlgItem (Hwnd, IDC_DETECTFILTERS), BM_GETCHECK, 0, 0))
        LS->DetectFilters = true;
    else
        LS->DetectFilters = false;

    // Tab size
    NewTabSize = GetDlgItemInt (Hwnd, IDC_TABSIZE, NULL, FALSE);

    if (NewTabSize != LS->TabSize)
        DoReset = true;

    LS->TabSize = NewTabSize;

    // Non-cached
    if (BST_CHECKED == SendMessage (GetDlgItem (Hwnd, IDC_ALLOWNONCACHED), BM_GETCHECK, 0, 0))
        LS->AllowNonCached = true;
    else
        LS->AllowNonCached = false;

    LS->NonCachedMinSize = 1024 * GetDlgItemInt (Hwnd, IDC_NONCACHEDMINSIZE, NULL, FALSE);

    // File cache size
    LS->FileCacheSize = 1024 * GetDlgItemInt (Hwnd, IDC_FILECACHESIZE, NULL, FALSE);

    // Line cache size
    LS->LineCacheSize = GetDlgItemInt (Hwnd, IDC_LINECACHESIZE, NULL, FALSE);

    // Seek granularity
    LS->SeekGranularity = GetDlgItemInt (Hwnd, IDC_SEEKGRANULARITY, NULL, FALSE);

    // Font name
    Index = SendDlgItemMessage (Hwnd, IDC_FONTSELECT, CB_GETCURSEL, 0, 0);
    if (Index != CB_ERR)
    {
        //StrLength = SendDlgItemMessage (Hwnd, IDC_FONTSELECT, CB_GETLBTEXTLEN, Index, 0);
        SendDlgItemMessage (Hwnd, IDC_FONTSELECT, CB_GETLBTEXT, Index, (LPARAM)TextBuf);    
        LS->FontName = string(TextBuf);
    }

    // Font size
    Index = SendDlgItemMessage (Hwnd, IDC_FONTSIZESELECT, CB_GETCURSEL, 0, 0);
    if (Index != CB_ERR)
    {
        SendDlgItemMessage (Hwnd, IDC_FONTSIZESELECT, CB_GETLBTEXT, Index, (LPARAM)TextBuf);
        LS->FontSize = atoi(TextBuf);
    }

    // Colors
    LS->InfoColor.Background = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 0, 0);
    LS->InfoColor.Foreground = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 1, 0);
    LS->TextColor.Background = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 2, 0);
    LS->TextColor.Foreground = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 3, 0);

    // Transparency
    if (BST_CHECKED == SendMessage (GetDlgItem (Hwnd, IDC_TRANSCHECK), BM_GETCHECK, 0, 0))
        LS->Transparent = true;
    else
        LS->Transparent = false;

    // Alpha level
    LS->AlphaLevel = GetDlgItemInt (Hwnd, IDC_ALPHAEDIT, NULL, FALSE);
    
    // Hive enabled
    if (BST_CHECKED == SendMessage (GetDlgItem (Hwnd, IDC_HIVEENABLED), BM_GETCHECK, 0, 0))
        LS->HiveEnabled = true;
    else
        LS->HiveEnabled = false;

    // Hive path
    char Path[4096];
    GetDlgItemText (Hwnd, IDC_HIVEPATH, Path, sizeof(Path));
    LS->HivePath = string (Path);

    // Validate and return.
    ValidateSettings (LS);
    return (DoReset);
}


// Returns true if the dialog choices are valid
// Returns false is the dialog choices had to be "fixed"
bool ValidateDialogChoices (HWND Hwnd, ListSettings *LS)
{
    ListSettings Copy;

    Copy = *LS;
    if (!ValidateSettings (&Copy))
    {
        *LS = Copy;
        return (false);
    }

    return (true);
}


void EnforceValidChoices (HWND Hwnd)
{
    ListSettings Settings;

    SaveDialogChoices (Hwnd, &Settings);
    ValidateDialogChoices (Hwnd, &Settings);
    SetDialogChoices (Hwnd, &Settings);

    return;
}


class PrefContext
{
public:
    ListThreadContext *LC;
    ListSettings OldSettings;
    COLORREF OldInfoBG;
    COLORREF OldInfoFG;
    COLORREF OldTextBG;
    COLORREF OldTextFG;
    bool OldTrans;
    uint8 OldAlpha;
};


INT_PTR CALLBACK PreferencesDialog (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    ListThreadContext *LC;
    PrefContext *PC;
    char NumString[32];
    int i;

    PC = (PrefContext *)GetWindowLongPtr (Hwnd, GWLP_USERDATA);
    if (PC != NULL)
        LC = PC->LC;

    switch (Msg)
    {
        case WM_INITDIALOG:
            PC = (PrefContext *) new PrefContext;
            PC->LC = (ListThreadContext *)LParam;
            SetWindowLongPtr (Hwnd, GWLP_USERDATA, (LONG_PTR)PC);
            LC = PC->LC;

            PC->OldInfoBG = LC->Settings.InfoColor.Background;
            PC->OldInfoFG = LC->Settings.InfoColor.Foreground;
            PC->OldTextBG = LC->Settings.TextColor.Background;
            PC->OldTextFG = LC->Settings.TextColor.Foreground;
            PC->OldTrans  = LC->Settings.Transparent;
            PC->OldAlpha  = LC->Settings.AlphaLevel;

            SendDlgItemMessage (Hwnd, IDC_TABSIZE_SPIN, UDM_SETRANGE32, 1, 64);
            SendDlgItemMessage (Hwnd, IDC_NONCACHEDMINSIZE_SPIN, UDM_SETRANGE32, 1, 2147483647);
            SendDlgItemMessage (Hwnd, IDC_FILECACHESIZE_SPIN, UDM_SETRANGE32, 4, 1024);
            SendDlgItemMessage (Hwnd, IDC_LINECACHESIZE_SPIN, UDM_SETRANGE32, 1, 100000);
            SendDlgItemMessage (Hwnd, IDC_SEEKGRANULARITY_SPIN, UDM_SETRANGE32, 1, 1000000);
            SendDlgItemMessage (Hwnd, IDC_ALPHA_SPIN, UDM_SETRANGE32, 0, 255);

            // Color info
            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_ADDSTRING, 0, (LPARAM)"Infobar Background");
            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_ADDSTRING, 0, (LPARAM)"Infobar Foreground");
            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_ADDSTRING, 0, (LPARAM)"Text Background");
            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_ADDSTRING, 0, (LPARAM)"Text Foreground");
            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETCURSEL, 0, 2); // Text Background

            // Fill font names dialog
            SendDlgItemMessage (Hwnd, IDC_FONTSELECT, CB_RESETCONTENT, 0, 0);
            for (i = 0; i < ValidFontsCount; i++)
            {
                SendDlgItemMessage (Hwnd, IDC_FONTSELECT, CB_ADDSTRING, 0, (LPARAM)ValidFonts[i]);
            }

            // Fill font size dialog
            SendDlgItemMessage (Hwnd, IDC_FONTSIZESELECT, CB_RESETCONTENT, 0, 0);
            for (i = ValidFontMinSize; i <= ValidFontMaxSize; i++)
            {
                sprintf (NumString, "%d", i);
                SendDlgItemMessage (Hwnd, IDC_FONTSIZESELECT, CB_ADDSTRING, 0, (LPARAM)NumString);
            }

            // Only allow the transparency options for Windows 2000 or later (or really, for any
            // Windows OS that supports SetLayeredWindowAttributes)
            if (!IsWindowAlphaAllowed())
            {
                EnableWindow (GetDlgItem (Hwnd, IDC_TRANSCHECK), FALSE);
                EnableWindow (GetDlgItem (Hwnd, IDC_ALPHA_SPIN), FALSE);
                EnableWindow (GetDlgItem (Hwnd, IDC_ALPHAEDIT), FALSE);
                EnableWindow (GetDlgItem (Hwnd, IDC_ALPHASTATIC), FALSE);
            }

            // Shell integration check mark
            SendDlgItemMessage (Hwnd, IDC_INTEGRATION, BM_SETCHECK, GetShellIntegration() ? BST_CHECKED : BST_UNCHECKED, 0);

            SetDialogChoices (Hwnd, &LC->Settings);
            SaveDialogChoices (Hwnd, &PC->OldSettings);

            // Center our ass
            RECT ParentRect;
            RECT OurRect;
            int NewX;
            int NewY;

            GetWindowRect (GetParent (Hwnd), &ParentRect);
            GetClientRect (Hwnd, &OurRect);

            NewX = (ParentRect.left + ParentRect.right - OurRect.right) / 2;
            NewY = (ParentRect.top + ParentRect.bottom - OurRect.bottom) / 2;

            SetWindowPos (Hwnd, 0, NewX, NewY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            return (TRUE);

        case WM_SYSCOMMAND:
            switch (WParam)
            {
                case SC_CLOSE:
                    SendDlgItemMessage (Hwnd, ID_CANCEL, BM_CLICK, 0, 0);
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
                case IDC_FONTSIZESELECT:
                case IDC_FONTSELECT:
                    if (Event == CBN_SELCHANGE)
                    {
                        EnforceValidChoices (Hwnd);                        
                    }
                    break;

                case IDC_TRANSCHECK:
                    SetWindowAlphaValues (LC->Screen->GetHWND(),
                        SendDlgItemMessage (Hwnd, IDC_TRANSCHECK, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false,
                        GetDlgItemInt (Hwnd, IDC_ALPHAEDIT, NULL, FALSE));

                    break;

                case ID_PICKCOLOR:
                    CHOOSECOLOR CC;
                    LONG Index;

                    ZeroMemory (&CC, sizeof (CC));
                    Index = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETCURSEL, 0, 0);

                    if (Index == CB_ERR)
                        break;

                    CC.lStructSize = sizeof (CC);
                    CC.hwndOwner = Hwnd;
                    CC.hInstance = NULL;

                    switch (Index)
                    {
                        default:
                        case 0: // InfoColor.Background
                            CC.rgbResult = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 0, 0);
                            break;

                        case 1: // InfoColor.Foreground
                            CC.rgbResult = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 1, 0);
                            break;

                        case 2: // TextColor.Background
                            CC.rgbResult = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 2, 0);
                            break;

                        case 3: // TextColor.Foreground
                            CC.rgbResult = SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_GETITEMDATA, 3, 0);
                            break;
                    }

                    CC.lpCustColors = LC->Settings.CustomColors;
                    CC.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
                    CC.lCustData = NULL;
                    CC.lpfnHook = NULL;
                    CC.lpTemplateName = NULL;

                    ChooseColor (&CC);
                    
                    switch (Index)
                    {
                        default:
                        case 0:
                            LC->Settings.InfoColor.Background = CC.rgbResult;
                            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 0, (LPARAM)CC.rgbResult);
                            break;

                        case 1:
                            LC->Settings.InfoColor.Foreground = CC.rgbResult;
                            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 1, (LPARAM)CC.rgbResult);
                            break;

                        case 2:
                            LC->Settings.TextColor.Background = CC.rgbResult;
                            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 2, (LPARAM)CC.rgbResult);
                            break;

                        case 3:
                            LC->Settings.TextColor.Foreground = CC.rgbResult;
                            SendDlgItemMessage (Hwnd, IDC_COLORLIST, CB_SETITEMDATA, 3, (LPARAM)CC.rgbResult);
                            break;
                    }

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case ID_OK:
                    bool SI;

                    if (SaveDialogChoices (Hwnd, &LC->Settings))
                        AddSimpleCommand (LC, LCMD_FILE_RESET);

                    PC->OldInfoBG = LC->Settings.InfoColor.Background;
                    PC->OldInfoFG = LC->Settings.InfoColor.Foreground;
                    PC->OldTextBG = LC->Settings.TextColor.Background;
                    PC->OldTextFG = LC->Settings.TextColor.Foreground;
                    PC->OldTrans  = LC->Settings.Transparent;
                    PC->OldAlpha  = LC->Settings.AlphaLevel;

                    // Apply font
                    if (LC->Settings.FontName != ((C_OutputGDI *)LC->Screen->GetOutput())->GetFontName()
                        || LC->Settings.FontSize != ((C_OutputGDI *)LC->Screen->GetOutput())->GetFSize())
                    {
                        ((C_OutputGDI *)LC->Screen->GetOutput())->SetFont ((char *)LC->Settings.FontName.c_str(), LC->Settings.FontSize);
                        ((C_OutputGDI *)LC->Screen->GetOutput())->FitConsoleInParent();

                        if (IsZoomed (LC->Screen->GetHWND()))
                        {
                            ShowWindow (LC->Screen->GetHWND(), SW_RESTORE);
                            ShowWindow (LC->Screen->GetHWND(), SW_MAXIMIZE);
                        }

                        LC->Screen->RedrawAll();
                    }

                    if (SendDlgItemMessage (Hwnd, IDC_INTEGRATION, BM_GETCHECK, 0, 0))
                        SI = true;
                    else
                        SI = false;

                    if (SI != GetShellIntegration())
                        SetShellIntegration (SI);

                    SaveSettings (LC);
                    LoadSettings (LC);
                    // fall through
                    
                case IDCANCEL: // so that the ESC key works
                case ID_CANCEL:
                    LC->Settings.InfoColor.Background = PC->OldInfoBG;
                    LC->Settings.InfoColor.Foreground = PC->OldInfoFG;
                    LC->Settings.TextColor.Background = PC->OldTextBG;
                    LC->Settings.TextColor.Foreground = PC->OldTextFG;
                    LC->Settings.Transparent = PC->OldTrans;
                    LC->Settings.AlphaLevel  = PC->OldAlpha;

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    delete PC;

                    SetWindowAlphaValues (LC->Screen->GetHWND(), LC->Settings.Transparent, LC->Settings.AlphaLevel);

                    EndDialog (Hwnd, 0);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    PostMessage (LC->Screen->GetHWND(), WM_UPDATE_DISPLAY, 0, 0);
                    LC->Screen->RedrawAll ();
                    return (0);

                case ID_DEFAULTS:
                    ListSettings LS;

                    SetToDefaults (&LS);
                    SetDialogChoices (Hwnd, &LS);

                    LC->Settings.InfoColor.Background = LS.InfoColor.Background;
                    LC->Settings.InfoColor.Foreground = LS.InfoColor.Foreground;
                    LC->Settings.TextColor.Background = LS.TextColor.Background;
                    LC->Settings.TextColor.Foreground = LS.TextColor.Foreground;
                    LC->Settings.Transparent = LS.Transparent;
                    LC->Settings.AlphaLevel  = LS.AlphaLevel;

                    SetWindowAlphaValues (LC->Screen->GetHWND(), LC->Settings.Transparent, LC->Settings.AlphaLevel);

                    LC->Screen->RedrawAll ();
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    return (0);
            }

            break;

        case WM_NOTIFY:
			LPNMHDR      Header;
			LPNMUPDOWN   UpDown;
			int          ControlID;
			int          Code;
			int          Delta;
            int          CurrentVal;
            int          NewVal;

			UpDown    = (LPNMUPDOWN) LParam;
			Header    = (LPNMHDR)    LParam;
			Code      = Header->code;
			ControlID = (int) WParam;

			switch (Code)
            {
                case UDN_DELTAPOS:
                    Delta = UpDown->iDelta;

                    switch (ControlID)
                    {
                        case IDC_TABSIZE_SPIN:
                            CurrentVal = GetDlgItemInt (Hwnd, IDC_TABSIZE, NULL, FALSE);
                            NewVal = CurrentVal + Delta;

                            if (NewVal < 0)
                                NewVal = 0;

                            SetDlgItemInt (Hwnd, IDC_TABSIZE, NewVal, FALSE);
                            break;

                        case IDC_NONCACHEDMINSIZE_SPIN:
                            CurrentVal = GetDlgItemInt (Hwnd, IDC_NONCACHEDMINSIZE, NULL, FALSE);
                            NewVal = CurrentVal + Delta;

                            if (NewVal < 0)
                                NewVal = 0;

                            SetDlgItemInt (Hwnd, IDC_NONCACHEDMINSIZE, NewVal, FALSE);
                            break;

                        case IDC_FILECACHESIZE_SPIN:
                            CurrentVal = GetDlgItemInt (Hwnd, IDC_FILECACHESIZE, NULL, FALSE);
                            NewVal = CurrentVal + (Delta * 4);

                            if (NewVal < 0)
                                NewVal = 4;

                            SetDlgItemInt (Hwnd, IDC_FILECACHESIZE, NewVal, FALSE);
                            break;

                        case IDC_LINECACHESIZE_SPIN:
                            CurrentVal = GetDlgItemInt (Hwnd, IDC_LINECACHESIZE, NULL, FALSE);
                            NewVal = CurrentVal + Delta;

                            if (NewVal < 0)
                                NewVal = 0;

                            SetDlgItemInt (Hwnd, IDC_LINECACHESIZE, NewVal, FALSE);
                            break;

                        case IDC_SEEKGRANULARITY_SPIN:
                            CurrentVal = GetDlgItemInt (Hwnd, IDC_SEEKGRANULARITY, NULL, FALSE);
                            NewVal = CurrentVal + Delta;

                            if (NewVal < 0)
                                NewVal = 0;

                            SetDlgItemInt (Hwnd, IDC_SEEKGRANULARITY, NewVal, FALSE);
                            break;

                        case IDC_ALPHA_SPIN:
                            CurrentVal = GetDlgItemInt (Hwnd, IDC_ALPHAEDIT, NULL, FALSE);
                            NewVal = CurrentVal + Delta;

                            if (NewVal < 0)
                                NewVal = 0;

                            if (NewVal > 255)
                                NewVal = 255;

                            SetDlgItemInt (Hwnd, IDC_ALPHAEDIT, NewVal, FALSE);
                            SetWindowAlphaValues (LC->Screen->GetHWND(),
                                SendDlgItemMessage (Hwnd, IDC_TRANSCHECK, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false,
                                GetDlgItemInt (Hwnd, IDC_ALPHAEDIT, NULL, FALSE));

                            break;
                    }

                    EnforceValidChoices (Hwnd);
                    break;
            }

            break;
    }

    return (FALSE);
}




#endif