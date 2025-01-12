/***
*findcom.c - C find file functions
*
*	Copyright (c) 1991-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines _findfirst(), _findnext(), and _findclose().
*
*Revision History:
*	08-21-91  BWM	Wrote Win32 versions.
*	09-13-91  BWM	Changed handle type to long.
*	08-18-92  SKS	Add a call to FileTimeToLocalFileTime
*			as a temporary fix until _dtoxtime takes UTC
*	08-26-92  SKS	creation and last access time should be same as the
*			last write time if ctime/atime are not available.
*	01-08-93  SKS	Remove change I made 8-26-92.  Previous behavior
*			was deemed "by design" and preferable.
*	03-30-93  GJF	Replaced reference to _dtoxtime with __gmtotime_t. Also
*			made _timet_from_ft a static function.
*	04-06-93  SKS	Replace _CRTAPI* with _cdecl
*	07-21-93  GJF	Repaced use of _gmtotime_t by __loctotime_t.
*	11-01-93  CFW	Enable Unicode variant.
*	12-28-94  GJF	Added _[w]findfirsti64, _[w]findnexti64.
*       09-25-95  GJF   __loctotime_t now takes a DST flag, pass -1 in this
*                       slot to indicate DST status is unknown.
*	10-06-95  SKS	Add "const" to "char *" in prototypes for *findfirst().
*			Prepend missing underscores to func names in comments.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <errno.h>
#include <io.h>
#include <time.h>
#include <ctime.h>
#include <string.h>
#include <internal.h>
#include <tchar.h>

#ifndef _WIN32
#error ERROR - ONLY WIN32 TARGET SUPPORTED!
#endif

time_t __cdecl __timet_from_ft(FILETIME * pft);

/***
*int _findclose(hfind) - Release resources of find
*
*Purpose:
*	Releases resources of a group of files found by _findfirst and
*	_findnext
*
*Entry:
*	hfind - handle from _findfirst
*
*Exit:
*	Good return:
*	0 if success
*	-1 if fail, errno set
*
*Exceptions:
*	None.
*
*******************************************************************************/

int __cdecl _findclose(long hFile)
{
    if (!FindClose((HANDLE)hFile)) {
	errno = EINVAL;
	return (-1);
    }
    return (0);
}


/***
*time_t _fttotimet(ft) - convert Win32 file time to Xenix time
*
*Purpose:
*	converts a Win32 file time value to Xenix time_t
*
*	Note: We cannot directly use the ft value. In Win32, the file times
*	returned by the API are ambiguous. In Windows NT, they are UTC. In
*	Win32S, and probably also Win32C, they are local time values. Thus,
*	the value in ft must be converted to a local time value (by an API)
*	before we can use it.
*
*Entry:
*	int yr, mo, dy -	date
*	int hr, mn, sc -	time
*
*Exit:
*	returns Xenix time value
*
*Exceptions:
*
*******************************************************************************/

time_t __cdecl __timet_from_ft(FILETIME * pft)
{
    SYSTEMTIME st;
    FILETIME lft;

    /* 0 FILETIME returns a -1 time_t */

    if (!pft->dwLowDateTime && !pft->dwHighDateTime) {
	return (-1L);
    }

    /*
     * Convert to a broken down local time value
     */
    if ( !FileTimeToLocalFileTime(pft, &lft) ||
	 !FileTimeToSystemTime(&lft, &st) )
    {
	return (-1L);
    }

    return ( __loctotime_t(st.wYear,
			   st.wMonth,
			   st.wDay,
			   st.wHour,
			   st.wMinute,
			   st.wSecond,
                           -1) );
}
