/*****************************************************************************

  C_Linkage

  This is included by all files in the C_APILib project. It determines 
  whether the functions are being imported from or exported to a DLL, or if
  they are being used in the static link library.

  #define one of the following values before #including :

  1. (don't define) - This will assume static linkage.
  2. CAPIDLL_IMP    - You are importing these functions from a DLL.
  3. CAPIDLL_EXP    - You are exporting these to a DLL (for compiling C_APILib.DLL)

*****************************************************************************/

#ifndef C_LINKAGE_H
#define C_LINKAGE_H

// DLL importing
#ifdef CAPIDLL_IMP
#define CAPILINK __declspec(dllimport)
#endif

#ifdef CAPIDLL_EXP
#define CAPILINK __declspec(dllexport)
#endif

#ifndef CAPILINK
#define CAPILINK
#endif

#endif // C_LINKAGE_H
