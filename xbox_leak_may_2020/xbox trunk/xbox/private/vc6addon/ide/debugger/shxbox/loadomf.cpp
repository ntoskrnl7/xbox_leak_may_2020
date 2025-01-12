// loadomf.cxx - load
//
//  Copyright <C> 1989-94, Microsoft Corporation
//
//  Purpose:
//
//  10-Nov-94   BryanT
//      Merge in NT changes.
//      Change the load code so we first call the Shell to see
//      if the symbol load s/b deferred or ignored.
//      Functions changed: OLStart, OLLoadOmf
//      New Functions: OLContinue (the part of OLStart that determines)
//                          what type of file we're looking at).
//                     LoadOmfForReal (the part of OLLoadOmf that actually
//                          performs the symbol load)
//      Replace all the hexg param's with lpexg's.  We have it everywhere
//      it's needed and every function calls LLLock/LLUnlock to get it...
//      Define UnloadOmf.
//
//  07-Jan-96   BryanT
//

#include "shinc.hpp"
#pragma hdrstop

#include <imagehlp.h>

// The exe file information

static LSZ          lszFName;           // file name
static LONG         lfaBase;            // offset of directory info from end of file
static ULONG        cDir;               // number of directory entries
static OMFDirEntry *lpdss;              // pointer to directory table
static OMFDirEntry *lpdssCur;           // pointer to current directory entry
static LONG         lcbFilePos;
static WORD         csegExe;
static PIMAGE_SECTION_HEADER  SecHdr;
static unsigned int SecCount;
static WORD         btAlign;            // Alignment bit
static DWORD        fLoadExports;		// load Exports as debug info

typedef struct _PDB_INFO {
    SIG sig;
    AGE age;
    char sz[_MAX_PATH];
} PDB_INFO;

static  PDB_INFO pdbInfo;

const ULONG sigNB11 = '11BN';
const ULONG sigNB10 = '01BN';
const ULONG sigNB09 = '90BN';
const ULONG sigNB08 = '80BN';
const ULONG sigNB07 = '70BN';
const ULONG sigNB06 = '60BN';
const ULONG sigNB05 = '50BN';

// compile time assert
#if !defined(cassert)
#define cassert(x) extern char dummyAssert[ (x) ]
#endif


SHE  CheckSignature (INT , OMFSignature *, PDB_INFO *);
SHE  OLStart (LPEXG);
BOOL OLMkSegDir (WORD, LPSGD *, LPSGE *, LPEXG);
SHE  OLLoadTypes (LPEXG);
SHE  OLLoadSym (LPEXG);
SHE  OLLoadSrc (LPEXG);
SHE  OLGlobalPubs (LPEXG);
SHE  OLGlobalSym (LPEXG);
SHE  OLStaticSym (LPEXG);
SHE  OLLoadSegMap (LPEXG);
SHE  OLLoadNameIndex (LPEXG);
LPCH OLRwrSrcMod (OMFSourceModule *);
BOOL OLLoadHashSubSec (LPGST, LPB, WidenTi * =0);
SHE  NB10LoadOmf (LPEXG, HEXG);
SHE  LoadPdb (LPEXG, PDB_INFO *);
SHE  NB10LoadModules (LPEXG, ULONG*, HEXG);
VOID LoadSymbols(HPDS, HEXG, BOOL);
SHE  LoadOmfForReal(LPEXG, HEXG);
SHE  LoadFpo(LPEXG, int, PIMAGE_DEBUG_DIRECTORY);
SHE  LoadPdata(LPEXG, int, ULONG, ULONG, ULONG, ULONG, BOOL);
SHE  LoadOmap(LPEXG, int, PIMAGE_DEBUG_DIRECTORY);
int  OLMkModule(LPEXG, HEXG);
SHE  OLValidate(int, void *, LPSTR);
void QueryRegistry();

SHE  SheFixupConvertedSyms(LPEXG);
void ConvertGlobal16bitSyms(WidenTi*, LPGST, PB, ULONG);
SHE  LoadDebugDataFromPdb(LPEXG lpexg, PDB_INFO *ppdb);

#define MAX_SEARCH_PATH_LEN   512
#define SHECONV(s)  (((s)==sheExportsConverted)?2:TRUE)

static TCHAR szSearchPath[MAX_SEARCH_PATH_LEN];
BOOL  fQueriedRegistry = FALSE;
PFNPDBOpenValidateEx pAltPDBOpen = 0;

// CFile is a simple helper class which will force its file to be closed
// as soon as the CFile object is destructed.

class CFile {
    public:
        INT m_hfile;

        CFile() { m_hfile = -1; }
        void ReInit() {
            if (m_hfile != -1) {
                SYClose(m_hfile);
                m_hfile = -1;
            }
        }
        INT Open(LSZ lszName) {
            m_hfile = SYOpen(lszName);
            return(m_hfile);
        }

        ~CFile() {
            if(m_hfile != -1) {
                SYClose (m_hfile);
                m_hfile = -1;
            }
        }

        operator INT&() { return m_hfile; }
};

VOID
LoadDefered(
    HEXG  hexg
    )
{
    LoadSymbols(hpdsCur, hexg, TRUE);
    return;
}

VOID
UnloadDefered(
    HEXG hexg
    )
{
    return;
}

inline BOOL
fSheGoodReturn(SHE she)
{
    return she == sheNone || she == sheSymbolsConverted || she == sheConvertTIs || she == sheExportsConverted;
}

//  OLLoadOmf - load omf information from exe
//
//  error = OLLoadOmf (hexg)
//
//  Entry   hexg = handle to executable information struct
//
//  Exit
//
//  Returns An error code suitable for errno.

SHE
OLLoadOmf(
    HEXG    hexg,
    VLDCHK *pVldChk,
    DWORD   dllLoadAddress
    )
{
    SHE     sheRet = sheNone;
    LSZ     lszFname = NULL;
    LPEXG   lpexg = (LPEXG) LLLock (hexg);

    if (lpexg->fOmfLoaded) {
        return sheNone;
    }

    // Query the shell and see if we should load this one now.

    lszFname = lpexg->lszName;

#if 0

        //      SYGetDefaultShe () always returns TRUE and leavs the parameters
        //      unchanged; so this bit of code is dead.

    if (!SYGetDefaultShe(lszFname, &sheRet)) {
        if (lpexg->lszAltName) {
            lszFname = lpexg->lszAltName;
            if (!SYGetDefaultShe(lszFname, &sheRet)) {
                SYGetDefaultShe(NULL, &sheRet);
                lszFname = lpexg->lszName;
            }
        } else {
            SYGetDefaultShe(NULL, &sheRet);
        }
    }

    // SYGetDefaultShe is expected to return one of the following
    // values:
    //
    // sheSuppressSyms - Don't load, just keep track of the name/start
    // sheNoSymbols - This module has already been processed and there are no symbols
    // sheDeferSyms - Defer symbol loading until needed
    // sheSymbolsConverted - The symbols are already loaded
    // sheNone - Go ahead and load the symbols now.

#endif

    // Regardless of the load type, save some stuff

    lpexg->LoadAddress  = dllLoadAddress;
    lpexg->ulTimeStamp = pVldChk->TimeDateStamp;

    lpexg->debugData.she = sheRet;

    // If we made it this far, we must load the symbols

    LoadSymbols(hpdsCur, hexg, FALSE);

    if (lpexg->fOmfMissing)
        sheRet = sheNoSymbols;
    else if (lpexg->fOmfSkipped)
        sheRet = sheSuppressSyms;
    else if (lpexg->fOmfDefered)
        sheRet = sheDeferSyms;
    else if (lpexg->fSymConverted==2)
        sheRet = sheExportsConverted;
    else if (lpexg->fSymConverted)
        sheRet = sheSymbolsConverted;

    lpexg->debugData.she = sheRet;

    LLUnlock(hexg);

    return(sheRet);
}


//  LoadSymbols
//
//  Purpose: This function loads a defered module's symbols.  After
//      the symbols are loaded the shell is notified of the completed
//      module load.
//
//  Input:  hpds - Handle to process to load the symbols for
//          hexg - exg handle for the module to be added
//          fNotifyShell - Should shell be notified on load.
//
//  Return: None

VOID
LoadSymbols(
    HPDS hpds,
    HEXG hexg,
    BOOL fNotifyShell
    )
{
    SHE     sheRet;
    HEXE    hexe;
    LPEXG   lpexg = NULL;
    LPPDS   lppds = NULL;
    LPEXE   lpexe = NULL;
    LPSTR   lpname = NULL;
    HPDS    hpdsLast;

//    EnterCriticalSection( &CsSymbolLoad );

    //  find the exe for this exg

    hexe = NULL;
    while ((hexe=SHGetNextExe(hexe))) {
        lpexe = (LPEXE) LLLock(hexe);
        if ((hexg == lpexe->hexg) &&
            (hpds == lpexe->hpds))
        {
                break;
        } else {
            LLUnlock(hexe);
        }
    }

    if (!hexe) {
        goto done;              // didn't find a hexg/hpds match
    }

    //  lock down the necessary data structures

    lpexg = (LPEXG) LLLock(hexg);
    if (!lpexg) {
        goto done;
    }

    lppds = (LPPDS) LLLock(lpexe->hpds);
    if (!lppds) {
        goto done;
    }

    //  mark the module as being loaded
    lpexg->fOmfLoading = TRUE;

//    LeaveCriticalSection( &CsSymbolLoad );

    //  load the symbols (yes, pass both lpexg and hexg.
    //          OlMkModule needs hexg for creating the lpmds)

    sheRet = LoadOmfForReal(lpexg, hexg);

//    EnterCriticalSection( &CsSymbolLoad );

    switch (sheRet) {
        case sheNoSymbols:
            lpexg->fOmfMissing = TRUE;
            break;

        case sheSuppressSyms:
            lpexg->fOmfSkipped = TRUE;
            break;

        case sheNone:
        case sheSymbolsConverted:
        case sheExportsConverted:
            lpexg->fOmfLoaded   = TRUE;
            break;

        default:
            lpexg->fOmfMissing = TRUE;
            break;
    }

    if (fNotifyShell) {
        //
        // notify the shell that symbols have been loaded
        //
        if (lpexg->lszAltName) {
            lpname = lpexg->lszAltName;
        } else {
            lpname = lpexg->lszName;
        }
//        hpdsLast = SHChangeProcess(hpds, TRUE);
        hpdsLast = SHChangeProcess(hpds);
        DLoadedSymbols(sheRet, lppds->hpid, lpname);
//        SHChangeProcess(hpdsLast, FALSE);
        SHChangeProcess(hpdsLast);
    }

    // update the module flags

    lpexg->fOmfDefered = FALSE;
    lpexg->fOmfLoading = FALSE;

done:

//    LeaveCriticalSection( &CsSymbolLoad );

    // free resources

    if (lpexe) {
        if (lppds) {
            LLUnlock(lpexe->hpds);
        }
        LLUnlock(hexe);
    }

    if (lpexg) {
        LLUnlock(hexg);
    }

    return;
}

//  LoadOmfForReal
//
//  Purpose: Here's where the symbolic is actually loaded from the image.
//
//  Input:  lpexg - The pointer to the exg structure
//          hexg  - The handle of the exg structure
//
//  Return: Standard she error codes.

