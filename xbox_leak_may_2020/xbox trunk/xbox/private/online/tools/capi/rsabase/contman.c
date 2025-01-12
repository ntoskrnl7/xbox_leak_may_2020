/*++

Copyright (c) 1997, 1998, 1999  Microsoft Corporation

Module Name:

    keyman.cpp

Abstract:

    This module contains routines to read and write data (key containers) from and to files.


Author:

    16 Mar 98 jeffspel

--*/

#include <precomp.h>
//#include <userenv.h>
#include <wincrypt.h>
#include <xwinreg.h>
#include <rpc.h>
//#include <shlobj.h>
#include "contman.h"
#include "msr_md5.h"
#include "des.h"
#include "modes.h"
#include "csprc.h"

#ifdef USE_HW_RNG
#ifdef _M_IX86

#include <winioctl.h>

// INTEL h files for on chip RNG
#include "deftypes.h"   //ISD typedefs and constants
#include "ioctldef.h"   //ISD ioctl definitions

#endif // _M_IX86
#endif // USE_HW_RNG

#if DBG         // NOTE:  This section not compiled for retail builds
DWORD   g_dwDebugCount = 0;
#endif // DBG

BYTE *g_pbStringBlock = NULL;
CSP_STRINGS g_Strings = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL};

typedef struct _OLD_KEY_CONTAINER_LENS_ {
    DWORD                   cbSigPub;
    DWORD                   cbSigEncPriv;
    DWORD                   cbExchPub;
    DWORD                   cbExchEncPriv;
} OLD_KEY_CONTAINER_LENS, *POLD_KEY_CONTAINER_LENS;

#define OLD_KEY_CONTAINER_FILE_FORMAT_VER   1

#define ContInfoAlloc(cb)       LocalAlloc(LMEM_ZEROINIT, cb)
#define ContInfoReAlloc(pb, cb) LocalReAlloc(pb, cb, LMEM_ZEROINIT | LMEM_MOVEABLE)
#define ContInfoFree(pb)        LocalFree(pb)

#define MACHINE_KEYS_DIR    L"MachineKeys"

// Location of the keys in the registry (minus the logon name)
// Length of the full location (including the logon name)
#define RSA_REG_KEY_LOC         "Software\\Microsoft\\Cryptography\\UserKeys"
#define RSA_REG_KEY_LOC_LEN     sizeof(RSA_REG_KEY_LOC)
#define RSA_MACH_REG_KEY_LOC        "Software\\Microsoft\\Cryptography\\MachineKeys"
#define RSA_MACH_REG_KEY_LOC_LEN    sizeof(RSA_MACH_REG_KEY_LOC)

#define DSS_REG_KEY_LOC		    "Software\\Microsoft\\Cryptography\\DSSUserKeys"
#define DSS_REG_KEY_LOC_LEN	    sizeof(DSS_REG_KEY_LOC)
#define DSS_MACH_REG_KEY_LOC        "Software\\Microsoft\\Cryptography\\DSSUserKeys"
#define DSS_MACH_REG_KEY_LOC_LEN    sizeof(DSS_MACH_REG_KEY_LOC)

#define MAX_DPAPI_RETRY_COUNT   5

BOOL
MyCryptProtectData(
    IN              DATA_BLOB*      pDataIn,
    IN              LPCWSTR         szDataDescr,
    IN OPTIONAL     DATA_BLOB*      pOptionalEntropy,
    IN              PVOID           pvReserved,
    IN OPTIONAL     CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
    IN              DWORD           dwFlags,
    OUT             DATA_BLOB*      pDataOut            // out encr blob
    )
{
    pDataOut->cbData = pDataIn->cbData;
    RtlCopyMemory( pDataOut->pbData, pDataIn->pbData, pDataIn->cbData );

    ASSERT( !"MyCryptProtectData" );
    return TRUE;
}

BOOL
MyCryptUnprotectData(
    IN              DATA_BLOB*      pDataIn,             // in encr blob
    OUT OPTIONAL    LPWSTR*         ppszDataDescr,       // out
    IN OPTIONAL     DATA_BLOB*      pOptionalEntropy,
    IN              PVOID           pvReserved,
    IN OPTIONAL     CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
    IN              DWORD           dwFlags,
    OUT             DATA_BLOB*      pDataOut
    )
{
    pDataOut->cbData = pDataIn->cbData;
    RtlCopyMemory( pDataOut->pbData, pDataIn->pbData, pDataIn->cbData );

    ASSERT( !"MyCryptUnprotectData" );
    return TRUE;
}

void FreeEnumOldMachKeyEntries(
                               PKEY_CONTAINER_INFO pInfo
                               )
{
    if (pInfo)
    {
        if (pInfo->pchEnumOldMachKeyEntries)
        {
            ContInfoFree(pInfo->pchEnumOldMachKeyEntries);
            pInfo->dwiOldMachKeyEntry = 0;
            pInfo->cMaxOldMachKeyEntry = 0;
            pInfo->cbOldMachKeyEntry = 0;
            pInfo->pchEnumOldMachKeyEntries = NULL;
        }
    }
}

void FreeEnumRegEntries(
                       PKEY_CONTAINER_INFO pInfo
                       )
{
    if (pInfo)
    {
        if (pInfo->pchEnumRegEntries)
        {
            ContInfoFree(pInfo->pchEnumRegEntries);
            pInfo->dwiRegEntry = 0;
            pInfo->cMaxRegEntry = 0;
            pInfo->cbRegEntry = 0;
            pInfo->pchEnumRegEntries = NULL;
        }
    }
}

void FreeContainerInfo(
                       PKEY_CONTAINER_INFO pInfo
                       )
{
    if (pInfo)
    {
        if (pInfo->pbSigPub)
        {
            ContInfoFree(pInfo->pbSigPub);
            pInfo->ContLens.cbSigPub = 0;
            pInfo->pbSigPub = NULL;
        }
        if (pInfo->pbSigEncPriv)
        {
            ZeroMemory(pInfo->pbSigEncPriv, pInfo->ContLens.cbSigEncPriv);
            ContInfoFree(pInfo->pbSigEncPriv);
            pInfo->ContLens.cbSigEncPriv = 0;
            pInfo->pbSigEncPriv = NULL;
        }
        if (pInfo->pbExchPub)
        {
            ContInfoFree(pInfo->pbExchPub);
            pInfo->ContLens.cbExchPub = 0;
            pInfo->pbExchPub = NULL;
        }
        if (pInfo->pbExchEncPriv)
        {
            ZeroMemory(pInfo->pbExchEncPriv, pInfo->ContLens.cbExchEncPriv);
            ContInfoFree(pInfo->pbExchEncPriv);
            pInfo->ContLens.cbExchEncPriv = 0;
            pInfo->pbExchEncPriv = NULL;
        }
        if (pInfo->pbRandom)
        {
            ContInfoFree(pInfo->pbRandom);
            pInfo->ContLens.cbRandom = 0;
            pInfo->pbRandom = NULL;
        }
        if (pInfo->pszUserName)
        {
            ContInfoFree(pInfo->pszUserName);
            pInfo->ContLens.cbName = 0;
            pInfo->pszUserName = NULL;
        }

        FreeEnumOldMachKeyEntries(pInfo);

        FreeEnumRegEntries(pInfo);

        if (pInfo->hFind)
            FindClose(pInfo->hFind);
    }
}

DWORD GetHashOfContainer(
                         LPCSTR pszContainer,
                         LPWSTR pszHash
                         )
{
    MD5_CTX     MD5;
    LPSTR       pszLowerContainer = NULL;
    DWORD       *pdw1;
    DWORD       *pdw2;
    DWORD       *pdw3;
    DWORD       *pdw4;
    DWORD       dwErr = 0;

    if (NULL == (pszLowerContainer =
        (LPSTR)ContInfoAlloc(strlen(pszContainer) + sizeof(CHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    lstrcpy(pszLowerContainer, pszContainer);
    _strlwr(pszLowerContainer);

    MD5Init(&MD5);
    MD5Update(&MD5, pszLowerContainer, strlen(pszLowerContainer) + sizeof(CHAR));
    MD5Final(&MD5);

    pdw1 = (DWORD*)&MD5.digest[0];
    pdw2 = (DWORD*)&MD5.digest[4];
    pdw3 = (DWORD*)&MD5.digest[8];
    pdw4 = (DWORD*)&MD5.digest[12];
    wsprintfW(pszHash, L"%08hx%08hx%08hx%08hx", *pdw1, *pdw2, *pdw3, *pdw4);
Ret:
    if (pszLowerContainer)
        ContInfoFree(pszLowerContainer);
    return dwErr;
}

DWORD GetMachineGUID(
                     LPWSTR *ppwszUuid
                     )
{
#ifndef _XBOX
    HKEY    hRegKey = 0;
    UUID    Uuid;
    LPSTR   pszUuid = NULL;
    DWORD   cbUuid = sizeof(Uuid);
    DWORD   cch = 0;
    DWORD   dwErr = 0;

    // read the GUID from the Local Machine portion of the registry
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZLOCALMACHINECRYPTO,
                                      0, KEY_READ, &hRegKey))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
    if (ERROR_SUCCESS != RegQueryValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                                         0, NULL, NULL,
                                         &cbUuid))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
    if (NULL == (pszUuid = (LPSTR)ContInfoAlloc(cbUuid)))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    if (ERROR_SUCCESS != RegQueryValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                                         0, NULL, pszUuid,
                                         &cbUuid))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }

    // convert from ansi to unicode
    if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                        pszUuid,
                                        -1, NULL, cch)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if (NULL == (*ppwszUuid = ContInfoAlloc((cch + 1) * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                        pszUuid,
                                        -1, *ppwszUuid, cch)))
    {
         dwErr = GetLastError();
         goto Ret;
    }
Ret:
    if (pszUuid)
        ContInfoFree(pszUuid);

    if (hRegKey)
        RegCloseKey(hRegKey);

    return dwErr;
#else
    DWORD   dwErr = 0;

    if (NULL == (*ppwszUuid = ContInfoAlloc((2) * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    (*ppwszUuid)[0] = 'X';
    (*ppwszUuid)[1] = '\0';
    
    return dwErr;
#endif
}

DWORD SetMachineGUID()
{
#ifndef _XBOX
    HKEY    hRegKey = 0;
    UUID    Uuid;
    LPSTR   pszUuid = NULL;
    DWORD   cbUuid;
    HANDLE  hRPCRT4Dll = 0;
    FARPROC pUuidCreate = NULL;
    FARPROC pUuidToStringA = NULL;
    FARPROC pRpcStringFreeA = NULL;
    LPWSTR  pwszOldUuid = NULL;
    DWORD   dwResult;
    DWORD   dwErr = 0;

    if (0 == GetMachineGUID(&pwszOldUuid))
    {
        goto Ret;
    }

    if (NULL == (hRPCRT4Dll = LoadLibraryA("rpcrt4.dll")))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }

    if (NULL == (pUuidCreate = GetProcAddress(hRPCRT4Dll, "UuidCreate")))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
    if (NULL == (pUuidToStringA = GetProcAddress(hRPCRT4Dll, "UuidToStringA")))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
    if (NULL == (pRpcStringFreeA = GetProcAddress(hRPCRT4Dll, "RpcStringFreeA")))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }

    // read the GUID from the Local Machine portion of the registry
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                        SZLOCALMACHINECRYPTO,
                                        0, NULL, REG_OPTION_NON_VOLATILE,
                                        KEY_WRITE, NULL, &hRegKey,
                                        &dwResult))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }

    if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                                         0, NULL, NULL,
                                         &cbUuid))
    {
        goto Ret;
    }

    pUuidCreate(&Uuid);

    if (RPC_S_OK != pUuidToStringA(&Uuid, &pszUuid))
    {
        dwErr = (DWORD)NTE_FAIL;
    }

    if (ERROR_SUCCESS != RegSetValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                                       0, REG_SZ, (BYTE*)pszUuid,
                                       strlen(pszUuid) + 1))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
Ret:
    if (pRpcStringFreeA && pszUuid)
        pRpcStringFreeA(&pszUuid); 
    if (hRPCRT4Dll)
        FreeLibrary(hRPCRT4Dll);
    if (pwszOldUuid)
        ContInfoFree(pwszOldUuid);
    if (hRegKey)
        RegCloseKey(hRegKey);
    return dwErr;
#else
    return 0;
#endif
}

DWORD AddMachineGuidToContainerName(
                                    LPSTR pszContainer,
                                    LPWSTR pwszNewContainer
                                    )
{
    WCHAR   rgwszHash[33];
    LPWSTR  pwszUuid = NULL;
    DWORD   dwErr = 0;

    memset(rgwszHash, 0, sizeof(rgwszHash));

    // get the stringized hash of the container name
    if (0 != (dwErr = GetHashOfContainer(pszContainer, rgwszHash)))
        goto Ret;

    // get the GUID of the machine
    if (0 != (dwErr = GetMachineGUID(&pwszUuid)))
        goto Ret;

    wcscpy(pwszNewContainer, rgwszHash);
    wcscat(pwszNewContainer, L"_");
    wcscat(pwszNewContainer, pwszUuid);
Ret:
    if (pwszUuid)
        ContInfoFree(pwszUuid); 
 
    return dwErr;
}

//
//    Just tries to use DPAPI to make sure it works before creating a key
//    container.
//
DWORD TryDPAPI()
{
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CRYPT_DATA_BLOB             ExtraEntropy;
    DWORD                       dwJunk = 0;
    DWORD                       dwErr = 0;

    memset(&PromptStruct, 0, sizeof(PromptStruct));
    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));

    PromptStruct.cbSize = sizeof(PromptStruct);

    DataIn.cbData = sizeof(DWORD);
    DataIn.pbData = (BYTE*)&dwJunk;
    ExtraEntropy.cbData = sizeof(STUFF_TO_GO_INTO_MIX);
    ExtraEntropy.pbData = STUFF_TO_GO_INTO_MIX;
    if (!MyCryptProtectData(&DataIn, L"Export Flag", &ExtraEntropy, NULL,
                            &PromptStruct, 0, &DataOut))
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }
Ret:
    if (DataOut.pbData)
        LocalFree(DataOut.pbData);
    return dwErr;
}

