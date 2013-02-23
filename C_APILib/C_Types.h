#ifndef C_TYPES_H
#define C_TYPES_H

#include <stdlib.h>
#include "C_Linkage.h"
#include "C_ShObj.h"

// all the different classes
class CAPILINK Console;
class CAPILINK ConsoleObj;
class CAPILINK C_Input;
class CAPILINK C_InputKbd;
class CAPILINK C_InputTelnet;
class CAPILINK C_Memory;
class CAPILINK C_Output;
class CAPILINK C_OutputGDI;
class CAPILINK C_OutputTelnet;
class CAPILINK C_Object;
class CAPILINK C_ObjectHeap;
class CAPILINK C_ObjectHeapObj;
class CAPILINK C_Settings;
class CAPILINK C_Security;
class CAPILINK C_IP;
class CAPILINK C_ThreadObj;
class CAPILINK C_Kernel;
class CAPILINK C_Program;
class CAPILINK C_ProgramObj;
class CAPILINK C_AnsiFile;
class CAPILINK C_DLLObj;
class CAPILINK PMessage;


// a.b.c.d
// a = major
// b = minor
// c = bugfixes
// d = absolute build #
#define CF_VERSION_STR "0.05.0.36"
#define CF_VERSION_NUM  0.050036f


// Console text stuff
typedef unsigned char CIKey;


class cFormat
{
public:
	COLORREF Foreground;
	COLORREF Background;
    bool     Underline;
};


static cFormat CMakeCFormat (COLORREF FG, COLORREF BG, bool Underline = false)
{
    cFormat CF;

    CF.Foreground = FG;
    CF.Background = BG;
    CF.Underline = Underline;

    return (CF);
}


typedef wchar_t cChar;


typedef struct ConsoleText
{
	cChar   Char;
	bool    Refresh;   // Set to TRUE if you want re-drawn on next call to Refresh() family call
    bool    UseFormat; // Set to TRUE to use Format field. Otherwise use defaults
                       // NOTE: THIS IS NOT USED BY CONSFRAME. I added this for ListXP ...
                       // i.e. it is for "client usage" ... use it for whatever you want I guess

//    char    Extra[2];  // 16-byte alignment. Will not ever be used. You may use this for whatever you want
	cFormat Format;
} cText;


// Macro to check if two cFormat's are equal
#define CFORMATEQ(a,b) ((a).Foreground == (b).Foreground  && \
                        (a).Background == (b).Background  && \
                        (a).Underline  == (b).Underline)     


// for ConsoleOutput drivers -- C_DIRECT says that you are in a mode that allows
// doing a Poke() and then a Refresh() to draw it. C_LINE means you are in a mode
// where output will only be outputted if WriteChar or WriteText[f] are used.
// Output drivers are expected to support both, but may choose either as their
// default depending on what's faster.
#define C_DIRECT   0
#define C_LINE     1

// size of remote hostname char strings
#define RMTHOSTLEN 256

// for ANSI color conversion
#define Black24       RGB(  0,  0,  0)
#define DarkRed24     RGB(192,  0,  0)
#define Red24         RGB(255,  0,  0)
#define DarkGreen24   RGB(  0,192,  0)
#define DarkYellow24  RGB(192,192,  0)
#define Orange24      RGB(255,192,  0)
#define Green24       RGB(  0,255,  0)
#define NeonGreen24   RGB(192,255,  0)
#define Yellow24      RGB(255,255,  0)

#define DarkBlue24    RGB(  0,  0,192)
#define DarkPurple24  RGB(192,  0,192)
#define Magenta24     RGB(255,  0,192)
#define DarkCyan24    RGB(  0,192,192)
#define Grey24        RGB(192,192,192)
#define Peach24       RGB(255,192,192)
#define LimeA24       RGB(  0,255,192)
#define LimeB24       RGB(192,255,192)
#define Lemon24       RGB(255,255,192)

