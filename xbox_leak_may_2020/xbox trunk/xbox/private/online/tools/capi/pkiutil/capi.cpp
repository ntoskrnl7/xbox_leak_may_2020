#include "global.hxx"

#ifndef CE_BUILD

#ifndef _M_IX86
BOOL
WINAPI
UnicodeDllMain(
    HMODULE hInst,
    ULONG  ulReason,
    LPVOID lpReserved
    )
{
    return TRUE;
}

// Bug in CyptAcquireContextW
BOOL WINAPI CryptAcquireContextU(
    HCRYPTPROV *phProv,
    LPCWSTR lpContainer,
    LPCWSTR lpProvider,
    DWORD dwProvType,
    DWORD dwFlags) {

    BYTE rgb1[_MAX_PATH];
    BYTE rgb2[_MAX_PATH];
    char *  szContainer = NULL;
    char *  szProvider = NULL;
    BOOL    err;

    err = FALSE;
    if(
        MkMBStr(rgb1, _MAX_PATH, lpContainer, &szContainer)  &&
        MkMBStr(rgb2, _MAX_PATH, lpProvider, &szProvider)    ) {

        err = CryptAcquireContextA (
                phProv,
                szContainer,
                szProvider,
                dwProvType,
                dwFlags
               );
    }

    FreeMBStr(rgb1, szContainer);
    FreeMBStr(rgb2, szProvider);

    return(err);
}

#else

#ifdef LINK_REDIR
#define pfnAcquireContextW      CryptAcquireContextW
#define pfnSignHashW            CryptSignHashW
#define pfnVerifySignatureW     CryptVerifySignatureW
#define pfnSetProviderW         CryptSetProviderW
#else
static HINSTANCE                hCrypt                  = NULL;
static LPSTR                    pszCryptName            = "advapi32.dll";
static CRYPTACQUIRECONTEXTW     *pfnAcquireContextW     = NULL;
static CRYPTSIGNHASHW           *pfnSignHashW           = NULL;
static CRYPTVERIFYSIGNATUREW    *pfnVerifySignatureW    = NULL;
static CRYPTSETPROVIDERW        *pfnSetProviderW        = NULL;
#endif

BOOL
WINAPI
UnicodeDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
#ifndef LINK_REDIR
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        hCrypt = LoadLibraryA(pszCryptName);

        pfnAcquireContextW = (CRYPTACQUIRECONTEXTW*)GetProcAddress(
                                    hCrypt,
                                    "CryptAcquireContextW");
        pfnSignHashW = (CRYPTSIGNHASHW*)GetProcAddress(
                                    hCrypt,
                                    "CryptSignHashW");
        pfnVerifySignatureW = (CRYPTVERIFYSIGNATUREW*)GetProcAddress(
                                    hCrypt,
                                    "CryptVerifySignatureW");
        pfnSetProviderW = (CRYPTSETPROVIDERW*)GetProcAddress(
                                    hCrypt,
                                    "CryptSetProviderW");
        break;

    case DLL_PROCESS_DETACH:
        FreeLibrary( hCrypt);
        pfnAcquireContextW  = NULL;
        pfnSignHashW        = NULL;
        pfnVerifySignatureW = NULL;
        pfnSetProviderW     = NULL;
        break;

    default:
        break;
    }
#endif

    return TRUE;
}


BOOL WINAPI CryptAcquireContextU(
    HCRYPTPROV *phProv,
    LPCWSTR lpContainer,
    LPCWSTR lpProvider,
    DWORD dwProvType,
    DWORD dwFlags) {

    BYTE rgb1[_MAX_PATH];
    BYTE rgb2[_MAX_PATH];
    char *  szContainer = NULL;
    char *  szProvider = NULL;
    LONG    err;

    /* Bug in CyptAcquireContextW
    if(FIsWinNT())
        return( pfnAcquireContextW (
            phProv,
            lpContainer,
            lpProvider,
            dwProvType,
            dwFlags
            ));
            */

    err = FALSE;
    if(
        MkMBStr(rgb1, _MAX_PATH, lpContainer, &szContainer)  &&
        MkMBStr(rgb2, _MAX_PATH, lpProvider, &szProvider)    )
        err = CryptAcquireContextA (
                phProv,
                szContainer,
                szProvider,
                dwProvType,
                dwFlags
               );

    FreeMBStr(rgb1, szContainer);
    FreeMBStr(rgb2, szProvider);

    return(err);
}

BOOL WINAPI CryptSignHashU(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR lpDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen) {

    BYTE rgb[_MAX_PATH];
    char *  szDescription;
    LONG    err;

    if(FIsWinNT())
        return(pfnSignHashW (
            hHash,
            dwKeySpec,
            lpDescription,
            dwFlags,
            pbSignature,
            pdwSigLen
            ));

    err = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpDescription, &szDescription))
        err = CryptSignHashA (
                hHash,
                dwKeySpec,
                szDescription,
                dwFlags,
                pbSignature,
                pdwSigLen
            );

    FreeMBStr(rgb, szDescription);

    return(err);
}