DWORD ProtectExportabilityFlag(
                               IN BOOL fExportable,
                               IN BOOL fMachineKeyset,
                               OUT BYTE **ppbProtectedExportability,
                               OUT DWORD *pcbProtectedExportability
                               )
{
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CRYPT_DATA_BLOB             ExtraEntropy;
    DWORD                       dwProtectFlags = 0;
    DWORD                       dwErr = 0;

    memset(&PromptStruct, 0, sizeof(PromptStruct));
    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));

    if (fMachineKeyset)
        dwProtectFlags = CRYPTPROTECT_LOCAL_MACHINE;

    PromptStruct.cbSize = sizeof(PromptStruct);

    DataIn.cbData = sizeof(BOOL);
    DataIn.pbData = (BYTE*)&fExportable;
    ExtraEntropy.cbData = sizeof(STUFF_TO_GO_INTO_MIX);
    ExtraEntropy.pbData = STUFF_TO_GO_INTO_MIX;
    if (!MyCryptProtectData(&DataIn, L"Export Flag", &ExtraEntropy, NULL,
                            &PromptStruct, dwProtectFlags, &DataOut))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
    if (NULL == (*ppbProtectedExportability = ContInfoAlloc(DataOut.cbData)))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    *pcbProtectedExportability = DataOut.cbData;
    memcpy(*ppbProtectedExportability, DataOut.pbData, DataOut.cbData);
Ret:
    if (DataOut.pbData)
        LocalFree(DataOut.pbData);
    return dwErr;
}

DWORD UnprotectExportabilityFlag(
                                 IN BOOL fMachineKeyset,
                                 IN BYTE *pbProtectedExportability,
                                 IN DWORD cbProtectedExportability,
                                 IN BOOL *pfExportable
                                 )
{
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CRYPT_DATA_BLOB             ExtraEntropy;
    DWORD                       dwProtectFlags = 0;
    DWORD                       dwErr = 0;

    memset(&PromptStruct, 0, sizeof(PromptStruct));
    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));
    memset(&ExtraEntropy, 0, sizeof(ExtraEntropy));

    if (fMachineKeyset)
        dwProtectFlags = CRYPTPROTECT_LOCAL_MACHINE;

    PromptStruct.cbSize = sizeof(PromptStruct);

    DataIn.cbData = cbProtectedExportability;
    DataIn.pbData = pbProtectedExportability;
    ExtraEntropy.cbData = sizeof(STUFF_TO_GO_INTO_MIX);
    ExtraEntropy.pbData = STUFF_TO_GO_INTO_MIX;
    if (!MyCryptUnprotectData(&DataIn, NULL, &ExtraEntropy, NULL,
                              &PromptStruct, dwProtectFlags, &DataOut))
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }
    if (sizeof(BOOL) != DataOut.cbData)
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }
    *pfExportable = *((BOOL*)DataOut.pbData);
Ret:
    // free the DataOut struct if necessary
    if (DataOut.pbData)
        LocalFree(DataOut.pbData);

    return dwErr;
}



BOOL
IsLocalSystem(
              BOOL *pfIsLocalSystem
              )
/*++

    This function determines if the user associated with the
    specified token is the Local System account.

--*/
{
    *pfIsLocalSystem = TRUE;
    
    return TRUE;
}

BOOL
IsThreadLocalSystem(
                    BOOL *pfIsLocalSystem
                    )
/*++

    This function determines if the user associated with the
    specified token is the Local System account.

--*/
{
    *pfIsLocalSystem = TRUE;
    
    return TRUE;
}

#define FAST_BUF_SIZE 256

BOOL
GetUserTextualSidA(
    LPSTR lpBuffer,
    LPDWORD nSize
    )
{
    if (*nSize < 2)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    lpBuffer[0] = '1';
    lpBuffer[1] = '\0';
    *nSize = 2;
    
    return TRUE;
}

BOOL
GetUserTextualSidW(
    LPWSTR lpBuffer,
    LPDWORD nSize
    )
{
    if (*nSize < 2)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    lpBuffer[0] = '1';
    lpBuffer[1] = '\0';
    *nSize = 2;

    return TRUE;
}

DWORD GetUserDirectory(
                       IN BOOL fMachineKeyset,
                       OUT LPWSTR pwszUser,
                       OUT DWORD *pcbUser
                       )
{
    DWORD   dwErr = 0;

    if (fMachineKeyset)
    {
        wcscpy(pwszUser, MACHINE_KEYS_DIR);
        *pcbUser = wcslen(pwszUser) + 1;
    }
    else
    {
        if (FIsWinNT())
        {
            if (!GetUserTextualSidW(pwszUser, pcbUser))
            {
                SetLastError((DWORD) NTE_BAD_KEYSET);
                goto Ret;
            }
        }
        else
        {
            dwErr = (DWORD)NTE_FAIL;
            goto Ret;
        }
    }
Ret:
    return dwErr;
}

#define WSZRSAPRODUCTSTRING  L"\\Microsoft\\Crypto\\RSA\\"
#define WSZDSSPRODUCTSTRING  L"\\Microsoft\\Crypto\\DSS\\"
#define PRODUCTSTRINGLEN    sizeof(WSZRSAPRODUCTSTRING) - sizeof(WCHAR)

DWORD
GetUserStorageArea(
    IN      DWORD dwProvType,
    IN      BOOL fMachineKeyset,
    IN      BOOL fOldWin2KMachineKeyPath,
    OUT     BOOL *pfIsLocalSystem,      // used if fMachineKeyset is FALSE, in this
                                        // case TRUE is returned if running as Local System
    IN  OUT LPWSTR *ppwszUserStorageArea
    )
{
    DWORD dwLastError = 0;

    *ppwszUserStorageArea = (LPWSTR)ContInfoAlloc(
                                    10 * sizeof(WCHAR)   // trailing slash and NULL
                                    );
    if (*ppwszUserStorageArea) {
        wcscpy(*ppwszUserStorageArea, L"T:\\Keys\\");
    }
    return dwLastError;
}

DWORD GetFilePath(
    IN      LPCWSTR  pwszUserStorageArea,
    IN      LPCWSTR  pwszFileName,
    IN OUT  LPWSTR   *ppwszFilePath
    )
{
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD dwLastError = ERROR_SUCCESS;

    cbUserStorageArea = wcslen( pwszUserStorageArea ) * sizeof(WCHAR);

    cbFileName = wcslen( pwszFileName ) * sizeof(WCHAR);

    *ppwszFilePath = (LPWSTR)ContInfoAlloc( cbUserStorageArea + cbFileName + sizeof(WCHAR) );

    if( *ppwszFilePath == NULL )
    {
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    CopyMemory(*ppwszFilePath, pwszUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)*ppwszFilePath+cbUserStorageArea, pwszFileName, cbFileName + sizeof(WCHAR));
Ret:
    return dwLastError;
}

static DWORD rgdwCreateFileRetryMilliseconds[] =
    { 1, 10, 100, 500, 1000, 5000 };

#define MAX_CREATE_FILE_RETRY_COUNT     \
            (sizeof(rgdwCreateFileRetryMilliseconds) / \
                sizeof(rgdwCreateFileRetryMilliseconds[0]))

HANDLE MyCreateFile(
  IN BOOL fMachineKeyset,         // indicates if this is a machine keyset
  IN LPCWSTR wszFilePath,          // pointer to name of the file
  IN DWORD dwDesiredAccess,       // access (read-write) mode
  IN DWORD dwShareMode,           // share mode
  IN DWORD dwCreationDisposition,  // how to create
  IN DWORD dwAttribs  // file attributes
)
{
    HANDLE          hFile = INVALID_HANDLE_VALUE;
    LPSTR           szFilePath = NULL;
    DWORD           dwFilePath;

    dwFilePath = wcslen(wszFilePath)*2 + 2;
    szFilePath = _alloca( dwFilePath );
    if (0 == (dwFilePath = WideCharToMultiByte(CP_ACP, 0,
                                        wszFilePath,
                                        -1,
                                        szFilePath,
                                        dwFilePath,
                                        NULL,
                                        NULL)))
    {
        goto Ret;
    }

    hFile = CreateFileA(
                szFilePath,
                dwDesiredAccess,
                dwShareMode,
                NULL,
                dwCreationDisposition,
                dwAttribs,
                NULL
                );

Ret:

    return hFile;
}

DWORD OpenFileInStorageArea(
    IN      BOOL    fMachineKeyset,
    IN      DWORD   dwDesiredAccess,
    IN      LPCWSTR wszUserStorageArea,
    IN      LPCWSTR wszFileName,
    IN OUT  HANDLE  *phFile
    )
{
    LPWSTR wszFilePath = NULL;
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD dwShareMode = 0;
    DWORD dwCreationDistribution = OPEN_EXISTING;
    DWORD dwRetryCount;
    DWORD dwAttribs = 0;
    DWORD dwLastError = ERROR_SUCCESS;

    *phFile = INVALID_HANDLE_VALUE;

    if( dwDesiredAccess & GENERIC_READ ) {
        dwShareMode |= FILE_SHARE_READ;
        dwCreationDistribution = OPEN_EXISTING;
    }

    if( dwDesiredAccess & GENERIC_WRITE ) {
        dwShareMode = 0;
        dwCreationDistribution = OPEN_ALWAYS;
        dwAttribs = FILE_ATTRIBUTE_SYSTEM;
    }

    if (0 != (dwLastError = GetFilePath(wszUserStorageArea, wszFileName,
                                        &wszFilePath)))
    {
        goto Ret;
    }

    dwRetryCount = 0;
    while (1)
    {
        *phFile = MyCreateFile(fMachineKeyset,
                               wszFilePath,
                               dwDesiredAccess,
                               dwShareMode,
                               dwCreationDistribution,
                               dwAttribs | FILE_FLAG_SEQUENTIAL_SCAN
                               );

        if( *phFile == INVALID_HANDLE_VALUE )
        {
            dwLastError = GetLastError();
            if (((ERROR_SHARING_VIOLATION == dwLastError) ||
                 (ERROR_ACCESS_DENIED == dwLastError)) &&
                (MAX_CREATE_FILE_RETRY_COUNT > dwRetryCount))
            {
                Sleep(rgdwCreateFileRetryMilliseconds[dwRetryCount]);
                dwRetryCount++;
            }
            else
            {
                goto Ret;
            }
        }
        else
        {
            break;
        }
    }
Ret:
    if(wszFilePath)
        ContInfoFree(wszFilePath);

    return dwLastError;
}

DWORD FindClosestFileInStorageArea(
    IN      LPCWSTR  pwszUserStorageArea,
    IN      LPCSTR   pszContainer,
    OUT     LPWSTR   pwszNewFileName,
    IN OUT  HANDLE  *phFile
    )
{
#ifndef _XBOX
    LPWSTR pwszFilePath = NULL;
    WCHAR  rgwszNewFileName[35];
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD dwShareMode = 0;
    DWORD dwCreationDistribution = OPEN_EXISTING;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindData;
    DWORD dwLastError = ERROR_SUCCESS;

    memset(&FindData, 0, sizeof(FindData));
    memset(rgwszNewFileName, 0, sizeof(rgwszNewFileName));

    *phFile = INVALID_HANDLE_VALUE;

    dwShareMode |= FILE_SHARE_READ;
    dwCreationDistribution = OPEN_EXISTING;

    // get the stringized hash of the container name
    if (0 != (dwLastError = GetHashOfContainer(pszContainer, rgwszNewFileName)))
    {
        goto Ret;
    }

    // ContInfoAlloc zeros memory so no need to set NULL terminator
    rgwszNewFileName[32] = '_';
    rgwszNewFileName[33] = '*';

    if (0 != (dwLastError = GetFilePath(pwszUserStorageArea, rgwszNewFileName,
                                        &pwszFilePath)))
    {
        goto Ret;
    }

    hFind = FindFirstFileExW(
                pwszFilePath,
                FindExInfoStandard,
                &FindData,
                FindExSearchNameMatch,
                NULL, 
                0
                );
    
    if( hFind == INVALID_HANDLE_VALUE )
    {
        dwLastError = NTE_BAD_KEYSET;
        goto Ret;
    }

    ContInfoFree(pwszFilePath);
    pwszFilePath = NULL;

    if (0 != (dwLastError = GetFilePath(pwszUserStorageArea, FindData.cFileName,
                                        &pwszFilePath)))
    {
        goto Ret;
    }

    *phFile = CreateFileW(
                pwszFilePath,
                GENERIC_READ,
                dwShareMode,
                NULL,
                dwCreationDistribution,
                FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );

    if( *phFile == INVALID_HANDLE_VALUE )
    {
        dwLastError = NTE_BAD_KEYSET;
        goto Ret;
    }

    // allocate and copy in the real file name to be returned
    wcscpy(pwszNewFileName, FindData.cFileName);
Ret:
    if (hFind)
        FindClose(hFind);
    if(pwszFilePath)
        ContInfoFree(pwszFilePath);

    return dwLastError;
#else
    return NTE_BAD_KEYSET;
#endif
}

//
//  This function gets the determines if the user associated with the
//  specified token is the Local System account.
//
DWORD ZeroizeFile(
    IN      LPCWSTR  wszFilePath
    )
{
#ifndef _XBOX
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    BYTE    *pb = NULL;
    DWORD   cb;
    DWORD   dwBytesWritten = 0;
    DWORD   dwErr = ERROR_SUCCESS;

    if (INVALID_HANDLE_VALUE == (hFile = CreateFileW(wszFilePath,
                                                     GENERIC_WRITE,
                                                     0,
                                                     NULL,
                                                     OPEN_EXISTING,
                                                     FILE_ATTRIBUTE_SYSTEM,
                                                     NULL)))
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }

    if (0xFFFFFFFF == (cb = GetFileSize(hFile, NULL)))
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }

    if (NULL == (pb = ContInfoAlloc(cb)))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    if (!WriteFile(hFile, pb, cb, &dwBytesWritten, NULL))
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }
    if (cb != dwBytesWritten)
    {
        dwErr = (DWORD)NTE_FAIL;
    }
Ret:
    if (pb)
        ContInfoFree(pb);

    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    return dwErr;
#else
    return 0;
#endif
}

