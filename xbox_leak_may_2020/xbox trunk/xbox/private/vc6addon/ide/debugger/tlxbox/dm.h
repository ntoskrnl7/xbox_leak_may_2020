
#ifdef __cplusplus
extern "C" {
#endif

XOSD AttachOrReboot(void);

//
// Forward Declarations

typedef struct _OLERG *POLERG;
typedef struct _OLERET *POLERET;

struct _HPRCX;
typedef struct _HPRCX* HPRCX;

struct _HTHDX;




#define _tsizeof(str) (sizeof(str)/sizeof(TCHAR))

#define DONT_NOTIFY     ((METHOD*)0)

#define NO_ACTION       ((ACVECTOR)0)

#define INVALID         (-1L)

#define NO_SUBCLASS     ((DWORD)(-1L))

#define DWORDLONG ULONGLONG

// WaitForDebugEvent() timeout, milliseconds
#define WAITFORDEBUG_MS (50L)

// Wait for loader breakpoint timeout sec * ticks/sec
#define LDRBP_MAXTICKS  (60L * 1000L/WAITFORDEBUG_MS)

#define SetFile()

#ifndef XBOX
extern DBF *lpdbf;
#endif


#ifndef KERNEL

//extern BOOL CrashDump;

#endif

void UserTimerStop();
void UserTimerGo();

#if defined(TARGET_i386)

#if 0
#define PC(x)               ((x)->context.Eip)
#define cPC(x)              ((x)->Eip)
#define STACK_POINTER(x)    ((x)->context.Esp)
#define FRAME_POINTER(x)    ((x)->context.Ebp)
#define PcSegOfHthdx(x)     ((SEGMENT) (x->context.SegCs))
#define SsSegOfHthdx(x)     ((SEGMENT) (x->context.SegSs))
#endif
#define PC(x)               (HthdReg(x,Eip))
#define cPC(x)              ((x)->Eip)
#define STACK_POINTER(x)    (HthdReg(x,Esp))
#define FRAME_POINTER(x)    (HthdReg(x,Ebp))
#define PcSegOfHthdx(x)     ((SEGMENT) HthdReg(x,SegCs))
#define SsSegOfHthdx(x)     ((SEGMENT) HthdReg(x,SegSs))

#define MPT_CURRENT mptix86

#elif defined (TARGET_MIPS)

#define PC(x)               ((x)->context.XFir)
#define cPC(x)              ((x)->XFir)
#define STACK_POINTER(x)    ((x)->context.XIntSp)
#define FRAME_POINTER(x)    ((x)->context.XIntSp)
#define PcSegOfHthdx(x)     (0)
#define SsSegOfHthdx(x)     (0)

#define MPT_CURRENT mptmips

// Define MIPS nonvolatile register test macros. */
//

#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)

#include <mipsinst.h> // get all mips opcodes and associated cruft


//
//
// Define stack register and zero register numbers.
//

#define RA 0x1f                 /* integer register 31 */
#define SP 0x1d                 /* integer register 29 */
#define ZERO 0x0                /* integer register 0 */

//
// Define saved register masks. */
//

#define SAVED_FLOATING_MASK 0xfff00000 /* saved floating registers */
#define SAVED_INTEGER_MASK 0xf3ffff02 /* saved integer registers */


typedef union instr {
    ULONG   instruction;
    struct _jump_instr {
        ULONG   Target  : 26;
        ULONG   Opcode  : 6;
    } jump_instr;
    struct _trap_instr {
        ULONG   Opcode  : 6;
        ULONG   Value   : 10;
        ULONG   RT      : 5;
        ULONG   RS      : 5;
        ULONG   Special : 6;
    } trap_instr;
    struct _immed_instr {
        ULONG   Value   : 16;
        ULONG   RT      : 5;
        ULONG   RS      : 5;
        ULONG   Opcode  : 6;
    } immed_instr;
    struct _special_instr {
        ULONG   Funct   : 6;
        ULONG   RE      : 5;
        ULONG   RD      : 5;
        ULONG   RT      : 5;
        ULONG   RS      : 5;
        ULONG   Opcode  : 6;
    } special_instr;
    struct _float_instr {
        ULONG   Funct   : 6;
        ULONG   FD      : 5;
        ULONG   FS      : 5;
        ULONG   FT      : 5;
        ULONG   Format  : 5;
        ULONG   Opcode  : 6;
    } float_instr;
    struct {
        ULONG Function  : 6;
        ULONG Unused    : 10;
        ULONG Code      : 10;
        ULONG Opcode    : 6;
    } break_instr;
} INSTR, *PINSTR;

#elif defined (TARGET_PPC)

#define PC(x)               ((x)->context.Iar)
#define cPC(x)              ((x)->Iar)
#define STACK_POINTER(x)    ((x)->context.Gpr1)
#define FRAME_POINTER(x)    ((x)->context.Gpr1)

#define PcSegOfHthdx(x)     (0)
#define SsSegOfHthdx(x)     (0)

#define MPT_CURRENT mptmppc

#define GetThreadContext PPCGetThreadContext
#define SetThreadContext PPCSetThreadContext
BOOL
PPCGetThreadContext(
    HANDLE   hThread,
    PCONTEXT PRegContext
    );
BOOL
PPCSetThreadContext(
    HANDLE   hThread,
    PCONTEXT PRegContext
    );


//
// Define PPC nonvolatile register test macros. */
//

//#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
//#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)

#include <ppcinst.h> // get all ppc opcodes and associated cruft


//
// Define saved register masks. */
//

//#define SAVED_FLOATING_MASK 0xffffc000 /* saved floating registers */
//#define SAVED_INTEGER_MASK 0xffffe000  /* saved integer registers */



// This comes straight from ppcinst.h
typedef PPC_INSTRUCTION INSTR;

#elif defined (TARGET_ALPHA)

#include "alphaops.h"
#include "ctxptrs.h"


#define PC(x)               ((DWORD)((x)->context.Fir))
#define cPC(x)              ((DWORD)((x)->Fir))
#define STACK_POINTER(x)    ((DWORD)((x)->context.IntSp))
#define FRAME_POINTER(x)    ((DWORD)((x)->context.IntSp))

#define PcSegOfHthdx(x)         (0)
#define SsSegOfHthdx(x)         (0)

#define MPT_CURRENT mptdaxp

#else

#error "Undefined processor"

#endif


//
// Breakpoint stuff
//

#if defined(TARGET_i386)

typedef BYTE BP_UNIT;
#define BP_OPCODE   0xCC
#define DELAYED_BRANCH_SLOT_SIZE    0


#define HAS_DEBUG_REGS
// #undef NO_TRACE_FLAG

#define NUMBER_OF_DEBUG_REGISTERS   4
#define DEBUG_REG_DATA_SIZES        { 1, 2, 4 }
#define MAX_DEBUG_REG_DATA_SIZE     4
#define DEBUG_REG_LENGTH_MASKS      {   \
                            0xffffffff,          \
                            0,          \
                            1,          \
                            0Xffffffff, \
                            3           \
                            }


#define TF_BIT_MASK 0x00000100  /* This is the right bit map for */
/* the 286, make sure its correct */
/* for the 386. */

#elif defined(TARGET_PPC)

typedef DWORD BP_UNIT;
#define BP_OPCODE   0x0FE00016 // twi 31,0,DEBUG_STOP_BREAKPOINT
#define PPC_KERNEL_BREAKIN_OPCODE 0x0fe00002
#define DELAYED_BRANCH_SLOT_SIZE    0

#define HAS_DEBUG_REGS
#define NO_TRACE_FLAG

#define NUMBER_OF_DEBUG_REGISTERS   1
#define DEBUG_REG_DATA_SIZES        { 8 }
#define MAX_DEBUG_REG_DATA_SIZE     8
#define DEBUG_REG_LENGTH_MASKS      {   \
                            0,          \
                            0xffffffff, \
                            0xffffffff, \
                            0xffffffff, \
                            0xffffffff, \
                            0xffffffff, \
                            0xffffffff, \
                            0xffffffff, \
                            3           \
                            }

#elif defined(TARGET_MIPS)

