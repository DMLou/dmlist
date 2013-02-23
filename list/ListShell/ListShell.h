#ifndef _LISTSHELL_H
#define _LISTSHELL_H


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LISTSHELL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LISTSHELL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LISTSHELL_EXPORTS
#define LISTSHELL_API __declspec(dllexport)
#else
#define LISTSHELL_API __declspec(dllimport)
#endif


extern HINSTANCE GlobalInstance;
extern UINT GlobalRefCount;


#define TRACE(msg) MessageBox (NULL, msg, "Trace", MB_OK);


#endif // _LISTSHELL_H