DWORD DeleteFileInStorageArea(
    IN      LPCWSTR  wszUserStorageArea,
    IN      LPCWSTR  wszFileName
    )
{
    LPWSTR wszFilePath = NULL;
    LPSTR szFilePath = NULL;
    DWORD dwFilePath;
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD dwErr = ERROR_SUCCESS;

    cbUserStorageArea = wcslen( wszUserStorageArea ) * sizeof(WCHAR);
    cbFileName = wcslen( wszFileName ) * sizeof(WCHAR);

    wszFilePath = (LPWSTR)ContInfoAlloc( (cbUserStorageArea + cbFileName + 1) * sizeof(WCHAR) );

    if( wszFilePath == NULL )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    CopyMemory(wszFilePath, wszUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)wszFilePath + cbUserStorageArea, wszFileName,
               cbFileName + sizeof(WCHAR));

    // write a file of the same size with all zeros first
    if (ERROR_SUCCESS != (dwErr = ZeroizeFile(wszFilePath)))
        goto Ret;

    dwFilePath = cbUserStorageArea + cbFileName + 2;
    szFilePath = _alloca( dwFilePath );
    if (0 == (dwFilePath = WideCharToMultiByte(CP_ACP, 0,
                                        wszFilePath,
                                        -1,
                                        szFilePath,
                                        dwFilePath,
                                        NULL,
                                        NULL)))
    {
        dwErr = GetLastError();
        goto Ret;
    }
    
    if (!DeleteFileA(szFilePath))
    {
        dwErr = GetLastError();
        SetLastError((DWORD)NTE_BAD_KEYSET);
    }
Ret:
    if(wszFilePath)
        ContInfoFree(wszFilePath);
    return dwErr;
}


