// Easy way to include the thunks

// Define PROGPTR to your C_Program instance before #including
// also #define MEMPTR to your C_Memory instance if you want faster
// access to your memory functions


#ifdef UNDEFTHUNKS
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef GetCurrentDirectoryA
#undef SetCurrentDirectoryA
#undef fopen
#endif

#ifdef PROGPTR

// Disabled debug memory allocation routines becuase they're hella slow
// ... WAY too slow in fact
//#ifdef NDEBUG
// Normal memory allocation
#define malloc(a)    PROGPTR->Malloc(a)
#define calloc(a,b)  PROGPTR->Calloc(a,b)
#define realloc(a,b) PROGPTR->Realloc(a,b)
#define free(a)      PROGPTR->Free(a)
/*
#else
// Debug memory allocation
#define malloc(a)    PROGPTR->DbgMalloc  (a,__LINE__,__FILE__)
#define calloc(a,b)  PROGPTR->DbgCalloc  (a,b,__LINE__,__FILE__)
#define realloc(a,b) PROGPTR->DbgRealloc (a,b,__LINE__,__FILE__,#a)
#define free(a)      PROGPTR->DbgFree    (a,__LINE__,__FILE__,#a)
#endif
*/


#define GetCurrentDirectoryA(a,b)  PROGPTR->GetCurrentDir(a,b)
#define SetCurrentDirectoryA(a)    PROGPTR->SetCurrentDir(a)

#define fopen(a,b)                 PROGPTR->FileOpen(a,b)

#endif // PROGPTR


#ifdef MEMPTR

#ifdef PROGPTR
#undef malloc
#undef calloc
#undef realloc
#undef free
#endif // PROGPTR

#define malloc(a)     MEMPTR->Malloc(a)
#define calloc(a,b)   MEMPTR->Calloc(a,b)
#define realloc(a,b)  MEMPTR->Realloc(a,b)
#define free(a)       MEMPTR->Free(a)

#endif