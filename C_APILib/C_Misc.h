/*****************************************************************************

	C_Misc.h

  Miscellanious functions

*****************************************************************************/


#ifndef C_MISC_H
#define C_MISC_H

#include <windows.h>
#include <stdio.h>
#include "C_Linkage.h"
#include "C_Types.h"

CAPILINK void  TickCountToTime (DWORD ToConvert, DTime *Result); 
CAPILINK void  TimeToTickCount (DTime *ToConvert, DWORD *Result); 

CAPILINK BOOL  SystemTimeToString      (SYSTEMTIME *SysTime, char *String);

// you should have allocates (3 chars * Significance) + 2 for *Result
CAPILINK BOOL  DeltaTimeToShortString (char *Result, int NumPlaces, DTime *Time);

CAPILINK char *AddCommas (char *Result, ULONG64 Number);
CAPILINK char *AddCommas (char *Result, DWORD   Number);

#endif
