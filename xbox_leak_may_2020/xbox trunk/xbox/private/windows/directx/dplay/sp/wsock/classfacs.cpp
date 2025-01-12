/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       classfac.cpp
 *  Content:	a generic class factory
 *
 *
 *	This is a generic C class factory.  All you need to do is implement
 *	a function called DoCreateInstance that will create an instace of
 *	your object.
 *
 *	GP_ stands for "General Purpose"
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/13/98	jwo		Created it.
 ***************************************************************************/

#include	"wsockspi.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// class factory class definition
//
typedef struct GPCLASSFACTORY
{
   IClassFactoryVtbl 		*lpVtbl;
   DWORD					dwRefCnt;
   CLSID					clsid;
} GPCLASSFACTORY, *LPGPCLASSFACTORY;

//
// function prototype for CoLockPbjectExternal()
//
typedef	HRESULT (WINAPI * PCOLOCKOBJECTEXTERNAL)(LPUNKNOWN, BOOL, BOOL );

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************




/*
 * GP_QueryInterface
 */
STDMETHODIMP GP_QueryInterface(
                LPCLASSFACTORY This,
                REFIID riid,
                LPVOID *ppvObj )
{
    LPGPCLASSFACTORY	pcf;
	HRESULT hr;

    pcf = (LPGPCLASSFACTORY)This;
    *ppvObj = NULL;


        pcf->dwRefCnt++;
        *ppvObj = This;
		hr = S_OK;

	return hr;

} /* GP_QueryInterface */

#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Initialize"

/*
 * GP_AddRef
 */
STDMETHODIMP_(ULONG) GP_AddRef( LPCLASSFACTORY This )
{
    LPGPCLASSFACTORY pcf;

    pcf = (LPGPCLASSFACTORY)This;
    pcf->dwRefCnt++;
    return pcf->dwRefCnt;
} /* GP_AddRef */



#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Initialize"

/*
 * GP_Release
 */
STDMETHODIMP_(ULONG) GP_Release( LPCLASSFACTORY This )
{
    LPGPCLASSFACTORY	pcf;

    pcf = (LPGPCLASSFACTORY)This;
    pcf->dwRefCnt--;

    if( pcf->dwRefCnt != 0 )
    {
        return pcf->dwRefCnt;
    }

    DNFree( pcf );
    return 0;

} /* GP_Release */




/*
 * GP_CreateInstance
 *
 * Creates an instance of a DNServiceProvider object
 */
STDMETHODIMP GP_CreateInstance(
                LPCLASSFACTORY This,
                LPUNKNOWN pUnkOuter,
                REFIID riid,
    			LPVOID *ppvObj
				)
{
    HRESULT					hr = S_OK;
    LPGPCLASSFACTORY		pcf;

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

	pcf = (LPGPCLASSFACTORY) This;
	*ppvObj = NULL;


    /*
     * create the object by calling DoCreateInstance.  This function
     *	must be implemented specifically for your COM object
     */
	hr = DoCreateInstance(This, pUnkOuter, pcf->clsid, riid, ppvObj);
	if (FAILED(hr))
	{
		*ppvObj = NULL;
		return hr;
	}

    return S_OK;

} /* GP_CreateInstance */



/*
 * GP_LockServer
 *
 * Called to force our DLL to stayed loaded
 */
STDMETHODIMP GP_LockServer(
                LPCLASSFACTORY This,
                BOOL fLock
				)
{
	return S_OK;

} /* GP_LockServer */

static IClassFactoryVtbl GPClassFactoryVtbl =
{
        GP_QueryInterface,
        GP_AddRef,
        GP_Release,
        GP_CreateInstance,
        GP_LockServer
};


/*
 * DllGetClassObject
 *
 * Entry point called by COM to get a ClassFactory pointer
 */
HRESULT  SPDllGetClassObject(
                REFCLSID rclsid,
                REFIID riid,
                LPVOID *ppvObj )
{
    LPGPCLASSFACTORY	pcf;
    HRESULT		hr;

    *ppvObj = NULL;

    /*
     * is this our class id?
     */
//	you must implement GetClassID() for your specific COM object
	if (!IsClassImplemented(rclsid))
    {
		return CLASS_E_CLASSNOTAVAILABLE;
	}

    /*
     * create a class factory object
     */
    pcf = static_cast<GPCLASSFACTORY*>( DNMalloc( sizeof( *pcf ) ) );
    if( NULL == pcf)
    {
        return E_OUTOFMEMORY;
    }

	pcf->lpVtbl = &GPClassFactoryVtbl;
    pcf->dwRefCnt = 0;
	pcf->clsid = rclsid;

    hr = GP_QueryInterface( (LPCLASSFACTORY) pcf, riid, ppvObj );
    if( FAILED( hr ) )
    {
        DNFree ( pcf );
        *ppvObj = NULL;
    }
    else
    {
    }

    return hr;

} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
HRESULT SPDllCanUnloadNow( void )
{
    HRESULT	hr = S_FALSE;

	
	if ( g_lOutstandingInterfaceCount == 0 )
	{
		hr = S_OK;
	}

    return hr;

} /* DllCanUnloadNow */

