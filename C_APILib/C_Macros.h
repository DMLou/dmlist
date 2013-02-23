// Provides faster access to member functions that are normally
// accessed via functions. Only useful for the Console class
// functions. Does not work with Instance->MemberFunction access
// (ie, Console *blah; blah->PokeChar(1,1) = ' '; )

// This file has been deprecated.
#define CONSOLE_MACROS_H
#ifndef CONSOLE_MACROS_H
#define CONSOLE_MACROS_H

#ifndef CSAFEPOKE
#define Poke(x,y)        ((ConsoleText[(y)][(x)]))
#define PokeRefresh(x,y) ((ConsoleText[(y)][(x)]).Refresh)
#define PokeChar(x,y)    ((ConsoleText[(y)][(x)]).Char)
#define PokeFormat(x,y)  ((ConsoleText[(y)][(x)]).Format)
#define Peek(x,y)        ((ConsoleText[(y)][(x)]))
#define PeekRefresh(x,y) ((ConsoleText[(y)][(x)]).Refresh)
#define PeekChar(x,y)    ((ConsoleText[(y)][(x)]).Char)
#define PeekFormat(x,y)  ((ConsoleText[(y)][(x)]).Format)
#endif

#define GetWinX1()       (WinX1)
#define GetWinY1()       (WinY1)
#define GetWinX2()       (WinX2)
#define GetWinY2()       (WinY2)

#define SetWinX1(x)      (WinX1 = (x))
#define SetWinY1(y)      (WinY1 = (y))
#define SetWinX2(x)      (WinX2 = (x))
#define SetWinY2(x)      (WinY2 = (x))

// Cardinal values. ie, if it returns 80, then 0..79 are valid values
#define GetConsWidth()   (ConsoleXSize)
#define GetConsHeight()  (ConsoleYSize)

// Absolute coords within the entire console
#define GetMaxConsX()    (ConsoleXSize-1)
#define GetMaxConsY()    (ConsoleYSize-1)
#define GetAbsXPos()     (CurrentXPos + WinX1)
#define GetAbsYPos()     (CurrentYPos + WinY1)

// Logical coords within the current 'window'
#define GetXPos()        (CurrentXPos)
#define GetYPos()        (CurrentYPos)
#define GetMaxX()        (WinX2 - WinX1)
#define GetMaxY()        (WinY2 - WinY1)
#define GetMinX()        (0)
#define GetMinY()        (0)

#endif
