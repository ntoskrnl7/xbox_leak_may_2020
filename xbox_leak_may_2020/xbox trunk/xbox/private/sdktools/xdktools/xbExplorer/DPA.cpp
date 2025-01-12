#include "stdafx.h"
#include "dpda.h" 
#include <assert.h>
  
#ifdef _DEBUG 
#define DPA_MAGIC   ('P' | ('A' << 256)) 
#define IsDPA(pdpa) ((pdpa) && (pdpa)->magic == DPA_MAGIC) 
#else 
#define IsDPA(pdsa) 

#endif   
#define ControlAlloc(hheap, cb)       HeapAlloc((hheap), HEAP_ZERO_MEMORY, (cb)) 
#define ControlReAlloc(hheap, pb, cb) HeapReAlloc((hheap), HEAP_ZERO_MEMORY, (pb),(cb)) 
#define ControlFree(hheap, pb)        HeapFree((hheap), 0, (pb)) 
#define ControlSize(hheap, pb)        HeapSize((hheap), 0, (LPCVOID)(pb))  
#define hmemcpy memcpy    
typedef struct {     
    void **pp;
    PFNDPACOMPARE pfnCmp;
    LPARAM lParam;
    int cp;
    void **ppT;
 } SORTPARAMS;

  BOOL DPA_MergeSort(SORTPARAMS * psp);
  void DPA_MergeSort2(SORTPARAMS * psp, int iFirst, int cItems);    

//================== Dynamic pointer array implementation ===========  
typedef struct _DPA
 { 
    // NOTE: The following two fields MUST be defined in this order, at 
    // the beginning of the structure in order for the macro APIs to work. 
    //     
    int cp;
    void **pp;
    HANDLE hheap;        
    // Heap to allocate from if NULL use shared
    int cpAlloc;
    int cpGrow; 
#ifdef DEBUG   
    UINT magic; 
#endif 
} DPA;

HANDLE g_hSharedHeap = NULL;
void * Alloc(long cb) 
{     
    // I will assume that this is the only one that needs the checks to     
    // see if the heap has been previously created or not   
   if (g_hSharedHeap == NULL)     
   {
        g_hSharedHeap = HeapCreate(0, 1, 0);          
        // If still NULL we have problems!   
        if (g_hSharedHeap == NULL)       
            return(NULL);     
   }
   return HeapAlloc(g_hSharedHeap, HEAP_ZERO_MEMORY, cb);
}

void * ReAlloc(void * pb, long cb) 
{
    if (pb == NULL)         
        return Alloc(cb);     
    
    return HeapReAlloc(g_hSharedHeap, HEAP_ZERO_MEMORY, pb, cb); 
}

BOOL Free(void * pb)
{     
    return HeapFree(g_hSharedHeap, 0, pb); 
}

DWORD GetSize(void * pb) 
{  
   return HeapSize(g_hSharedHeap, 0, pb); 
}   

HDPA DPA_Create(int cpGrow) 
{     
    HDPA pdpa = (HDPA)Alloc(sizeof(DPA));     
    if (pdpa)    
    {         
        pdpa->cp = 0;         
        pdpa->cpAlloc = 0;         
        pdpa->cpGrow = (cpGrow < 8 ? 8 : cpGrow);         
        pdpa->pp = NULL;         
        pdpa->hheap = g_hSharedHeap;   
   
        // Defaults to use shared one (for now...) 
#ifdef DEBUG         
        pdpa->magic = DPA_MAGIC; 
#endif     
    }     
    return pdpa; 
}

// Should nuke the standard DPA above...
HDPA DPA_CreateEx(int cpGrow, HANDLE hheap) 
{  
   HDPA pdpa;     
   if (hheap == NULL)     
   {      
       pdpa = (HDPA)Alloc(sizeof(DPA));  
       hheap = g_hSharedHeap; 
   }    
   else   
       pdpa = (HDPA)ControlAlloc(hheap, sizeof(DPA));  
   if (pdpa)  
   {        
       pdpa->cp = 0;    
       pdpa->cpAlloc = 0;       
       pdpa->cpGrow = (cpGrow < 8 ? 8 : cpGrow);    
       pdpa->pp = NULL;       
       pdpa->hheap = hheap; 
#ifdef DEBUG    
       pdpa->magic = DPA_MAGIC; 
#endif   
   }   
   return pdpa; 
}  

BOOL DPA_Destroy(HDPA pdpa) 
{     
   //assert(IsDPA(pdpa));  
    if (pdpa == NULL)    // allow NULL for low memory cases, still assert        
        return TRUE;      
    assert (pdpa->hheap);  

#ifdef DEBUG    
    pdpa->cp = 0;  
    pdpa->cpAlloc = 0;
    pdpa->magic = 0; 
#endif    
    if (pdpa->pp && !ControlFree(pdpa->hheap, pdpa->pp))      
        return FALSE;   
    return ControlFree(pdpa->hheap, pdpa);
}