BOOL WINAPI CryptVerifySignatureU(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR lpDescription,
    DWORD dwFlags) {

    BYTE rgb[_MAX_PATH];
    char *  szDescription;
    LONG    err;

    if(FIsWinNT())
        return(pfnVerifySignatureW (
            hHash,
            pbSignature,
            dwSigLen,
            hPubKey,
            lpDescription,
            dwFlags
            ));

    err = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpDescription, &szDescription))
        err = CryptVerifySignatureA (
                hHash,
                pbSignature,
                dwSigLen,
                hPubKey,
                szDescription,
                dwFlags
             );

    FreeMBStr(rgb, szDescription);

    return(err);
}

BOOL WINAPI CryptSetProviderU(
    LPCWSTR lpProvName,
    DWORD dwProvType) {

    BYTE rgb[_MAX_PATH];
    char *  szProvName;
    LONG    err;

    if(FIsWinNT())
        return(pfnSetProviderW (
            lpProvName,
            dwProvType
            ));

    err = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpProvName, &szProvName))
        err = CryptSetProviderA (
                szProvName,
                dwProvType
            );

    FreeMBStr(rgb, szProvName);

    return(err);
}

#endif
#endif // CE_BUILD

#ifndef UNDER_CE

// IN LINE CRYPT FUNCTION FOR BOTH ALPHA and X86

/*
 -      CryptEnumProvidersU
 -
 *      Purpose:
 *                Enumerate the providers.
 *
 *      Parameters:
 *                IN  dwIndex        - Index to the providers to enumerate
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter
 *                OUT pdwProvType    - The type of the provider
 *                OUT pwszProvName    - Name of the enumerated provider
 *                IN OUT pcbProvName - Length of the enumerated provider
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */

#define PROVREG L"SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"
#define PROVTYPEREG L"Type"

BOOL
WINAPI CryptEnumProvidersU(
                           IN DWORD dwIndex,
                           IN DWORD *pdwReserved,
                           IN DWORD dwFlags,
                           OUT DWORD *pdwProvType,
                           OUT LPWSTR pwszProvName,
                           IN OUT DWORD *pcbProvName
                           )
{
    HKEY        hRegKey = 0;
    HKEY        hProvRegKey = 0;
    LONG        err;
    DWORD       cbClass;
    FILETIME    ft;
    DWORD       dwKeyType;
    DWORD       cbProvType;
    DWORD       dw;
    DWORD       cSubKeys;
    DWORD       cbMaxKeyName;
    DWORD       cbMaxClass;
    DWORD       cValues;
    DWORD       cbMaxValName;
    DWORD       cbMaxValData;
    LPWSTR      pwszTmpProvName = NULL;
    DWORD       cbTmpProvName;
    BOOL        fRet = CRYPT_FAILED;

    if (NULL != pdwReserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    if (0 != dwFlags)
    {
        SetLastError((DWORD)NTE_BAD_FLAGS);
        goto Ret;
    }

    if (ERROR_SUCCESS != (err = RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                                             PROVREG,
                                             0L, KEY_READ, &hRegKey)))
    {
	    SetLastError((DWORD) err);
        goto Ret;
    }

    if (ERROR_SUCCESS != (err = RegQueryInfoKey(hRegKey,
                                                NULL,
                                                &cbClass,
                                                NULL,
                                                &cSubKeys,
                                                &cbMaxKeyName,
                                                &cbMaxClass,
                                                &cValues,
                                                &cbMaxValName,
                                                &cbMaxValData,
                                                NULL,
                                                &ft)))
    {
	    SetLastError((DWORD) err);
        goto Ret;
    }
    cbMaxKeyName += sizeof(CHAR);

    if (NULL == (pwszTmpProvName = (LPWSTR) _alloca(cbMaxKeyName * sizeof(WCHAR))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Ret;
    }

    if (ERROR_SUCCESS != (err = RegEnumKeyExU(hRegKey, dwIndex, pwszTmpProvName,
                                             &cbMaxKeyName, NULL,
                                             NULL, &cbClass, &ft)))
    {
        SetLastError((DWORD)err);
        goto Ret;
    }

    if (ERROR_SUCCESS != (err = RegOpenKeyExU(hRegKey,
                                             pwszTmpProvName,
                                             0L, KEY_READ, &hProvRegKey)))
    {
	    SetLastError((DWORD) err);
        goto Ret;
    }

    cbProvType = sizeof(dw);
    if (ERROR_SUCCESS != (err = RegQueryValueExU(hProvRegKey,
                                                PROVTYPEREG,
                                                NULL, &dwKeyType, (BYTE*)&dw,
                                                &cbProvType)))
    {
	    SetLastError((DWORD) NTE_PROV_TYPE_ENTRY_BAD);
        goto Ret;
    }
    *pdwProvType = dw;

    cbTmpProvName = (wcslen(pwszTmpProvName) + 1) * sizeof(WCHAR);

    if (NULL != pwszProvName)
    {
        if (*pcbProvName < cbTmpProvName)
        {
            *pcbProvName = cbTmpProvName;
            SetLastError(ERROR_MORE_DATA);
            goto Ret;
        }
        wcscpy(pwszProvName, pwszTmpProvName);
    }

    *pcbProvName = cbTmpProvName;

    fRet = CRYPT_SUCCEED;

Ret:
    if (hRegKey)
        RegCloseKey(hRegKey);
    if (hProvRegKey)
        RegCloseKey(hProvRegKey);
    return fRet;
}

#endif	// ndef UNDER_CE