typedef DWORD BP_UNIT;
#define BP_OPCODE   0x0016000D
#define DELAYED_BRANCH_SLOT_SIZE    (sizeof(DWORD))

// #undef HAS_DEBUG_REGS
#define NO_TRACE_FLAG

#elif defined(TARGET_ALPHA)

typedef DWORD BP_UNIT;
#define BP_OPCODE   0x80L
#define DELAYED_BRANCH_SLOT_SIZE    0

// #undef HAS_DEBUG_REGS
#define NO_TRACE_FLAG

#else

#error "Unknown target CPU"

#endif

//
// constant from windbgkd.h:
//
#define MAX_KD_BPS  BREAKPOINT_TABLE_SIZE
//
// machine-dependent BP instruction size
//

#define BP_SIZE     sizeof(BP_UNIT)

typedef struct DEBUGREG {
    DWORD       DataAddr;       //  Data Address
    DWORD       DataSize;       //  Data Size
    BPTP        BpType;         //  read, write, execute, etc
    BOOL        InUse;          //  In use
    DWORD       ReferenceCount;
} DEBUGREG;
typedef DEBUGREG *PDEBUGREG;

extern DWORD DebugRegDataSizes[];



#define EXADDR(pde)    ((pde)->u.Exception.ExceptionRecord.ExceptionAddress)

#define AddrFromHthdx(paddr, hthd) \
        AddrInit(paddr, 0, PcSegOfHthdx(hthd), (DWORD)PC(hthd), \
                hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal)

typedef struct _CALLSTRUCT *PCALLSTRUCT;

/*
 * These are "debug events" which are generated internally by the DM.
 * They are either remappings of certain exceptions or events which
 * do not correspond directly to a system-generated event or exception.
 */

enum    {
    BREAKPOINT_DEBUG_EVENT=(RIP_EVENT+1),
    CHECK_BREAKPOINT_DEBUG_EVENT,
    SEGMENT_LOAD_DEBUG_EVENT,
    DESTROY_PROCESS_DEBUG_EVENT,
    DESTROY_THREAD_DEBUG_EVENT,
    ATTACH_DEADLOCK_DEBUG_EVENT,
    ENTRYPOINT_DEBUG_EVENT,
    LOAD_COMPLETE_DEBUG_EVENT,
    INPUT_DEBUG_STRING_EVENT,
    MESSAGE_DEBUG_EVENT,
    MESSAGE_SEND_DEBUG_EVENT,
    FUNC_EXIT_EVENT,
    OLE_DEBUG_EVENT,
    FIBER_DEBUG_EVENT,
    GENERIC_DEBUG_EVENT,
    BOGUS_WIN95_SINGLESTEP_EVENT,
	SPECIAL_DEBUGGER_EVENT,
#ifdef OMNI
	OMNI_DLL_LOAD_EVENT,
#endif
    MAX_EVENT_CODE
  };

/*
 * This is the set of legal return values from IsCall.  The function of
 *      that routine is to analyze the instruction and determine if the
 *      debugger can simply step over it.
 */

typedef enum    {
    INSTR_TRACE_BIT,            /* Use the trace bit stepping or emulation
                                        thereof */
    INSTR_BREAKPOINT,           /* This is a breakpoint instruction     */
    INSTR_CANNOT_TRACE,         /* Can not trace this instruction       */
    INSTR_SOFT_INTERRUPT,       /* This is an interrupt opcode          */
    INSTR_IS_CALL,              /* This is a call instruction           */
    INSTR_CANNOT_STEP,                  /* In Win95 system code                                 */
} INSTR_TYPES;

typedef enum {
    THUNK_NONE = 0,
    THUNK_USER,
    THUNK_SYSTEM,
} DM32ThunkTypes;

typedef enum {
    RETURN_NONE = 0,
    RETURN_USER,
    RETURN_SYSTEM,
} DM32ReturnTypes;

typedef enum {
    ps_root       = 0x0001,     /* This is the root process, do not send a */
                                /* dbcDeleteProc when this is continued */
                                /* after a dbcProcTerm. */
    ps_preStart   = 0x0002,     /* Process is expecting loader BP */
    ps_preEntry   = 0x0004,     /* Process is expecting Entry BP */
    ps_dead       = 0x0010,     /* This process is dead. */
    ps_deadThread = 0x0020,     /* This process owns dead threads */
    ps_exited     = 0x0040,     /* We have notified the debugger that this */
                                /* process has exited. */
    ps_destroyed  = 0x0080,     /* This process has been destroyed (deleted) */
    ps_killed     = 0x0100,     /* This process is being killed */
    ps_connect    = 0x0200,
	ps_breaking	  = 0x4000
} DMPSTATE;

typedef void (*VECTOR)();

typedef struct  _EXCEPTION_LIST {
    struct _EXCEPTION_LIST *next;
    EXCEPTION_DESCRIPTION  excp;
} EXCEPTION_LIST, *LPEXCEPTION_LIST;

typedef struct _DLLLOAD_ITEM {
    BOOL        fValidDll;         // is this entry filled?
    DWORD       offBaseOfImage;    // offset for base of Image
    DWORD       cbImage;           // size of image in bytes
    LPTSTR      szDllName;         // dll name

    PIMAGE_SECTION_HEADER Sections;          // pointer to section headers
    DWORD                 NumberOfSections;  // number of section headers

#ifndef KERNEL

    BOOL        fReal;
    BOOL        fWow;
    OFFSET      offTlsIndex;    // The offset of the TLS index for the DLL
    // kentf The following comment is what I found in the sources which I
    //       hacked the OLE stuff from.
                                // ptr (in debuggee's memory space) to this
                                // DLL's 1-byte boolean flag indicating whether
                                // OLE RPC debugging is enabled.  If this DLL
                                // does not support OLE RPC, then this field
                                // will be zero.
    // However, the code in dmole.c uses this as a pointer to a function which
    // takes two args, the first of which is the above described flag, and the
    // second of which is zero.
    LPVOID      lpvOleRpc;

    BOOL        fContainsOle;   // does this DLL contain any OLE RPC segments?
    //DEBUG_EVENT de;             // load-dll notification for this DLL

#else

    DWORD                 TimeStamp;         //
    DWORD                 CheckSum;          //
    WORD                  SegCs;             //
    WORD                  SegDs;             //
    PIMAGE_SECTION_HEADER sec;               //

#endif

} DLLLOAD_ITEM, * PDLLLOAD_ITEM;

#if defined(INTERNAL)
typedef struct _DLL_DEFER_LIST {
    struct _DLL_DEFER_LIST  * next;
    LOAD_DLL_DEBUG_INFO       LoadDll;
} DLL_DEFER_LIST, *PDLL_DEFER_LIST;
#endif

/*
 * CWPI is the number of Wndproc-invoking functions that exist:
 *              SendMessage
 *              SendMessageTimeout
 *              SendMessageCallback
 *              SendNotifyMessage
 *              SendDlgItemMessage
 *              DispatchMessage
 *              CallWindowProc
 * times two (A version and W version)
 */

#define CWPI    14


//
// Data structures for timer-events
//

typedef VOID
(*TIMER_EVENT_CALLBACK) (
	HPRCX	hprc,
	LPVOID	lparam
	);

typedef struct _TIMER_EVENT {
	TIMER_EVENT_CALLBACK	callback;
	LPVOID					param;
	DWORD					ticks;
	struct _TIMER_EVENT*	next;
	HANDLE					handle;
} TIMER_EVENT;


typedef struct _TIMER_EVENT_QUEUE {
	TIMER_EVENT*		head;
	TIMER_EVENT*		waitlist;
	CRITICAL_SECTION	cs;
	DWORD				count;
} TIMER_EVENT_QUEUE;

//
// When the user requests that we begin orpc debugging, we set the
// OrpcDebugging variable in the process structure to be
// ORPC_START_DEBUGGING.  The next appropiate time -- during a step, for
// example -- we check the value of the OrpcDebgging and if it's
// ORPC_START_DEBUGGING, we call the trojan and set OrpcDebugging to to
// ORPC_DEBUGGING.  When we are ORPC_DEBUGGING and the user requests to
// stop orpc debugging we set the OrpcDebugging value to ORPC_STOP_DEBUGGING
// and at the next appropiate time, call the trojan to stop debugging.
//
// We cannot call the trojan immediately because this fails on W95.
//