HDPA DPA_Clone(HDPA pdpa, HDPA pdpaNew)
{
    BOOL fAlloc = FALSE;   
    if (!pdpaNew)   
    {         
        pdpaNew = DPA_CreateEx(pdpa->cpGrow, pdpa->hheap);  
        if (!pdpaNew)        
            return NULL;       
        fAlloc = TRUE;    
    }
    
    if (!DPA_Grow(pdpaNew, pdpa->cpAlloc)) 
    {
        if (!fAlloc)    
            DPA_Destroy(pdpaNew); 
        return NULL;     
    }
    pdpaNew->cp = pdpa->cp;   
    hmemcpy(pdpaNew->pp, pdpa->pp, pdpa->cp * sizeof(void *));  
    return pdpaNew;
 }  

void * DPA_GetPtr(HDPA pdpa, int index) 
{ 
     //    assert(IsDPA(pdpa));   
    if (index < 0 || index >= pdpa->cp)         
        return NULL;    
    return pdpa->pp[index]; 
}  

int DPA_GetPtrIndex(HDPA pdpa, void * p) 
{   
    void **pp;
    void **ppMax;  
    if (pdpa->pp)   
    {      
        pp = pdpa->pp;   
        ppMax = pp + pdpa->cp;    
        for ( ; pp < ppMax; pp++)     
        {            
            if (*pp == p)     
                return (pp - pdpa->pp); 
        }  
    }  
    return -1;
}

BOOL DPA_Grow(HDPA pdpa, int cpAlloc)
{   
    if (cpAlloc > pdpa->cpAlloc) 
    {     
        void **ppNew;   
        cpAlloc = ((cpAlloc + pdpa->cpGrow - 1) / pdpa->cpGrow) * pdpa->cpGrow;       
        if (pdpa->pp)       
            ppNew = (void * *)ControlReAlloc(pdpa->hheap, pdpa->pp, cpAlloc * sizeof(void *));       
        else         
            ppNew = (void * *)ControlAlloc(pdpa->hheap, cpAlloc * sizeof(void *));        
        if (!ppNew)        
            return FALSE;      
        pdpa->pp = ppNew;   
        pdpa->cpAlloc = cpAlloc; 
    }   
    return TRUE; 
}  

BOOL DPA_SetPtr(HDPA pdpa, int index, void * p) 
{    
    if (index < 0)   
    {         
        // DebugMsg(DM_ERROR, "DPA: Invalid index: %d", index); 
        return FALSE;   
    }    
    if (index >= pdpa->cp)  
    {      
        if (!DPA_Grow(pdpa, index + 1))  
            return FALSE;       
        pdpa->cp = index + 1;    
    }     
    pdpa->pp[index] = p;   
    return TRUE;
} 

int DPA_InsertPtr(HDPA pdpa, int index, void * p) 
{
     if (index < 0) 
     {   
         return -1;    
     }   
     if (index > pdpa->cp)   
         index = pdpa->cp;      
     // Make sure we have room for one more item     
     //     
     if (pdpa->cp + 1 > pdpa->cpAlloc)   
     {  
         if (!DPA_Grow(pdpa, pdpa->cp + 1))   
             return -1;    
     }      
     // If we are inserting, we need to slide everybody up     
     //   
     if (index < pdpa->cp)   
     {    
         hmemcpy(&pdpa->pp[index + 1], &pdpa->pp[index], (pdpa->cp - index) * sizeof(void *)); 
     }     
 
     pdpa->pp[index] = p;
     pdpa->cp++;   
     return index; 
}  

void * DPA_DeletePtr(HDPA pdpa, int index)
{   
    void * p;  
    //    assert(IsDPA(pdpa));  
    if (index < 0 || index >= pdpa->cp) 
    {      
        //   DebugMsg(DM_ERROR, "DPA: Invalid index: %d", index);    
        return NULL;
    }   
    p = pdpa->pp[index];
    if (index < pdpa->cp - 1)
    {       
        hmemcpy(&pdpa->pp[index], &pdpa->pp[index + 1], (pdpa->cp - (index + 1)) * sizeof(void *));     
    }     
    pdpa->cp--;  
    if (pdpa->cpAlloc - pdpa->cp > pdpa->cpGrow)   
    {     
        void **ppNew;   
        ppNew = (void**)ControlReAlloc(pdpa->hheap, pdpa->pp, (pdpa->cpAlloc - pdpa->cpGrow) * sizeof(void *));    
        assert(ppNew);  
        pdpa->pp = ppNew;   
        pdpa->cpAlloc -= pdpa->cpGrow;  
   }  
    return p;
 } 

 BOOL DPA_DeleteAllPtrs(HDPA pdpa) 
 {    
     if (pdpa->pp && !ControlFree(pdpa->hheap, pdpa->pp))   
         return FALSE;
     pdpa->pp = NULL;   
     pdpa->cp = pdpa->cpAlloc = 0;   
     return TRUE; 
 }  