#define Blue24        RGB(  0,  0,255)
#define Purple24      RGB(192,  0,255)
#define LightRed24    RGB(255,  0,255)
#define WarmBlue24    RGB(  0,192,255)
#define LightPurple24 RGB(192,192,255)
#define Pink24        RGB(255,192,255)
#define Cyan24        RGB(  0,255,255)
#define BrightCyan24  RGB(192,255,255)
#define White24       RGB(255,255,255)

// as per Photoshop
//#define Cyan24      RGB(0,157,216)


// These are the "foreground" values. Add 10 for "background" values
#define BlackANSI   30
#define RedANSI     31
#define GreenANSI   32
#define YellowANSI  33
#define BlueANSI    34
#define MagentaANSI 35
#define CyanANSI    36
#define WhiteANSI   37


#define BUILTIN(name)  DWORD CAPILINK WINAPI name (LPVOID parm)
#define PROGRAM(name)  BUILTIN(name)


#include <string>
using namespace std;


typedef unsigned __int64 ULONG64;
typedef signed __int64   SLONG64;


// Log file categorization constants
#define CF_LOG_INFO      1
#define CF_LOG_USER      2
#define CF_LOG_WARNING   3
#define CF_LOG_ERROR     4
#define CF_LOG_CRITICAL  5


// from C_Crypt.h
#define USER_LEN 64
#define PASS_LEN 64
#define CRYPT_LEN 20


#define INVALID_USER 0xFFFFFFFF
#define NO_USER_FILE 0xFFFFFFFE


typedef struct
{
	char user  [USER_LEN];
	char pass  [PASS_LEN];
	char crypt [CRYPT_LEN];

	DWORD UID;      // unique for every user.
	DWORD GID;      // group ID.
	DWORD Flags;    // this can pose certain restrictions, such as "no remote logins"
} UserInfo;


// Flags fields
#define UF_NO_REMOTE_LOGIN         (1 <<  0)   // no telnet access
#define UF_NO_LOCAL_LOGIN          (1 <<  1)   // no physical local access
#define UF_DAEMON_LOGIN            (1 <<  2)   // ie, telnetd service has to register itself SOMEHOW. well, not really, but it's nice to
#define UF_LOGIN_IP_RESTRICTED     (1 <<  3)   // check for restricted ip masks
#define UF_STATIC_PASSWORD         (1 <<  4)   // user can't change password
#define UF_CAN_ADD_USERS           (1 <<  5)   // can add users (for users other than root)
#define UF_CAN_DEL_USERS           (1 <<  6)   // can delete users (for users other than root, still can't delete root)
#define UF_CAN_SHUTDOWN            (1 <<  7)   // can use the 'shutdown' command to shut down Consframe
#define UF_ROAM_READ               (1 <<  8)   // user can go all over the hard drive w/ read-only access
#define UF_ROAM_WRITE              (1 <<  9)   // user can go all over the hard drive w/ write access
#define UF_UNLIMITED_PIDS          (1 << 10)   // user is not restricted in how many PIDs they may create


// these are dynamic, for use at run time.
#define UF_LOGGED_IN_LOCAL         (1 << 28)   // logged in locally. 'physically'
#define UF_LOGGED_IN_REMOTE        (1 << 29)   // logged in remotely, via telnet
#define UF_LOGGED_IN_DETACHED      (1 << 30)   // a detached process
#define UF_LOGGED_IN_DAEMON        (1 << 31)   // a daemon/server/driver, whatever.


// These are the infos for the "daemon" user.
#define UF_DEF_DAEMON_FLAGS       (UF_NO_REMOTE_LOGIN | UF_NO_LOCAL_LOGIN \
                                    | UF_DAEMON_LOGIN | UF_LOGIN_IP_RESTRICTED)
#define UF_DEF_DAEMON_GID         ~0
#define UF_DEF_DAEMON_UID         ~0
#define UF_DEF_DAEMON_USER        "daemon"
#define UF_DEF_DAEMON_PASS        "daemon"
#define UF_DEF_DAEMON_HOST        "localdaemon"
#define UF_DEF_DAEMON_IP1         0
#define UF_DEF_DAEMON_IP2         0
#define UF_DEF_DAEMON_IP3         0
#define UF_DEF_DAEMON_IP4         0