typedef enum _ORPC_DEBUGGING_STATE {
    ORPC_NOT_DEBUGGING      = 0,
    ORPC_START_DEBUGGING,
    ORPC_DEBUGGING,
    ORPC_STOP_DEBUGGING
} ORPC_DEBUGGING_STATE;

typedef struct _HFBRX {
    // linked list
    struct _HFBRX   *next;
    LPVOID  fbrstrt;
    LPVOID  fbrcntx;

} HFBRXSTRUCT,*HFBRX;


typedef struct  _HPRCX {
    // linked lists
    struct _HPRCX   *next;
    struct _HTHDX   *hthdChild;
    struct _HFBRX   *FbrLst;
    PID             pid;            // OS provided process ID
    HANDLE          rwHand;         // OS provided Process handle
    BOOL            CloseProcessHandle; // If we have a private
                                    // handle to this process, close it.
                                    // Otherwise, it belongs to smss.
    DWORD           dwExitCode;     // Process exit status

    HPID            hpid;           // binding to EM object

    DMPSTATE        pstate;         // DM state model
    BOOL            f16bit;         // CreateProcess EXE was 16 bit
    EXCEPTION_LIST *exceptionList;  // list of exceptions to silently
                                    // continue unhandled
    int             cLdrBPWait;     // timeout counter while waiting for ldr BP
	LONG			fExited;		// synchronization object for
                                    // process termination
    PDLLLOAD_ITEM   rgDllList;      // module list
    int             cDllList;       // item count for module list

    HANDLE          hEventCreateThread;  // Sync object for thread creation
	ULONG			StopCount;
	//BOOL			fSoftBroken;

	BOOL            fUseFbrs;       // Use fiber context or thread context
    PVOID           pFbrCntx;       // Pointer to a fiber context to display
                                    // NULL = use thread context
    DWORD           dwKernel32Base; // lpBaseOfDll for kernel32.

    DWORD           colerg;         // number of OLE ranges in *rgolerg
    POLERG          rgolerg;        // array of OLERGs: sorted list of all
                                    // addresses in this process (including
                                    // its DLLs) which are special OLE
                                    // segments.  May be
                                    // NULL if colerg is zero.

    ORPC_DEBUGGING_STATE    OrpcDebugging;  // orpc debugging state (see above)
    UOFFSET					gwpoff[CWPI];  // addrs of Wndproc-invoking functions
    HLLI					llnlg;          // non-local goto
    TID						lastTidDebugEvent;

	BOOL				fStepInto;		// Are we stepping into on this proc
	TIMER_EVENT_QUEUE	TimerQueue;		// For timer-events.

	HANDLE				hExitFailed;	// TIMER_EVENT for when the process
										// does not return a valid
										// EXIT_PROCESS event upon exit.
	
	HANDLE				hAsyncStopRequest;	// TIMER_EVENT for async breaking

#if defined(TARGET_i386)
    SEGMENT         segCode;
#endif

#ifdef KERNEL

    BOOL            fRomImage;      // rom image
    BOOL            fRelocatable;   // relocatable code

#endif


#if defined(INTERNAL)

	//
    // during process startup, dll name resolution may be
    // deferred until the loader BP.  Once the process is
    // fully initialized, this deferral is no longer allowed.
	//
	
    BOOL            fNameRequired;
    PDLL_DEFER_LIST pDllDeferList;

#endif


} HPRCXSTRUCT, *HPRCX;

#define hprcxNull       ((HPRCX) 0)

typedef enum {
    //ts_preStart =0x1000,        /* Before the starting point of the thread */
                                /* from this state a registers and trace   */
                                /* are dealt with specially                */
    ts_running  = 0x001,        /* Execution is proceeding on the thead    */
    ts_stopped  = 0x002,        /* An event has stopped execution          */
    ts_frozen   = 0x010,        /* Debugger froze thread.                  */
    ts_dead     = 0x020,        /* Thread is dead.                         */
    ts_destroyed =0x040,        /* Thread is destroyed (deleted)           */
    ts_first    = 0x100,        /* Thread is at first chance exception     */
    ts_second   = 0x200,        /* Thread is at second chance exception    */
    ts_rip      = 0x400,        /* Thread is in RIP state                  */
    ts_stepping = 0x800,		/*                                         */
	ts_breaking = 0x1000,		/* Trying async break this thread          */
    ts_funceval = 0x40000000    /* Thread is being used for function call  */
} TSTATEX;

typedef struct  _WTNODE {
    struct _WTNODE      *caller;      // caller's wtnode
    struct _WTNODE      *callee;      // current function called by this function
    DWORD               offset;       // address of this function
    DWORD               sp;           // SP for this frame
    int                 icnt;         // number of instructions executed
    int                 scnt;         // subordinate count
    int                 lex;          // lexical level of this function
    LPSTR               fname;        // function name
} WTNODE, *LPWTNODE;


typedef struct  _HTHDX {
    struct  _HTHDX    *next;
    struct  _HTHDX    *nextSibling;
    HPRCX             hprc;
    HTID              htid;
    TID               tid;
    HANDLE            rwHand;
    LPVOID            lpStartAddress;
    CONTEXT           ctx;
    LPVOID            atBP;
    TSTATEX           tstate;
    BOOL              fExceptionHandled;
    DWORD             stackRA;
    DWORD             stackBase;
    int               cFuncEval;
    DWORD             dwExitCode;
    OFFSET            offTeb;

    BOOL              fContextDirty;// has the context changed?
    BOOL              fContextStale;// does the context need to be refreshed?
    BOOL              fContextExtendedDirty; //extended registers need be written 	

    BOOL              fAddrIsFlat;  // Is this address segmented?
    BOOL              fAddrIsReal;  // Is this address in real mode?
    BOOL              fAddrOff32;   // Is the offset of this addres 32 bits?
    BOOL              fDontStepOff; //

    BOOL              fWowEvent;    // Was the last event WOW?

    ADDR              addrIsCall;
    int               iInstrIsCall;

    EXCEPTION_RECORD  ExceptionRecord;

    BOOL              fIsCallDone;
    BOOL              fDisplayReturnValues;
    BOOL              fStopOnNLG;
    BOOL              fReturning;

    ADDR              addrFrom;
    ADDR              addrStack;

    WTNODE            wthead;       // root of the call tree for a wt command
    LPWTNODE          wtcurr;       // current wtnode
    DWORD             wtmode;       // wt command executing?

    LIST_ENTRY        WalkList;     // Walks associated with this thread
    PVOID             WalkData;     // Per-thread walk data

	BOOL			  fSoftBroken;	// this is for Async-stop fake soft break

#ifdef HAS_DEBUG_REGS
    DEBUGREG          DebugRegs[NUMBER_OF_DEBUG_REGISTERS];
#endif

#ifndef KERNEL
    char              szThreadName[32];
#endif // !KERNEL

 	BOOL       	      fPassException;
} HTHDXSTRUCT, *HTHDX;

typedef void (*ACVECTOR)(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);
typedef void (*DDVECTOR)(DEBUG_EVENT*, HTHDX);

#define hthdxNull ((HTHDX) NULL)

typedef struct _BREAKPOINT {
    struct _BREAKPOINT *next;
    HPRCX       hprc;       // The process the BP belongs to
    HTHDX       hthd;       // The thread the BP belongs to
    int         cthd;       // The number of threads pointing at this bp (atBP)
    BPTP        bpType;     // OSDebug BP type
    BPNS        bpNotify;   // OSDebug notify type

    ADDR        addr;       // The address of the Breakpoint

    HANDLE      hWalk;      // The associated walk handle.
    
    BYTE        instances;   // The # of instances that exist
    HPID        id;          // Id supplied by the EM
    BOOL        isStep;      // Single step flag
    DWORD       hBreakPoint; // kernel debugger breakpoint handle
} BREAKPOINT;
typedef BREAKPOINT *PBREAKPOINT;