DWORD SetContainerUserName(
                           IN LPSTR pszUserName,
                           IN PKEY_CONTAINER_INFO pContInfo
                           )
{
    DWORD   dwErr = 0;

    if (NULL == (pContInfo->pszUserName =
        (LPSTR)ContInfoAlloc((strlen(pszUserName) + 1) * sizeof(CHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    strcpy(pContInfo->pszUserName, pszUserName);
Ret:
    return dwErr;
}

DWORD NT5ToNT5Migration(
                        IN DWORD dwProvType,
                        IN BOOL fMachineKeyset,
                        IN LPSTR pszContainer,
                        IN LPWSTR pwszFilePath,
                        IN DWORD dwFlags,
                        OUT PKEY_CONTAINER_INFO pContInfo,
                        OUT HANDLE *phFile
                        )
{
#ifndef _XBOX
    LPWSTR  pwszOldFilePath = NULL;
    LPWSTR  pwszOldFullFilePath = NULL;
    LPWSTR  pwszNewFullFilePath = NULL;
    LPWSTR  pwszFileName = NULL;
    BOOL    fAllocedFileName = FALSE;
    BOOL    fIsLocalSystem;
    BOOL    fRetryWithHashedName = TRUE;
    DWORD   cch = 0;
    DWORD   dwErr = 0;

    // get the correct storage area (directory)
    if (0 != (dwErr = GetUserStorageArea(dwProvType,
                                         fMachineKeyset,
                                         TRUE,
                                         &fIsLocalSystem,
                                         &pwszOldFilePath)))
    {
        goto Ret;
    }

    // check if the length of the container name is the length of a new unique container,
    // then try with the container name which was passed in, if this fails
    // then try with the container name with the machine GUID appended
    if (69 == strlen(pszContainer))
    {
        // convert to UNICODE pszContainer -> pwszFileName
        if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                            pszContainer,
                                            -1, NULL, cch)))
        {
            dwErr = GetLastError();
            goto Ret;
        }

        if (NULL == (pwszFileName = ContInfoAlloc((cch + 1) * sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        fAllocedFileName = TRUE;

        if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                            pszContainer,
                                            -1, pwszFileName, cch)))
        {
             dwErr = GetLastError();
             goto Ret;
        }

        if (ERROR_SUCCESS != OpenFileInStorageArea(fMachineKeyset,
                                                   GENERIC_READ,
                                                   pwszOldFilePath,
                                                   pwszFileName,
                                                   phFile))
        {
            ContInfoFree(pwszFileName);
            pwszFileName = NULL;
            fAllocedFileName = FALSE;
            fRetryWithHashedName = TRUE;
        }
        else
        {
            fRetryWithHashedName = FALSE;
        }
    }

    if (fRetryWithHashedName)
    {
        pwszFileName = pContInfo->rgwszFileName;
        if(ERROR_SUCCESS != (dwErr = OpenFileInStorageArea(fMachineKeyset,
                                                           GENERIC_READ,
                                                           pwszOldFilePath,
                                                           pwszFileName,
                                                           phFile)))
        {
            if ((ERROR_ACCESS_DENIED == dwErr) && (dwFlags & CRYPT_NEWKEYSET))
            {
                dwErr = (DWORD)NTE_EXISTS;
            }

            goto Ret;
        }
    }

    // found an old file so move it to the new directory
    CloseHandle(*phFile);
    *phFile = INVALID_HANDLE_VALUE;

    if (0 != (dwErr = GetFilePath(pwszOldFilePath, pwszFileName, &pwszOldFullFilePath)))
    {
        goto Ret;
    }
    if (0 != (dwErr = GetFilePath(pwszFilePath, pwszFileName, &pwszNewFullFilePath)))
    {
        goto Ret;
    }

    if (!MoveFileW(pwszOldFullFilePath, pwszNewFullFilePath))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if(ERROR_SUCCESS != (dwErr = OpenFileInStorageArea(fMachineKeyset,
                                              GENERIC_READ,
                                              pwszFilePath,
                                              pwszFileName,
                                              phFile)))
    {
        goto Ret;
    }
Ret:
    if (fAllocedFileName && (NULL != pwszFileName))
    {
        ContInfoFree(pwszFileName);
    }
    if (pwszOldFullFilePath)
        ContInfoFree(pwszOldFullFilePath);
    if (pwszNewFullFilePath)
        ContInfoFree(pwszNewFullFilePath);
    if (pwszOldFilePath)
        ContInfoFree(pwszOldFilePath);

    return dwErr;
#else
    return 0;
#endif
}

DWORD ReadContainerInfo(
                        IN DWORD dwProvType,
                        IN LPSTR pszContainerName,
                        IN BOOL fMachineKeyset,
                        IN DWORD dwFlags,
                        OUT PKEY_CONTAINER_INFO pContInfo
                        )
{
    HANDLE                  hMap = NULL;
    BYTE                    *pbFile = NULL;
    DWORD                   cbFile;
    DWORD                   cb;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    KEY_EXPORTABILITY_LENS  Exportability;
    LPWSTR                  pwszFileName = NULL;
    LPWSTR                  pwszFilePath = NULL;
    WCHAR                   rgwszOtherMachineFileName[84];
    DWORD                   cbFileName;
    BOOL                    fGetUserNameFromFile = FALSE;
    BOOL                    fIsLocalSystem = FALSE;
    BOOL                    fRetryWithHashedName = TRUE;
    DWORD                   cch = 0;
    DWORD                   dwErrOpen = 0;
    DWORD                   dwErr = 0;

    memset(&Exportability, 0, sizeof(Exportability));

    // get the correct storage area (directory)
    if (0 != (dwErr = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                                         &fIsLocalSystem, &pwszFilePath)))
    {
        goto Ret;
    }

    // check if the length of the container name is the length of a new unique container,
    // then try with the container name which was passed in, if this fails
    // then try with the container name with the machine GUID appended
    if (69 == strlen(pszContainerName))
    {
        // convert to UNICODE pszContainerName -> pwszFileName
        if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                            pszContainerName,
                                            -1, NULL, cch)))
        {
            dwErr = GetLastError();
            goto Ret;
        }

        if (NULL == (pwszFileName = ContInfoAlloc((cch + 1) * sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }

        if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                            pszContainerName,
                                            -1, pwszFileName, cch)))
        {
             dwErr = GetLastError();
             goto Ret;
        }

        if (ERROR_SUCCESS == OpenFileInStorageArea(fMachineKeyset,
                                                   GENERIC_READ,
                                                   pwszFilePath,
                                                   pwszFileName,
                                                   &hFile))
        {
            wcscpy(pContInfo->rgwszFileName, pwszFileName);

            // set the flag so the name of the key container will be retrieved from the file
            fGetUserNameFromFile = TRUE;
            fRetryWithHashedName = FALSE;
        }
    }

    if (fRetryWithHashedName)
    {
        if (0 != (dwErr = AddMachineGuidToContainerName(pszContainerName,
                                                        pContInfo->rgwszFileName)))
        {
            goto Ret;
        }
        if(ERROR_SUCCESS != (dwErrOpen = OpenFileInStorageArea(fMachineKeyset, GENERIC_READ,
                                                  pwszFilePath,
                                                  pContInfo->rgwszFileName,
                                                  &hFile)))
        {
            if ((ERROR_ACCESS_DENIED == dwErrOpen) && (dwFlags & CRYPT_NEWKEYSET))
            {
                dwErr = (DWORD)NTE_EXISTS;
                goto Ret;
            }

            if (fMachineKeyset || fIsLocalSystem)
            {
                if (0 != (dwErr = NT5ToNT5Migration(dwProvType,
                                                    fMachineKeyset,
                                                    pszContainerName,
                                                    pwszFilePath,
                                                    dwFlags,
                                                    pContInfo,
                                                    &hFile)))
                {
                    if (ERROR_ACCESS_DENIED == dwErr)
                    {
                        dwErr = (DWORD)NTE_EXISTS;
                    }
                    else
                    {
                        dwErr = (DWORD)NTE_BAD_KEYSET;
                    }
                    goto Ret;
                }
            }
            else
            {
                memset(rgwszOtherMachineFileName, 0, sizeof(rgwszOtherMachineFileName));
                // try to open any file from another machine with this container name
                if(ERROR_SUCCESS != FindClosestFileInStorageArea(pwszFilePath,
                                                             pszContainerName,
                                                             rgwszOtherMachineFileName,
                                                             &hFile))
                {
                    dwErr = (DWORD)NTE_BAD_KEYSET;
                    goto Ret;
                }
                else
                { 
                    wcscpy(pContInfo->rgwszFileName, rgwszOtherMachineFileName);
                }
            }
        }
    }

    if (dwFlags & CRYPT_NEWKEYSET)
    {
        dwErr = (DWORD)NTE_EXISTS;
        goto Ret;
    }

    if (0xFFFFFFFF == (cbFile = GetFileSize(hFile, NULL)))
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }
    if (sizeof(KEY_CONTAINER_LENS) > cbFile)
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }

    pbFile = _alloca( cbFile );
    
    if (!ReadFile(hFile,
                (LPVOID)pbFile,
                cbFile,
                &cbFile,
                NULL))
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }

    // get the length information out of the file
    memcpy(&pContInfo->dwVersion, pbFile, sizeof(DWORD));
    cb = sizeof(DWORD);
    if (KEY_CONTAINER_FILE_FORMAT_VER != pContInfo->dwVersion)
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }
    memcpy(&pContInfo->ContLens, pbFile + cb, sizeof(KEY_CONTAINER_LENS));
    cb += sizeof(KEY_CONTAINER_LENS);

    if (pContInfo->fCryptSilent && (0 != pContInfo->ContLens.dwUIOnKey))
    {
        dwErr = (DWORD)NTE_SILENT_CONTEXT;
        goto Ret;
    }

    // get the private key exportability stuff
    memcpy(&Exportability, pbFile + cb, sizeof(KEY_EXPORTABILITY_LENS)); 
    cb += sizeof(KEY_EXPORTABILITY_LENS);

    // get the user name
    if (NULL == (pContInfo->pszUserName =
        ContInfoAlloc(pContInfo->ContLens.cbName)))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    memcpy(pContInfo->pszUserName, pbFile + cb, pContInfo->ContLens.cbName);
    cb += pContInfo->ContLens.cbName;

    // get the random seed
    if (NULL == (pContInfo->pbRandom =
        ContInfoAlloc(pContInfo->ContLens.cbRandom)))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    memcpy(pContInfo->pbRandom, pbFile + cb, pContInfo->ContLens.cbRandom);
    cb += pContInfo->ContLens.cbRandom;

    // get the signature key info out of the file
    if (pContInfo->ContLens.cbSigPub && pContInfo->ContLens.cbSigEncPriv)
    {
        if (NULL == (pContInfo->pbSigPub =
            ContInfoAlloc(pContInfo->ContLens.cbSigPub)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        memcpy(pContInfo->pbSigPub, pbFile + cb, pContInfo->ContLens.cbSigPub);
        cb += pContInfo->ContLens.cbSigPub;

        if (NULL == (pContInfo->pbSigEncPriv =
            ContInfoAlloc(pContInfo->ContLens.cbSigEncPriv)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        memcpy(pContInfo->pbSigEncPriv, pbFile + cb,
               pContInfo->ContLens.cbSigEncPriv);
        cb += pContInfo->ContLens.cbSigEncPriv;

        // get the exportability info for the sig key
        if (0 != (dwErr = UnprotectExportabilityFlag(fMachineKeyset, pbFile + cb,
                                 Exportability.cbSigExportability,
                                 &pContInfo->fSigExportable)))
        {
            dwErr = (DWORD)NTE_BAD_KEYSET;
            goto Ret;
        }
        cb += Exportability.cbSigExportability;
    }

    // get the signature key info out of the file
    if (pContInfo->ContLens.cbExchPub && pContInfo->ContLens.cbExchEncPriv)
    {
        if (NULL == (pContInfo->pbExchPub =
            ContInfoAlloc(pContInfo->ContLens.cbExchPub)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        memcpy(pContInfo->pbExchPub, pbFile + cb,
               pContInfo->ContLens.cbExchPub);
        cb += pContInfo->ContLens.cbExchPub;

        if (NULL == (pContInfo->pbExchEncPriv =
            ContInfoAlloc(pContInfo->ContLens.cbExchEncPriv)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        memcpy(pContInfo->pbExchEncPriv, pbFile + cb,
               pContInfo->ContLens.cbExchEncPriv);
        cb += pContInfo->ContLens.cbExchEncPriv;

        // get the exportability info for the sig key
        if (0 != (dwErr = UnprotectExportabilityFlag(fMachineKeyset, pbFile + cb,
                                 Exportability.cbExchExportability,
                                 &pContInfo->fExchExportable)))
        {
            dwErr = (DWORD)NTE_BAD_KEYSET;
            goto Ret;
        }
        cb += Exportability.cbExchExportability;
    }
Ret:
    if (pwszFileName)
        ContInfoFree(pwszFileName);

    if (0 != dwErr)
    {
        if (pContInfo)
        {
            FreeContainerInfo(pContInfo);
        }
    }

    if (pwszFilePath)
        ContInfoFree(pwszFilePath);

    if(hMap)
        CloseHandle(hMap);

    if(INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    return dwErr;
}

DWORD WriteContainerInfo(
                         IN DWORD dwProvType,
                         IN LPWSTR pwszFileName,
                         IN BOOL fMachineKeyset,
                         IN PKEY_CONTAINER_INFO pContInfo
                         )
{
    BYTE                    *pbProtectedSigExportFlag = NULL;
    DWORD                   cbProtectedSigExportFlag;
    BYTE                    *pbProtectedExchExportFlag = NULL;
    DWORD                   cbProtectedExchExportFlag;
    KEY_EXPORTABILITY_LENS  ExportabilityLens;
    BYTE                    *pb = NULL;
    DWORD                   cb;
    LPWSTR                  pwszFilePath = NULL;
    HANDLE                  hFile = 0;
    DWORD                   dwBytesWritten;
    BOOL                    fIsLocalSystem = FALSE;
    DWORD                   dwErr = 0;

    memset(&ExportabilityLens, 0, sizeof(ExportabilityLens));

    // protect the signature exportability flag if necessary
    if (pContInfo->ContLens.cbSigPub && pContInfo->ContLens.cbSigEncPriv)
    {
        if (0 != (dwErr = ProtectExportabilityFlag(pContInfo->fSigExportable,
                                fMachineKeyset, &pbProtectedSigExportFlag,
                                &ExportabilityLens.cbSigExportability)))
        {
            goto Ret;
        }
    }
    
    // protect the key exchange exportability flag if necessary
    if (pContInfo->ContLens.cbExchPub && pContInfo->ContLens.cbExchEncPriv)
    {
        if (0 != (dwErr = ProtectExportabilityFlag(pContInfo->fExchExportable,
                                fMachineKeyset, &pbProtectedExchExportFlag,
                                &ExportabilityLens.cbExchExportability)))
        {
            goto Ret;
        }
    }

    pContInfo->ContLens.cbName = strlen(pContInfo->pszUserName) + sizeof(CHAR);
    
    // calculate the buffer length required for the container info
    cb = pContInfo->ContLens.cbSigPub + pContInfo->ContLens.cbSigEncPriv + 
         pContInfo->ContLens.cbExchPub + pContInfo->ContLens.cbExchEncPriv +
         ExportabilityLens.cbSigExportability +
         ExportabilityLens.cbExchExportability +
         pContInfo->ContLens.cbName +
         pContInfo->ContLens.cbRandom +
         sizeof(KEY_EXPORTABILITY_LENS) + sizeof(KEY_CONTAINER_INFO) +
         sizeof(DWORD);

    if (NULL == (pb = (BYTE*)ContInfoAlloc(cb)))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    // copy the length information
    pContInfo->dwVersion = KEY_CONTAINER_FILE_FORMAT_VER;
    memcpy(pb, &pContInfo->dwVersion, sizeof(DWORD));
    cb = sizeof(DWORD);
    memcpy(pb + cb, &pContInfo->ContLens, sizeof(KEY_CONTAINER_LENS));
    cb += sizeof(KEY_CONTAINER_LENS);
    if (KEY_CONTAINER_FILE_FORMAT_VER != pContInfo->dwVersion)
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }

    memcpy(pb + cb, &ExportabilityLens, sizeof(KEY_EXPORTABILITY_LENS)); 
    cb += sizeof(KEY_EXPORTABILITY_LENS);

    // copy the name of the container to the file
    memcpy(pb + cb, pContInfo->pszUserName, pContInfo->ContLens.cbName);
    cb += pContInfo->ContLens.cbName;

    // copy the random seed to the file
    memcpy(pb + cb, pContInfo->pbRandom, pContInfo->ContLens.cbRandom);
    cb += pContInfo->ContLens.cbRandom;

    // copy the signature key info to the file
    if (pContInfo->ContLens.cbSigPub || pContInfo->ContLens.cbSigEncPriv)
    {
        memcpy(pb + cb, pContInfo->pbSigPub, pContInfo->ContLens.cbSigPub);
        cb += pContInfo->ContLens.cbSigPub;

        memcpy(pb + cb, pContInfo->pbSigEncPriv,
               pContInfo->ContLens.cbSigEncPriv);
        cb += pContInfo->ContLens.cbSigEncPriv;

        // write the exportability info for the sig key
        memcpy(pb + cb, pbProtectedSigExportFlag,
               ExportabilityLens.cbSigExportability);
        cb += ExportabilityLens.cbSigExportability;
    }

    // get the signature key info out of the file
    if (pContInfo->ContLens.cbExchPub || pContInfo->ContLens.cbExchEncPriv)
    {
        memcpy(pb + cb, pContInfo->pbExchPub, pContInfo->ContLens.cbExchPub);
        cb += pContInfo->ContLens.cbExchPub;

        memcpy(pb + cb, pContInfo->pbExchEncPriv,
               pContInfo->ContLens.cbExchEncPriv);
        cb += pContInfo->ContLens.cbExchEncPriv;

        // write the exportability info for the sig key
        memcpy(pb + cb, pbProtectedExchExportFlag,
               ExportabilityLens.cbExchExportability);
        cb += ExportabilityLens.cbExchExportability;
    }

    // get the correct storage area (directory)
    if (0 != (dwErr = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                                         &fIsLocalSystem, &pwszFilePath)))
    {
        goto Ret;
    }

    // open the file to write the information to
    if(ERROR_SUCCESS != OpenFileInStorageArea(fMachineKeyset, GENERIC_WRITE,
                                              pwszFilePath, pwszFileName,
                                              &hFile))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }

    if (!WriteFile(hFile, pb, cb, &dwBytesWritten, NULL))
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
    if (cb != dwBytesWritten)
    {
        dwErr = (DWORD)NTE_FAIL;
        goto Ret;
    }
Ret:
    if (pwszFilePath)
        ContInfoFree(pwszFilePath);
    if (pbProtectedSigExportFlag)
        ContInfoFree(pbProtectedSigExportFlag);
    if (pbProtectedExchExportFlag)
        ContInfoFree(pbProtectedExchExportFlag);
    if (pb)
        ContInfoFree(pb);

    if (hFile)
        CloseHandle(hFile);
    return dwErr;
}

DWORD DeleteKeyContainer(
                         IN LPWSTR pwszFilePath,
                         IN LPSTR pszContainer
                         )
{
    LPWSTR  pwszFileName = NULL;
    WCHAR   rgwchNewFileName[80];
    BOOL    fRetryWithHashedName = TRUE;
    DWORD   cch = 0;
    DWORD   dwErr = 0;

    memset(rgwchNewFileName, 0, sizeof(rgwchNewFileName));

    // first try with the container name which was passed in, if this fails
    if (69 == strlen(pszContainer))
    {
        // convert to UNICODE pszContainer -> pwszFileName
        if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                            pszContainer,
                                            -1, NULL, cch)))
        {
            dwErr = GetLastError();
            goto Ret;
        }

        if (NULL == (pwszFileName = ContInfoAlloc((cch + 1) * sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }

        if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                            pszContainer,
                                            -1, pwszFileName, cch)))
        {
             dwErr = GetLastError();
             goto Ret;
        }

        if (0 == DeleteFileInStorageArea(pwszFilePath, pwszFileName))
        {
            fRetryWithHashedName = FALSE;
        }
    }

    // then try with hash of container name and the machine GUID appended
    if (fRetryWithHashedName)
    {
        if (0 != (dwErr = AddMachineGuidToContainerName(pszContainer,
                                                        rgwchNewFileName)))
        {
            goto Ret;
        }

        dwErr = DeleteFileInStorageArea(pwszFilePath, rgwchNewFileName);
    }
Ret:
    if (pwszFileName)
        ContInfoFree(pwszFileName);

    return dwErr;
}


DWORD DeleteContainerInfo(
                          IN DWORD dwProvType,
                          IN LPSTR pszContainer,
                          IN BOOL fMachineKeyset
                          )
{
    LPWSTR  pwszFilePath = NULL;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    BOOL    fIsLocalSystem = FALSE;
    WCHAR   rgwchNewFileName[80];
    DWORD   dwErr = 0;

    // get the correct storage area (directory)
    if (0 != (dwErr = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                                         &fIsLocalSystem, &pwszFilePath)))
    {
        goto Ret;
    }

    if (0 != (dwErr = DeleteKeyContainer(pwszFilePath, pszContainer)))
    {
        // for migration of machine keys from system to All Users\App Data
        if (fMachineKeyset)
        {
            ContInfoFree(pwszFilePath);
            pwszFilePath = NULL;

            if (0 != (dwErr = GetUserStorageArea(dwProvType, fMachineKeyset, TRUE,
                                                 &fIsLocalSystem, &pwszFilePath)))
            {
                goto Ret;
            }

            if (0 != (dwErr = DeleteKeyContainer(pwszFilePath, pszContainer)))
            {
                goto Ret;
            }
        }
        else
        {
            goto Ret;
        }
    }

    // there may be other keys created with the same container name on
    // different machines and these also need to be deleted
    while (1)
    {
        memset(rgwchNewFileName, 0, sizeof(rgwchNewFileName));

        if (0 != FindClosestFileInStorageArea(pwszFilePath, pszContainer,
                                              rgwchNewFileName, &hFile))
        {
            break;
        }

        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        dwErr = DeleteFileInStorageArea(pwszFilePath, rgwchNewFileName);
    }
    if (0 != dwErr)
    {
        dwErr = (DWORD)NTE_BAD_KEYSET;
        goto Ret;
    }

Ret:
    if(INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    if (pwszFilePath)
        ContInfoFree(pwszFilePath);
    return dwErr;
}

DWORD GetUniqueContainerName(
                             IN KEY_CONTAINER_INFO *pContInfo,
                             OUT BYTE *pbData,
                             OUT DWORD *pcbData
                             )
{
    LPSTR   pszUniqueContainer = NULL;
    DWORD   cch = 0;
    DWORD   dwErr = 0;

    if (0 == (cch = WideCharToMultiByte(CP_ACP, 0,
                                        pContInfo->rgwszFileName,
                                        -1,
                                        NULL,
                                        cch,
                                        NULL,
                                        NULL)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if (NULL == (pszUniqueContainer = (LPSTR)ContInfoAlloc((cch + 1) * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    if (0 == (cch = WideCharToMultiByte(CP_ACP, 0,
                                        pContInfo->rgwszFileName,
                                        -1,
                                        pszUniqueContainer,
                                        cch,
                                        NULL,
                                        NULL)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if (pbData == NULL || *pcbData < (strlen(pszUniqueContainer) + 1))
    {
        *pcbData = strlen(pszUniqueContainer) + 1;

        if (pbData != NULL)
        {
            dwErr = ERROR_MORE_DATA;
        }
        goto Ret;
    }

    *pcbData = strlen(pszUniqueContainer) + 1;

    strcpy(pbData, pszUniqueContainer);
Ret:
    if (pszUniqueContainer)
        ContInfoFree(pszUniqueContainer);

    return dwErr;
}

//
// Function : MachineGuidInFilename
//
// Description : Check if the given Machine GUID is in the given filename.
//               Returns TRUE if it is FALSE if it is not.
//

BOOL MachineGuidInFilename(
                           LPWSTR pwszFileName,
                           LPWSTR pwszMachineGuid
                           )
{
    DWORD   cbFileName;
    BOOL    fRet = FALSE;

    cbFileName = wcslen(pwszFileName);

    // make sure the length of the filename is longer than the GUID
    if (cbFileName < (DWORD)wcslen(pwszMachineGuid))
        goto Ret;

    // compare the GUID with the last 36 characters of the file name
    if (0 == memcmp(pwszMachineGuid, &(pwszFileName[cbFileName - 36]),
                    36 * sizeof(WCHAR)))
        fRet = TRUE;
Ret:
    return fRet;
}

// Converts to UNICODE and uses RegOpenKeyExW
DWORD MyRegOpenKeyEx(IN HKEY hRegKey,
                     IN LPSTR pszKeyName,
                     IN DWORD dwReserved,
                     IN REGSAM SAMDesired,
                     OUT HKEY *phNewRegKey)
{
    WCHAR   rgwchFastBuff[(MAX_PATH + 1) * 2];
    LPWSTR  pwsz = NULL;
    BOOL    fAlloced = FALSE;
    DWORD   cch = 0;
    DWORD   dwErr = ERROR_SUCCESS;

    memset(rgwchFastBuff, 0, sizeof(rgwchFastBuff));

    // convert reg key name to unicode
    if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                        pszKeyName,
                                        -1, NULL, cch)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if ((cch + 1) > ((MAX_PATH + 1) * 2))
    {
        if (NULL == (pwsz = ContInfoAlloc((cch + 1) * sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        fAlloced = TRUE;
    }
    else
    {
        pwsz = rgwchFastBuff;
    }

    if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                        pszKeyName,
                                        -1, pwsz, cch)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    dwErr = RegOpenKeyExW(hRegKey,
                          pwsz,
                          dwReserved,
                          SAMDesired,
                          phNewRegKey);
Ret:
    if (fAlloced && pwsz)
    {
        ContInfoFree(pwsz);
    }

    return dwErr;
}

// Converts to UNICODE and uses RegDeleteKeyW
DWORD MyRegDeleteKey(IN HKEY hRegKey,
                     IN LPSTR pszKeyName)
{
    WCHAR   rgwchFastBuff[(MAX_PATH + 1) * 2];
    LPWSTR  pwsz = NULL;
    BOOL    fAlloced = FALSE;
    DWORD   cch = 0;
    DWORD   dwErr = ERROR_SUCCESS;

    memset(rgwchFastBuff, 0, sizeof(rgwchFastBuff));

    // convert reg key name to unicode
    if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                        pszKeyName,
                                        -1, NULL, cch)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if ((cch + 1) > ((MAX_PATH + 1) * 2))
    {
        if (NULL == (pwsz = ContInfoAlloc((cch + 1) * sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        fAlloced = TRUE;
    }
    else
    {
        pwsz = rgwchFastBuff;
    }

    if (0 == (cch = MultiByteToWideChar(CP_ACP, 0,
                                        pszKeyName,
                                        -1, pwsz, cch)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    dwErr = RegDeleteKeyW(hRegKey, pwsz);
Ret:
    if (fAlloced && pwsz)
    {
        ContInfoFree(pwsz);
    }

    return dwErr;
}

DWORD AllocAndSetLocationBuff(
                              BOOL fMachineKeySet,
                              DWORD dwProvType,
                              CONST char *pszUserID,
                              HKEY *phTopRegKey,
                              TCHAR **ppszLocBuff,
                              BOOL fUserKeys,
                              BOOL *pfLeaveOldKeys
                              )
{
    CHAR    szSID[MAX_PATH];
    DWORD   cbSID = MAX_PATH;
    DWORD   cbLocBuff = 0;
    DWORD   cbTmp = 0;
    CHAR    *pszTmp;
    BOOL    fIsThreadLocalSystem = FALSE;
    DWORD   i;
    DWORD   cbRet = 0;

    if (fMachineKeySet)
    {
        *phTopRegKey = HKEY_LOCAL_MACHINE;
        if ((PROV_RSA_FULL == dwProvType) ||
            (PROV_RSA_SCHANNEL == dwProvType))
        {
            cbTmp = RSA_MACH_REG_KEY_LOC_LEN;
            pszTmp = RSA_MACH_REG_KEY_LOC;
        }
        else if ((PROV_DSS == dwProvType) ||
                 (PROV_DSS_DH == dwProvType) ||
                 (PROV_DH_SCHANNEL == dwProvType))
        {
            cbTmp = DSS_MACH_REG_KEY_LOC_LEN;
            pszTmp = DSS_MACH_REG_KEY_LOC;
        }
        else
        {
            SetLastError((DWORD) NTE_FAIL);
            goto Ret;
        }
    }
    else
    {
        if ((PROV_RSA_FULL == dwProvType) ||
            (PROV_RSA_SCHANNEL == dwProvType))
        {
            cbTmp = RSA_REG_KEY_LOC_LEN;
            pszTmp = RSA_REG_KEY_LOC;
        }
        else if ((PROV_DSS == dwProvType) ||
                 (PROV_DSS_DH == dwProvType) ||
                 (PROV_DH_SCHANNEL == dwProvType))
        {
            cbTmp = DSS_REG_KEY_LOC_LEN;
            pszTmp = DSS_REG_KEY_LOC;
        }
        else
        {
            SetLastError((DWORD) NTE_FAIL);
            goto Ret;
        }

        if (FIsWinNT())
        {
            IsThreadLocalSystem(&fIsThreadLocalSystem);

            if (!GetUserTextualSidA(szSID, &cbSID))
            {
                SetLastError((DWORD) NTE_BAD_KEYSET);
                goto Ret;
            }

            // this checks to see if the key to the current user may be opened
            // TODO - this should be cleaned up
            if (!fMachineKeySet)
            {
                if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_USERS,
                                                  szSID,
                                                  0,      // dwOptions
                                                  KEY_READ,
                                                  phTopRegKey))
                {
                    //
                    // if that failed, try HKEY_USERS\.Default (for services on NT).
                    //
                    cbSID = strlen(".DEFAULT") + 1;
                    strcpy(szSID, ".DEFAULT");
                    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_USERS,
                                                      szSID,
                                                      0,        // dwOptions
                                                      KEY_READ,
                                                      phTopRegKey))
                    {
                        goto Ret;
                    }
                    *pfLeaveOldKeys = TRUE;
                }
            }
        }
        else
        {
            *phTopRegKey = HKEY_CURRENT_USER;
        }
    }

    if (!fUserKeys)
        cbLocBuff = strlen(pszUserID);
    cbLocBuff = cbLocBuff + cbTmp + 2;

    if (NULL == (*ppszLocBuff = (CHAR*)ContInfoAlloc(cbLocBuff)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Ret;
    }

    // Copy the location of the key groups, append the userID to it
    memcpy(*ppszLocBuff, pszTmp, cbTmp);
    if (!fUserKeys)
    {
        (*ppszLocBuff)[cbTmp-1] = '\\';
        strcpy(&(*ppszLocBuff)[cbTmp], pszUserID);
    }

    cbRet = cbLocBuff;
Ret:
    return (cbRet);
}

//
// Enumerates the old machine keys in the file system
// keys were in this location in Beta 2 and Beta 3 of NT5/Win2K
//
DWORD EnumOldMachineKeys(
                         IN DWORD dwProvType,
                         IN OUT PKEY_CONTAINER_INFO pContInfo
                         )
{
#ifndef _XBOX
    HANDLE              hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    FindData;
    LPWSTR              pwszUserStorageArea = NULL;
    LPWSTR              pwszTmp = NULL;
    BOOL                fIsLocalSystem;
    DWORD               i;
    LPSTR               pszNextContainer;
    DWORD               cbNextContainer;
    LPSTR               pszTmpContainer;
    DWORD               cb;
    DWORD               dwErr = 0;

    // first check if the enumeration table is already set up
    if (NULL != pContInfo->pchEnumOldMachKeyEntries)
    {
        goto Ret;
    }

    memset(&FindData, 0, sizeof(FindData));

    if (0 != (dwErr = GetUserStorageArea(dwProvType, TRUE, TRUE,
                                         &fIsLocalSystem, &pwszUserStorageArea)))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    // last character is backslash, so strip that off
    if (NULL == (pwszTmp = (LPWSTR)ContInfoAlloc((wcslen(pwszUserStorageArea) + 3) * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    wcscpy(pwszTmp, pwszUserStorageArea);
    wcscat(pwszTmp, L"*");

    // figure out how many files are in the directroy
    
    if (INVALID_HANDLE_VALUE == (hFind = FindFirstFileExW(pwszTmp,
                                                FindExInfoStandard,
                                                &FindData,
                                                FindExSearchNameMatch,
                                                NULL, 
                                                0)))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    // skip past . and ..
    if (!FindNextFileW(hFind, &FindData))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }
    if (!FindNextFileW(hFind, &FindData))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    for (i = 1; ; i++)
    {
        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFileW(hFind, &FindData))
        {
            if (ERROR_ACCESS_DENIED != (dwErr = GetLastError()))
            {
                break;
            }
            dwErr = 0;
        }
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;

    pContInfo->cbOldMachKeyEntry = MAX_PATH + 1;
    pContInfo->dwiOldMachKeyEntry = 0;
    pContInfo->cMaxOldMachKeyEntry = i;

    // allocate space for the file names
    if (NULL == (pContInfo->pchEnumOldMachKeyEntries =
        ContInfoAlloc(i * (MAX_PATH + 1))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    // enumerate through getting file name from each
    memset(&FindData, 0, sizeof(FindData));
    if (INVALID_HANDLE_VALUE == (hFind = FindFirstFileExW(pwszTmp,
                                                FindExInfoStandard,
                                                &FindData,
                                                FindExSearchNameMatch,
                                                NULL, 
                                                0)))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    // skip past . and ..
    if (!FindNextFileW(hFind, &FindData))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }
    memset(&FindData, 0, sizeof(FindData));
    if (!FindNextFileW(hFind, &FindData))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    pszNextContainer = pContInfo->pchEnumOldMachKeyEntries;

    for (i = 0; i < pContInfo->cMaxOldMachKeyEntry; i++)
    {
        cbNextContainer = MAX_PATH;

        // return the container name, in order to do that we need to open the
        // file and pull out the container name
        if (0 != (dwErr = ReadContainerNameFromFile(TRUE,
                                                    FindData.cFileName,
                                                    pwszUserStorageArea,
                                                    pszNextContainer,
                                                    &cbNextContainer)))
        {
            pszTmpContainer = pszNextContainer;
        }
        else
        {
            pszTmpContainer = pszNextContainer + MAX_PATH + 1;
        }

        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFileW(hFind, &FindData))
        {
            if (ERROR_ACCESS_DENIED != (dwErr = GetLastError()))
            {
                if (ERROR_NO_MORE_FILES == dwErr)
                {
                    dwErr = 0;
                }
                break;
            }
            dwErr = 0;
        }
        pszNextContainer = pszTmpContainer;
    }

Ret:
    if (pwszTmp)
        ContInfoFree(pwszTmp);
    if (pwszUserStorageArea)
        ContInfoFree(pwszUserStorageArea);
    if (INVALID_HANDLE_VALUE != hFind)
        FindClose(hFind);

    return dwErr;
#else
    ASSERT( !"EnumOldMachineKeys" );
    return 0;
#endif
}

DWORD GetNextEnumedOldMachKeys(
                              IN PKEY_CONTAINER_INFO pContInfo,
                              IN BOOL fMachineKeyset,
                              IN DWORD dwProvType,
                              OUT BYTE *pbData,
                              OUT DWORD *pcbData
                              )
{
    CHAR    *psz;
    DWORD   dwErr = 0;

    if (!fMachineKeyset)
    {
        goto Ret;
    }

    if ((NULL == pContInfo->pchEnumOldMachKeyEntries) ||
        (pContInfo->dwiOldMachKeyEntry >= pContInfo->cMaxOldMachKeyEntry))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    if (NULL == pbData)
    {
        goto Ret;
    }

    if (*pcbData < pContInfo->cbRegEntry)
    {
        dwErr = ERROR_MORE_DATA;
        goto Ret;
    }

    psz = pContInfo->pchEnumOldMachKeyEntries + (pContInfo->dwiOldMachKeyEntry *
          pContInfo->cbOldMachKeyEntry);
    memcpy(pbData, psz, strlen(psz) + 1);
    pContInfo->dwiOldMachKeyEntry++;
Ret:
    if (fMachineKeyset)
        *pcbData = pContInfo->cbOldMachKeyEntry;

    return dwErr;
}

//
// Enumerates the keys in the registry into a list of entries
//
DWORD EnumRegKeys(
                  IN OUT PKEY_CONTAINER_INFO pContInfo,
                  IN BOOL fMachineKeySet,
                  IN DWORD dwProvType,
                  OUT BYTE *pbData,
                  IN OUT DWORD *pcbData
                  )
{
#ifndef _XBOX
    HKEY        hTopRegKey = 0;
    LPSTR       pszBuff = NULL;
    DWORD       cbBuff;
    BOOL        fLeaveOldKeys = FALSE;
    HKEY        hKey = 0;
    DWORD       cSubKeys;
    DWORD       cchMaxSubkey;
    DWORD       cchMaxClass;
    DWORD       cValues;
    DWORD       cchMaxValueName;
    DWORD       cbMaxValueData;
    DWORD       cbSecurityDesriptor;
    FILETIME    ftLastWriteTime;
    CHAR        *psz;
    DWORD       i;
    DWORD       dwErr = 0;

    // first check if the enumeration table is already set up
    if (NULL != pContInfo->pchEnumRegEntries)
    {
        goto Ret;
    }

    // get the path to the registry keys
    if (0 == (cbBuff = AllocAndSetLocationBuff(fMachineKeySet,
                                               dwProvType,
                                               pContInfo->pszUserName,
                                               &hTopRegKey,
                                               &pszBuff,
                                               TRUE,
                                               &fLeaveOldKeys)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    // open the reg key
    if ((dwErr = MyRegOpenKeyEx(hTopRegKey,
                                pszBuff,
                                0,
                                KEY_READ,
                                &hKey)) != ERROR_SUCCESS)
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    // find out info on old key containers
    if ((dwErr = RegQueryInfoKey(hKey,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &cSubKeys,
                                   &cchMaxSubkey,
                                   &cchMaxClass,
                                   &cValues,
                                   &cchMaxValueName,
                                   &cbMaxValueData,
                                   &cbSecurityDesriptor,
                                   &ftLastWriteTime
                                   )) != ERROR_SUCCESS)
    {
        goto Ret;
    }

    // if there are old keys then enumerate them into a table
    if (0 != cSubKeys)
    {
        pContInfo->cMaxRegEntry = cSubKeys;
        pContInfo->cbRegEntry = cchMaxSubkey + 1;

        if (NULL == (pContInfo->pchEnumRegEntries =
            ContInfoAlloc(pContInfo->cMaxRegEntry * pContInfo->cbRegEntry *
                          sizeof(CHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }

        for (i = 0; i < pContInfo->cMaxRegEntry; i++)
        {
            psz = pContInfo->pchEnumRegEntries + (i * pContInfo->cbRegEntry);
            if ((dwErr = RegEnumKey(hKey,
                                      i,
                                      psz,
                                      pContInfo->cbRegEntry)) != ERROR_SUCCESS)
            {
                goto Ret;
            }
        }

        if ((pbData == NULL) || (*pcbData < pContInfo->cbRegEntry))
        {
            *pcbData = pContInfo->cbRegEntry;

            if (pbData == NULL)
            {
                goto Ret;
            }
            dwErr = ERROR_MORE_DATA;
            goto Ret;
        }
    }
Ret:
    if (hTopRegKey && (HKEY_CURRENT_USER != hTopRegKey) &&
        (HKEY_LOCAL_MACHINE != hTopRegKey))
    {
        RegCloseKey(hTopRegKey);
    }

    if (pszBuff)
        ContInfoFree(pszBuff);
    if (hKey)
        RegCloseKey(hKey);

    return dwErr;
#else
    ASSERT( !"EnumRegKeys" );
    return 0;
#endif
}

DWORD GetNextEnumedRegKeys(
                           IN PKEY_CONTAINER_INFO pContInfo,
                           IN BOOL fMachineKeySet,
                           IN DWORD dwProvType,
                           OUT BYTE *pbData,
                           OUT DWORD *pcbData
                           )
{
    CHAR    *psz;
    DWORD   dwErr = 0;

    if ((NULL == pContInfo->pchEnumRegEntries) ||
        (pContInfo->dwiRegEntry >= pContInfo->cMaxRegEntry))
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        goto Ret;
    }

    if (NULL == pbData)
    {
        goto Ret;
    }

    if (*pcbData < pContInfo->cbRegEntry)
    {
        dwErr = ERROR_MORE_DATA;
        goto Ret;
    }

    psz = pContInfo->pchEnumRegEntries + (pContInfo->dwiRegEntry *
          pContInfo->cbRegEntry);
    memcpy(pbData, psz, pContInfo->cbRegEntry);
    pContInfo->dwiRegEntry++;
Ret:
    *pcbData = pContInfo->cbRegEntry;

    return dwErr;
}


//+ ===========================================================================
//
//      The function adjusts the token priviledges so that SACL information
//      may be gotten and then opens the indicated registry key.  If the token
//      priviledges may be set then the reg key is opened anyway but the
//      flags field will not have the PRIVILEDGE_FOR_SACL value set.
//
//- ============================================================================
BOOL OpenRegKeyWithTokenPriviledges(
                                    IN HKEY hTopRegKey,
                                    IN LPSTR pszRegKey,
                                    OUT HKEY *phRegKey,
                                    OUT DWORD *pdwFlags
                                    )
{
#ifndef _XBOX
    TOKEN_PRIVILEGES    tp;
    TOKEN_PRIVILEGES    tpPrevious;
    DWORD               cbPrevious = sizeof(TOKEN_PRIVILEGES);
    LUID                luid;
    HANDLE              hToken = 0;
    HKEY                hRegKey = 0;
    BOOL                fStatus;
    BOOL                fImpersonating = FALSE;
    BOOL                fAdjusted = FALSE;
    DWORD               dwAccessFlags = 0;
    BOOL                fRet = FALSE;

    // check if there is a registry key to open
    if (ERROR_SUCCESS != MyRegOpenKeyEx(hTopRegKey, pszRegKey, 0,
                                        KEY_ALL_ACCESS, &hRegKey))
    {
        goto Ret;
    }
    RegCloseKey(hRegKey);
    hRegKey = 0;

    // check if there is a thread token
    if (FALSE == (fStatus = OpenThreadToken(GetCurrentThread(),
                                 MAXIMUM_ALLOWED, TRUE,
                                 &hToken)))
    {
        if (!ImpersonateSelf(SecurityImpersonation))
        {
            goto Ret;
        }
        fImpersonating = TRUE;
        // get the process token
        fStatus = OpenThreadToken(GetCurrentThread(),
                                  MAXIMUM_ALLOWED,
                                  TRUE,
                                  &hToken);
    }

    // set up the new priviledge state
    if (fStatus)
    {
        memset(&tp, 0, sizeof(tp));
        memset(&tpPrevious, 0, sizeof(tpPrevious));

        fStatus = LookupPrivilegeValueA(NULL,
                                        SE_SECURITY_NAME,
                                        &luid);
        if (fStatus)
        {
            //
            // first pass.  get current privilege setting
            //
            tp.PrivilegeCount           = 1;
            tp.Privileges[0].Luid       = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // adjust privilege
            fStatus = AdjustTokenPrivileges(hToken,
                                  FALSE,
                                  &tp,
                                  sizeof(TOKEN_PRIVILEGES),
                                  &tpPrevious,
                                  &cbPrevious);

            if (fStatus && (ERROR_SUCCESS == GetLastError()))
            {
                fAdjusted = TRUE;
                *pdwFlags |= PRIVILEDGE_FOR_SACL;
                dwAccessFlags = ACCESS_SYSTEM_SECURITY;
            }
        }
    }

    // open the registry key
    if (ERROR_SUCCESS != MyRegOpenKeyEx(hTopRegKey,
                                        pszRegKey,
                                        0,
                                        KEY_ALL_ACCESS | dwAccessFlags,
                                        phRegKey))
    {
        goto Ret;
    }


    fRet = TRUE;
Ret:
    // now set the privilege back if necessary
    if (fAdjusted)
    {
        // adjust the priviledge and with the previous state
        fStatus = AdjustTokenPrivileges(hToken,
                                        FALSE,
                                        &tpPrevious,
                                        sizeof(TOKEN_PRIVILEGES),
                                        NULL,
                                        NULL);
    }

    if (hToken)
        CloseHandle(hToken);
    if (fImpersonating)
        RevertToSelf();
#endif
    ASSERT( !"OpenRegKeyWithTokenPriviledges" );
    return TRUE;
}

#define FRENCHCHECKKEY  "Software\\Microsoft\\Cryptography\\Defaults\\CheckInfo"
#define FRENCHCHECKVALUE  "Mask"

#define DH_PROV_ENABLED     1
#define RSA_PROV_ENABLED    2
#define DH_SCH_ENABLED      4
#define RSA_SCH_ENABLED     8

BOOL IsEncryptionPermitted(
                           IN DWORD dwProvType,
                           OUT BOOL *pfInFrance
                           )
/*++

Routine Description:

    This routine checks whether encryption is getting the system default
    LCID and checking whether the country code is CTRY_FRANCE.

Arguments:

    none


Return Value:

    TRUE - encryption is permitted
    FALSE - encryption is not permitted


--*/

{
#ifndef NO_FRANCE_CHECK
    LCID    DefaultLcid;
    CHAR    CountryCode[10];
    ULONG   CountryValue;
    HKEY    hKey = 0;
    DWORD   cb = sizeof(DWORD);
    DWORD   dw = 0;
    DWORD   dwType;
    BOOL    fRet = FALSE;

    *pfInFrance = FALSE;

    DefaultLcid = GetSystemDefaultLCID();

    //
    // Check if the default language is Standard French
    // or if the users's country is set to FRANCE
    //

    if (GetLocaleInfoA(DefaultLcid,LOCALE_ICOUNTRY,CountryCode,10) == 0)
    {
        goto Ret;
    }
    CountryValue = (ULONG) atol(CountryCode);
    if ((CountryValue == CTRY_FRANCE) || (LANGIDFROMLCID(DefaultLcid) == 0x40c))
    {
        // this is a check to see if a registry key to enable encryption is
        // available, do not remove, use or publicize this check without
        // thorough discussions with Microsoft Legal handling French Import
        // issues (tomalb and/or irar)
	    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		                                  FRENCHCHECKKEY,
		                                  0,		// dwOptions
		                                  KEY_READ,
		                                  &hKey))
        {
            hKey = 0;
            goto Ret;
        }

        // get the mask value from the registry indicating which crypto
        // services are to be allowed
	    if (ERROR_SUCCESS != RegQueryValueExA(hKey,
		                                  FRENCHCHECKVALUE,
		                                  NULL, &dwType,		// dwOptions
		                                  (BYTE*)&dw,
		                                  &cb))
        {
            goto Ret;
        }

        switch(dwProvType)
        {
            case PROV_RSA_FULL:
            case PROV_RSA_SIG:
            {
                if (dw & RSA_PROV_ENABLED)
                {
                    fRet = TRUE;
                }
                break;
            }

            case PROV_RSA_SCHANNEL:
            {
                if (dw & RSA_SCH_ENABLED)
                {
                    fRet = TRUE;
                }
                break;
            }

            case PROV_DSS:
            case PROV_DSS_DH:
            {
                if (dw & DH_PROV_ENABLED)
                {
                    fRet = TRUE;
                }
                break;
            }

            case PROV_DH_SCHANNEL:
            {
                if (dw & DH_SCH_ENABLED)
                {
                    fRet = TRUE;
                }
                break;
            }

            default:
                goto Ret;
        }
    }
    else
    {
        fRet = TRUE;
    }
Ret:
    if (hKey)
        RegCloseKey(hKey);
    return fRet;
#else
    *pfInFrance = FALSE;
    return TRUE;
#endif
}

//
// Function : FreeOffloadInfo
//
// Description : The function takes a pointer to Offload Information as the
//               first parameter of the call.  The function frees the
//               information.
//
void FreeOffloadInfo(
                     IN OUT PEXPO_OFFLOAD_STRUCT pOffloadInfo
                     )
{
    if (pOffloadInfo)
    {
#ifndef _XBOX
        if (pOffloadInfo->hInst)
            FreeLibrary(pOffloadInfo->hInst);
        ContInfoFree(pOffloadInfo);
#endif
    }
}

//
// Function : InitExpOffloadInfo
//
// Description : The function takes a pointer to Offload Information as the
//               first parameter of the call.  The function checks in the
//               registry to see if an offload module has been registered. 
//               If a module is registered then it loads the module
//               and gets the OffloadModExpo function pointer.
//
BOOL InitExpOffloadInfo(
                        IN OUT PEXPO_OFFLOAD_STRUCT *ppOffloadInfo
                        )
{
    *ppOffloadInfo = NULL;

    return TRUE;
}

//
// Function : ModularExpOffload
//
// Description : This function does the offloading of modular exponentiation.
//               The function takes a pointer to Offload Information as the
//               first parameter of the call.  If this pointer is not NULL
//               then the function will use this module and call the function.
//               The exponentiation with MOD function will implement
//               Y^X MOD P  where Y is the buffer pbBase, X is the buffer
//               pbExpo and P is the buffer pbModulus.  The length of the
//               buffer pbExpo is cbExpo and the length of pbBase and
//               pbModulus is cbModulus.  The resulting value is output
//               in the pbResult buffer and has length cbModulus.
//               The pReserved and dwFlags parameters are currently ignored.
//               If any of these functions fail then the function fails and
//               returns FALSE.  If successful then the function returns
//               TRUE.  If the function fails then most likely the caller
//               should fall back to using hard linked modular exponentiation.
//
BOOL ModularExpOffload(
                       IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
                       IN BYTE *pbBase,  
                       IN BYTE *pbExpo,
                       IN DWORD cbExpo,
                       IN BYTE *pbModulus,
                       IN DWORD cbModulus,
                       OUT BYTE *pbResult,
                       IN VOID *pReserved,
                       IN DWORD dwFlags
                       )
{
    DWORD   dwErr = 0;
    BOOL    fRet = FALSE;

    // wrap with try/except
    try
    {
        if (NULL == pOffloadInfo)
        {
            goto Ret;
        }

        // call the offload module
        if (!pOffloadInfo->pExpoFunc(pbBase, pbExpo, cbExpo, pbModulus,
                                     cbModulus, pbResult, pReserved, dwFlags))
        {
            goto Ret;
        }

        fRet = TRUE;
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        goto Ret;
    }
Ret:
    return fRet;
}

// 
// The following section of code is for the loading and unloading of
// unicode string resources from a resource DLL (csprc.dll).  This
// allows the resources to be localize even though the CSPs
// themselves are signed.
//

#define MAX_STRING_RSC_SIZE 512


#define GLOBAL_STRING_BUFFERSIZE 2000

BOOL LoadStrings()
{
    return TRUE;
}

void UnloadStrings()
{
    if(g_pbStringBlock)
    {
        ContInfoFree(g_pbStringBlock);
        g_pbStringBlock = NULL;
        memset(&g_Strings, 0, sizeof(g_Strings));
    }
}

#ifdef USE_HW_RNG
#ifdef _M_IX86

// stuff for INTEL RNG usage


// 
// Function : GetRNGDriverHandle
//
// Description : Gets the handle to the INTEL RNG driver if available, then
//               checks if the chipset supports the hardware RNG.  If so
//               the previous driver handle is closed if necessary and the
//               new handle is assigned to the passed in parameter.
//
BOOL GetRNGDriverHandle(
                        IN OUT HANDLE *phDriver
                        )
{
  	ISD_Capability  ISD_Cap;                //in/out for GetCapability
    DWORD           dwBytesReturned;
    char            szDeviceName[80] = "";  //Name of device
	HANDLE			hDriver = INVALID_HANDLE_VALUE; //Driver handle
    BOOLEAN         fReturnCode;            //Return code from IOCTL call
    BOOL            fRet = TRUE;

    memset(&ISD_Cap, 0, sizeof(ISD_Cap));

	wsprintf(szDeviceName,"\\\\.\\"DRIVER_NAME);
    if (INVALID_HANDLE_VALUE == (hDriver = 
        CreateFileA(szDeviceName, FILE_SHARE_READ | FILE_SHARE_WRITE 
                   | GENERIC_READ | GENERIC_WRITE, 0, NULL,
	               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)))
    {
        SetLastError((DWORD) NTE_FAIL);
        goto Ret;
    }

    //Get RNG Enabled
    ISD_Cap.uiIndex = ISD_RNG_ENABLED;  //Set input member
    fReturnCode = DeviceIoControl(hDriver,
	            				  IOCTL_ISD_GetCapability,
				        		  &ISD_Cap, sizeof(ISD_Cap),
						          &ISD_Cap, sizeof(ISD_Cap),
						          &dwBytesReturned,
						          NULL);
	if(fReturnCode == FALSE || ISD_Cap.iStatus != ISD_EOK)  
    {
        SetLastError((DWORD) NTE_FAIL);
        goto Ret;
    }

    // close the previous handle if already there
    if (INVALID_HANDLE_VALUE != *phDriver)
    {
        CloseHandle(*phDriver);
    }

    *phDriver = hDriver;

    fRet = TRUE;
Ret:
    if ((FALSE == fRet) && (INVALID_HANDLE_VALUE != hDriver))
    {
        CloseHandle(hDriver);
    }

    return fRet;
}

// 
// Function : CheckIfRNGAvailable
//
// Description : Checks if the INTEL RNG driver is available, if so then
//               checks if the chipset supports the hardware RNG.
//
BOOL CheckIfRNGAvailable()
{
	HANDLE			hDriver = INVALID_HANDLE_VALUE; //Driver handle
    BOOL            fRet = TRUE;

    fRet = GetRNGDriverHandle(&hDriver);

    if (INVALID_HANDLE_VALUE != hDriver)
        CloseHandle(hDriver);

    return fRet;
}

// 
// Function : HWRNGGenRandom
//
// Description : Uses the passed in handle to the INTEL RNG driver
//               to fill the buffer with random bits.  Actually uses
//               XOR to fill the buffer so that the passed in buffer
//               is also mixed in.
//
unsigned int
HWRNGGenRandom(
               IN HANDLE hRNGDriver,
               IN OUT BYTE *pbBuffer,
               IN DWORD dwLen
               )
{
	ISD_RandomNumber    ISD_Random;             //in/out for GetRandomNumber
    DWORD               dwBytesReturned = 0;
    DWORD               i;
    DWORD               *pdw;
    BYTE                *pb;
    BYTE                *pbRand;
    BOOLEAN             fReturnCode;            //Return code from IOCTL call
    unsigned int        iRet = FALSE;

    memset(&ISD_Random, 0, sizeof(ISD_Random));

    for (i = 0; i < (dwLen / sizeof(DWORD)); i++)
    {
        pdw = (DWORD*)(pbBuffer + i * sizeof(DWORD));

        //No input needed in the ISD_Random structure for this operation,
        //so just send it in as is.
        fReturnCode = DeviceIoControl(hRNGDriver,
					                  IOCTL_ISD_GetRandomNumber,
					                  &ISD_Random, sizeof(ISD_Random),
					                  &ISD_Random, sizeof(ISD_Random),
					                  &dwBytesReturned,
					                  NULL);
        if (fReturnCode == 0 || ISD_Random.iStatus != ISD_EOK)  
        {
            //Error - ignore the data returned
            SetLastError((DWORD) NTE_FAIL);
            goto Ret;
        }

        *pdw = *pdw ^ ISD_Random.uiRandomNum;
    }

    pb = pbBuffer + i * sizeof(DWORD);
    fReturnCode = DeviceIoControl(hRNGDriver,
					              IOCTL_ISD_GetRandomNumber,
					              &ISD_Random, sizeof(ISD_Random),
					              &ISD_Random, sizeof(ISD_Random),
					              &dwBytesReturned,
					              NULL);
    if (fReturnCode == 0 || ISD_Random.iStatus != ISD_EOK)  
    {
        //Error - ignore the data returned
        SetLastError((DWORD) NTE_FAIL);
        goto Ret;
    }
    pbRand = (BYTE*)&ISD_Random.uiRandomNum;

    for (i = 0; i < (dwLen % sizeof(DWORD)); i++)
    {
        pb[i] = pb[i] ^ pbRand[i];
    }

    iRet = TRUE;
Ret:
    return iRet;
}

#ifdef TEST_HW_RNG
// 
// Function : SetupHWRNGIfRegistered
//
// Description : Checks if there is a registry setting indicating the HW RNG
//               is to be used.  If the registry entry is there then it attempts
//               to get the HW RNG driver handle.
//
BOOL SetupHWRNGIfRegistered(
                            OUT HANDLE *phRNGDriver
                            )
{
    HKEY    hRegKey = 0;
    BOOL    fRet = FALSE;

    // first check the registry entry to see if supposed to use HW RNG
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      "Software\\Microsoft\\Cryptography\\UseHWRNG",
		                              0,		// dwOptions
		                              KEY_READ,
		                              &hRegKey))
    {
        // get the driver handle
        if (!GetRNGDriverHandle(phRNGDriver))
        {
            goto Ret;
        }
    }

    fRet = TRUE;
Ret:
    if (hRegKey)
    {
        RegCloseKey(hRegKey);
    }

    return fRet;
}
#endif // TEST_HW_RNG

#endif // _M_IX86
#endif // USE_HW_RNG

// SIG in file
#define SIG_RESOURCE_NAME   "#666"
// MAC in file
#define MAC_RESOURCE_NAME   "#667"

// The function MACs the given bytes.
void MACBytes(
              IN DESTable *pDESKeyTable,
              IN BYTE *pbData,
              IN DWORD cbData,
              IN OUT BYTE *pbTmp,
              IN OUT DWORD *pcbTmp,
              IN OUT BYTE *pbMAC,
              IN BOOL fFinal
              )
{
    DWORD   cb = cbData;
    DWORD   cbMACed = 0;

    while (cb)
    {
        if ((cb + *pcbTmp) < DES_BLOCKLEN)
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, cb);
            *pcbTmp += cb;
            break;
        }
        else
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, DES_BLOCKLEN - *pcbTmp);
            CBC(des, DES_BLOCKLEN, pbMAC, pbTmp, pDESKeyTable,
                ENCRYPT, pbMAC);
            cbMACed = cbMACed + (DES_BLOCKLEN - *pcbTmp);
            cb = cb - (DES_BLOCKLEN - *pcbTmp);
            *pcbTmp = 0;
        }
    }
}

#define CSP_TO_BE_MACED_CHUNK  4096

// Given hFile, reads the specified number of bytes (cbToBeMACed) from the file
// and MACs these bytes.  The function does this in chunks.
BOOL MACBytesOfFile(
                     IN HANDLE hFile,
                     IN DWORD cbToBeMACed,
                     IN DESTable *pDESKeyTable,
                     IN BYTE *pbTmp,
                     IN DWORD *pcbTmp,
                     IN BYTE *pbMAC,
                     IN BYTE fFinal
                     )
{
    BYTE    rgbChunk[CSP_TO_BE_MACED_CHUNK];
    DWORD   cbRemaining = cbToBeMACed;
    DWORD   cbToRead;
    DWORD   dwBytesRead;
    BOOL    fRet = FALSE;

    //
    // loop over the file for the specified number of bytes
    // updating the hash as we go.
    //

    while (cbRemaining > 0)
    {
        if (cbRemaining < CSP_TO_BE_MACED_CHUNK)
            cbToRead = cbRemaining;
        else
            cbToRead = CSP_TO_BE_MACED_CHUNK;

        if(!ReadFile(hFile, rgbChunk, cbToRead, &dwBytesRead, NULL))
            goto Ret;
        if (dwBytesRead != cbToRead)
            goto Ret;

        MACBytes(pDESKeyTable, rgbChunk, dwBytesRead, pbTmp, pcbTmp,
                 pbMAC, fFinal);
        cbRemaining -= cbToRead;
    }

    fRet = TRUE;
Ret:
    return fRet;
}

BYTE rgbMACDESKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};


#if DBG         // NOTE:  This section not compiled for retail builds

#define MAX_DEBUG_BUFFER 2048

BOOL CSPCheckIfShouldDisplayOutput()
{
    return TRUE;
}

// 
void CSPDebugOutput(
                    IN char *szOutString
                    )
{
/*
    DWORD dwWritten;

    if (NULL != g_hfLogFile)
    {
        WriteFile(
            g_hfLogFile,
            szOutString,
            lstrlen(szOutString),
            &dwWritten,
            NULL);
    }
*/
    //XDBGTRC("rsabase",szOutString);
    //OutputDebugStringA(szOutString);
}

void CSPDumpString(
                   LPSTR psz
                   )
{
    DWORD   i;
    CHAR    pszLine[MAX_PATH];

    memset(pszLine, 0, sizeof(pszLine));

    if (psz == NULL)
    {
        wsprintf(pszLine, "<null> buffer!!!\n");
        CSPDebugOutput(pszLine);
        goto Ret;
    }

    CSPDebugOutput(psz);
    CSPDebugOutput("\n");
Ret:
    return;
}

void CSPDumpAddress(
                    IN BYTE *pb,
                    IN OUT LPSTR psz
                    )
{
    CHAR        pszDigits[]="0123456789abcdef";
    DWORD       cch = 0;
    DWORD_PTR   pAddress = (DWORD_PTR)pb;

#if defined(_WIN64)

    psz[cch++] = pszDigits[(pAddress >> 0x3c) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x38) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x34) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x30) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x2c) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x28) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x24) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x20) & 0x0f];

#endif

    psz[cch++] = pszDigits[(pAddress >> 0x1c) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x18) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x14) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x10) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x0c) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x08) & 0x0f];
    psz[cch++] = pszDigits[(pAddress >> 0x04) & 0x0f];
    psz[cch++] = pszDigits[(pAddress) & 0x0f];

    psz[cch++] = ' ';
    psz[cch++] = 0x00;
}