// login flags -- note this is only used when the daemon user is added
#define LF_DEF_DAEMON       (UF_NO_REMOTE_LOGIN       | \
                             UF_NO_LOCAL_LOGIN        | \
                             UF_DAEMON_LOGIN          | \
                             UF_LOGIN_IP_RESTRICTED   | \
                             UF_LOGGED_IN_DAEMON      | \
                             UF_STATIC_PASSWORD       | \
                             UF_ROAM_READ             | \
                             UF_ROAM_WRITE)


#define LF_DEF_REMOTE       (UF_NO_LOCAL_LOGIN | UF_LOGGED_IN_REMOTE)
#define LF_DEF_LOCAL        (UF_NO_REMOTE_LOGIN | UF_LOGGED_IN_LOCAL)




// from C_Kernel.h
#define CPROGENTRY LPTHREAD_START_ROUTINE


// user-IDs
#define ROOT 0
#define ROOT_FLAGS  UF_NO_REMOTE_LOGIN


typedef char ProgNameStr[128]; 
typedef char HelpOptStr[32];


// This is to maintain a list of programs available to be executed
typedef struct CProgramList
{
	ProgNameStr ProgramName;
	
	HelpOptStr  HelpOption;     // i.e., "/?" or "--help".   used by the ? builtin
				    // NULL if not supported (please don't do that)

	char       ExternalDLL[_MAX_FNAME];    // if not a builtin

	BOOL       Builtin;         // ie, compiled internally to ConsFrame.exe. FALSE if in a DLL
	CPROGENTRY EntryPoint;      // NULL if in a DLL. this will be determined when LoadLibrary() is run
} CPList;



typedef struct CEnvironVar
{
	char *VarName;
	char *VarValue;
} CEnv;


// from C_Program.h
// Program was abnormally terminated by ProgramTerminate()
#define CP_TERMINATED    0xF8F8F8F8


// simple helper
const char ConstMonths[12][4] = 
{ 
	"Jan", "Feb", "Mar", 
	"Apr", "May", "Jun", 
	"Jul", "Aug", "Sep", 
	"Oct", "Dec" 
};


typedef struct DTime
{
	DWORD Weeks;
	DWORD Days;
	DWORD Hours;
	DWORD Minutes;
	DWORD Seconds;
	DWORD Milliseconds;
} DeltaTime;


// To get a process snapshot:
// 1. Lock the PID Heap -> C_Kernel::LockPIDHeap();
// 2. Allocate enough memory based on C_Kernel::GetPIDCount();
// 3. Take snapshot -> C_Kernel::GetPIDInfo (...) for each PID
// 4. Unlock the PID Heap -> C_Kernel::UnlockPIDHeap();
#define PROCNAMELENGTH 32
typedef struct ProcessSnapShotType
{
	BOOL       Valid;
	DWORD      PID;
	char       UserName[USER_LEN];
	DWORD      UID;
	DWORD      GID;
	DWORD      MemUsage;
	DWORD      IdleTime;
	DWORD      LogonTick;
	SYSTEMTIME LogonTime;
	DWORD      CPUTimeTotal; // total CPU time in minutes/seconds
	DWORD      CPUUsageNow;  // current CPU usage -> divide by 100 to get a %
	DWORD      CPUUsageAvg;  // average CPU usage since the PID started -> / by 100 to get %
	char       ProcName[PROCNAMELENGTH]; // only copy the first 31 chars of argv[0]
	char       RemoteHostname[RMTHOSTLEN];
    DWORD      UserFlags;
    DWORD      LoginFlags;
	DWORD      IP32;
	UCHAR      IP1;
	UCHAR      IP2;
	UCHAR      IP3;
	UCHAR      IP4;
} PIDSnapshot;