SHE
LoadOmfForReal(
    LPEXG  lpexg,
    HEXG   hexg
    )
{
    SHE     sheRet = sheNone;
    SHE     sheRetSymbols = sheNone;
    WORD    cbMod = 0;
    ULONG   cMod;
    ULONG   iDir;

    csegExe = 0;

    __try {

        // Open and verify the exe.

        sheRet = sheRetSymbols = OLStart(lpexg);

        // If there was an error, bail.
        //  (sheNone doesn't mean "no symbols", it means "error None")

        if (!fSheGoodReturn(sheRet)) {
            goto returnhere;
        }

        if (lpexg->ppdb) {
            sheRet = NB10LoadOmf(lpexg, hexg);
            goto returnhere;
        }

        if (sheRet == sheConvertTIs) {
            // set up to do the conversions
            if (!WidenTi::fCreate(lpexg->pwti)) {
                sheRet = sheOutOfMemory;
                goto returnhere;
            }
        }

        btAlign = (WORD)(lfaBase & 1);

        lpdssCur = lpdss;
        iDir = 0;

        // Load up the module table.

        // First, count up how many sstModule entries we have.  The spec
        // requires all the sstModules to be before any other.

        while (iDir < cDir && lpdssCur->SubSection == sstModule) {
            lpdssCur++;
            iDir++;
        }

        // If there's no modules, there's no sense continuing.
        if (iDir == 0) {
            sheRet = sheNoSymbols;
            goto returnhere;
        }

        lpexg->cMod = cMod = iDir;

        // Allocate the rgMod buffer and load each dir entry in.

        lpexg->rgMod = (LPMDS)MHAlloc((cMod+2) * sizeof(MDS));
        if (lpexg->rgMod == NULL) {
            sheRet = sheOutOfMemory;
            goto returnhere;
        }
        memset(lpexg->rgMod, 0, sizeof(MDS)*(cMod+2));
        lpexg->rgMod[cMod+1].imds = (WORD) -1;

        // Go through the list of directory entries and process all of the sstModule records.

        for (iDir = 0, lpdssCur = lpdss;
             iDir < cMod;
             iDir += 1, lpdssCur++) {

            if (!OLMkModule (lpexg, hexg)) {
                sheRet = sheOutOfMemory;
                goto returnhere;
            }
        }

        // Set up map of modules.  This function is used to create a map
        // of contributer segments to modules.  This is then used when
        // determining which module is used for an address.
        lpexg->csgd = csegExe;
        if (!OLMkSegDir (csegExe, &lpexg->lpsgd, &lpexg->lpsge, lpexg)) {
            sheRet = sheOutOfMemory;
            goto returnhere;
        }

        // continue through the directory entries

        for (; iDir < cDir; lpdssCur++, iDir++) {
            if (lpdssCur->cb == 0) {
                // if nothing in this entry
                continue;
            }

            switch (lpdssCur->SubSection) {
                case sstSrcModule:
                    sheRet = OLLoadSrc(lpexg);
                    break;

                case sstAlignSym:
                    sheRet = OLLoadSym(lpexg);
                    break;

                case sstGlobalTypes:
                    sheRet = OLLoadTypes(lpexg);
                    break;

                case sstGlobalPub:
                    sheRet = OLGlobalPubs(lpexg);
                    break;

                case sstGlobalSym:
                    sheRet = OLGlobalSym(lpexg);
                    break;

                case sstSegMap:
                    sheRet = OLLoadSegMap(lpexg);
                    break;

                case sstLibraries:          // ignore this table
                case sstMPC:                // until this table is implemented
                case sstSegName:            // until this table is implemented
                case sstModule:             // Handled elsewhere
                    break;

                case sstFileIndex:
                    sheRet = OLLoadNameIndex(lpexg);
                    break;

                case sstStaticSym:
                    sheRet = OLStaticSym(lpexg);
                    break;

                default:
                    sheRet = sheCorruptOmf;
                    break;
            }

            if (sheRet == sheCorruptOmf) {
                sheRet = sheNoSymbols;
            }
        }
        if (lpexg->pwti) {
            sheRet = SheFixupConvertedSyms(lpexg);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        sheRet = sheNoSymbols;
    }

returnhere:

    if (SecHdr) {
        MHFree(SecHdr);
        SecHdr = NULL;
        SecCount = 0;
    }

    return sheRet;
}

SHE
NB10LoadOmf(
    LPEXG   lpexg,
    HEXG    hexg
    )
{
    SHE     sheRet = sheNone;
    WORD    cbMod = 0;
    ULONG   ModCnt = 0;

    btAlign = (WORD)(lfaBase & 1);

    // we need to allocate a buffer large enough to read the largest module
    // table entry

    if ((sheRet = NB10LoadModules (lpexg, &ModCnt, hexg)) != sheNone) {
        return sheRet;
    }

    if (ModCnt == 0L) {
        // if no symbols found
        return sheNoSymbols;
    }

    lpexg->cMod = ModCnt;

    if(!DBIOpenGlobals(lpexg->pdbi, &(lpexg->pgsiGlobs)) ||
       !DBIOpenPublics(lpexg->pdbi, &(lpexg->pgsiPubs)))
    {
        return sheOutOfMemory;
    }

    if((sheRet = OLLoadSegMap(lpexg)) != sheNone ||
       (sheRet = OLLoadNameIndex(lpexg)) != sheNone)
    {
        return sheRet;
    }

    return sheRet;
}

#define cbFileMax   (_MAX_CVFNAME + _MAX_CVEXT)

class NoCriticalErrors
{
public:
	NoCriticalErrors()
	{
		m_nOldMode = ::SetErrorMode( SEM_FAILCRITICALERRORS );
	}
	~NoCriticalErrors()
	{
		::SetErrorMode( m_nOldMode );
	}
private:
	UINT m_nOldMode;
};

// OLStart - get exe debug information
//
//  Purpose: To open the file specified and get the offset to the directory
//           and get the base that everyone is offset from.
//
//  Entry   hexg = handle to exe to get info for
//
//  Exit    lfaBase = base offset of debug information
//          cDir = count of number of directory entries
//          lpdss = directory entries
//
//  Returns file open status

#define UNKNOWN_IMAGE   0
#define DOS_IMAGE       1
#define VCDBG_IMAGE     2
#define WIN16_IMAGE     3
#define PE_IMAGE        4
#define ROM_IMAGE       5
#define NTDBG_IMAGE     6

SHE
OLStart(
    LPEXG   lpexg
    )
{
    SHE                     sheRet;
    ULONG                   DirSize;
    OMFSignature            Signature;
    OMFDirHeader            DirHeader;
    IMAGE_DOS_HEADER        doshdr;            // Old format MZ header
    IMAGE_NT_HEADERS        pehdr;
    IMAGE_ROM_HEADERS       romhdr;
    IMAGE_SEPARATE_DEBUG_HEADER  sepHdr;
    PIMAGE_FILE_HEADER      pfile;
    IMAGE_DEBUG_DIRECTORY   dbgDir;
    IMAGE_DEBUG_DIRECTORY   cvDbgDir;
    DWORD                   cbData;
    DWORD                   dllLoadAddress;
    DWORD                   ul;
    VLDCHK                  vldChk;
    LSZ                     szFName = NULL;
    char                    szNewName[_MAX_PATH];
    int                     ImageType = UNKNOWN_IMAGE;
    DWORD                   cDebugDir;
    DWORD                   offDbgDir;
    DWORD                   cObjs;
    CFile                   hfile;
    enum
    {
        stateStart,
        stateTryingDBG,
        stateTryingExport,
    } stateLoad;

	// disable critical errors (e.g. for drive E: on msvcrt) VS98:38390
	NoCriticalErrors SeeAbove;

    QueryRegistry();

    stateLoad = stateStart;

    if (lpexg->lszAltName) {
        szFName = lpexg->lszAltName;
    } else {
        szFName = lpexg->lszName;
    }

    // lpexg->lszDebug is the file where we pull the symbolic from.

    dllLoadAddress          = lpexg->LoadAddress;
    vldChk.TimeDateStamp    = lpexg->ulTimeStamp;
	lpexg->ulImageAlign     = 0;

    hfile.Open(szFName);

    // zero some globals, to make it easier to catch when we forget to set them (!)
    lpdss = NULL;
    cDir = 0;
    lfaBase = 0;

    if (hfile == -1) {
retry:
        if (lpexg->lszDebug) {
            MHFree(lpexg->lszDebug);
            lpexg->lszDebug = 0;
        }
        if (stateLoad==stateTryingExport)
        {
            // tried DBG, now try exports
            _tcscpy( szNewName, szFName );
            hfile.Open(szNewName);
        }
        else
        {
            hfile = SYFindExeFile(szFName, szNewName, sizeof(szNewName), &vldChk, (PFNVALIDATEEXE)OLValidate);
            stateLoad = stateTryingDBG;
        }
        if (hfile == -1) {
            if (stateLoad==stateTryingDBG)
            {
                // try exports next if set
                if (fLoadExports)
                {
                    stateLoad = stateTryingExport;
                    goto retry;
                }
            }
            sheRet = sheFileOpen;
            goto ReturnHere;
        }

        if ( ! ( lpexg->lszDebug = (LSZ) MHAlloc ( _ftcslen ( szNewName ) + 1 ) ) ) {
            sheRet = sheOutOfMemory;
            goto ReturnHere;
        }
        _tcscpy ( lpexg->lszDebug, szNewName );

    } else {
        // Assert that the input file is OK.  We only get here
        // when using the file name as passed in from the DM.

        sheRet = OLValidate(hfile, &vldChk, NULL);
        if ((sheRet == sheBadChecksum) ||
            (sheRet == sheBadTimeStamp))
        {
            hfile.ReInit();
            goto retry;
        }
        if ( (sheRet == sheNoSymbols) && (stateLoad!=stateTryingExport) )
        {
            hfile.ReInit();
            goto retry;
        }

        if ( ! ( lpexg->lszDebug = (LSZ) MHAlloc ( _ftcslen ( szFName ) + 1 ) ) ) {
            sheRet = sheOutOfMemory;
            goto ReturnHere;
        }
        _tcscpy ( lpexg->lszDebug, szFName );

    }

    // HACK: If we are pre-loading symbols lpexg->ulTimeStamp will be 0
    // at this point. However vldChk will be updated to have the appropriate
    // information. Update the lpexg structures with the right value.

    if (lpexg->ulTimeStamp == 0) {
        lpexg->ulTimeStamp = vldChk.TimeDateStamp;
    }

    // Now figure out what we're looking at.  Here are the possible formats:
    // 1. Image starts with a DOS MZ header and e_lfanew is zero
    //     - Standard DOS exe.
    // 2. Image starts with a DOS NE header and e_lfanew is non-zero
    //     - If e_lfanew points to a PE header, this is a PE image
    //     - Otherwise, it's probably a Win16 image.
    // 3. Image starts with a PE header.
    //     - Image is a PE image built with -stub:none
    // 4. Image starts with a ROM PE header.
    //     - Image is a ROM image.  If characteristics flag
    //          doesn't have IMAGE_FILE_DEBUG_STRIPPED set, the debug
    //          directory is at the start of rdata.
    // 5. Image starts with a DBG file header
    //     - Image is an NT DBG file (symbols only).
    // 6. None of the signatures match.
    //     - This may be a Languages DBG file.  Seek to the end
    //       of the file and attempt to read the CV signature/offset
    //       from there (a Languages DBG file is made by chopping an
    //       image at the start of the debug data and writing the end
    //       in a new file.  In the CV format, the signature/offset at the
    //       end of the file points back to the beginning of the data).

    if ((SYSeek(hfile, 0, SEEK_SET) == 0) &&
        sizeof(doshdr) == SYReadFar (hfile, (LPB) &doshdr, sizeof(doshdr)))
    {
        switch (doshdr.e_magic) {
            case IMAGE_DOS_SIGNATURE:
                //  This is a DOS NE header.
                if (doshdr.e_lfanew == 0) {
                    ImageType = DOS_IMAGE;
                } else {
                    if ((SYSeek(hfile, doshdr.e_lfanew, SEEK_SET) == doshdr.e_lfanew) &&
                        (SYReadFar(hfile, (LPB) &pehdr, sizeof(pehdr)) == sizeof(pehdr)))
                    {
                        if (pehdr.Signature == IMAGE_NT_SIGNATURE) {
                            ImageType = PE_IMAGE;
                            lpexg->ulImageAlign = pehdr.OptionalHeader.SectionAlignment;
                            pfile = &pehdr.FileHeader;

                            //
                            // Correct for different sized OptionalHeader in PEs.
                            //

                            if (sizeof (pehdr.OptionalHeader) != pehdr.FileHeader.SizeOfOptionalHeader) {

                                if (!SYSeek (hfile, pehdr.FileHeader.SizeOfOptionalHeader - sizeof (pehdr.OptionalHeader), SEEK_CUR)) {

                                    sheRet = sheCorruptOmf;
                                    goto ReturnHere;
                                }
                            }

                        } else {
                            ImageType = WIN16_IMAGE;
                        }
                    } else {
                        // No luck reading from the image.  Must be corrupt.
                        sheRet = sheCorruptOmf;
                        goto ReturnHere;
                    }
                }
                break;

            case IMAGE_SEPARATE_DEBUG_SIGNATURE:
                // This image is an NT DBG file.
                ImageType = NTDBG_IMAGE;
                if ((SYSeek(hfile, 0, SEEK_SET) != 0) ||
                    (SYReadFar(hfile, (LPB) &sepHdr, sizeof(sepHdr)) != sizeof(sepHdr)))
                {
                    // No luck reading from the image.  Must be corrupt.
                    sheRet = sheCorruptOmf;
                    goto ReturnHere;
                }

                // If there's no debug info, we can't continue further.

                if (sepHdr.DebugDirectorySize / sizeof(dbgDir) == 0) {
                    sheRet = sheNoSymbols;
                    goto ReturnHere;
                }
                break;

            default:
                // None of the above.  See if it's a ROM image.
                // Note: The only way we think we're working on a ROM image
                // is if the optional header size is correct.  Not really foolproof.

                if ((SYSeek(hfile, 0, SEEK_SET) == 0) &&
                    (SYReadFar(hfile, (LPB) &romhdr, sizeof(romhdr)) == sizeof(romhdr)))
                {
                    if (romhdr.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                        // If we think we have a ROM image, make sure there's
                        // symbolic to look for.
                        if (romhdr.FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                            sheRet = sheNoSymbols;
                            goto ReturnHere;
                        } else {
                            ImageType = ROM_IMAGE;
                            pfile = &romhdr.FileHeader;
                        }
                    } else {
                        ImageType = VCDBG_IMAGE;
                    }
                } else {
                    // No luck reading from the image.  Must be corrupt.
                    sheRet = sheCorruptOmf;
                    goto ReturnHere;
                }
                break;
        }

    } else {
        // No luck reading from the image.  Must be corrupt.
        sheRet = sheCorruptOmf;
        goto ReturnHere;
    }

    // Now, we know what kind of image we're looking at.
    // Either obtain the pointer to the CV debug data (and other
    // relevant data along the way) or convert whatever we do find
    // to CV debug data.

    lpexg->fSymConverted = FALSE;

    switch (ImageType) {
        case DOS_IMAGE:
        case VCDBG_IMAGE:
        case WIN16_IMAGE:
            // Easy.  Skip to the end and look back.
            ul = SYSeek (hfile, -((LONG)sizeof (OMFSignature)), SEEK_END);
            if ((sheRet = CheckSignature (hfile, &Signature, &pdbInfo)) == sheNone) {
                // seek to the base and read in the new key

                lfaBase = SYSeek (hfile, -Signature.filepos, SEEK_END);
                sheRet = CheckSignature(hfile, &Signature, &pdbInfo);
                cbData = ul - lfaBase;
            }
            // If the CV signature is invalid, see if we can convert what we do
            // have (perhaps a .sym file?)

            if (sheRet != sheNone) {
                if (fLoadExports) {
                    lpexg->lpbData = (LPB) (pfConvertSymbolsForImage)(
                                             (HANDLE)(int)hfile, lpexg->lszDebug, &sheRet);
                }
                // If no symbols converted, bail.  Nothing more we can do.
                if (lpexg->lpbData == 0) {
                    sheRet = sheNoSymbols;
                    goto ReturnHere;
                }
                Signature = *(OMFSignature*)lpexg->lpbData;
                lpexg->fSymConverted = SHECONV(sheRet);
            }
            break;

        case PE_IMAGE:
        case ROM_IMAGE:
            // In both the PE image and ROM image, we're past the FILE
            // and OPTIONAL header by now.  Walk through the section
            // headers and pick up interesting data.  We make a
            // a copy of the section headers in case we need to
            // reconstruct the original values for a Lego'd image

            cObjs = pfile->NumberOfSections;
            SecCount = pfile->NumberOfSections;

            ul = SecCount * sizeof(IMAGE_SECTION_HEADER);

            // Note: SecHdr is free'd by LoadOmfForReal.
            SecHdr = (PIMAGE_SECTION_HEADER) MHAlloc(ul);

            if (!SecHdr) {
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }

            if (SYReadFar(hfile, (LPB) SecHdr, ul) != ul) {
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }


            if (ImageType == PE_IMAGE) {
                // look for the .pdata section on RISC platforms
                if ((pfile->Machine == IMAGE_FILE_MACHINE_ALPHA) ||
                    (pfile->Machine == IMAGE_FILE_MACHINE_R4000) ||
                    (pfile->Machine == IMAGE_FILE_MACHINE_R10000) ||
                    (pfile->Machine == IMAGE_FILE_MACHINE_POWERPC))
                {

                    for (ul=0; ul < cObjs; ul++) {
                        if (_tcscmp((char *) SecHdr[ul].Name, ".pdata") == 0) {
                            LoadPdata(lpexg,
                                      hfile,
                                      dllLoadAddress,
                                      pehdr.OptionalHeader.ImageBase,
                                      SecHdr[ul].PointerToRawData,
                                      SecHdr[ul].SizeOfRawData,
                                      FALSE);
                            break;
                        }
                    }
                }

                // If the debug info has been stripped, close this handle
                // and look for the .dbg file...

                if (pfile->Characteristics & IMAGE_FILE_DEBUG_STRIPPED){
                    // The debug info has been stripped from this image.
                    // Close this file handle and look for the .DBG file.
                    if (stateLoad!=stateTryingExport)
                    {
                        hfile.ReInit();
                        ImageType = UNKNOWN_IMAGE;
                        MHFree(SecHdr);
                        SecHdr = 0;
                        goto retry;
                    }
                    else
                    {
                        // let vcsymcvt find any exports for us
                        if (fLoadExports) {
                            lpexg->lpbData = (LPB)(pfConvertSymbolsForImage)( (HANDLE)(int)hfile, lpexg->lszDebug, &sheRet);
                        }
                        if (lpexg->lpbData == 0) {
                            sheRet = sheNoSymbols;
                            goto ReturnHere;
                        }
                        Signature = *(OMFSignature*)lpexg->lpbData;
                        lpexg->fSymConverted = SHECONV(sheRet);
                        break;
                    }
                }

                // Find the debug directory and the number of entries in it.

                // For PE images, walk the section headers looking for the
                // one that's got the debug directory.
                for (ul=0; ul < cObjs; ul++) {
                    if ((SecHdr[ul].VirtualAddress <=
                         pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress) &&
                        (pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress <
                         SecHdr[ul].VirtualAddress + SecHdr[ul].SizeOfRawData)) {

                        // This calculation really isn't necessary nor is the range test
                        // above.  Like ROM images, it s/b at the beginning of .rdata.  The
                        // only time it won't be is when a pre NT 1.0 image is split sym'd
                        // creating a new MISC debug entry and relocating the directory
                        // to the DEBUG section...

                        offDbgDir = SecHdr[ul].PointerToRawData +
                            pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress -
                            SecHdr[ul].VirtualAddress;
                        cDebugDir = pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                                 sizeof(IMAGE_DEBUG_DIRECTORY);
                        break;
                    }
                }
            } else {
                // For ROM images, there's much less work to do.  We only
                // need to search for the .rdata section.  There's no need
                // to look for .pdata (it will never exist) or worry about
                // stripped debug symbolic (that case was already handled above).
                for (ul=0; ul < cObjs; ul++) {
                    if (!_tcscmp((char *)SecHdr[ul].Name, ".rdata")) {
                        offDbgDir = SecHdr[ul].PointerToRawData;
                        if (SYSeek(hfile, offDbgDir, SEEK_SET) != (LONG) offDbgDir) {
                            sheRet = sheCorruptOmf;
                            goto ReturnHere;
                        }

                        // The linker stores an empty directory entry for ROM
                        // images to terminate the list.

                        cDebugDir = 0;
                        do {
                            if (SYReadFar(hfile, (LPB) &dbgDir, sizeof(dbgDir)) != sizeof(dbgDir)) {
                                sheRet = sheNoSymbols;
                                goto ReturnHere;
                            }
                            cDebugDir++;
                        } while (dbgDir.Type != 0);

                        break;
                    }
                }
            }

            // Assuming we haven't exhausted the list of section headers,
            // we should have the debug directory now.
            if (ul == cObjs) {
                // We didn't find any CV info.  Try converting what we did
                // find.
                if (fLoadExports) {
                    lpexg->lpbData = (LPB)(pfConvertSymbolsForImage)( (HANDLE)(int)hfile, lpexg->lszDebug, &sheRet);
                }
                if (lpexg->lpbData == 0) {
                    sheRet = sheNoSymbols;
                    goto ReturnHere;
                }
                Signature = *(OMFSignature*)lpexg->lpbData;
                lpexg->fSymConverted = SHECONV(sheRet);
                break;
            }

            // Now search the debug directory for relevant entries.

            if (SYSeek(hfile, offDbgDir, SEEK_SET) != (LONG) offDbgDir) {
                sheRet = sheCorruptOmf;
                goto ReturnHere;
            }

            ZeroMemory(&cvDbgDir, sizeof(cvDbgDir) );

            for (ul=0; ul < cDebugDir; ul++) {
                if (SYReadFar(hfile, (LPB) &dbgDir, sizeof(dbgDir)) != sizeof(dbgDir)) {
                    sheRet = sheCorruptOmf;
                    goto ReturnHere;
                }

                if (dbgDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
                    cvDbgDir = dbgDir;
                    continue;
                }

                if (dbgDir.Type == IMAGE_DEBUG_TYPE_FPO) {
                    LoadFpo(lpexg, hfile, &dbgDir);
                }

                if (dbgDir.Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC ||
                    dbgDir.Type == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) {
                    LoadOmap(lpexg, hfile, &dbgDir);
                }
            }

            if (cvDbgDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
                // We didn't find any CV info.  Try converting what we did
                // find.
                if (fLoadExports) {
                    lpexg->lpbData = (LPB)(pfConvertSymbolsForImage)( (HANDLE)(int)hfile, lpexg->lszDebug, &sheRet);
                }
                if (lpexg->lpbData == 0) {
                    sheRet = sheNoSymbols;
                    goto ReturnHere;
                }
                Signature = *(OMFSignature*)lpexg->lpbData;
                lpexg->fSymConverted = SHECONV(sheRet);
            } else {
                // Otherwise, calculate the location/size so we can load it.
                lfaBase = cvDbgDir.PointerToRawData;
                cbData =  cvDbgDir.SizeOfData;
                if (SYSeek(hfile, lfaBase, SEEK_SET) != lfaBase) {
                    sheRet = sheCorruptOmf;
                    goto ReturnHere;
                }
                sheRet = CheckSignature (hfile, &Signature, &pdbInfo);
                if (sheRet != sheNone && sheRet != sheConvertTIs)
                {
                    goto ReturnHere;
                }
            }
            break;

        case NTDBG_IMAGE:
            SecCount = sepHdr.NumberOfSections;

            if (sepHdr.SectionAlignment) {
                lpexg->ulImageAlign = sepHdr.SectionAlignment;
            }

            ul = SecCount * sizeof(IMAGE_SECTION_HEADER);

            // Note: SecHdr is free'd by LoadOmfForReal.

            SecHdr = (PIMAGE_SECTION_HEADER) MHAlloc(ul);
            if (!SecHdr) {
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }

            // Read in the section headers.

            if (SYReadFar(hfile, (LPB) SecHdr, ul) != ul) {
                sheRet = sheCorruptOmf;
                goto ReturnHere;
            }

            // Skip over the exported names.

            SYSeek(hfile, sepHdr.ExportedNamesSize, SEEK_CUR);

            // Look for the interesting debug data.

            ZeroMemory(&cvDbgDir, sizeof(cvDbgDir));

            for (ul=0; ul < (sepHdr.DebugDirectorySize/sizeof(dbgDir)); ul++) {
                if (SYReadFar(hfile, (LPB) &dbgDir, sizeof(dbgDir)) != sizeof(dbgDir)) {
                    sheRet = sheCorruptOmf;
                    goto ReturnHere;
                }

                switch (dbgDir.Type) {
                    case IMAGE_DEBUG_TYPE_CODEVIEW :
                    cvDbgDir = dbgDir;
                        break;

                    case IMAGE_DEBUG_TYPE_FPO :
                    LoadFpo(lpexg, hfile, &dbgDir);
                        break;

                    case IMAGE_DEBUG_TYPE_EXCEPTION :
                        // UNDONE: We can eliminate this load for images
                        // that we've already processed the pdata from the
                        // real image...

                    LoadPdata(lpexg,
                              hfile,
                              dllLoadAddress,
                              sepHdr.ImageBase,
                              dbgDir.PointerToRawData,
                              dbgDir.SizeOfData,
                              TRUE);
                        break;

                    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC :
                    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC :
                    LoadOmap(lpexg, hfile, &dbgDir);
                        break;
                }
            }

            if (cvDbgDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
                if (fLoadExports) {
                    lpexg->lpbData = (LPB)(pfConvertSymbolsForImage)( (HANDLE)(int)hfile, szFName /*lpexg->lszDebug*/, &sheRet);
                }
                if (lpexg->lpbData == 0) {
                    sheRet = sheNoSymbols;
                    goto ReturnHere;
                }
                Signature = *(OMFSignature*)lpexg->lpbData;
                lpexg->fSymConverted = SHECONV(sheRet);
            } else {
                lfaBase = cvDbgDir.PointerToRawData;
                cbData =  cvDbgDir.SizeOfData;
                if (SYSeek(hfile, lfaBase, SEEK_SET) != lfaBase) {
                    sheRet = sheCorruptOmf;
                    goto ReturnHere;
                }
                sheRet = CheckSignature (hfile, &Signature, &pdbInfo);

                if (sheRet != sheNone && sheRet != sheConvertTIs)
                {
                    goto ReturnHere;
                }
            }
            break;

        default:
            // No way we should get here, but assert if we do.
            assert(FALSE);
    }

    // O.K.  Everything's loaded.  If we're looking at a pdb file,
    // load it and get out.

    if ((*(LONG UNALIGNED *)(Signature.Signature)) == sigNB10) {
        sheRet = LoadPdb(lpexg, &pdbInfo);
        if (sheRet!=sheNone)
        {
            // PDB failed (not found, didn't match) so fallback
            // to exports
            if (fLoadExports)
            {
                lpexg->lpbData = (LPB)(pfConvertSymbolsForImage)( (HANDLE)(int)hfile, szFName, &sheRet);
                if (lpexg->lpbData == 0) {
                    sheRet = sheNoSymbols;
                    goto ReturnHere;
                }
                Signature = *(OMFSignature*)lpexg->lpbData;
                lpexg->fSymConverted = SHECONV(sheRet);
                goto ReadCVDir;
            }
        }
    } else {
        // No PDB.
        // If the symbols weren't synthesized, allocate a buffer and
        //  copy them in...

        if (!lpexg->fSymConverted) {
            HANDLE hMap;
            HANDLE hFileMap;

            hFileMap = CreateFile(lpexg->lszDebug,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

            if (hFileMap != INVALID_HANDLE_VALUE) {
                hMap = CreateFileMapping(hFileMap,
                                        NULL,
                                        PAGE_WRITECOPY,
                                        0,
                                        0,
                                        NULL);
                if (hMap != NULL) {
                    // Map in the symbolic (only).
                    SYSTEM_INFO si;
                    DWORD dwAllocStart, dwAllocDiff;

                    GetSystemInfo(&si);

                    dwAllocStart = lfaBase & (~(si.dwAllocationGranularity - 1));
                    dwAllocDiff = lfaBase - dwAllocStart;

                    lpexg->pvSymMappedBase = MapViewOfFile(hMap,
                                                           FILE_MAP_COPY,
                                                           0,
                                                           dwAllocStart,
                                                           cbData + dwAllocDiff);
                    if (lpexg->pvSymMappedBase) {
                        lpexg->lpbData = ((BYTE *) lpexg->pvSymMappedBase) + dwAllocDiff;
                    }

                    CloseHandle(hMap);
                }

                CloseHandle(hFileMap);
            }

            if (lpexg->lpbData == NULL) {

                // Unable to map the image.  Read the whole blob in from disk.

                lpexg->lpbData = (LPB)MHAlloc(cbData);
                if (!lpexg->lpbData) {
                    sheRet = sheNoSymbols;
                    goto ReturnHere;
                }

                if ((SYSeek (hfile, lfaBase, SEEK_SET) != lfaBase) ||
                    (SYReadFar (hfile, lpexg->lpbData, cbData) != cbData))
                {
                    // Failed to read in the data... Must be corrupt.
                    MHFree(lpexg->lpbData);
                    lpexg->lpbData = 0;
                    sheRet = sheCorruptOmf;
                    goto ReturnHere;
                }
            }
        }

ReadCVDir:
        // We now have a pointer to the CV debug data.  Setup the
        //  pointers to the CV Directory header and return.

        LPB     lpb;
        lpexg->ppdb = NULL;
        lpexg->ptpi = NULL;
        lpexg->pdbi = NULL;

        lpb = Signature.filepos + lpexg->lpbData;

        DirHeader = *(OMFDirHeader *) lpb;
        cDir = DirHeader.cDir;

        // check to make sure somebody has not messed with omf structure
        if (DirHeader.cbDirEntry != sizeof (OMFDirEntry)) {
            sheRet = sheCorruptOmf;
            goto ReturnHere;
        }

        lpdss = (OMFDirEntry *)(lpb + sizeof(DirHeader));

        if (lpexg->fSymConverted) {
            sheRet = sheSymbolsConverted;
            goto ReturnHere;
        }

        if (sheRet == sheConvertTIs) {
            goto ReturnHere;
        }

        sheRet = sheNone;
    }

ReturnHere:

    lpexg->debugData.she = sheRet;

    return sheRet;
}


SHE
LoadFpo(
    LPEXG                   lpexg,
    int                     hfile,
    PIMAGE_DEBUG_DIRECTORY  dbgDir
    )
{
    LONG fpos;

    fpos = SYTell(hfile);

    lpexg->fIsRisc = FALSE;

    if (SYSeek(hfile, dbgDir->PointerToRawData, SEEK_SET) != (LONG) dbgDir->PointerToRawData) {
        return(sheCorruptOmf);
    }

    if(!(lpexg->debugData.lpFpo = (PFPO_DATA) MHAlloc(dbgDir->SizeOfData)))
        return sheOutOfMemory;

    SYReadFar(hfile, (LPB) lpexg->debugData.lpFpo, dbgDir->SizeOfData);
    lpexg->debugData.cRtf = dbgDir->SizeOfData / SIZEOF_RFPO_DATA;

    SYSeek(hfile, fpos, SEEK_SET);

    return sheNone;
}

SHE
LoadOmap(
    LPEXG                   lpexg,
    int                     hfile,
    PIMAGE_DEBUG_DIRECTORY  dbgDir
    )
{
    LONG    fpos;
    LPVOID  lpOmap;
    DWORD   dwCount;

    fpos = SYTell(hfile);

    if (SYSeek(hfile, dbgDir->PointerToRawData, SEEK_SET) != (LONG) dbgDir->PointerToRawData) {
        return(sheCorruptOmf);
    }
    if(!(lpOmap = (LPVOID) MHAlloc(dbgDir->SizeOfData)))
        return sheOutOfMemory;
    SYReadFar(hfile, (LPB) lpOmap, dbgDir->SizeOfData);

    dwCount = dbgDir->SizeOfData / sizeof(OMAP);

    SYSeek(hfile, fpos, SEEK_SET);

    if(dbgDir->Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC) {
        lpexg->debugData.lpOmapFrom = (LPOMAP) lpOmap;
        lpexg->debugData.cOmapFrom = dwCount;
    } else
    if(dbgDir->Type == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) {
        lpexg->debugData.lpOmapTo = (LPOMAP) lpOmap;
        lpexg->debugData.cOmapTo = dwCount;
    } else {
        MHFree(lpOmap);
    }
	assert( lpexg->ulImageAlign!=0 );
	lpexg->debugData.dwOrigAlign = lpexg->ulImageAlign;
    return sheNone;
}

SHE
LoadPdata(
    LPEXG                   lpexg,
    int                     hfile,
    ULONG                   loadAddress,
    ULONG                   imageBase,
    ULONG                   start,
    ULONG                   size,
    BOOL                    fDbgFile
    )
{
    ULONG                          cFunc;
    LONG                           diff;
    ULONG                          index;
    PIMAGE_RUNTIME_FUNCTION_ENTRY  rf;
    PIMAGE_RUNTIME_FUNCTION_ENTRY  tf;
    PIMAGE_FUNCTION_ENTRY          dbgRf;
    LONG                           fpos;

    lpexg->debugData.lpRtf = NULL;
    lpexg->debugData.cRtf  = 0;

    if(size == 0) {
        return sheNone;    // No data to read...  Just return.
    }

    if(fDbgFile) {
        cFunc = size / sizeof(IMAGE_FUNCTION_ENTRY);
        diff = 0;
    } else {
        cFunc = size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
#if defined(TARGET_ALPHA)
        // Executable's load address is never initialized

        if (loadAddress) {
            diff = loadAddress - imageBase;
        } else {
                diff = 0;
                }
#else
        diff = loadAddress - imageBase;
#endif
    }

    lpexg->fIsRisc = TRUE;

    fpos = SYTell(hfile);

    if (SYSeek(hfile, start, SEEK_SET) != (LONG) start) {
        return(sheCorruptOmf);
    }

    if(fDbgFile) {
        if(!(dbgRf = (PIMAGE_FUNCTION_ENTRY) MHAlloc(size))) {
            return sheOutOfMemory;
        }
        SYReadFar(hfile, (LPB)dbgRf, size);
        size = cFunc * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
        if(!(tf = rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) MHAlloc(size))) {
            MHFree(dbgRf);
            return sheOutOfMemory;
        }
        for(index=0; index<cFunc; index++) {
            rf[index].BeginAddress       = dbgRf[index].StartingAddress + loadAddress;
            rf[index].EndAddress         = dbgRf[index].EndingAddress + loadAddress;
            rf[index].PrologEndAddress   = dbgRf[index].EndOfPrologue + loadAddress;
            rf[index].ExceptionHandler   = 0;
            rf[index].HandlerData        = 0;
        }
        MHFree(dbgRf);

    } else {
        if(!(tf = rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) MHAlloc(size))) {
            return sheOutOfMemory;
        }
        SYReadFar(hfile, (LPB)rf, size);
    }

    // If this is an ilink'd image, there'll be padding at the end of the pdata section
    //  (to allow for insertion later).  Shrink the table if this is true.

    // Find the start of the padded page (end of the real data)

    for(index=0; index<cFunc && tf->BeginAddress; tf++,index++) {
        ;
    }

    if(index < cFunc) {
        cFunc = index;
        size  = index * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
        if(!(rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) MHRealloc(rf, size))) {
            return sheOutOfMemory;
        }
    }

    if (diff != 0) {
        for (index=0; index<cFunc; index++) {
            rf[index].BeginAddress += diff;
            rf[index].EndAddress += diff;
            rf[index].PrologEndAddress += diff;
            rf[index].ExceptionHandler = 0;
            rf[index].HandlerData = 0;
        }
    }

    lpexg->debugData.lpRtf = rf;
    lpexg->debugData.cRtf  = cFunc;

    SYSeek(hfile, fpos, SEEK_SET);
    return sheNone;
}


