#include "C_InputKbd.h"


C_InputKbd::C_InputKbd ()
: C_Input ()
{
	cHWND = NULL;
}


C_InputKbd::~C_InputKbd ()
{

}


// stuffs Esc [ (a), ie, ^[[a  into the input queue
#define AddEscChar(a)  (Cons->AddKeyToQ (27), Cons->AddKeyToQ ('['), Cons->AddKeyToQ((a)))
//#define AddEscChar(a)  (Cons->AddKeyToQ (0), Cons->AddKeyToQ (nVirtKey))

BOOL C_InputKbd::WndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL ReturnVal = FALSE;
	int nVirtKey = (int) wParam;

	switch (message)
	{
		case WM_KEYDOWN:
			{
				// handle escape codes!
				switch (nVirtKey)
				{
			    	case VK_F1       : AddEscChar ('M'); ReturnVal = TRUE; break;
    				case VK_F2       : AddEscChar ('N'); ReturnVal = TRUE; break;
				    case VK_F3       : AddEscChar ('O'); ReturnVal = TRUE; break;
				    case VK_F4       : AddEscChar ('P'); ReturnVal = TRUE; break;
    				case VK_F5       : AddEscChar ('Q'); ReturnVal = TRUE; break;
				    case VK_F6       : AddEscChar ('R'); ReturnVal = TRUE; break;
				    case VK_F7       : AddEscChar ('S'); ReturnVal = TRUE; break;
    				case VK_F8       : AddEscChar ('T'); ReturnVal = TRUE; break;
				    case VK_F9       : AddEscChar ('U'); ReturnVal = TRUE; break;
    				case VK_F10      : AddEscChar ('V'); ReturnVal = TRUE; break;
				    case VK_F11      : AddEscChar ('W'); ReturnVal = TRUE; break;
				    case VK_F12      : AddEscChar ('X'); ReturnVal = TRUE; break;
				    case VK_PRIOR    : AddEscChar ('I'); ReturnVal = TRUE; break;
    				case VK_NEXT     : AddEscChar ('G'); ReturnVal = TRUE; break;
				    case VK_END      : AddEscChar ('F'); ReturnVal = TRUE; break; 
    				case VK_HOME     : AddEscChar ('H'); ReturnVal = TRUE; break; 
				    case VK_UP       : AddEscChar ('A'); ReturnVal = TRUE; break;
				    case VK_DOWN     : AddEscChar ('B'); ReturnVal = TRUE; break;
				    case VK_LEFT     : AddEscChar ('D'); ReturnVal = TRUE; break;
    				case VK_RIGHT    : AddEscChar ('C'); ReturnVal = TRUE; break;
				    case VK_INSERT   : AddEscChar ('L'); ReturnVal = TRUE; break;
                    case VK_DELETE   : Cons->AddKeyToQ (127); ReturnVal = TRUE; break;
				}
			}
    		break;

		// for regular ANSI/ASCII (whichever) characters
		case WM_CHAR:
            if (wParam != 27)
                Cons->AddKeyToQ ((CIKey)wParam);
            else
            {   // Esc key = double escape insertion, to avoid confusion with other escape sequences
                Cons->AddKeyToQ (27);
                Cons->AddKeyToQ (27);
            }

            ReturnVal = TRUE;
			break;
	}

	return (ReturnVal);
}


