/*****************************************************************************

  NoticeBox.h

  Provides a method of popping up a dialog box that displays a bit of notice
  or warning text along with a checkbox that states "Do not show this again".

*****************************************************************************/

#ifndef _NOTICEBOX_H
#define _NOTICEBOX_H


#include "VarList.h"


extern bool NoticeBox (HWND Parent,
                       const char *Text,
                       const char *Title,
                       bool InitialState,
                       bool *ResultantState);


#endif // _NOTICEBOX_H