//  CheckSignature - check file signature
//
//  she = CheckSignature (INT hfile, OMFSignature *pSig)
//
//  Entry   hfile = handle to file
//          pSig  = location where signature should be written to
//          ppdb  = PDB information.
//
//  Exit    none
//
//  Return  sheNoSymbols if exe has no signature
//          sheMustRelink if exe has NB00 to NB06 or NB07 (qcwin) signature
//          sheNotPacked if exe has NB08 signature
//          sheNone if exe has NB11 signature
//          sheConvertTIs if exe has NB09 signature
//          sheFutureSymbols if exe has NB12 to NB99 signature

SHE
CheckSignature(
    INT hfile,
    OMFSignature *pSig,
    PDB_INFO *ppdb
    )
{
    UINT    uSig;

    if ((SYReadFar (hfile, (LPB) pSig, sizeof (*pSig)) != sizeof (*pSig)) ||
         (pSig->Signature[0] != 'N') ||
         (pSig->Signature[1] != 'B') ||
         (!isdigit(pSig->Signature[2])) ||
         (!isdigit(pSig->Signature[3]))) {
        return sheNoSymbols;
    }

    switch (*(LONG UNALIGNED *)(pSig->Signature)) {
        case sigNB05:
        case sigNB06:
        case sigNB07:
            return sheMustRelink;
        case sigNB08:
            return sheNotPacked;
        case sigNB09:
            return sheConvertTIs;
        case sigNB10:
            SYReadFar(hfile, (LPB)ppdb, sizeof(PDB_INFO));
        case sigNB11:
            return sheNone;
        default:
            return sheFutureSymbols;
    }
}