// the PID info nation
typedef struct ProcessInformation
{
	BOOL Valid;

	DWORD PID;
	DWORD ArrayElement;
	DWORD ExitCode;

	ShObj PIDLock;

	// The Currently Selected Set
	Console     *pCons;
	C_Input     *pCin;
	C_Output    *pCout;
	C_Program   *pProg;

    // User's settings. Initially NULL until a user logs in.
    C_Settings  *pSet;

	// ie, if the stuff is reassigned with AttachPID() from an outside source,
	// then we delete what's already there, attach the given item, then assign
	// FALSE to one of these so we don't delete it later on, as it *could* be
	// in use by another process.
	BOOL DeleteConsole;

	// for when the process isn't 'attached' ... use these
	C_Input     *pCinNULL;
	C_Output    *pCoutNULL;

	DWORD StartTick;
	
	// how much CPU time a process/thread has taken up since it was started -- a "tick" total
	DWORD CPUTimeTotal; 

	// number from 0 to 10000 ... divide by 100 to get % ... updated within BUILTIN(KERNEL)
	DWORD CPUUsageNow;  // 'now' is the interval between _CPUTickLast and _CPUTickNow
	DWORD CPUUsageAvg;

	// numbers used by BUILTIN(KERNEL) to figure out CPUUsage and CPUTimeTotal
	DWORD _CPUTickLast;
	DWORD _CPUTickNow;
	DWORD _CPUUsageTickLast;
	DWORD _CPUUsageTickNow;

} ProcInfo;



// Messages
class CAPILINK PMessage
{
public:
    PMessage () { }
    PMessage (DWORD _Message, DWORD _ParmA, DWORD _ParmB, DWORD _ParmC, void *_Data = NULL)
    {
        Message = _Message;
        ParmA = _ParmA;
        ParmB = _ParmB;
        ParmC = _ParmC;
        Data = _Data;
    }

    DWORD  Message;
    DWORD  ParmA;
    DWORD  ParmB;
    DWORD  ParmC;
    void  *Data;
}; 


#define MSG_SHUTDOWN    1000
#define MSG_QUITNOW     1001


// Kernel messages
#define MSG_KERNEL_DETACH_PID        1003   // ParmA = PID to detach, ParmB = PID to send receipt message to
#define MSG_KERNEL_DETACH_RECEIPT    1004   // ParmA = PID that was detached
#define MSG_KERNEL_ATTACH_PID        1005   // ParmA = PID attaching *from*, ParmB = PID attaching *to*, ParmC = PID to send receipt message to
#define MSG_KERNEL_ATTACH_RECEIPT    1006   // ParmA = PID that was attached from, ParmB = PID attached to, ParmC = 0/1 for failure/success
#define MSG_KERNEL_KILL_PID          1007   // ParmA = PID to kill, ParmB = PID to send receipt message to
#define MSG_KERNEL_KILL_RECEIPT      1008   // ParmA = PID that was killed (sent to process that asked for kill), ParmB = 0/1 for failure/success
#define MSG_KERNEL_START_PID         1009   // ParmA = PID to be ->ProgramStart()ed

#define MSG_KERNEL_BROADCAST         1010   // ParmA = PID to sent receipt to, Data = message to broadcast
#define MSG_KERNEL_BROADCAST_RECEIPT 1011   // Data = message that was broadcast
#define MSG_KERNEL_BROADCAST_RC      1012   // No parameters. Broadcasts "MSG_RELOAD_CONFIGS"

// Normal process messages
#define MSG_RELOAD_CONFIGS           2000   // No parameters. Process should reload its configs using the C_Settings

// Console exceptions
#define CON_ACTION_NEXT      1000   // attach to next console in user's list of PIDs (CTRL+A, A)
#define CON_ACTION_PREV      1001   // attach to prev console in user's list of PIDs (CTRL+A, Z)
#define CON_ACTION_QUIT      1002   // detach the current process (CTRL+A, X)
#define CON_ACTION_NEWP      1003   // new login process, then attach to it


#endif // C_TYPES_H