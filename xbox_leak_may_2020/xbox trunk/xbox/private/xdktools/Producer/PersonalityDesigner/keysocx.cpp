// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.


#include "stdafx.h"
#include "keysocx.h"

/////////////////////////////////////////////////////////////////////////////
// CKeysOCX

IMPLEMENT_DYNCREATE(CKeysOCX, CWnd)

/////////////////////////////////////////////////////////////////////////////
// CKeysOCX properties

/////////////////////////////////////////////////////////////////////////////
// CKeysOCX operations

void CKeysOCX::SetNoteState(short nKey, LPCTSTR pszName, short nState)
{
	static BYTE parms[] =
		VTS_I2 VTS_BSTR VTS_I2;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 nKey, pszName, nState);
}