//  OLMkSegDir - MakeSegment directory
//
//  Entry
//
//  Returns non-zero for success

BOOL
OLMkSegDir(
    WORD  csgd,
    LPSGD *lplpsgd,
    LPSGE *lplpsge,
    LPEXG lpexg
    )
{
    LPSGD lpsgd;
    LPSGE lpsge = NULL;
    int  *lpisge;
    int   csgc = 0;
    int   isge = 0;
    int   isgd = 0;
    DWORD iMod;

    if (!(lpsgd = (LPSGD) MHAlloc (csgd * sizeof (SGD)))) {
        return FALSE;
    }

    if (!(lpisge = (int *) MHAlloc (csgd * sizeof (int)))) {
        MHFree(lpsgd);
        return FALSE;
    }

    memset(lpsgd,  0, csgd * sizeof(SGD));
    memset(lpisge, 0, csgd * sizeof(int));

    // Count the number of contributers per segment

    for (iMod = 1; iMod <= lpexg->cMod; iMod++) {
        LPMDS lpmds = &lpexg->rgMod[iMod];
        int cseg = lpmds->csgc;
        int iseg = 0;
        int isegT = 0;

        for (iseg = 0; iseg < cseg; iseg++) {
            isegT = lpmds->lpsgc [ iseg ].seg;
            if (isegT != 0) {
                lpsgd [ isegT - 1 ].csge += 1;
                csgc += 1;
            }
        }
    }

    // Allocate subtable for each all segments

    lpsge = (LPSGE) MHAlloc (csgc * sizeof (SGE));

    if (!lpsge) {
        MHFree (lpsgd);
        MHFree (lpisge);
        return FALSE;
    }

    // Set up sgd's with pointers into appropriate places in the sge table

    isge = 0;
    for (isgd = 0; isgd < (int) csgd; isgd++) {
        lpsgd[ isgd ].lpsge = lpsge + isge;
        isge += lpsgd[ isgd ].csge;
    }

    // Fill in the sge table

    for (iMod = 1; iMod <= lpexg->cMod; iMod += 1) {
        LPMDS lpmds = &lpexg->rgMod[iMod];
        int cseg = lpmds->csgc;
        int iseg = 0;

        for (iseg = 0; iseg < cseg; iseg++) {
            int isgeT = lpmds->lpsgc[ iseg ].seg - 1;

            if (isgeT != -1) {
                lpsgd[ isgeT ].lpsge[ lpisge[ isgeT ]].sgc =
                    lpmds->lpsgc[ iseg ];
                lpsgd[ isgeT ].lpsge[ lpisge[ isgeT ]].hmod =
                    (HMOD)lpmds;
                lpisge[ isgeT ] += 1;
            }
        }
    }

    MHFree (lpisge);

    *lplpsge = lpsge;
    *lplpsgd = lpsgd;

    return TRUE;
}