BOOL DPA_Sort(HDPA pdpa, PFNDPACOMPARE pfnCmp, LPARAM lParam)
 {   
    SORTPARAMS sp;  
    sp.cp = pdpa->cp;     
    sp.pp = pdpa->pp;   
    sp.pfnCmp = pfnCmp;   
    sp.lParam = lParam;      
    return DPA_MergeSort(&sp);
 }  
#define SortCompare(psp, pp1, i1, pp2, i2) \
 (psp->pfnCmp(pp1[i1], pp2[i2], psp->lParam))  
// 
//  This function merges two sorted lists and makes one sorted list. 
//   psp->pp[iFirst, iFirst+cItes/2-1], psp->pp[iFirst+cItems/2, iFirst+cItems-1] 
// 
void DPA_MergeThem(SORTPARAMS * psp, int iFirst, int cItems) 
{    
    //     
    // Notes:     
    //  This function is separated from DPA_MergeSort2() to avoid comsuming     
    // stack variables. Never inline this.     
    //   
    int cHalf = cItems/2;     
    int iIn1, iIn2, iOut; 
    LPVOID * ppvSrc = &psp->pp[iFirst];      
    // Copy the first part to temp storage so we can write directly into     
    // the final buffer.  Note that this takes at most psp->cp/2 DWORD's    
    hmemcpy(psp->ppT, ppvSrc, cHalf*sizeof(LPVOID));   
    for (iIn1=0, iIn2=cHalf, iOut=0;;)   
    {
        if (SortCompare(psp, psp->ppT, iIn1, ppvSrc, iIn2) <= 0)
        { 
            ppvSrc[iOut++] = psp->ppT[iIn1++];    
            if (iIn1==cHalf)
            { 
                // We used up the first half; the rest of the second half 
                // should already be in place 
                break;  
            }
        }
        else
        { 
            ppvSrc[iOut++] = ppvSrc[iIn2++];   
            if (iIn2==cItems) 
            { 
               // We used up the second half; copy the rest of the first half 
               // into place
                hmemcpy(&ppvSrc[iOut], &psp->ppT[iIn1], (cItems-iOut)*sizeof(LPVOID));
                break;   
            } 
        } 
    } 
}  

// 
//  This function sorts a give list (psp->pp[iFirst,iFirst-cItems-1]). 
//

 void DPA_MergeSort2(SORTPARAMS * psp, int iFirst, int cItems) {     
   //     
   // Notes:     
   //   This function is recursively called. Therefore, we should minimize     

   //  the number of local variables and parameters. At this point, we     
   //  use one local variable and three parameters.     
   //    
     int cHalf;   
     switch(cItems)     
    {   
     case 1: return;
     case 2: 
        // Swap them, if they are out of order. 
         if (SortCompare(psp, psp->pp, iFirst, psp->pp, iFirst+1) > 0)
         {   
             psp->ppT[0] = psp->pp[iFirst]; 
             psp->pp[iFirst] = psp->pp[iFirst+1];    
             psp->pp[iFirst+1] = psp->ppT[0];
         } 
         break;    
     default: 
         cHalf = cItems/2; 
        // Sort each half
         DPA_MergeSort2(psp, iFirst, cHalf); 
         DPA_MergeSort2(psp, iFirst+cHalf, cItems-cHalf); 
        // Then, merge them. 
         DPA_MergeThem(psp, iFirst, cItems); 
         break;  
     }
 }  
 