void CSPDumpHexString(
                      IN BYTE *pb,
                      IN DWORD cb
                      )
{
    DWORD   i;
    CHAR    pszLine[MAX_PATH];
    CHAR    pszTmp[MAX_PATH];

    memset(pszLine, 0, sizeof(pszLine));

    if(cb == 0)
    {
        wsprintf(pszLine, "zero length buffer.\n");
        CSPDebugOutput(pszLine);
        goto Ret;
    }

    if((pb == NULL) && (cb != 0))
    {
        wsprintf(pszLine, "<null> buffer but length is %d!!!\n", cb);
        CSPDebugOutput(pszLine);
        goto Ret;
    }

    // cycle through the buffer displaying the data
    for(i = 0; i < cb; i++)
    {
        wsprintf(pszTmp, "0x%02X, ", pb[i]);
        strcat(pszLine, pszTmp);


        if ((0 == ((i + 1) % 8)) || ((i + 1) == cb))
        {
            strcat(pszLine, "\n");
            CSPDebugOutput(pszLine);
            if ((i + 1) != cb)
            {
                memset(pszLine, 0, sizeof(pszLine));
                CSPDebugOutput("    ");
            }
        }
    }
Ret:
    return;
}

void OutputDebugHeader(
                       IN LPSTR pszFunc
                       )
{
    SYSTEMTIME  stTime;
    DWORD       Level = 0;
    ULONG       ClientProcess;
    ULONG       ClientThread;
    char        szOutString[MAX_DEBUG_BUFFER];
    DWORD       cbOut;

    // Output the function being called
    cbOut = wsprintf(szOutString, "Header#: %d  ", g_dwDebugCount);
    CSPDebugOutput(szOutString);
    CSPDebugOutput(pszFunc);

    GetLocalTime(&stTime);

    cbOut = wsprintf(szOutString,
                     "[%2d/%2d %02d:%02d:%02d.%03d] %d.%d\n",
                     stTime.wMonth, stTime.wDay,
                     stTime.wHour, stTime.wMinute, stTime.wSecond, 
                     stTime.wMilliseconds,
                     1,
                     GetCurrentThreadId());

    CSPDebugOutput(szOutString);
}