//  OLMkModule - make module entries for module
//
//  Entry   lpexg  - Supplies the pointer to the EXG structure for current exe
//          hexg   - Supplies the handle EXG structure
//
//  Returns non-zero for success


int
OLMkModule(
    LPEXG   lpexg,
    HEXG    hexg
    )
{
    LSZ     lszModName;
    LPMDS   lpmds;
    LPB     lpbName;
    WORD    cbName;
    WORD    i;
    OMFModule *pMod;

    // Point to the OMFModule table.  This structure describes the name and
    // segments for the current Module being processed.  There is a one to one
    // correspondance of modules to object files.

    pMod = (OMFModule *) (lpexg->lpbData + lpdssCur->lfo);

    // Point to the name field in the module table.  This location is variable
    // and is dependent on the number of contributuer segments for the module.

    lpbName = ((LPB)pMod) +
      offsetof (OMFModule, SegInfo[0]) +
      (sizeof (OMFSegDesc) * pMod->cSeg);
    cbName = *((LPB)lpbName)++;
    lszModName = (LPCH) MHAlloc (cbName + 1);
    memmove(lszModName, lpbName, cbName);
    *(lszModName + cbName) = 0;

    lpmds = &lpexg->rgMod[lpdssCur->iMod];

    lpmds->imds   = lpdssCur->iMod;
    lpmds->hexg   = hexg;
    lpmds->name   = lszModName;

    // step thru making the module entries
    //
    // NOTENOTE -- This can most likely be optimized as the data
    //          is just being copied from the debug data.

    lpmds->csgc = pMod->cSeg;
    lpmds->lpsgc = (LPSGC)MHAlloc ( pMod->cSeg * sizeof ( SGC ) );

    for ( i = 0; i < pMod->cSeg; i++ ) {
        if ( pMod->SegInfo[i].Seg > csegExe ) {
            csegExe = pMod->SegInfo[i].Seg;
        }
        lpmds->lpsgc[i].seg = pMod->SegInfo[i].Seg;
        lpmds->lpsgc[i].off = pMod->SegInfo[i].Off;
        lpmds->lpsgc[i].cb  = pMod->SegInfo[i].cbSeg;
    }

    return TRUE;
}

SHE
NB10LoadModules(
    LPEXG   lpexg,
    ULONG*  pcMods,
    HEXG    hexg
    )
{
    Mod* pmod = NULL;
    ULONG   cMod = 0;
    IMOD    imod;

    // First count up the number of Mods

    while (DBIQueryNextMod(lpexg->pdbi, pmod, &pmod) && pmod) {
        if(!ModQueryImod(pmod, &imod))
            return sheCorruptOmf;
        if(imod > *pcMods)
            cMod = imod;
    }

    *pcMods = cMod;

    // Got the count.  Allocate rgMod.

    lpexg->rgMod = (LPMDS) MHAlloc((cMod+2) * sizeof(MDS));
    if (lpexg->rgMod == NULL) {
        return sheOutOfMemory;
    }
    memset(lpexg->rgMod, 0, sizeof(MDS)*(cMod+2));
    lpexg->rgMod[cMod+1].imds = (WORD) -1;

    // Now fill in the blanks.

    pmod = NULL;

    for (; cMod; cMod--) {
        LPMDS   lpmds;
        LPCH    lpchName;
        CB      cbName;

        DBIQueryNextMod(lpexg->pdbi, pmod, &pmod);

        if(!ModQueryImod(pmod, &imod))
            return sheCorruptOmf;

        lpmds = &lpexg->rgMod[imod];

        lpmds->imds = imod;
        lpmds->pmod = pmod;
        lpmds->hexg = hexg;

        if(!ModQueryName(pmod, NULL, &cbName))
            return sheCorruptOmf;
        lpmds->name  = (LSZ) MHAlloc(cbName);
        if(!lpmds->name)
            return sheOutOfMemory;
        if(!ModQueryName(pmod, lpmds->name, &cbName))
            return sheCorruptOmf;
        if(!ModSetPvClient(pmod, lpmds))
            return sheCorruptOmf;
    }
    return sheNone;
}


BOOL
OLLoadHashTable(
    LPB     lpbData,
    ULONG   cbTable,
    LPSHT   lpsht,
    BOOL    fDWordChains
    )
{
    WORD    ccib   = 0;
    LPUL    rgib   = NULL;
    LPUL    rgcib  = NULL;
    ULONG   cbHeader = 0;
    LPB     lpHashStart = lpbData;

    memset(lpsht, 0, sizeof(SHT));

    ccib = *(WORD *)lpbData;        // First, get the hash bucket counts
    lpbData += 4;                   // the 2 byte hash count and 2 bytes padding
    rgib = (LPUL) lpbData;

    lpbData += ccib * sizeof(ULONG);

    rgcib = (LPUL) lpbData;

    lpbData += ccib * sizeof(ULONG);

    // Subtract off what we've processed already.

    cbTable     -= (lpbData - lpHashStart);

    lpsht->ccib  = ccib;
    lpsht->rgib  = rgib;
    lpsht->rgcib = rgcib;
    lpsht->lpalm = BuildALM(FALSE,
                            btAlign,
                            lpbData,
                            cbTable,
                            cbAlign);

    if (lpsht->lpalm == NULL) {
        return FALSE;
    }

    return TRUE;
}