BOOL DPA_MergeSort(SORTPARAMS * psp) 
{  
    if (psp->cp==0) 
       return TRUE;      
   // Note that we divide by 2 below; we want to round down   
   psp->ppT = (void**)LocalAlloc(LPTR, psp->cp/2 * sizeof(LPVOID)); 
   if (!psp->ppT) return FALSE;   
   DPA_MergeSort2(psp, 0, psp->cp);   
   LocalFree(psp->ppT);  
   return TRUE; 

}  
// Search function 
//
int DPA_Search(HDPA pdpa, void * pFind, int iStart,             
        PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT options) 
{  
    int cp = DPA_GetPtrCount(pdpa);     
    assert(pfnCompare);   
    assert(0 <= iStart);      

   // Only allow these wierd flags if the list is sorted  
    assert((options & DPAS_SORTED) || !(options & (DPAS_INSERTBEFORE | DPAS_INSERTAFTER)));  
    if (!(options & DPAS_SORTED))  
    {         
        // Not sorted: do lisearch.       
        int i;     
        for (i = iStart; i < cp; i++)   
        {        
            if (0 == pfnCompare(pFind, DPA_FastGetPtr(pdpa, i), lParam))     
                return i;  
        }      
        return -1;   
    }   
    else     
    {         
        // Search the array using binary search.  If several adjacent          
        // elements match the target element, the index of the first         
        // matching element is returned.  
        int iRet = -1;      
        // assume no match   
        BOOL bFound = FALSE;  
        int nCmp = 0;      
        int iLow = 0;       
        // Don't bother using iStart for binary search 
        int iMid = 0;      
        int iHigh = cp - 1;          
        // (OK for cp == 0)    
        while (iLow <= iHigh)   
        {          
            iMid = (iLow + iHigh) / 2;        
            nCmp = pfnCompare(pFind, DPA_FastGetPtr(pdpa, iMid), lParam);       
            if (0 > nCmp)   
                iHigh = iMid - 1;       
            // First is smaller      
            else if (0 < nCmp)    
                iLow = iMid + 1;        
            // First is larger 
            else         
            {                 
                // Match; search back for first match        
                bFound = TRUE;     
                while (0 < iMid)                 
                {
                    if (0 != pfnCompare(pFind, DPA_FastGetPtr(pdpa, iMid-1), lParam))   
                        break;
                    else     
                        iMid--;   
                }           
                break;     
            }         
        }   
        if (bFound)  
        {       
            assert(0 <= iMid);   
            iRet = iMid;       
        }          
        // Did the search fail AND         
        // is one of the strange search flags set?   
        if (!bFound && (options & (DPAS_INSERTAFTER | DPAS_INSERTBEFORE)))      
        {            
            // Yes; return the index where the target should be inserted             
            // if not found       
            if (0 < nCmp)       
                // First is larger           
                iRet = iLow;     
            else 
                iRet = iMid;             
            // (We don't distinguish between the two flags anymore)   
        }       
        else if ( !(options & (DPAS_INSERTAFTER | DPAS_INSERTBEFORE)) )    
        {             
            // Sanity check with lisearch       
            assert(DPA_Search(pdpa, pFind, iStart, pfnCompare, lParam, options & ~DPAS_SORTED) == iRet);         
        }
        return iRet;     
    } 
}  

//=========================================================================== 
// 
// String ptr management routines 
// 
// Copy as much of *psz to *pszBuf as will fit 
// 
int Str_GetPtr(LPCSTR psz, LPSTR pszBuf, int cchBuf)
{ 
    int cch = 0;      
    // if pszBuf is NULL, just return length of string.     
    //    
    if (!pszBuf && psz)       
        return MyStrlen(psz);   
    if (cchBuf)
    {         
        if (psz) 
        {           
            cch = MyStrlen(psz);    
            if (cch > cchBuf - 1)           
                cch = cchBuf - 1;    
            hmemcpy(pszBuf, psz, cch);  
        }  
        pszBuf[cch] = 0; 
    }   
    return cch; 
}

BOOL Str_Set(LPSTR *ppsz, LPCSTR psz) 
{ 
    if (!psz) 
    {        
        if (*ppsz)      
        {        
            LocalFree(*ppsz); 
            *ppsz = NULL;      
        } 
    }    
    else  
    { 
        LPSTR pszNew;
        UINT cbSize = MyStrlen(psz) + 1; 
        if (*ppsz)   
            pszNew = (char*)LocalReAlloc(*ppsz, cbSize, LMEM_MOVEABLE | LMEM_ZEROINIT);
        else    
            pszNew = (char*)LocalAlloc(LPTR, cbSize);   
        if (!pszNew)  
            return FALSE;   
       lstrcpy(pszNew, psz);  
       *ppsz = pszNew;   
    }    
    return TRUE; 
}  
// Set *ppsz to a copy of psz, reallocing as needed 
//

BOOL Str_SetPtr(LPSTR * ppsz, LPCSTR psz) 
{    
    if (!psz)  
    {        
        if (*ppsz)  
        {          
            Free(*ppsz);   
            *ppsz = NULL;      
       }    
     }   
    else    
    {    
        LPSTR pszNew = (LPSTR)ReAlloc(*ppsz, MyStrlen(psz) + 1);
        if (!pszNew)        
            return FALSE;    
        lstrcpy(pszNew, psz);   
        *ppsz = pszNew;  
   }    
    return TRUE;
 }  
 