void CSPDebugOutputFuncReturn(
                              IN BOOL fReturn
                              )
{
    CHAR    pszTmp[MAX_PATH];
    CHAR    pszTmp2[MAX_PATH];
    DWORD   cbOut;

    cbOut = wsprintf(pszTmp, "Header#: %d  ", g_dwDebugCount);
    CSPDebugOutput(pszTmp);

    memset(pszTmp, 0, sizeof(pszTmp));
    strcat(pszTmp, "Return Value - ");
    if (fReturn)
    {
        strcat(pszTmp, "SUCCEEDED\n");
    }
    else
    {
        wsprintf(pszTmp2, "FAILED with Last Error 0x%08X\n", GetLastError());
        strcat(pszTmp, pszTmp2);
    }
    CSPDebugOutput(pszTmp);

}

void CSPDebugOutputAcqCtxt(
                           IN BOOL fEnter,
                           IN BOOL fReturn,
                           IN HCRYPTPROV *phProv,
                           IN CHAR *pUserID,
                           IN DWORD dwFlags,
                           IN PVTableProvStruc pVTable
                           )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("AcquireContext ");
            // dump container name
            CSPDebugOutput("Container Name :\n    ");
            CSPDumpString(pUserID);
            // dump flags
            CSPDebugOutput("Flags          :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            // dump provider struct
            CSPDebugOutput("Prov Struct    :\n    ");
            CSPDumpHexString((BYTE*)pVTable, sizeof(VTableProvStruc));
            g_dwDebugCount++;
        }
        else
        {
            // dump hProv
            CSPDebugOutput("HCRYPTPROV*    :\n    ");
            CSPDumpHexString((BYTE*)phProv, sizeof(HCRYPTPROV));
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputReleaseCtxt(
                               IN BOOL fEnter,
                               IN BOOL fReturn,
                               IN HCRYPTPROV hProv,
                               IN DWORD dwFlags
                               )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("ReleaseContext ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump flags
            CSPDebugOutput("Flags          :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputSetProvParam(
                                IN BOOL fEnter,
                                IN BOOL fReturn,
                                IN HCRYPTPROV hProv,
                                IN DWORD dwParam,
                                IN BYTE *pbData,
                                IN DWORD dwFlags
                                )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("SetProvParam ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump dwParam
            CSPDebugOutput("dwParam        :\n    ");
            CSPDumpHexString((BYTE*)&dwParam, sizeof(dwParam));
            // dump data pointer - don't have a length so can't do much
            CSPDebugOutput("pbData         : No length so no data display\n    ");
            // dump flags
            CSPDebugOutput("Flags          :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputGetProvParam(
                                IN BOOL fEnter,
                                IN BOOL fReturn,
                                IN HCRYPTPROV hProv,
                                IN DWORD dwParam,
                                IN BYTE *pbData,
                                IN DWORD *pdwDataLen,
                                IN DWORD dwFlags
                                )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("GetProvParam ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump dwParam
            CSPDebugOutput("dwParam        :\n    ");
            CSPDumpHexString((BYTE*)&dwParam, sizeof(dwParam));
            // dump data pointer - output buffer so no dump
            CSPDebugOutput("pbData         : Output buffer\n    ");
            // dump pdwDataLen
            CSPDebugOutput("pdwDataLen     :\n    ");
            CSPDumpHexString((BYTE*)pdwDataLen, sizeof(DWORD));
            // dump flags
            CSPDebugOutput("Flags          :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputDeriveKey(
                             IN BOOL fEnter,
                             IN BOOL fReturn,
                             IN HCRYPTPROV hProv,
                             IN ALG_ID Algid,
                             IN HCRYPTHASH hHash,
                             IN DWORD dwFlags,
                             IN HCRYPTKEY *phKey
                             )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("DeriveKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump Algid
            CSPDebugOutput("Algid          :\n    ");
            CSPDumpHexString((BYTE*)&Algid, sizeof(Algid));
            // dump hHash
            CSPDebugOutput("hHash          :\n    ");
            if (hHash)
            {
                CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTPROV));
            }
            else
            {
                CSPDebugOutput("Zero Handle!!    ");
            }
            // dump flags
            CSPDebugOutput("Flags          :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            // dump phKey
            CSPDebugOutput("HCRYPTKEY *    :\n    ");
            g_dwDebugCount++;
        }
        else
        {
            CSPDumpHexString((BYTE*)phKey, sizeof(HCRYPTKEY));
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputDestroyKey(
                              IN BOOL fEnter,
                              IN BOOL fReturn,
                              IN HCRYPTKEY hProv,
                              IN HCRYPTKEY hKey
                              )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("DestroyKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputGenKey(
                          IN BOOL fEnter,
                          IN BOOL fReturn,
                          IN HCRYPTKEY hProv,
                          IN ALG_ID Algid,
                          IN DWORD dwFlags,
                          IN HCRYPTKEY *phKey
                          )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("GenKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump Algid
            CSPDebugOutput("Algid          :\n    ");
            CSPDumpHexString((BYTE*)&Algid, sizeof(Algid));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            // dump phKey
            CSPDebugOutput("HCRYPTKEY*     :\n    ");
            CSPDumpHexString((BYTE*)phKey, sizeof(HCRYPTKEY));
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputGetKeyParam(
                               IN BOOL fEnter,
                               IN BOOL fReturn,
                               IN HCRYPTPROV hProv,
                               IN HCRYPTKEY hKey,
                               IN DWORD dwParam,
                               IN BYTE *pbData,
                               IN DWORD *pdwDataLen,
                               IN DWORD dwFlags
                               )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("GetKeyParam ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump dwParam
            CSPDebugOutput("dwParam        :\n    ");
            CSPDumpHexString((BYTE*)&dwParam, sizeof(dwParam));
            // dump data pointer - output buffer so no dump
            CSPDebugOutput("pbData         : Output buffer\n    ");
            // dump pdwDataLen
            CSPDebugOutput("pdwDataLen     :\n    ");
            CSPDumpHexString((BYTE*)pdwDataLen, sizeof(DWORD));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputGetUserKey(
                              IN BOOL fEnter,
                              IN BOOL fReturn,
                              IN HCRYPTPROV hProv,
                              IN DWORD dwKeySpec,
                              IN HCRYPTKEY *phKey
                              )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("GetUserKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump dwParam
            CSPDebugOutput("dwKeySpec      :\n    ");
            CSPDumpHexString((BYTE*)&dwKeySpec, sizeof(dwKeySpec));
            g_dwDebugCount++;
        }
        else
        {
            // dump hKey
            CSPDebugOutput("HCRYPTKEY*     :\n    ");
            CSPDumpHexString((BYTE*)phKey, sizeof(HCRYPTKEY));
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputSetKeyParam(
                               IN BOOL fEnter,
                               IN BOOL fReturn,
                               IN HCRYPTPROV hProv,
                               IN HCRYPTKEY hKey,
                               IN DWORD dwParam,
                               IN BYTE *pbData,
                               IN DWORD dwFlags
                               )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("SetKeyParam ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump dwParam
            CSPDebugOutput("dwParam        :\n    ");
            CSPDumpHexString((BYTE*)&dwParam, sizeof(dwParam));
            // dump data pointer - don't have a length so can't do much
            switch (dwParam)
            {
                case KP_IV:
                {
                    CSPDebugOutput("pbData         :\n    ");
                    CSPDumpHexString(pbData, 8);
                    break;
                }
                case KP_SCHANNEL_ALG:
                {
                    CSPDebugOutput("pbData         :\n    ");
                    CSPDumpHexString(pbData, sizeof(SCHANNEL_ALG));
                    break;
                }

                case KP_CLIENT_RANDOM:
                case KP_SERVER_RANDOM:
                case KP_CERTIFICATE:
                case KP_CLEAR_KEY:
                {
                    PCRYPT_DATA_BLOB    pDataBlob = (PCRYPT_DATA_BLOB)pbData;

                    CSPDebugOutput("pbData         :\n    ");
                    CSPDumpHexString(pDataBlob->pbData, pDataBlob->cbData);
                    break;
                }
                default:
                    CSPDebugOutput("pbData         : No length so no data display\n    ");
            }
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputGenRandom(
                             IN BOOL fEnter,
                             IN BOOL fReturn,
                             IN HCRYPTPROV hProv,
                             IN DWORD dwLen,
                             IN BYTE *pbBuffer
                             )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("GenRandom ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump dwLen
            CSPDebugOutput("dwLen          :\n    ");
            CSPDumpHexString((BYTE*)&dwLen, sizeof(dwLen));
            // dump buffer
            CSPDebugOutput("pbBuffer       :\n    ");
            CSPDumpHexString(pbBuffer, dwLen);
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputExportKey(
                             IN BOOL fEnter,
                             IN BOOL fReturn,
                             IN HCRYPTPROV hProv,
                             IN HCRYPTKEY hKey,
                             IN HCRYPTKEY hExpKey,
                             IN DWORD dwBlobType,
                             IN DWORD dwFlags,
                             IN BYTE *pbData,
                             IN DWORD *pdwDataLen
                             )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("ExportKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump hExpKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hExpKey, sizeof(HCRYPTKEY));
            // dump dwBlobType
            CSPDebugOutput("dwBlobType     :\n    ");
            CSPDumpHexString((BYTE*)&dwBlobType, sizeof(dwBlobType));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(dwFlags));
            g_dwDebugCount++;
        }
        else
        {
            if (pbData)
            {
                // dump pbData pointer
                CSPDebugOutput("pbData         : \n    ");
                CSPDumpHexString((BYTE*)pbData, *pdwDataLen);
                // dump pdwDataLen
                CSPDebugOutput("pdwDataLen     :\n    ");
                CSPDumpHexString((BYTE*)pdwDataLen, sizeof(DWORD));
            }
            else
            {
                // dump pbData pointer
                CSPDebugOutput("pbData         : NULL\n    ");
                // dump pdwDataLen
                CSPDebugOutput("pdwDataLen     :\n    ");
                CSPDumpHexString((BYTE*)pdwDataLen, sizeof(DWORD));
            }
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputImportKey(
                             IN BOOL fEnter,
                             IN BOOL fReturn,
                             IN HCRYPTPROV hProv,
                             IN BYTE *pbData,
                             IN DWORD dwDataLen,
                             IN HCRYPTKEY hImpKey,
                             IN DWORD dwFlags,
                             IN HCRYPTKEY *phKey
                             )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("ImportKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump pbData pointer
            CSPDebugOutput("pbData         :\n    ");
            CSPDumpHexString(pbData, dwDataLen);
            // dump dwDataLen
            CSPDebugOutput("dwDataLen      :\n    ");
            CSPDumpHexString((BYTE*)&dwDataLen, sizeof(DWORD));
            // dump hImpKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hImpKey, sizeof(HCRYPTKEY));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(dwFlags));
            g_dwDebugCount++;
        }
        else
        {
            // dump phKey
            CSPDebugOutput("HCRYPTKEY*     :\n    ");
            CSPDumpHexString((BYTE*)phKey, sizeof(HCRYPTKEY));
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputDuplicateKey(
                                IN BOOL fEnter,
                                IN BOOL fReturn,
                                IN HCRYPTPROV hProv,
                                IN HCRYPTKEY hKey,
                                IN DWORD *pdwReserved,
                                IN DWORD dwFlags,
                                IN HCRYPTKEY *phKey
                                )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("DuplicateKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump pdwReserved
            CSPDebugOutput("pdwReserved    : Not used\n    ");
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            // dump phKey
            CSPDebugOutput("HCRYPTKEY*     :\n    ");
            CSPDumpHexString((BYTE*)phKey, sizeof(HCRYPTKEY));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputGetHashParam(
                                IN BOOL fEnter,
                                IN BOOL fReturn,
                                IN HCRYPTPROV hProv,
                                IN HCRYPTHASH hHash,
                                IN DWORD dwParam,
                                IN BYTE *pbData,
                                IN DWORD *pdwDataLen,
                                IN DWORD dwFlags
                                )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("GetHashParam ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump dwParam
            CSPDebugOutput("dwParam        :\n    ");
            CSPDumpHexString((BYTE*)&dwParam, sizeof(DWORD));
            // dump pdwDataLen
            CSPDebugOutput("pdwDataLen     :\n    ");
            CSPDumpHexString((BYTE*)pdwDataLen, sizeof(DWORD));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            if (fReturn)
            {
                if (pbData)
                {
                    CSPDebugOutput("pbData         :\n    ");
                    CSPDumpHexString((BYTE*)pbData, *pdwDataLen);
                }
                else
                {
                    CSPDebugOutput("pbData         : NULL\n    ");
                }
            }
            else
            {
                // dump data pointer - output buffer so no dump
                CSPDebugOutput("pbData         : Output buffer\n    ");
            }
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputSetHashParam(
                                IN BOOL fEnter,
                                IN BOOL fReturn,
                                IN HCRYPTPROV hProv,
                                IN HCRYPTHASH hHash,
                                IN DWORD dwParam,
                                IN BYTE *pbData,
                                IN DWORD dwFlags
                                )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("SetHashParam ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump dwParam
            CSPDebugOutput("dwParam        :\n    ");
            CSPDumpHexString((BYTE*)&dwParam, sizeof(DWORD));
            // dump data pointer - don't have a length so can't do much
            CSPDebugOutput("pbData         : No length so no data display\n    ");
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputEncrypt(
                           IN BOOL fEnter,
                           IN BOOL fReturn,
                           IN HCRYPTPROV hProv,
                           IN HCRYPTHASH hKey,
                           IN HCRYPTHASH hHash,
                           IN BOOL Final,
                           IN DWORD dwFlags,
                           IN BYTE *pbData,
                           IN DWORD *pdwDataLen,
                           IN DWORD dwBufLen
                           )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("Encrypt ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump Final
            CSPDebugOutput("Final flag     :\n    ");
            CSPDumpHexString((BYTE*)&Final, sizeof(BOOL));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            // dump data pointer
            if (NULL == pbData)
            {
                CSPDebugOutput("pbData         : NULL buffer \n");
            }
            else
            {
                CSPDebugOutput("pbData     :\n    ");
                CSPDumpHexString(pbData, *pdwDataLen);
            }
            // dump pdwDataLen
            CSPDebugOutput("pdwDataLen     :\n    ");
            CSPDumpHexString((BYTE*)pdwDataLen, sizeof(DWORD));
            // dump dwBufLen
            CSPDebugOutput("dwBufLen       :\n    ");
            CSPDumpHexString((BYTE*)&dwBufLen, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputDecrypt(
                           IN BOOL fEnter,
                           IN BOOL fReturn,
                           IN HCRYPTPROV hProv,
                           IN HCRYPTHASH hKey,
                           IN HCRYPTHASH hHash,
                           IN BOOL Final,
                           IN DWORD dwFlags,
                           IN BYTE *pbData,
                           IN DWORD *pdwDataLen
                           )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("Decrypt ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump Final
            CSPDebugOutput("Final flag     :\n    ");
            CSPDumpHexString((BYTE*)&Final, sizeof(BOOL));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            // dump data pointer
            if (NULL == pbData)
            {
                CSPDebugOutput("pbData         : NULL buffer \n");
            }
            else
            {
                CSPDebugOutput("pbData     :\n    ");
                CSPDumpHexString(pbData, *pdwDataLen);
            }
            // dump pdwDataLen
            CSPDebugOutput("pdwDataLen     :\n    ");
            CSPDumpHexString((BYTE*)pdwDataLen, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputSignHash(
                            IN BOOL fEnter,
                            IN BOOL fReturn,
                            IN HCRYPTPROV hProv,
                            IN HCRYPTHASH hHash,
                            IN DWORD dwKeySpec,
                            IN LPCWSTR pszDescription,
                            IN DWORD dwFlags,
                            IN BYTE *pbSignature,
                            IN DWORD *pdwSigLen
                            )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("SignHash ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump dwKeySpec
            CSPDebugOutput("dwKeySpec      :\n    ");
            CSPDumpHexString((BYTE*)&dwKeySpec, sizeof(DWORD));
            // dump sDescription
            if (NULL == pszDescription)
            {
                CSPDebugOutput("sDescription   : NULL pointer\n    ");
            }
            else
            {
                CSPDebugOutput("sDescription   : non-NULL\n    ");
            }
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            // dump data pointer - output buffer so no use dumping
            CSPDebugOutput("pbData         : Output buffer \n");
            // dump pdwSigLen
            CSPDebugOutput("pdwSigLen      :\n    ");
            CSPDumpHexString((BYTE*)pdwSigLen, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputVerifySignature(
                                   IN BOOL fEnter,
                                   IN BOOL fReturn,
                                   IN HCRYPTPROV hProv,
                                   IN HCRYPTHASH hHash,
                                   IN BYTE *pbSignature,
                                   IN DWORD dwSigLen,
                                   IN HCRYPTKEY hPubKey,
                                   IN LPCWSTR pszDescription,
                                   IN DWORD dwFlags
                                   )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("VerifySignature ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump signature pointer
            CSPDebugOutput("pbSignature    :\n");
            CSPDumpHexString(pbSignature, dwSigLen);
            // dump pdwSigLen
            CSPDebugOutput("dwSigLen       :\n    ");
            CSPDumpHexString((BYTE*)&dwSigLen, sizeof(DWORD));
            // dump hPubKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hPubKey, sizeof(HCRYPTKEY));
            // dump sDescription
            if (NULL == pszDescription)
            {
                CSPDebugOutput("sDescription   : NULL pointer\n    ");
            }
            else
            {
                CSPDebugOutput("sDescription   : non-NULL\n    ");
            }
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(DWORD));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputCreateHash(
                              IN BOOL fEnter,
                              IN BOOL fReturn,
                              IN HCRYPTPROV hProv,
                              IN ALG_ID Algid,
                              IN HCRYPTKEY hKey,
                              IN DWORD dwFlags,
                              IN HCRYPTHASH *phHash
                              )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("CreateHash ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump Algid
            CSPDebugOutput("Algid          :\n    ");
            CSPDumpHexString((BYTE*)&Algid, sizeof(Algid));
            // dump hKey
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(dwFlags));
            g_dwDebugCount++;
        }
        else
        {
            // dump phHash
            CSPDebugOutput("HCRYPTHASH*    :\n    ");
            CSPDumpHexString((BYTE*)phHash, sizeof(HCRYPTHASH));
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputDestroyHash(
                               IN BOOL fEnter,
                               IN BOOL fReturn,
                               IN HCRYPTPROV hProv,
                               IN HCRYPTHASH hHash
                               )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("DestroyHash ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputHashData(
                            IN BOOL fEnter,
                            IN BOOL fReturn,
                            IN HCRYPTPROV hProv,
                            IN HCRYPTHASH hHash,
                            IN BYTE *pbData,
                            IN DWORD dwDataLen,
                            IN DWORD dwFlags
                            )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("HashData ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump pbData
            CSPDebugOutput("pbData         :\n    ");
            CSPDumpHexString(pbData, dwDataLen);
            // dump dwDataLen
            CSPDebugOutput("dwDataLen      :\n    ");
            CSPDumpHexString((BYTE*)&dwDataLen, sizeof(dwDataLen));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(dwFlags));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputHashSessionKey(
                                  IN BOOL fEnter,
                                  IN BOOL fReturn,
                                  IN HCRYPTPROV hProv,
                                  IN HCRYPTHASH hHash,
                                  IN HCRYPTKEY hKey,
                                  IN DWORD dwFlags
                                  )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("HashSessionKey ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump hHash
            CSPDebugOutput("HCRYPTKEY      :\n    ");
            CSPDumpHexString((BYTE*)&hKey, sizeof(HCRYPTKEY));
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(dwFlags));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

void CSPDebugOutputDuplicateHash(
                                 IN BOOL fEnter,
                                 IN BOOL fReturn,
                                 IN HCRYPTPROV hProv,
                                 IN HCRYPTHASH hHash,
                                 IN DWORD *pdwReserved,
                                 IN DWORD dwFlags,
                                 IN HCRYPTHASH *phHash
                                 )
{
    if (CSPCheckIfShouldDisplayOutput())
    {
        if (fEnter)
        {
            OutputDebugHeader("DuplicateHash ");
            // dump hProv
            CSPDebugOutput("HCRYPTPROV     :\n    ");
            CSPDumpHexString((BYTE*)&hProv, sizeof(HCRYPTPROV));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH     :\n    ");
            CSPDumpHexString((BYTE*)&hHash, sizeof(HCRYPTHASH));
            // dump pdwReserved
            CSPDebugOutput("pdwReserved    : Reserved for future\n    ");
            // dump dwFlags
            CSPDebugOutput("dwFlags        :\n    ");
            CSPDumpHexString((BYTE*)&dwFlags, sizeof(dwFlags));
            // dump hHash
            CSPDebugOutput("HCRYPTHASH*    :\n    ");
            CSPDumpHexString((BYTE*)phHash, sizeof(HCRYPTHASH));
            g_dwDebugCount++;
        }
        else
        {
            g_dwDebugCount--;
            CSPDebugOutputFuncReturn(fReturn);
        }
    }
}

#endif // DBG -- NOTE:  This section not compiled for retail builds 