BOOL
OLLoadHashSubSec(
    LPGST       lpgst,
    LPB         lpbData,
    WidenTi *   pwti
    )
{
    LPB        lpbTbl = NULL;
    OMFSymHash hash;
    ULONG      cbSymbol;
    BOOL       fRet = TRUE;
    LPSHT      lpshtName = &lpgst->shtName;
    LPSHT      lpshtAddr = &lpgst->shtAddr;

    memset(lpshtAddr, 0, sizeof(SHT));
    memset(lpshtName, 0, sizeof(SHT));

    hash = *(OMFSymHash *)lpbData;

    lpbData += sizeof (OMFSymHash);

    cbSymbol = hash.cbSymbol;

    if (pwti) {
        ConvertGlobal16bitSyms(pwti, lpgst, lpbData, cbSymbol);
    }
    else
    {
        lpgst->lpalm = BuildALM(TRUE,
                                btAlign,
                                lpbData,
                                cbSymbol,
                                cbAlign);
    }

    if (lpgst->lpalm == NULL) {
        return FALSE;
    }

    lpbData += cbSymbol;

//    if (hash.symhash == 6 || hash.symhash == 10) {
    if (hash.symhash == 10) {
        fRet = OLLoadHashTable(lpbData,
                               hash.cbHSym,
                               &lpgst->shtName,
                               hash.symhash == 10);
        lpgst->shtName.HashIndex = hash.symhash;
    }

    lpbData += hash.cbHSym;

//    if (hash.addrhash == 8 || hash.addrhash == 12) {
    if (hash.addrhash == 12) {
        fRet = OLLoadHashTable(lpbData,
                               hash.cbHAddr,
                               &lpgst->shtAddr,
                               hash.addrhash == 12);
        lpgst->shtAddr.HashIndex = hash.addrhash;
    }

    return fRet;
}

//  OLLoadTypes - load compacted types table
//
//  Input:  lpexg - Pointer to exg we're working on.
//
//  Returns:    - An error code

SHE
OLLoadTypes(
    LPEXG lpexg
    )
{
    LPB         pTyp;
    LPB         pTypes;

    OMFTypeFlags flags;
    DWORD        cType  = 0;
    DWORD       *rgitd  = NULL;
    DWORD        ibType = 0;

    pTyp = pTypes = lpexg->lpbData + lpdssCur->lfo;

    flags = *(OMFTypeFlags *) pTypes;
    pTypes += sizeof(OMFTypeFlags);
    cType = *(ULONG *) pTypes;
    pTypes += sizeof(ULONG);

    if (!cType) {
        return sheNone;
    }

    // Point to the array of pointers to types

    rgitd = (DWORD *) pTypes;

    // Move past them

    pTypes += cType * sizeof(ULONG);

    // Read in the type index table

    ibType = pTypes - pTyp;
    lpexg->lpalmTypes = BuildALM (FALSE,
                                  btAlign,
                                  pTypes,
                                  lpdssCur->cb - ibType,
                                  cbAlignType);

    if (lpexg->lpalmTypes == NULL) {
        return sheOutOfMemory;
    }

    lpexg->rgitd = rgitd;
    lpexg->citd  = cType;

    return sheNone;
}


//  OLLoadSym - load symbol information
//
//  error = OLLoadSym (pMod)
//
//  Entry   lpexg - Pointer to exg structure to use.
//
//  Returns sheNone if symbols loaded

SHE
OLLoadSym(
    LPEXG lpexg
    )
{
    // UNDONE: If we run into problems with a stale VC, we'll have to
    //  revert to reading the file on demand.  The expectation is that the
    //  mapped I/O code will just work.

    // lpexg->rgMod[lpdssCur->iMod].symbols = NULL;

    if (lpexg->pwti) {
        // we be converting 16-bit symbols to 32-bit versions.
        SHE                 sheRet = sheOutOfMemory;
        WidenTi *           pwti = lpexg->pwti;
        PMDS                pmod = &lpexg->rgMod[lpdssCur->iMod];
        SymConvertInfo &    sci = pmod->sci;
        PB                  pbSyms = lpexg->lpbData + lpdssCur->lfo;
        CB                  cbSyms = lpdssCur->cb;

        // Remember the signature!
        if (pwti->fQuerySymConvertInfo (sci, pbSyms, cbSyms, sizeof ULONG)) {
            sci.pbSyms = PB(MHAlloc (sci.cbSyms));
            sci.rgOffMap = POffMap(MHAlloc (sci.cSyms * sizeof OffMap));
            if (sci.pbSyms && sci.rgOffMap) {
                memset (sci.pbSyms, 0, sci.cbSyms);
                memset (sci.rgOffMap, 0, sci.cSyms * sizeof OffMap);
                if (pwti->fConvertSymbolBlock (sci, pbSyms, cbSyms, sizeof ULONG) ) {
                    pmod->symbols = sci.pbSyms;
                    *(ULONG*)pmod->symbols = CV_SIGNATURE_C11;
                    pmod->cbSymbols = sci.cbSyms;
                    // REVIEW: What about pmod->ulsym?  how is it used?
                    sheRet = sheNone;
                }
                else {
                    sheRet = sheCorruptOmf;
                }
            }
        }
        return sheRet;
    }
    else
    {
        lpexg->rgMod[lpdssCur->iMod].symbols = lpexg->lpbData + lpdssCur->lfo;
        lpexg->rgMod[lpdssCur->iMod].cbSymbols = lpdssCur->cb;
        lpexg->rgMod[lpdssCur->iMod].ulsym = lpdssCur->lfo;
        return sheNone;
    }
}


__inline SHE
OLLoadSrc(
    LPEXG lpexg
    )
{
    lpexg->rgMod[lpdssCur->iMod].hst = (HST) (lpexg->lpbData + lpdssCur->lfo);
    return sheNone;
}


__inline SHE
OLGlobalPubs(
    LPEXG   lpexg
    )
{
    SHE   she   = sheNone;

    if (!OLLoadHashSubSec (&lpexg->gstPublics,
                           lpexg->lpbData + lpdssCur->lfo,
                           lpexg->pwti)) {
        she = sheOutOfMemory;
    }

    return she;
}


__inline SHE
OLGlobalSym(
    LPEXG   lpexg
    )
{
    SHE   she   = sheNone;

    if (!OLLoadHashSubSec (&lpexg->gstGlobals,
                           lpexg->lpbData + lpdssCur->lfo,
                           lpexg->pwti)) {
        she = sheOutOfMemory;
    }

    return she;
}

SHE
OLLoadSegMap(
    LPEXG   lpexg
    )
{
    LPB lpb;
    SHE sheRet = sheNone;

    if(lpexg->pdbi) {
        CB      cb;

        // load from the pdb
        if(!DBIQuerySecMap(lpexg->pdbi, NULL, &cb) ||
           !(lpb = (LPB) MHAlloc (cb))) {
            sheRet = sheOutOfMemory;
        } else
        if(!DBIQuerySecMap(lpexg->pdbi, lpb, &cb)) {
            MHFree(lpb);
            lpb = NULL;
            sheRet = sheOutOfMemory;
        }
    } else {
            lpb = lpexg->lpbData + lpdssCur->lfo;
    }

    lpexg->lpgsi = lpb;

    return sheRet;
}

SHE
OLLoadNameIndex(
    LPEXG   lpexg
    )
{
    OMFFileIndex *  lpefi;
    WORD            cmod = 0;
    WORD            cfile = 0;
    CB              cb;

    if(lpexg->pdbi) {
        if(!DBIQueryFileInfo(lpexg->pdbi, 0, &cb)) {
            return sheNoSymbols;
        }
        else if(!(lpefi = (OMFFileIndex *) MHAlloc(cb))) {
            return sheOutOfMemory;
        }
        else if(!DBIQueryFileInfo(lpexg->pdbi, (PB)lpefi, &cb)) {
            MHFree(lpefi);
            return sheNoSymbols;
        }
    } else {
        lpefi = (OMFFileIndex *)(lpexg->lpbData + lpdssCur->lfo);
        cb = (CB)lpdssCur->cb;
    }

    cmod  = lpefi->cmodules;
    // Make sure we found as many sstModule entries as we should have.
    assert(cmod == lpexg->cMod);
//    lpexg->cmod      = cmod;
    cfile = lpefi->cfilerefs;

    lpexg->lpefi     = (LPB) lpefi;
    lpexg->rgiulFile = lpefi->modulelist;
    lpexg->rgculFile = &lpefi->modulelist [cmod];
    lpexg->rgichFile = (LPUL) &lpefi->modulelist [cmod * 2];

    lpexg->lpchFileNames = (LPCH) &lpefi->modulelist [cmod * 2 + cfile * 2];

    lpexg->cbFileNames =
        (ULONG)(cb - ((LPB)lpexg->lpchFileNames - (LPB)lpefi + 1));

    return sheNone;
}

SHE
OLStaticSym(
    LPEXG   lpexg
    )
{
    SHE   she   = sheNone;

    if (!OLLoadHashSubSec (&lpexg->gstStatics,
                           lpexg->lpbData + lpdssCur->lfo,
                           lpexg->pwti)) {
        she = sheOutOfMemory;
    }

    return she;

}


const SHE mpECToShe[] = {
    sheNone,            // EC_OK
    sheNoSymbols,       // EC_USAGE (invalid parameter of call order)
    sheOutOfMemory,     // EC_OUT_OF_MEMORY (-, out of RAM)
    sheNoSymbols,       // EC_FILE_SYSTEM (pdb name, can't write file, out of disk, etc)
    shePdbNotFound,     // EC_NOT_FOUND (PDB file not found)
    shePdbBadSig,       // EC_INVALID_SIG (PDB::OpenValidate() and its clients only)
    shePdbInvalidAge,   // EC_INVALID_AGE (PDB::OpenValidate() and its clients only)
    sheNoSymbols,       // EC_PRECOMP_REQUIRED (obj name, Mod::AddTypes only)
    sheNoSymbols,       // EC_OUT_OF_TI (pdb name, TPI::QueryTiForCVRecord() only)
    sheNoSymbols,       // EC_NOT_IMPLEMENTED
    sheNoSymbols,       // EC_V1_PDB (pdb name, PDB::Open() only)
    shePdbOldFormat,    // EC_FORMAT (accessing pdb with obsolete format)
    sheNoSymbols,       // EC_LIMIT,
    sheNoSymbols,       // EC_CORRUPT,             // cv info corrupt, recompile mod
    sheNoSymbols,       // EC_TI16,                // no 16-bit type interface present
    sheNoSymbols,       // EC_ACCESS_DENIED,       // "pdb name", PDB file read-only
};

// Get the name of the pdb file (OMF name) for the specified exe.  If the
// LoadPdb hasn't been called on this exe OR it's not NB10, this will return
// an empty string!  Note: There will only be an lszPdbName if there was
// an error loading the pdb

VOID LOADDS
SHPdbNameFromExe(
    LSZ lszExe,
    LSZ lszPdbName,
    UINT cbMax
    )
{
    HEXE    hexe;

    // Zero out the destination
    memset(lszPdbName, 0, cbMax);

    // Look up the exe
    if (hexe = SHGethExeFromName(lszExe)) {
        HEXG    hexg = ((LPEXE)LLLock(hexe))->hexg;
        LPEXG   lpexg = (LPEXG)LLLock(hexg);

        // Only copy if there's a pdbname
        if (lpexg->lszPdbName) {
            _tcsncpy(lszPdbName, lpexg->lszPdbName, cbMax);
        }

        // Clean up
        LLUnlock(hexg);
        LLUnlock(hexe);
    }
}


BOOL
GetDefaultKeyName(
        LPCTSTR KeySuffix,
        LPTSTR  KeyBuffer,
        DWORD   nLength
        )
{
        DWORD   lnLength = nLength;

    if (!GetRegistryRoot(KeyBuffer, &lnLength)) {
                return FALSE;
    }

    if (KeyBuffer[lnLength - 1] != _T('\\')) {
        KeyBuffer[lnLength++] = _T('\\');
        KeyBuffer[lnLength] = _T('\0');
        }

    assert(*KeySuffix != _T('\\'));
    _tcscpy(&KeyBuffer[lnLength], KeySuffix);

        return TRUE;
}