//
// these are magic values used in the hthd->atBP field.
//

#define EMBEDDED_BP     ((PBREAKPOINT)(-1))

//
// These are used in the id field of a BREAKPOINT.
//
#define ENTRY_BP        ((ULONG) -2)
#define ASYNC_STOP_BP   ((ULONG) -3)

extern  BREAKPOINT      masterBP , *bpList;

typedef struct _METHOD {
    ACVECTOR notifyFunction; /* The notification function to call */
    void   *lparam;        /* The parameter to pass to it */
    void   *lparam2;       /* Extra pointer in case the method */
    /* needs to be freed afterwards */
} METHOD;
typedef METHOD *PMETHOD;

typedef struct _EXPECTED_EVENT {
    struct   _EXPECTED_EVENT  *next;
    HPRCX    hprc;
    HTHDX    hthd;
    DWORD    eventCode;
    DWORD    subClass;
    METHOD*  notifier;
    ACVECTOR action;
    BOOL     fPersistent;
    LPVOID   lparam;
} EXPECTED_EVENT;
typedef EXPECTED_EVENT *PEXPECTED_EVENT;


typedef VOID    (*STEPPER)(HTHDX,METHOD*,BOOL, BOOL);

typedef DWORD   (*CDVECTOR)(HPRCX,HTHDX,LPDBB);

typedef struct {
    DMF         dmf;
    CDVECTOR    function;
    WORD        type;
} CMD_DESC;


enum {
    BLOCKING,
    NON_BLOCKING,
    REPLY
};


#if 0
/*
 * Setup for a CreateProcess to occur
 */

typedef struct _SPAWN_STRUCT {
    BOOL                fSpawn;
    HANDLE              hEventApiDone;

    BOOL                fReturn;    // return from API
    DWORD               dwError;

    char *              szAppName;  // args to API etc
    char *              szArgs;
    char *              pszCurrentDirectory; // directory to spawn process.
    DWORD               fdwCreate;
    BOOL                fInheritHandles;
    STARTUPINFO         si;
} SPAWN_STRUCT, *PSPAWN_STRUCT;
#endif

/*
 *      Setup for a DebugActiveProcess to occur
 */

typedef struct _DEBUG_ACTIVE_STRUCT {
    volatile BOOL fAttach;          // tell DmPoll to act
    HANDLE        hEventApiDone;    // signal shell that API finished
    HANDLE        hEventReady;      // clear until finished loading

    BOOL          fReturn;          // API return value
    DWORD         dwError;          // GetLastError() value

    DWORD         dwProcessId;      // pid to debug
    HANDLE        hEventGo;         // signal after hitting ldr BP
} DEBUG_ACTIVE_STRUCT, *PDEBUG_ACTIVE_STRUCT;

//
// packet for starting WT (Watch Trace)
//
typedef struct _WT_STRUCT {
    BOOL          fWt;
    DWORD         dwType;
    HTHDX         hthd;
} WT_STRUCT, *LPWT_STRUCT;

//
// Packet for killing a process
//
typedef struct _KILLSTRUCT {
    struct _KILLSTRUCT * next;
    HPRCX                hprc;
} KILLSTRUCT, *PKILLSTRUCT;

extern  BOOL    StartDmPollThread(void);
extern  BOOL    StartCrashPollThread(void);


extern BOOL SearchPathSet;
extern char SearchPathString[];

//
//  Single stepping stuff
//
typedef struct _BRANCH_NODE {
    BOOL    TargetKnown;     //  Know target address
    BOOL    IsCall;          //  Is a call instruction
    ADDR    Addr;            //  Branch instruction address
    ADDR    Target;          //  Target address
} BRANCH_NODE;

#pragma warning( disable: 4200)

typedef struct _BRANCH_LIST {
    ADDR        AddrStart;      //  Start of range
    ADDR        AddrEnd;        //  End of range
    DWORD       Count;          //  Count of branch nodes
    BRANCH_NODE BranchNode[0];  //  List of branch nodes
} BRANCH_LIST;

#pragma warning( default: 4200 )

DWORD
BranchUnassemble(
    HTHDX   hthd,
    void   *Memory,
    DWORD   Size,
    ADDR   *Addr,
    BOOL   *IsBranch,
    BOOL   *TargetKnown,
    BOOL   *IsCall,
    BOOL   *IsTable,
    ADDR   *Target
    );


//
//  Structure for doing range stepping
//
typedef struct _RANGESTRUCT {
    HTHDX        hthd;          //  thread
    BOOL         fStepOver;     //  Step over flag
    BOOL         fStopOnBP;     //  Stop on BP flag
    METHOD       *Method;       //  Method
    DWORD        BpCount;       //  Count of temporary breakpoints
    ADDR         *BpAddrs;      //  List of breakpoint addresses
    BREAKPOINT   **BpList;      //  List of breakpoints
    BRANCH_LIST  *BranchList;   //  branch list
    ADDR         PrevAddr;      //  For single stepping
    BOOL         fSingleStep;   //  For single stepping
    ADDR         TmpAddr;       //  For single stepping
    BOOL         fInCall;       //  For single stepping
    BREAKPOINT   *TmpBp;        //  For single stepping
} RANGESTRUCT;

BOOL
SmartRangeStep(
    HTHDX       hthd,
    UOFF32      offStart,
    UOFF32      offEnd,
    BOOL        fStopOnBP,
    BOOL        fStepOver
    );

VOID
MethodSmartRangeStep(
    DEBUG_EVENT* pde,
    HTHDX hthd,
    DWORD unused,
    RANGESTRUCT* RangeStruct
    );

typedef struct  _RANGESTEP {
    HTHDX       hthd;           // The thread's structure
    SEGMENT     segCur;         // Segment to do range stepping in
    UOFF32      addrStart;      // starting address of range step
    UOFF32      addrEnd;        // ending address of range step
    SEGMENT     SavedSeg;       // Save locations for thunk stepping
    UOFF32      SavedAddrStart; //  "       "
    UOFF32      SavedAddrEnd;   //  "       "
	UOFF32      SavedEBP;       // Save the EBP register (may not be the Frame Pointer!)
    STEPPER     stepFunction;   // The step function to call
    METHOD      *method;        // The method to handle this event
    BREAKPOINT  *safetyBP;      // Safety BP
    BOOL        fIsCall;        // just traced a call instruction?
    BOOL        fIsRet;         // just traced a ret?
    BOOL        fInThunk;       // stepping in a thunk?
    BOOL        fSkipProlog;    // step past prolog on function entry
    BOOL        fGetReturnValue;// Getting a return value.
} RANGESTEP;

extern DEBUG_EVENT FuncExitEvent;
extern HINSTANCE hInstance; // The DM DLLs hInstance
extern DWORD g_ProcFlags;	// PROCESSOR_FLAGS_xxx

VOID
RangeStep(
    HTHDX       hthd,
    UOFF32      offStart,
    UOFF32      offEnd,
    BOOL        fStopOnBP,
    BOOL        fstepOver
    );

VOID
MethodRangeStep(
    DEBUG_EVENT* pde,
    HTHDX hthd,
    DWORD unused,
    RANGESTEP* RangeStep
    );

VOID
IsCall(
    HTHDX hthd,
    LPADDR lpAddr,
    LPINT lpFlag,
    BOOL fStepOver
    );

VOID
IsCallEx(
    HTHDX hthd,
    LPADDR lpAddr,
    LPINT lpFlag,
    BOOL fStepOver,
	LPADDR	addrDest
    );

VOID
DecrementIP(
    HTHDX hthd
    );

VOID
IncrementIP(
    HTHDX hthd
    );

BOOL
IsRet(
    HTHDX hthd,
    LPADDR addr
    );

VOID
ContinueFromBP(
    HTHDX hthd,
    PBREAKPOINT pbp
    );

#define CB_THUNK_MAX    32

