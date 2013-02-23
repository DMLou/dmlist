#include "C_Misc.h"


void TickCountToTime (DWORD ToConvert, DTime *Result) //DWORD *Weeks, DWORD *Days, DWORD *Hours, DWORD *Minutes, DWORD *Seconds, DWORD *Ms)
{
	DWORD LWeeks;
	DWORD LDays;
	DWORD LHours;
	DWORD LMinutes;
	DWORD LSeconds;
	DWORD LMs;

	if (Result == NULL)
		return;
	
	LMs      = ToConvert % 1000;
	LSeconds = (ToConvert / 1000) % 60;
	LMinutes = (ToConvert / 60000) % 60;
	LHours   = (ToConvert / 3600000) % 24;
	LDays    = (ToConvert / 86400000) % 7;
	LWeeks   = (ToConvert / 604800000);

	Result->Weeks        = LWeeks;
	Result->Days         = LDays;
	Result->Hours        = LHours;
	Result->Minutes      = LMinutes;
	Result->Seconds      = LSeconds;
	Result->Milliseconds = LMs;

	return;
}


void TimeToTickCount (DTime *ToConvert, DWORD *Result) //, DWORD Weeks, DWORD Days, DWORD Hours, DWORD Minutes, DWORD Seconds, DWORD Ms)
{
	*Result = (604800000 * ToConvert->Weeks) +
              (86400000 * ToConvert->Days)   +
              (3600000 * ToConvert->Hours)   +
              (60000 * ToConvert->Minutes)   +
              (1000 * ToConvert->Seconds)    +
               ToConvert->Milliseconds;
}


// allocate 26 chars (25 + null) for this string
CAPILINK BOOL SystemTimeToString (SYSTEMTIME *SysTime, char *String)
{
	const char Months[13][4] = { "ERR", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	const char Days[7][4]    = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	WORD Hour;
	BOOL PM = FALSE;

	if (SysTime == NULL)
		return (FALSE);

	if (String == NULL)
		return (FALSE);

	if (SysTime->wDayOfWeek > 6)
		return (FALSE);

	if (SysTime->wMonth > 12)
		return (FALSE);

	Hour = SysTime->wHour;
	if (Hour >= 12) PM = TRUE;
	if (Hour >  12) Hour -= 12;

	sprintf (String, "%3s %3s %2u, %4u %2u:%02u%s", 
		Days[SysTime->wDayOfWeek], 
		Months[SysTime->wMonth],
		SysTime->wDay,
		SysTime->wYear,
		Hour,
		SysTime->wMinute,
		PM ? "pm" : "am");

	return (TRUE);
}

CAPILINK BOOL  DeltaTimeToShortString (char *Result, int NumPlaces, DTime *Time)
{
	int   Fields; 
	int  *Values;
	char *Chars;

	if (IsBadWritePtr (Result, 1)  ||  IsBadReadPtr (Time, sizeof(DTime)))
		return (FALSE);

	Fields = 0;
	Values = (int *) calloc (NumPlaces, sizeof (int));
	Chars  = (char *) calloc (NumPlaces, sizeof (char));

	do
	{
		if (Time->Weeks > 0)
		{
			Values[Fields] = Time->Weeks;
			Chars[Fields] = 'w';
			Fields++;
		}

		if (Fields == NumPlaces)
			break;

		if (Time->Days > 0)
		{
			Values[Fields] = Time->Days;
			Chars[Fields] = 'd';
			Fields++;
		}

		if (Fields == NumPlaces)
			break;

		if (Time->Hours > 0)
		{
			Values[Fields] = Time->Hours;
			Chars[Fields] = 'h';
			Fields++;
		}

		if (Fields == NumPlaces)
			break;

		if (Time->Minutes > 0)
		{
			Values[Fields] = Time->Minutes;
			Chars[Fields] = 'm';
			Fields++;
		}

		if (Fields == NumPlaces)
			break;

		if (Time->Seconds > 0)
		{
			Values[Fields] = Time->Seconds;
			Chars[Fields] = 's';
			Fields++;
		}
	} while (FALSE);

	if (Fields == 0)
	{
		strcpy (Result, "");
	}
	else
	if (Fields == 1)
	{
		sprintf (Result, "%d%c", Values[0], Chars[0]);
	}
	else
	if (Fields == 2)
	{
		sprintf (Result, "%d%c %d%c", Values[0], Chars[0], Values[1], Chars[1]);
	}
	else
	if (Fields == 3)
	{
		sprintf (Result, "%d%c %d%c %d%c", 
			Values[0], Chars[0], 
			Values[1], Chars[1],
			Values[2], Chars[2]);
	}
	else
	if (Fields == 4)
	{
		sprintf (Result, "%d%c %d%c %d%c %d%c", 
			Values[0], Chars[0], 
			Values[1], Chars[1],
			Values[2], Chars[2],
			Values[3], Chars[3]);
	}
	else
	if (Fields == 5)
	{
		sprintf (Result, "%d%c %d%c %d%c %d%c %d%c",
			Values[0], Chars[0], 
			Values[1], Chars[1],
			Values[2], Chars[2],
			Values[3], Chars[3],
			Values[4], Chars[4]);
	}

	free (Values);
	free (Chars);

	return (TRUE);
}


CAPILINK char *AddCommas (char *Result, ULONG64 Number)
{
	char  Temp[128];
	int   TempLen;
	char *p = NULL;
	int   AddCommas = 0;
	char *StrPosResult = NULL;
	char *StrPosOrig = NULL;

	// we get the string form of the number, then we count down w/ AddCommas
	// while copying the string from Temp1 to Result. when AddCommas % 3  == 1,
	// slap in a commas as well, before the #.
	sprintf (Temp, "%I64u", Number);
	AddCommas = TempLen = strlen (Temp);
	StrPosOrig   = Temp;
	StrPosResult = Result;
	while (AddCommas)
	{
		if ((AddCommas % 3) == 0  &&  AddCommas != TempLen) // avoid stuff like ",345"
		{
			*StrPosResult = ',';
			StrPosResult++;
		}

		*StrPosResult = *StrPosOrig;
		StrPosResult++;
		StrPosOrig++;

		*StrPosResult = 0;

		AddCommas--;
	}

	return (Result);
}


CAPILINK char *AddCommas (char *Result, DWORD Number)
{
	char  Temp[128];
	int   TempLen;
	char *p = NULL;
	int   AddCommas = 0;
	char *StrPosResult = NULL;
	char *StrPosOrig = NULL;

	// we get the string form of the number, then we count down w/ AddCommas
	// while copying the string from Temp1 to Result. when AddCommas % 3  == 1,
	// slap in a commas as well, before the #.
	sprintf (Temp, "%u", Number);
	AddCommas = TempLen = strlen (Temp);
	StrPosOrig   = Temp;
	StrPosResult = Result;
	while (AddCommas)
	{
		if ((AddCommas % 3) == 0  &&  AddCommas != TempLen) // avoid stuff like ",345"
		{
			*StrPosResult = ',';
			StrPosResult++;
		}

		*StrPosResult = *StrPosOrig;
		StrPosResult++;
		StrPosOrig++;

		*StrPosResult = 0;

		AddCommas--;
	}

	return (Result);

}