// read stuff from the registry, but do it only once

void QueryRegistry()
{
    TCHAR szDefaultKeyName[300];
    static const TCHAR szKeySuffix[] =
        _T("Build System\\Components\\Platforms\\Win32 (x86)\\Directories");
    static const TCHAR szPdbDirs[] = "Pdb Dirs";

    if (fQueriedRegistry)
        return;

    fQueriedRegistry = TRUE;

    GetDefaultKeyName(szKeySuffix, szDefaultKeyName, sizeof(szDefaultKeyName));

    HKEY hSectionKey = NULL;
    DWORD nType = REG_SZ;
    DWORD nSize = sizeof(szSearchPath);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szDefaultKeyName, 0, KEY_READ, &hSectionKey) != ERROR_SUCCESS)
    {
        szSearchPath[0] = 0;
    }
    else
    {
        if (RegQueryValueEx(hSectionKey,
                            szPdbDirs,
                         NULL,
                         &nType,
                            (LPBYTE) szSearchPath,
                            &nSize) != ERROR_SUCCESS)
        {
            szSearchPath[0] = 0;
        }

        CloseHandle(hSectionKey);
    }

    GetDefaultKeyName("Debug", szDefaultKeyName, sizeof(szDefaultKeyName));
    nType = REG_DWORD;
    nSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szDefaultKeyName, 0, KEY_READ, &hSectionKey) != ERROR_SUCCESS)
    {
        fLoadExports = FALSE;
    }
    else
    {
        if (RegQueryValueEx(hSectionKey,
                            "LoadExports",
                         NULL,
                         &nType,
                            (LPBYTE) &fLoadExports,
                            &nSize) != ERROR_SUCCESS)
        {
            fLoadExports = FALSE;
        }

        CloseHandle(hSectionKey);
    }

}

SHE
LoadPdb(
    LPEXG lpexg,
    PDB_INFO *ppdb
    )
{
    EC ec;
    char szRefPath[_MAX_PATH];
    char szPDBOut[cbErrMax];
    BOOL fOpenValidate;

    assert(lpexg);

    // figure out the home directory of the EXE/DLL or DBG file - pass that along to
    // OpenValidate this will direct to dbi to search for it in that directory.

    _fullpath(szRefPath, lpexg->lszDebug ? lpexg->lszDebug : lpexg->lszName, _MAX_PATH);
    char *pcEndOfPath = _tcsrchr(szRefPath, '\\');
    *pcEndOfPath = '\0';        // null terminate it
    *szPDBOut = '\0';

    if (!fQueriedRegistry)
        QueryRegistry();

    if ( pAltPDBOpen == 0 ) {
        fOpenValidate = PDBOpenValidateEx(ppdb->sz,
                                           szRefPath,
                                           szSearchPath,
                                           pdbRead,
                                           ppdb->sig,
                                           ppdb->age,
                                           &ec,
                                           szPDBOut,
                                           &lpexg->ppdb);
    } else {
        fOpenValidate = (*pAltPDBOpen)   (ppdb->sz,
                                           szRefPath,
                                           szSearchPath,
                                           pdbRead,
                                           ppdb->sig,
                                           ppdb->age,
                                           &ec,
                                           szPDBOut,
                                           &lpexg->ppdb);
    }

    if (!fOpenValidate) {
        // Save the name of the pdb with the error

        if (!(lpexg->lszPdbName = (LSZ)MHAlloc(_tcslen(szPDBOut) + 1))) {
            return sheOutOfMemory;
        }

        _tcscpy(lpexg->lszPdbName, szPDBOut);

        return mpECToShe[ec];
    }

    // Store the name of the pdb in lszDebug.

    char *szPdb = PDBQueryPDBName(lpexg->ppdb, (char *)szPDBOut);
    assert(szPdb);

    // Save the name of the PDB

    if (lpexg->lszDebug) {
        MHFree(lpexg->lszDebug);
        lpexg->lszDebug = 0;
    }

    if (!(lpexg->lszDebug = (LSZ)MHAlloc(_tcslen(szPDBOut) + 1))) {
        return sheOutOfMemory;
    }

    _tcscpy(lpexg->lszDebug, szPDBOut);

    if (!PDBOpenTpi(lpexg->ppdb, pdbRead, &(lpexg->ptpi))) {
        ec = PDBQueryLastError(lpexg->ppdb, NULL);

        return mpECToShe[ec];
    }

#if CC_LAZYTYPES
    if (!PDBOpenDBIEx(lpexg->ppdb, pdbRead, lpexg->lszName, &(lpexg->pdbi), pSYFindDebugInfoFile))
#else
    if (!PDBOpenDBI(lpexg->ppdb, pdbRead, lpexg->lszName, &(lpexg->pdbi)))
#endif
    {
        ec = PDBQueryLastError(lpexg->ppdb, NULL);

        return mpECToShe[ec];
    }

    if (!STABOpen(&(lpexg->pstabUDTSym))) {
        return sheOutOfMemory;
    }

#ifndef ACTIVEDBG
	// stub out while V7 uses PDB5 APIs

	// try to load FPO/OMAP/etc information if they have not
	// been loaded yet from the executable
	return LoadDebugDataFromPdb(lpexg, ppdb);
}


// Routine Description:
//
//  This routine loads FPO / RTE / OMAP from the pdb
//  if the relevant tables have not already been loaded from the executable

LOCAL SHE
LoadDebugDataFromPdb(
    LPEXG lpexg,
    PDB_INFO *ppdb
    )
{
    EC ec;
    char szExePath[_MAX_PATH];
    char szPDBOut[cbErrMax];

	if (lpexg->debugData.cRtf == 0) {
		Dbg *pfpo, *pexcept;
		if (DBIOpenDbg(lpexg->pdbi, dbgtypeFPO, &pfpo)) {
			long cFpo = DbgQuerySize(pfpo);
			if (cFpo > 0) {
				lpexg->debugData.cRtf = cFpo;
				if (!(lpexg->debugData.lpFpo = (PFPO_DATA) MHAlloc(cFpo * sizeof (FPO_DATA)))) {
					DbgClose(pfpo);
					return sheOutOfMemory;
				}
				if (!DbgQueryNext(pfpo, cFpo, lpexg->debugData.lpFpo)) {
					ec = PDBQueryLastError(lpexg->ppdb, NULL);
					DbgClose(pfpo);
					return mpECToShe[ ec ];
				}
			}
		}
		else if (DBIOpenDbg(lpexg->pdbi, dbgtypeException, &pexcept)) {
			long cRtf = DbgQuerySize(pexcept);
			long index;

			if (cRtf > 0) {
				PIMAGE_FUNCTION_ENTRY dbgRf;
				PIMAGE_RUNTIME_FUNCTION_ENTRY rf;
				if(	!(dbgRf = (PIMAGE_FUNCTION_ENTRY) 
						MHAlloc(cRtf * sizeof (IMAGE_FUNCTION_ENTRY))) ||
					!(rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) 
						MHAlloc(cRtf * sizeof (IMAGE_RUNTIME_FUNCTION_ENTRY)))){
					DbgClose(pexcept);
					return sheOutOfMemory;
				}
				lpexg->debugData.cRtf = cRtf;
				if (!DbgQueryNext(pexcept, cRtf, dbgRf)) {
					ec = PDBQueryLastError(lpexg->ppdb, NULL);
					DbgClose(pexcept);
					return mpECToShe[ ec ];
				}

				for(index=0; index < cRtf; index++) {
					rf[index].BeginAddress       = dbgRf[index].StartingAddress + lpexg->LoadAddress;
					rf[index].EndAddress         = dbgRf[index].EndingAddress + lpexg->LoadAddress;
					rf[index].PrologEndAddress   = dbgRf[index].EndOfPrologue + lpexg->LoadAddress;
					rf[index].ExceptionHandler   = 0;
					rf[index].HandlerData        = 0;
				}
				MHFree(dbgRf);
				lpexg->debugData.lpRtf = rf;				
			}
		}
	}

	if (lpexg->debugData.cOmapFrom == 0) {
		Dbg *pomapf;
		if (DBIOpenDbg(lpexg->pdbi, dbgtypeOmapFromSrc, &pomapf)) {
			long cOmapFrom = DbgQuerySize(pomapf);
			if (cOmapFrom > 0) {
				lpexg->debugData.cOmapFrom = cOmapFrom;
				if (!(lpexg->debugData.lpOmapFrom = (LPOMAP) MHAlloc(cOmapFrom * sizeof (OMAP)))) {
					DbgClose(pomapf);
					return sheOutOfMemory;
				}
				if (!DbgQueryNext(pomapf, cOmapFrom, lpexg->debugData.lpOmapFrom)) {
					ec = PDBQueryLastError(lpexg->ppdb, NULL);
					DbgClose(pomapf);
					return mpECToShe[ ec ];
				}
			}
		}
	}

	if (lpexg->debugData.cOmapTo == 0) {
		Dbg *pomapt;
		if (DBIOpenDbg(lpexg->pdbi, dbgtypeOmapToSrc, &pomapt)) {
			long cOmapTo = DbgQuerySize(pomapt);
			if (cOmapTo > 0) {
				lpexg->debugData.cOmapTo = cOmapTo;
				if (!(lpexg->debugData.lpOmapTo = (LPOMAP) MHAlloc(cOmapTo * sizeof (OMAP)))) {
					DbgClose(pomapt);
					return sheOutOfMemory;
				}
				if (!DbgQueryNext(pomapt, cOmapTo, lpexg->debugData.lpOmapTo)) {
					ec = PDBQueryLastError(lpexg->ppdb, NULL);
					DbgClose(pomapt);
					return mpECToShe[ ec ];
				}
			}
		}
	}
	lpexg->debugData.dwOrigAlign = lpexg->ulImageAlign;
#endif

	return sheNone;
}


// Routine Description:
//
//  This routine is used to validate that the debug information
//  in a file matches the debug information requested
//
// Arguments:
//
//  hFile       - Supplies the file handle to be validated
//  lpv         - Supplies a pointer to the information to used in vaidation
//
// Return Value:
//
//  TRUE if matches and FALSE otherwise