BOOL
IsThunk (
    HTHDX       hthd,
    UOFFSET     uoffset,
    LPINT       lpfThunkType,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsDirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsIndirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsVCallThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsVTDispAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
GetPMFDest(
    HTHDX hthd,
    UOFFSET uThis,
    UOFFSET uPMF,
    UOFFSET *lpuOffDest
    );

BOOL
SetupSingleStep(
    HTHDX hthd,
    BOOL DoContinue,
    BOOL fTraceSet 
    );

BOOL
SetupReturnStep(
    HTHDX hthd,
    BOOL  DoContinue,
    LPADDR lpaddr,
    LPADDR addrStack
    );

DWORD
GetCanStep (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPCANSTEP lpCanStep
    );

DWORD
GetEndOfRange (
    HPRCX,
    HTHDX,
    DWORD
    );

VOID
SingleStep(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval
    );

VOID
SingleStepEx(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    BOOL fDoContinue
    );

VOID
ReturnStep(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    LPADDR addrRA,
    LPADDR addrStack
    );

VOID
ReturnStepEx(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    LPADDR addrRA,
    LPADDR addrStack,
    BOOL fDoContinue,
    BOOL fCheckOrpc
    );


VOID
StepOverEx(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    BOOL fCheckOrpc
    );

VOID
StepOver(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval
    );

VOID
MoveIPToException(
    HTHDX hthd,
    LPDEBUG_EVENT pde
    );

void
MethodContinueSS(
    DEBUG_EVENT *pde,
    HTHDX hthd,
    DWORD unused,
    METHOD *method
    );


BOOL
DecodeSingleStepEvent(
    HTHDX hthd,
    DEBUG_EVENT *de,
    PDWORD eventCode,
    PDWORD subClass
    );

VOID
WtRangeStep(
    HTHDX hthd
    );

VOID
WtMethodRangeStep(
    DEBUG_EVENT* pde,
    HTHDX hthd,
    DWORD unused,
    RANGESTEP* RangeStep
    );

BOOL
GetWndProcMessage(
    HTHDX   hthd,
    UINT*   message
    );

//
// Function calling, for internal use
//

typedef struct _CALLSTRUCT {
    PBREAKPOINT pbp;
    LPVOID      atBP;
    CONTEXT     context;
    ACVECTOR    Action;
    BOOL        HasReturnValue;
    LPARAM      lparam;
} CALLSTRUCT, *PCALLSTRUCT;

BOOL
WINAPIV
CallFunction(
    HTHDX       hthd,
    ACVECTOR    Action,
    LPARAM      lparam,
    BOOL        HasReturnValue,
    LPVOID      Function,
    int         cArgs,
    ...
    );

//
// This function is machine-specific
//
VOID
vCallFunctionHelper(
    HTHDX hthd,
    FARPROC lpFunction,
    int cArgs,
    va_list vargs
    );

//
// This function is machine-specific
//

DWORDLONG
GetFunctionResult(
    PCALLSTRUCT pcs
    );


void
SetDebugEventThreadState(
    HPRCX   hprc,
    TSTATEX RemoveState
    );
//
// Win95 support
//

BOOL IsInSystemDll ( UOFFSET uoffDest );
void SendDBCErrorStep(HPRCX hprc);

/*
 *
 */

#ifdef KERNEL
extern  void    ProcessDebugEvent( DEBUG_EVENT *de, DBGKD_WAIT_STATE_CHANGE  *sc );
extern  VOID    ProcessHandleExceptionCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessIgnoreExceptionCmd(HPRCX,HTHDX,LPDBB);
extern  BOOL    ProcessFrameStackWalkNextCmd( HPRCX, HTHDX, PCONTEXT, LPVOID );
extern  VOID    ProcessGetExtendedContextCmd(HPRCX hprc,HTHDX hthd,LPDBB lpdbb);
extern  VOID    ProcessSetExtendedContextCmd(HPRCX hprc,HTHDX hthd,LPDBB lpdbb);
extern  void    DeleteAllBps( VOID );
extern  VOID    DmPollTerminate( VOID );

#else

extern  void    ProcessDebugEvent( DEBUG_EVENT *de );
extern  VOID    ProcessBPAcceptedCmd( HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessGetDRegsCmd(HPRCX hprc, HTHDX hthd, LPDBB lpdbb);
extern  VOID    ProcessSetDRegsCmd(HPRCX hprc, HTHDX hthd, LPDBB lpdbb);

#endif


extern  void    ProcessExceptionEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessCreateThreadEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessCreateProcessEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessExitThreadEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessExitProcessEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessLoadDLLEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessUnloadDLLEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessOutputDebugStringEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessBreakpointEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessRipEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessBogusSSEvent(DEBUG_EVENT*, HTHDX);


extern  void    NotifyEM(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);
extern  void    ConsumeThreadEventsAndNotifyEM(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);
extern  void    FreeHthdx(HTHDX hthd);
extern  XOSD    FreeProcess( HPRCX hprc, BOOL fKillRoot);

extern  VOID    ProcessCreateProcessCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessProcStatCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessThreadStatCmd(HPRCX,HTHDX,LPDBB);
extern  void    ProcessSpawnOrphanCmd(HPRCX,HTHDX,LPDBB);
extern  void    ProcessProgLoadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessUnloadCmd(HPRCX,HTHDX,LPDBB);

extern  VOID    ProcessReadMemoryCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessWriteMemoryCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetContextCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetSectionsCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSetContextCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSingleStepCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessRangeStepCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessReturnStepCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessExecuteCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessContinueCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessFreezeThreadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessTerminateThreadCmd(HPRCX,HTHDX,LPDBB);
extern  BOOL	ProcessTerminateProcessCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessAsyncGoCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetFP(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessIoctlCmd( HPRCX, HTHDX, LPDBB );
extern  VOID    ProcessSSVCCustomCmd( HPRCX, HTHDX, LPDBB );
extern  VOID    ProcessSelLimCmd( HPRCX, HTHDX, LPDBB );
extern  VOID    ClearContextPointers(PKNONVOLATILE_CONTEXT_POINTERS);
extern  VOID    ProcessDebugActiveCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessAsyncStopCmd(HPRCX, HTHDX, LPDBB );
extern  VOID    ProcessAllProgFreeCmd( HPRCX hprcXX, HTHDX hthd, LPDBB lpdbb );
extern  VOID    ProcessSetPathCmd( HPRCX hprcXX, HTHDX hthd, LPDBB lpdbb );
extern  VOID    ProcessQueryTlsBaseCmd( HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessQuerySelectorCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessReloadModulesCmd(HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessVirtualQueryCmd(HPRCX hprcx, LPDBB lpdbb);
extern  VOID    ProcessGetDmInfoCmd(HPRCX hprc, LPDBB lpdbb, DWORD cb);
extern  VOID    ProcessRemoteQuit(VOID);
extern  ULONG   ProcessGetTimeStamp (HPRCX, HTHDX, LPDBB);
extern  ULONG   ProcessGetSpecialReg (HPRCX, HTHDX, LPDBB);
extern  ULONG   ProcessSetSpecialReg (HPRCX, HTHDX, LPDBB);

VOID
ProcessGetFrameContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

VOID
ProcessGetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

VOID
ProcessSetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

EXCEPTION_FILTER_DEFAULT
ExceptionAction(
    HPRCX hprc,
    DWORD dwExceptionCode
    );

VOID
RemoveExceptionList(
    HPRCX hprc
    );

EXCEPTION_LIST *
InsertException(
    EXCEPTION_LIST ** ppeList,
    LPEXCEPTION_DESCRIPTION lpexc
    );

VOID
ProcessBreakpointCmd(
    HPRCX hprcx,
    HTHDX hthdx,
    LPDBB lpdbb
    );

VOID
ProcessSystemServiceCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );


VOID
InitExceptionList(
    HPRCX hprc
    );

VOID
CompleteTerminateProcessCmd(
    VOID
    );


//
// Public functions from walk.c
//

VOID
ExprBPCreateThread(
    HPRCX hprc,
    HTHDX hthd
    );

VOID
ExprBPExitThread(
    HPRCX hprc,
    HTHDX hthd
    );

VOID
ExprBPContinue(
    HPRCX hprc,
    HTHDX hthd
    );

VOID
ExprBPRestoreDebugRegs(
    HTHDX hthd
    );

VOID
ExprBPClearBPForStep(
    HTHDX hthd
    );

VOID
ExprBPResetBP(
    HTHDX hthd,
    PBREAKPOINT bp
    );

VOID
ExprBPInitialize(
    VOID
    );

PBREAKPOINT
GetWalkBPFromBits(
    HTHDX   hthd,
    DWORD   bits
    );

BOOL
IsWalkInGroup(
    HANDLE hWalk,
    PVOID pWalk
    );

HANDLE
GetNewWalk(
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD   Addr,
    DWORD   Size,
    DWORD   BpType,
    BOOL    fEmulate
    );

BOOL
RemoveWalk(
    HANDLE hWalk,
    BOOL Global
    );

BOOL
CheckDataBP(
    HTHDX hthd,
    PBREAKPOINT Bp
    );

VOID
ClearThreadWalkFlags(
    HTHDX   hthd
    );

VOID
ClearProcessWalkFlags(
    HPRCX   hprc
    );

//
//
//


#ifdef HAS_DEBUG_REGS
BOOL
SetupDebugRegister(         // implemented in mach.c
    HTHDX       hthd,
    int         Register,
    int         DataSize,
    DWORD       DataAddr,
    DWORD       BpType
    );

VOID
ClearDebugRegister(
    HTHDX hthd,
    int Register
    );

#endif

VOID
ClearAllDebugRegisters(
    HPRCX hprc
	);



extern
void
SSActionReplaceByte(
    DEBUG_EVENT *de,
    HTHDX hthdx,
    DWORD unused,
    PBREAKPOINT bp
    );

extern
void
SSActionRBAndContinue(
    DEBUG_EVENT*,
    HTHDX,
    DWORD,
    BREAKPOINT*
    );


extern
void SSActionRemoveBP(
    DEBUG_EVENT *de,
    HTHDX hthd,
    DWORD unused,
    METHOD* method
    );

extern
void
SSActionStepOver(
    DEBUG_EVENT* de,
    HTHDX hthd,
    DWORD unused,
    METHOD* method
    );

extern
void
ActionDefineProcess(
    DEBUG_EVENT *de,
    HTHDX hthd,
    DWORD unused,
    HPRCX hprc
    );

extern
void ActionAllDllsLoaded(
    DEBUG_EVENT *de,
    HTHDX hthd,
    DWORD unused,
    HPRCX hprc
    );

extern
void
ActionDebugActiveReady(
    DEBUG_EVENT *pde,
    HTHDX hthd,
    DWORD unused,
    HPRCX hprc
    );

extern
void
ActionDebugNewReady(
    DEBUG_EVENT *pde,
    HTHDX hthd,
    DWORD unused,
    HPRCX hprc
    );

extern
void
ActionExceptionDuringStep(
    DEBUG_EVENT* de,
    HTHDX hthd,
    DWORD unused,
    LPVOID lpv
    );

extern
void *
InfoExceptionDuringStep(
    HTHDX hthd
    );


BOOL
CDECL
DMPrintShellMsg(
    PCHAR szFormat,
    ...
    );

BOOL
WINAPIV
DMPrintShellMsgEx(
	DWORD	ids,
	...
	);
	

//
// event.c
//

PEXPECTED_EVENT
RegisterExpectedEvent(
    HPRCX hprc,
    HTHDX hthd,
    DWORD eventcode,
    DWORD subclass,
    METHOD* method,
    ACVECTOR action,
    BOOL persistent,
    LPVOID lparam
    );

PEXPECTED_EVENT
PeeIsEventExpected(
    HTHDX hthd,
    DWORD eventcode,
    DWORD subclass,
        BOOL bRemove
    );

VOID
ConsumeAllThreadEvents(
    HTHDX hthd,
    BOOL ConsumePersistent
    );

VOID
ConsumeAllProcessEvents(
    HPRCX hprc,
    BOOL ConsumePersistent
    );

VOID
ConsumeSpecifiedEvent(
    PEXPECTED_EVENT ee
    );

#if 0
//
//
//

XOSD
Load(
    HPRCX hprc,
    LPCTSTR szAppName,
    LPCTSTR szArg,
    LPVOID pattrib,
    LPVOID tattrib,
    DWORD creationFlags,
    BOOL inheritHandles,
    CONST LPCTSTR* environment,
    LPCTSTR currentDirectory,
    STARTUPINFO FAR * pstartupInfo,
    LPPROCESS_INFORMATION lppi
    );
#endif

HPRCX
InitProcess(
    HPID hpid
    );

VOID
SetExceptionAddress(
    DEBUG_EVENT*    pde,
    ULONG           addr
    );

#if defined(TARGET_MIPS) || defined(TARGET_ALPHA) || defined(TARGET_PPC)
VOID
RemoveFuncList(
    HPRCX hprc
    );
#endif

#if defined(TARGET_MIPS)

typedef enum { Ctx32Bit, Ctx64Bit } MIPSCONTEXTSIZE;
extern MIPSCONTEXTSIZE MipsContextSize;

BOOL
CoerceContext64To32(
    PCONTEXT pContext
    );

BOOL
CoerceContext32To64 (
    PCONTEXT pContext
    );
#endif // TARGET_MIPS

VOID
WINAPI
DMFunc(
    DWORD cb,
    LPDBB lpdbb
    );


VOID
ReConnectDebugger(
    DEBUG_EVENT *de,
    BOOL fNoDllLoad
    );


#if defined(TARGET_MIPS) || defined(TARGET_ALPHA) || defined(TARGET_PPC)
extern  ULONG       GetNextOffset (HTHDX, BOOL);
#endif

extern void SetupEntryBP(HTHDX hthd);
void DestroyDllLoadItem(PDLLLOAD_ITEM pDll);


VOID
Reply(
    UINT length,
    void * lpbBuffer,
    HPID hpid
    );


// Use this specifically to send errors about process startup.
VOID
SendNTError(
        HPRCX hprc,
        DWORD dwErr,
        LPTSTR lszString
        );

// Used for other dbcError's.

VOID
SendDBCError(
        HPRCX hprc,
        DWORD dwErr,
        LPTSTR lszString
        );

/*
 **
 */

#if DBG

#ifndef XBOX
#define assert(exp) if (!(exp)) {lpdbf->lpfnLBAssert(#exp,__FILE__,__LINE__);}
#endif

extern BOOL FVerbose;
extern BOOL FUseOutputDebugString;
extern char rgchDebug[];
extern void DebugPrint(char *, ...);
extern ULONG gTraceFlag;

#define TR_RANGESTEP	0x00000001

#ifdef XBOX
// retain block compatibility with non-Xbox version
#define DPRINT(level, args) if(1) { (DbgPrint)args; }
#else
#define DPRINT(level, args) \
  if (FVerbose >= level) { ((FUseOutputDebugString)? (DebugPrint) : (DMPrintShellMsg)) args; }
#endif

#define DEBUG_PRINT(str) DPRINT(5, (str))
#define DEBUG_PRINT_1(str, a1) DPRINT(5, (str, a1))
#define DEBUG_PRINT_2(str, a1, a2) DPRINT(5, (str, a1, a2))
#define DEBUG_PRINT_3(str, a1, a2, a3) DPRINT(5, (str, a1, a2, a3))
#define DEBUG_PRINT_4(str, a1, a2, a3, a4) DPRINT(5, (str, a1, a2, a3, a4))
#define DEBUG_PRINT_5(str, a1, a2, a3, a4, a5) DPRINT(5, (str, a1, a2, a3, a4, a5))
#define DEBUG_LEVEL_PRINT(level, str) DPRINT(level, (str))
#define VERIFY(X) if (!(X)) assert(FALSE)

void
WINAPI
_DbgTrace(
	ULONG	Flag,
	LPCTSTR	szFormat,
	...
	);

#define DbgTrace(_x) _DbgTrace _x

#else

#ifndef XBOX
#define assert(exp)
#endif

#define DPRINT(level, args)

#define DEBUG_PRINT(str)
#define DEBUG_PRINT_1(str, a1)
#define DEBUG_PRINT_2(str, a1, a2)
#define DEBUG_PRINT_3(str, a1, a2, a3)
#define DEBUG_PRINT_4(str, a1, a2, a3, a4)
#define DEBUG_PRINT_5(str, a1, a2, a3, a4, a5)

#define DEBUG_LEVEL_PRINT(level, str)
#define VERIFY(X) ((void)(X))
#define DbgTrace(_x)

#endif

extern  DMTLFUNCTYPE        DmTlFunc;

/*
**   Win95/Chicago and other OS feature related functions
*/

BOOL IsChicago(VOID);
extern BOOL FXSAVESupported;

/*
**   WOW functions
*/

//#define WOW_IMPLEMENTED

#ifdef WOW_IMPLEMENTED
BOOL TranslateAddress(HPRCX, HTHDX, LPADDR, BOOL);
BOOL IsWOWPresent(VOID);
BOOL    WOWGetThreadContext(HTHDX hthdx, LPCONTEXT lpcxt);
BOOL    WOWSetThreadContext(HTHDX hthdx, LPCONTEXT lpcxt);
#else
#define TranslateAddress(a,b,c,d)    FALSE
#define IsWOWPresent()               FALSE
#define WOWGetThreadContext(a,b)     FALSE
#define WOWSetThreadContext(a,b)     FALSE
#endif

extern  void    ProcessSegmentLoadEvent(DEBUG_EVENT *, HTHDX);
extern  void    ProcessEntryPointEvent(DEBUG_EVENT *pde, HTHDX hthdx);

/*
**  Prototypes from util.c
*/

ULONG   SetReadPointer(ULONG cbOffset, int iFrom);
VOID    SetPointerToFile(HANDLE hFile);
VOID    SetPointerToMemory(HPRCX hprcx, LPVOID lpv);
BOOL    DoRead(LPVOID lpv, DWORD cb);
BOOL    AreAddrsEqual(HPRCX, HTHDX, LPADDR, LPADDR);

HTHDX HTHDXFromPIDTID(PID, TID);
HTHDX HTHDXFromHPIDHTID(HPID, HTID);
HPRCX HPRCFromPID(PID);
HPRCX HPRCFromHPID(HPID);
HPRCX HPRCFromHPRC(HANDLE);



BOOL
CheckBpt(
    HTHDX       hthd,
    PBREAKPOINT pbp
    );

LPTSTR
MHStrdup(
    LPCTSTR s
    );

XOSD
DMSendRequestReply (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD cbInput,
    LPVOID lpInput,
    DWORD cbOutput,
    LPVOID lpOutput
    );

XOSD
DMSendDebugPacket(
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD cbInput,
    LPVOID lpInput
    );

UOFFSET
FileOffFromVA(
    PDLLLOAD_ITEM           pdi,
    HFILE                   hfile,
    UOFFSET                 uoffBasePE,
    const IMAGE_NT_HEADERS *pnthdr,
    UOFFSET                 va
    );

DWORD
CbReadDllHdr(
    HFILE hfile,
    UOFFSET uoff,
    LPVOID lpvBuf,
    DWORD cb
    );

ULONGLONG
GetRegValue(
    PCONTEXT regs,
    int cvindex
    );

VOID
DmSetFocus (
    HPRCX phprc
    );

BOOL
FGetExport(
    PDLLLOAD_ITEM pdi,
    HFILE       hfile,
    LPCTSTR     szExport,
    LPVOID*     plpvValue
    );

VOID
GetTaskList(
    PTASK_LIST pTask,
    DWORD dwNumTasks
    );

BOOL
AddrReadMemory(
    HPRCX       hprc,
    HTHDX       hthd,
    LPADDR      paddr,
    LPVOID      lpb,
    DWORD       cb,
    LPDWORD     pcbRead
    );

BOOL
AddrWriteMemory(
    HPRCX       hprc,
    HTHDX       hthd,
    LPADDR      paddr,
    LPVOID      lpv,
    DWORD       cb,
    LPDWORD     pcbWritten
    );


//
// userapi.c / kdapi.c
//

BOOL
DbgReadMemory(
    HPRCX       hprc,
    LPCVOID     lpOffset,
    LPVOID      lpv,
    DWORD       cb,
    LPDWORD     pcbRead
    );

BOOL
DbgWriteMemory(
    HPRCX       hprc,
    LPVOID      lpOffset,
    LPVOID      lpb,
    DWORD       cb,
    LPDWORD     pcbWritten
    );

BOOL
DbgGetThreadContext(
    HTHDX hthd,
    LPCONTEXT lpContext
    );

BOOL
DbgSetThreadContext(
    IN HTHDX hthd,
    IN LPCONTEXT lpContext
    );      \

EXHDLR *
GetExceptionCatchLocations(
    IN HTHDX,
    IN LPVOID
    );

VOID
GetMachineType(
    LPPROCESSOR p
    );

void
AddQueue(
    DWORD dwType,
    DWORD dwProcessId,
    DWORD dwThreadId,
    DWORD dwData,
    DWORD dwLen
    );

BOOL
DequeueAllEvents(
    BOOL fForce,
    BOOL fConsume
    );

VOID
InitEventQueue(
    VOID
    );

typedef struct _DEQ {
	BOOL fNeedsContinue;
	DEBUG_EVENT de;
	union {
		DMN_MODLOAD dmml;
		DMN_BREAK dmbr;
	};
	struct _DEQ *pdeqNext;
} DEQ;

VOID InitDEQ(void);
BOOL WaitDEQ(DEQ *, DWORD);
void PostDEQ(DEBUG_EVENT *, BOOL);

BOOL
IsPassingException(
    HPRCX   hprc
    );

void
ThreadContinueDebugEventEx(
    HTHDX   hthd,
    ULONG   Continue
    );

void
ThreadContinueDebugEvent(
    HTHDX   hthd
    );


#define QT_CONTINUE_DEBUG_EVENT     1
#define QT_RELOAD_MODULES           2
#define QT_TRACE_DEBUG_EVENT        3
#define QT_REBOOT                   4
#define QT_RESYNC                   5
#define QT_DEBUGSTRING              6
#define QT_CRASH                    7


//
// any ssvc not recognized by ProcessSystemServiceCmd is
// punted to this, which is provided separately by the user
// and kernel versions.
//
VOID
LocalProcessSystemServiceCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    );


//
//
//

#ifndef KERNEL

//
// OLE debugging support
//

typedef enum _OLESEG OLESEG;
typedef enum _ORPC ORPC;

typedef VOID (*COMPLETION_FUNCTION) (HTHDX, LPVOID);

OLESEG  GetOleSegType(LPVOID);
OLESEG  OleSegFromAddr(HPRCX, UOFFSET);
#define EnsureOleRpcStatus(hthd, pfn, lpv) ((pfn)(hthd, lpv))
BOOL    FClientNotifyStep(HTHDX, DEBUG_EVENT*);
BOOL    FServerNotifyStop(HTHDX, DEBUG_EVENT*);
ORPC    OrpcFromPthd(HTHDX, DEBUG_EVENT*);
VOID    ProcessOleEvent(DEBUG_EVENT*, HTHDX);

BOOL
CheckAndSetupForOrpcSection(
    HTHDX   hthd
    );

UOFFSET
GetReturnDestination(
    HTHDX   hthd
    );

VOID
SetupOrpcRangeStep(
    HTHDX   hthd
    );

VOID ActionOrpcClientGetBufferSize(DEBUG_EVENT *, HTHDX, DWORD, LPVOID);
VOID ActionOrpcClientFillBuffer(DEBUG_EVENT *, HTHDX, DWORD, LPVOID);
VOID ActionOrpcClientNotify(DEBUG_EVENT *, HTHDX, DWORD, LPVOID);
VOID ActionOrpcServerNotify(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);
VOID ActionOrpcServerGetBufferSize(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);
VOID ActionOrpcSkipToSource(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);

BOOL
GetSinglePVOIDArgument(
	HTHDX	hthd,
	PVOID*	Argument
	);

BOOL
GetDoublePVOIDArgument(
	HTHDX	hthd,
	DWORD*	Argument1,
	DWORD*	Argument2
	);

//
//  Fiber Support
//
VOID ProcessFiberEvent(DEBUG_EVENT*,HTHDX);
VOID RemoveFiberList(HPRCX);

VOID ProcessSpecialEvent(DEBUG_EVENT*,HTHDX);
//
//  Non Local Goto support
//

typedef HDEP HNLG;  // Handle to NLG
typedef struct _NLG_DESTINATION {
    DWORD   dwSig;
    UOFFSET uoffDestination;
    DWORD   dwCode;
    UOFFSET uoffFramePointer;
} NLG_DESTINATION;
typedef NLG_DESTINATION FAR * LPNLG_DESTINATION;

#define NLG_LONGJMP						0x00000000
#define NLG_EXCEPT_ENTER				0x00000001
#define NLG_CATCH_LEAVE					0x00000002
#define NLG_LONGJMPEX					0x00000003
#define NLG_CATCH_ENTER					0x00000100
#define NLG_FINALLY_ENTER				0x00000101
#define NLG_FILTER_ENTER				0x00000102
#define NLG_DESTRUCTOR_ENTER			0x00000103
#define NLG_GLOBAL_CONSTRUCTOR_ENTER	0x00000104
#define NLG_GLOBAL_DESTRUCTOR_ENTER		0x00000105
#define NLG_DLL_ENTRY                   0x00000106
#define NLG_DLL_EXIT                    0x00000107
#define NLG_COM_PLUS_THUNK				0x00000108

#define NLG_SIG							0x19930520

typedef enum _NLG_LOCATION {
    NLG_DISPATCH,
    NLG_RETURN
} NLG_LOCATION, FAR * LPNLG_LOCATION;

#define hnlgNull    ((HNLG)NULL)

INT FAR PASCAL NLGComp (LPNLG, LPVOID, LONG);

VOID
ActionNLGDispatch(
    DEBUG_EVENT* de,
    HTHDX hthd,
    DWORD unused,
    LPVOID lpv
    );

VOID
ActionNLGDestination   (
    DEBUG_EVENT* de,
    HTHDX hthd,
    DWORD unused,
    LPVOID lpv
    );

HNLG CheckNLG ( HPRCX, HTHDX, NLG_LOCATION, LPADDR );
BOOL SetupNLG ( HTHDX, LPADDR );
DWORD GetSPFromNLGDest(HTHDX, LPNLG_DESTINATION);

void ProcessNonLocalGoto( HPRCX, HTHDX, LPDBB );

typedef enum _NFI {
    nfiHEMI,
} NFI; // NonLocalGoto Find Information
typedef NFI FAR * LPNFI;


#ifdef OMNI

//
//  Omni JIT support
//

VOID
ProcessOmniDllLoadEvent(
	DEBUG_EVENT*	event,
	HTHDX			hthd
	);

#endif

BOOL
CreateTimerEventQueue(
	TIMER_EVENT_QUEUE*	queue
	);

BOOL
DeleteTimerEventQueue(
	TIMER_EVENT_QUEUE*	queue
	);

HANDLE
CreateTimerEvent(
	HPRCX					hprc,
	TIMER_EVENT_CALLBACK	callback,
	LPVOID					param,
	DWORD					ticks,
	BOOL					fQueueEvent
	);

BOOL
EnqueueTimerEvent(
	HPRCX	hprc,
	HANDLE	hEvent
	);

BOOL
RemoveTimerEvent(
	HPRCX	hprc,
	HANDLE	hTimerEvent
	);

BOOL
FireOutstandingTimerEvents(
	HPRCX	hprc
	);

#endif // !KERNEL



enum {
	FLAG_ALREADY_SET,
	FLAG_SET,
	FLAG_ALREADY_CLEARED,
	FLAG_CLEARED
};


DWORD
InterlockedSetFlag(
	LONG*	lpFlag
	);


DWORD
InterlockedClearFlag(
	LONG*	lpFlag
	);

#ifdef REGSYNC
void EnsureHthdReg(HTHDX, DWORD);
#define HthdReg(hthd, reg) (*(EnsureHthdReg(hthd, _ctx_##reg), &hthd->ctx.reg))
#define _ctx_Eax CONTEXT_INTEGER
#define _ctx_Ebx CONTEXT_INTEGER
#define _ctx_Ecx CONTEXT_INTEGER
#define _ctx_Edx CONTEXT_INTEGER
#define _ctx_Esi CONTEXT_INTEGER
#define _ctx_Edi CONTEXT_INTEGER
#define _ctx_Esp CONTEXT_CONTROL
#define _ctx_Ebp CONTEXT_CONTROL
#define _ctx_Eip CONTEXT_CONTROL
#define _ctx_SegCs CONTEXT_CONTROL
#define _ctx_SegSs CONTEXT_CONTROL
#define _ctx_EFlags CONTEXT_CONTROL
#else
#define HthdReg(hthd, reg) ((hthd)->ctx.reg)
#define EnsureHthdReg(hthd, dw)
#endif

char XboxAssert(PDMN_DEBUGSTR);
void XboxRip(LPCSTR);

BOOL
UnpackLoadCmd(
    CONST LPVOID lpv,
    LPPRL lpprl
    );

#ifdef KERNEL

/*
**  Kernel Debugger Specific Functions
*/

BOOL  MyFindExecutable( PCHAR PathName, PCHAR OutputBuffer );
BOOLEAN DmKdConnectAndInitialize( LPSTR lpProgName );
VOID  QueueActionEvent( HTHDX hthd, DWORD dwAction, LPVOID lpv, DWORD dwLength );
VOID  ReloadModulesWorker( ULONG ListHead, LPSTR lpModules, DWORD dwLength );

BOOL
WriteBreakPointEx(
    IN HTHDX hthd,
    IN ULONG BreakPointCount,
    IN OUT PDBGKD_WRITE_BREAKPOINT BreakPoints,
    IN ULONG ContinueStatus
    );

BOOL
RestoreBreakPointEx(
    IN ULONG BreakPointCount,
    IN PDBGKD_RESTORE_BREAKPOINT BreakPointHandles
    );


VOID  ContinueTargetSystem(DWORD ContinueStatus, PDBGKD_CONTROL_SET ControlSet);
VOID  RestoreKernelBreakpoints(HTHDX hthd, UOFF32 Offset);
BOOL  ReadControlSpace(USHORT Processor, PVOID TargetBaseAddress, PVOID UserInterfaceBuffer, ULONG TransferCount, PULONG ActualBytesRead);

#if defined(HAS_DEBUG_REGS)
BOOL  GetExtendedContext(HTHDX hthd, PKSPECIAL_REGISTERS pksr);
BOOL  SetExtendedContext(HTHDX hthd, PKSPECIAL_REGISTERS pksr);
#endif

#define KERNEL_MODULE_NAME     "nt"
#define KERNEL_IMAGE_NAME      "xboxkrnl.exe"
#define KERNEL_IMAGE_NAME_MP   "ntkrnlmp.exe"
#define OSLOADER_IMAGE_NAME    "osloader.exe"
#define HAL_IMAGE_NAME         "hal.dll"
#define HAL_MODULE_NAME        "HAL"


extern BOOL ApiIsAllowed;


typedef struct MODULEALIAS {
    CHAR    ModuleName[16];
    CHAR    Alias[16];
    BOOL    Special;
} MODULEALIAS, *LPMODULEALIAS;

#define MAX_MODULEALIAS 100

LPMODULEALIAS
FindAliasByImageName(
    LPSTR lpImageName
    );

LPMODULEALIAS
FindAddAliasByModule(
    LPSTR lpImageName,
    LPSTR lpModuleName
    );

typedef struct IMAGEINFO {
    DWORD                 CheckSum;
    DWORD                 TimeStamp;
    DWORD                 SizeOfImage;
    DWORD                 BaseOfImage;
    DWORD                 NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} IMAGEINFO, *LPIMAGEINFO;

void ParseDmParams( LPSTR p );
BOOL ReadImageInfo(LPSTR,LPSTR,LPSTR,LPIMAGEINFO);



#endif  // KERNEL

#ifdef __cplusplus
}
#endif