SHE
OLValidate(
    int          hFile,
    void *       lpv,
    LPSTR        lpszErrText
    )
{
    VLDCHK *            pVldChk = (VLDCHK *) lpv;
    IMAGE_DOS_HEADER    exeHdr;
    IMAGE_NT_HEADERS    peHdr;
    int                 fPeExe = FALSE;
    int                 fPeDbg = FALSE;

    if (lpszErrText) {
        *lpszErrText = 0;
    }

    // Read in a dos exe header

    if ((SYSeek(hFile, 0, SEEK_SET) != 0) ||
        (SYReadFar( hFile, (LPB) &exeHdr, sizeof(exeHdr)) != sizeof(exeHdr))) {
        return sheNoSymbols;
    }

    // See if it is a dos exe hdr

    if (exeHdr.e_magic == IMAGE_DOS_SIGNATURE) {
        if ((SYSeek(hFile, exeHdr.e_lfanew, SEEK_SET) == exeHdr.e_lfanew) &&
            (SYReadFar(hFile, (LPB) &peHdr, sizeof(peHdr)) == sizeof(peHdr))) {
            if (peHdr.Signature == IMAGE_NT_SIGNATURE) {
                fPeExe = TRUE;
            }
        }
    } else if (exeHdr.e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
        fPeDbg = TRUE;
    }

    if (fPeExe) {
        if (pVldChk->TimeDateStamp == 0) {
            // If the timestamp has not been initialized do so now.

            pVldChk->TimeDateStamp = peHdr.FileHeader.TimeDateStamp;
        }

        if (peHdr.FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
            return sheNoSymbols;
        }

        if (peHdr.FileHeader.TimeDateStamp != pVldChk->TimeDateStamp) {
            if (lpszErrText) {
                sprintf(lpszErrText,
                        "*** WARNING: symbols timestamp is wrong 0x%08x 0x%08x",
                        peHdr.FileHeader.TimeDateStamp,
                        pVldChk->TimeDateStamp);
            }

            return sheBadTimeStamp;
        }
    } else if (fPeDbg) {
        IMAGE_SEPARATE_DEBUG_HEADER sepHdr;

        if ((SYSeek(hFile, 0, SEEK_SET) != 0) ||
            (SYReadFar(hFile, (LPB) &sepHdr, sizeof(sepHdr)) != sizeof(sepHdr))) {
            return sheNoSymbols;
        }

        if (sepHdr.TimeDateStamp != pVldChk->TimeDateStamp) {
            if (lpszErrText) {
                sprintf(lpszErrText,
                        "*** WARNING: symbols timestamp is wrong 0x%08x 0x%08x",
                        sepHdr.TimeDateStamp,
                        pVldChk->TimeDateStamp);
            }

            return sheBadTimeStamp;
        }
    } else {
            return sheNoSymbols;
        }

    return sheNone;
}


BOOL
OLUnloadOmf(
    LPEXG lpexg
    )
{
    ULONG i;
    // Cleanup the Module table;
    for (i = 1; i <= lpexg->cMod; i++) {
        KillMdsNode(&lpexg->rgMod[i]);
    }

    if (lpexg->rgMod) {
        MHFree(lpexg->rgMod);
        lpexg->rgMod = NULL;
        lpexg->cMod = 0;
    }

    // module map info
    if (lpexg->lpsgd) {
        MHFree(lpexg->lpsgd);
        lpexg->lpsgd = NULL;
        lpexg->csgd = 0;
    }

    //
    if (lpexg->lpsge) {
        MHFree(lpexg->lpsge);
        lpexg->lpsge = NULL;
    }

    if (lpexg->lpbData) {
        // Depending on how we got the data, free it.

        if (lpexg->pvSymMappedBase) {
            // Mapped view of file.
            UnmapViewOfFile(lpexg->pvSymMappedBase);
            lpexg->pvSymMappedBase = NULL;
        } else {
            if (lpexg->fSymConverted) {
                // Converted from coff/sym file
                LocalFree(lpexg->lpbData);
            } else {
                // Read the blob in from disk
                MHFree(lpexg->lpbData);
            }
        }

        lpexg->lpbData = NULL;
    }

    // OSDebug 4 FPO info
    if (lpexg->debugData.lpRtf) {
        MHFree(lpexg->debugData.lpRtf);
        lpexg->debugData.lpRtf = NULL;
    }

    if (lpexg->debugData.lpOmapFrom) {
        MHFree(lpexg->debugData.lpOmapFrom);
        lpexg->debugData.lpOmapFrom = NULL;
    }

    if (lpexg->debugData.lpOmapTo) {
        MHFree(lpexg->debugData.lpOmapTo);
        lpexg->debugData.lpOmapTo = NULL;
    }

    if (lpexg->debugData.lpSecStart) {
        MHFree(lpexg->debugData.lpSecStart);
        lpexg->debugData.lpSecStart = NULL;
    }

    // Segment map info
    if (lpexg->lpgsi) {
        if (lpexg->ppdb) {
            MHFree (lpexg->lpgsi);
        }
        lpexg->lpgsi = NULL;
    }

    // Source Module information
    if (lpexg->lpefi) {
        if (lpexg->ppdb) {
            MHFree(lpexg->lpefi);
        }
        lpexg->lpefi = NULL;
    }

    // Type Info array

    lpexg->citd = 0;
    lpexg->rgitd = NULL;

    // Publics, Globals, and Statics

    KillGst(&lpexg->gstPublics);
    KillGst(&lpexg->gstGlobals);
    KillGst(&lpexg->gstStatics);

    // If there's PDB info, clean up and close

    if (lpexg->ppdb) {
        if (lpexg->pgsiPubs) {
            if (!GSIClose(lpexg->pgsiPubs)) {
                assert(FALSE);
            }
            lpexg->pgsiPubs = 0;
        }

        if (lpexg->pgsiGlobs) {
            if (!GSIClose(lpexg->pgsiGlobs)) {
                assert(FALSE);
            }
            lpexg->pgsiGlobs = 0;
        }

        if (lpexg->pdbi) {
            if (!DBIClose(lpexg->pdbi)) {
                assert(FALSE);
            }
            lpexg->pdbi = 0;
        }

        if (lpexg->ptpi) {
            if (!TypesClose(lpexg->ptpi)) {
                assert(FALSE);
            }
            lpexg->ptpi = 0;
        }

        if (lpexg->pstabUDTSym) {
            STABClose(lpexg->pstabUDTSym);
            lpexg->pstabUDTSym = 0;
        }

        if (!PDBClose(lpexg->ppdb)) {
            assert(FALSE);
        }

        lpexg->ppdb = 0;
    }

    lpexg->fOmfLoaded = 0;

    return TRUE;
}

cassert(offsetof(OffMap,offOld) == 0);

int __cdecl
sgnCompareOffsFromOffMap(
    const void *  pOff1,
    const void *  pOff2
    )
{
    ULONG   off1 = POffMap(pOff1)->offOld;
    ULONG   off2 = POffMap(pOff2)->offOld;

    if (off1 < off2)
        return -1;
    if (off1 > off2)
        return 1;
    return 0;
}

void
FixupHash(
    SymConvertInfo &    sci,
    SHT &               sht
    )
{
    // for every offset in the hash, we need to fixup the offset which
    // references the old 16-bit pool with the associated new one for the
    // new pool of 32-bit types.
    assert(sht.HashIndex);
    assert(sci.cSyms);
    assert(sci.rgOffMap);

    unsigned    iBucketMax = sht.ccib;

    for (unsigned iBucket = 0; iBucket < iBucketMax; iBucket++) {
        unsigned    offChain = sht.rgib[iBucket];
        unsigned    iulpMax = sht.rgcib[iBucket];
        for (unsigned iulp = 0; iulp < iulpMax; iulp++, offChain += sizeof ULP ) {
            LPULP   pulp = LPULP(LpvFromAlmLfo(sht.lpalm, offChain));

            POffMap poffmap = POffMap(
                bsearch(
                    &pulp->ib,
                    sci.rgOffMap,
                    sci.cSyms,
                    sizeof OffMap,
                    sgnCompareOffsFromOffMap
                    )
                );
            // we should always find it
            assert(poffmap);
            if (poffmap) {
                pulp->ib = poffmap->offNew;
            }
        }
    }
}

//
// this routine will do all the post processing for the inter-relationships
// that exist between the module symbols and the global syms.  The global
// syms include procref and dataref records that include offsets into the
// module symbols.  We have to fix up those offsets to refer to the new
// offsets.  Each of these mappings is stored in the MDS structure and can
// be released after this operation.
//
// the hash offsets for the globals, publics, and statics are also fixed up here
//
void
FixupGst(
    LPEXG   pexg,
    GST &   gst,
    BOOL    fFixupRefSyms
    )
{
    assert(pexg->pwti);

    if (!gst.lpalm)
        return;

    // first off, we check to see if we need to and can fixup the refsyms
    // that may be present.
    if (fFixupRefSyms) {
        for (
            SYMPTR  psym = SYMPTR(gst.lpalm->pbData);
            psym;
            psym = GetNextSym(psym, gst.lpalm)
        ) {
            unsigned    rectyp = psym->rectyp;
            if (rectyp == S_PROCREF || rectyp == S_LPROCREF || rectyp == S_DATAREF) {
                // fix up the ibSym from the module's array of offset mappings
                REFSYM &            refsym = *(REFSYM *)psym;
                SymConvertInfo &    sci = pexg->rgMod[refsym.imod].sci;

                POffMap poffmap = POffMap(
                    bsearch(
                        &refsym.ibSym,
                        sci.rgOffMap,
                        sci.cSyms,
                        sizeof OffMap,
                        sgnCompareOffsFromOffMap
                        )
                    );
                // we should always find it.
                assert(poffmap);
                if (poffmap) {
                    refsym.ibSym = poffmap->offNew;
                }
            }
        }
    }

    // next, we check our hash tables and fix up all of the offsets there.
    if (gst.shtName.HashIndex) {
        // fixup name hash
        FixupHash(gst.sci, gst.shtName);
    }
    if (gst.shtAddr.HashIndex) {
        // fixup address hash
        FixupHash(gst.sci, gst.shtAddr);
    }
}


SHE
SheFixupConvertedSyms(
    LPEXG   pexg
    )
{
    // for each of the symbol blocks, iterate over all symbols,
    // and if they are REFSYMs of some sort, we go to the appropriate
    // module's sci.rgoffmap to find what new offset we need to plug into
    // the REFSYM.ibSym field.

    FixupGst(pexg, pexg->gstGlobals, TRUE);
    FixupGst(pexg, pexg->gstStatics, TRUE);
    FixupGst(pexg, pexg->gstPublics, FALSE);

    // we can safely get rid of all of our offmap buffers.

    // first, the module ones.
    unsigned    imod = 0;
    unsigned    imodMax = pexg->cMod;

    while (imod < imodMax) {
        MDS &   mds = pexg->rgMod[imod];
        if (mds.sci.rgOffMap) {
            MHFree(mds.sci.rgOffMap);
            mds.sci.rgOffMap = 0;
        }
        imod++;
    }

    // now, the gst versions
    if (pexg->gstGlobals.sci.rgOffMap) {
        MHFree(pexg->gstGlobals.sci.rgOffMap);
        pexg->gstGlobals.sci.rgOffMap = 0;
    }

    if (pexg->gstStatics.sci.rgOffMap) {
        MHFree(pexg->gstStatics.sci.rgOffMap);
        pexg->gstStatics.sci.rgOffMap = 0;
    }

    if (pexg->gstPublics.sci.rgOffMap) {
        MHFree(pexg->gstPublics.sci.rgOffMap);
        pexg->gstPublics.sci.rgOffMap = 0;
    }

    return sheNone;
}

void
ConvertGlobal16bitSyms(
    WidenTi *           pwti,
    LPGST               pgst,
    PB                  pbSymSrc,
    ULONG               cbSymSrc
    )
{
    SymConvertInfo &    sci = pgst->sci;

    memset ( &sci, 0, sizeof sci );
    if (pwti->fQuerySymConvertInfo(sci, pbSymSrc, cbSymSrc)) {
        // allocate the needed memory
        sci.pbSyms = PB(MHAlloc(sci.cbSyms));
        sci.rgOffMap = POffMap(MHAlloc(sci.cSyms * sizeof OffMap));
        if (sci.pbSyms && sci.rgOffMap) {
            memset(sci.pbSyms, 0, sci.cbSyms);
            memset(sci.rgOffMap, 0, sci.cSyms * sizeof OffMap);
            if (pwti->fConvertSymbolBlock(sci, pbSymSrc, cbSymSrc)) {
                // all cool, set up the ALM.
                pgst->lpalm = BuildALM(
                    FALSE,
                    0,
                    sci.pbSyms,
                    sci.cbSyms,
                    cbAlign);
            }
        }
    }

}
