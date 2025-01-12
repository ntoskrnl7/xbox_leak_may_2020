//      Copyright (c) 1996-1999 Microsoft Corporation
// UMSynth.h : Declaration of CUserModeSynth

#ifndef __UMSYNTH_H_
#define __UMSYNTH_H_

#include <ks.h>
#include <ksproxy.h>
#include <mmsystem.h>
#include "dmusicc.h"
#include "dmusics.h"
#include "CSynth.h"
#include "synth.h"
#include "..\shared\xsoundp.h" // For IDirectSoundSource
#include "..\shared\dmusiccp.h" // For class ids.
class CUserModeSynth;

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
#if 0 // The following section will only take affect in the DDK sample.
// @@END_DDKSPLIT
// {F5038F82-C052-11D2-872F-00600893B1BD}
DEFINE_GUID(CLSID_DDKSynth, 
0xf5038f82, 0xc052, 0x11d2, 0x87, 0x2f, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT

typedef HRESULT (CUserModeSynth::*GENPROPHANDLER)(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG cbBuffer);

#define GENPROP_F_STATIC                0x00000000
#define GENPROP_F_FNHANDLER             0x00000001


#include <pshpack4.h>
struct GENERICPROPERTY
{
    const GUID *pguidPropertySet;       // What property set?
    ULONG       ulId;                   // What item?

    ULONG       ulSupported;            // Get/Set flags for QuerySupported

    ULONG       ulFlags;                // GENPROP_F_xxx

    LPVOID      pPropertyData;          // Data to be returned
    ULONG       cbPropertyData;         // and its size    

    GENPROPHANDLER pfnHandler;          // Handler fn iff GENPROP_F_FNHANDLER
};
#include <poppack.h>


class CClock : public IReferenceClock
{
friend class CUserModeSynth;
public:

    CClock();

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /* IReferenceClock methods */
    HRESULT STDMETHODCALLTYPE GetTime( 
        /* [out] */ REFERENCE_TIME __RPC_FAR *pTime);
    
    HRESULT STDMETHODCALLTYPE AdviseTime( 
        /* [in] */ REFERENCE_TIME baseTime,
        /* [in] */ REFERENCE_TIME streamTime,
        /* [in] */ HANDLE hEvent,
        /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
    
    HRESULT STDMETHODCALLTYPE AdvisePeriodic( 
        /* [in] */ REFERENCE_TIME startTime,
        /* [in] */ REFERENCE_TIME periodTime,
        /* [in] */ HANDLE hSemaphore,
        /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
    
    HRESULT STDMETHODCALLTYPE Unadvise( 
        /* [in] */ DWORD dwAdviseCookie);
private:
    CUserModeSynth *	m_pParent;      // Pointer to parent structure.
    DWORD               m_dwLastPosition;
    LONGLONG            m_llSampleTime;
};


/////////////////////////////////////////////////////////////////////////////
// CDMSynth

class CUserModeSynth : public IDirectMusicSynthX, public IKsControl
{
friend class CClock;

public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IDirectMusicSynthX
public:
	virtual STDMETHODIMP Open(LPDMUS_PORTPARAMS pPortParams);
	virtual STDMETHODIMP Close(); 
	virtual STDMETHODIMP SetNumChannelGroups(DWORD dwGroups);
    virtual STDMETHODIMP SetMasterVolume(LONG lVolume);
    virtual STDMETHODIMP SendShortMsg(REFERENCE_TIME rt, DWORD dwGroup, DWORD dwMsg);
    virtual STDMETHODIMP SendLongMsg(REFERENCE_TIME rt, DWORD dwGroup, BYTE *pbMsg, DWORD dwLength);
//	virtual STDMETHODIMP Download(LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree);
//	virtual STDMETHODIMP Unload(HANDLE hDownload,
//								HRESULT ( CALLBACK *lpFreeHandle)(HANDLE,HANDLE),
//								HANDLE hUserData);

//	virtual STDMETHODIMP PlayBuffer(REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer);
//	virtual STDMETHODIMP GetRunningStats(LPDMUS_SYNTHSTATS pStats);
//	virtual STDMETHODIMP GetPortCaps(LPDMUS_PORTCAPS pCaps);
	virtual STDMETHODIMP GetMasterClock(IReferenceClock **ppClock);
//	virtual STDMETHODIMP GetLatencyClock(IReferenceClock **ppClock);
//	virtual STDMETHODIMP Activate(BOOL fEnable);
//	virtual STDMETHODIMP SetSynthSink(IDirectMusicSynthSink *pSynthSink);
//	virtual STDMETHODIMP Render(short *pBuffer, DWORD dwLength, LONGLONG llPosition);
    virtual STDMETHODIMP SetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority);
    virtual STDMETHODIMP GetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority);
//    virtual STDMETHODIMP GetFormat(LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize);
//    virtual STDMETHODIMP GetAppend(DWORD* pdwAppend);

	virtual STDMETHODIMP PlayWave(REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD dwChannel, PREL prPitch, VREL vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart, SAMPLE_TIME stLoopEnd, IDirectSoundWave *pIWave, DWORD *pdwVoiceID );
    virtual STDMETHODIMP StopWave(REFERENCE_TIME rt, DWORD dwVoiceId );
//    virtual STDMETHODIMP GetVoiceState(DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE VoiceState[] );
//    virtual STDMETHODIMP Refresh(DWORD dwDownloadID, DWORD dwFlags );
    virtual STDMETHODIMP AssignChannelToOutput(DWORD dwChannelGroup, DWORD dwChannel, IDirectSoundBuffer *pBuffer,DWORD dwMixBins, BYTE *pbControllers );

// IDirectSoundSource
/*public:
    virtual STDMETHODIMP GetFormat(LPWAVEFORMATEX pWaveFormatEx, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten);
    virtual STDMETHODIMP SetSink(IDirectSoundConnect* pSinkConnect);
    virtual STDMETHODIMP Seek(ULONGLONG sp);
    virtual STDMETHODIMP Read(LPVOID *ppvBuffer, LPDWORD pdwIDs, LPDWORD pdwFuncIDs, LPLONG plPitchBends, DWORD cpvBuffer, PULONGLONG pcb);
	virtual STDMETHODIMP GetSize(PULONGLONG pcb);*/

// IDirectSoundSource : members 
private:
//	ULONGLONG m_ullPosition;

// IKsControl
public:
 	virtual STDMETHODIMP KsProperty(
		PKSPROPERTY pPropertyIn, 
		ULONG ulPropertyLength,
		LPVOID pvPropertyData, 
		ULONG ulDataLength,
		PULONG pulBytesReturned);

    virtual STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );

    virtual STDMETHODIMP KsEvent(
        IN PKSEVENT Event,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );
    
	CUserModeSynth();
	~CUserModeSynth();
	HRESULT		Init();

private:
    CClock      m_MasterClock;  // Master clock for timing, derived from DSound.
	long		m_cRef;
	CSynth *	m_pSynth;		// Just one synth engine.
	BOOL		m_fActive;		// Currently active.
    LONG        m_lVolume;      // in 1/100 dB
    LONG        m_lBoost;       // in 1/100 dB
    LONG        m_lGainAdjust;  // in 1/100 dB
    CRITICAL_SECTION m_CriticalSection; // Critical section to manage access.

    HRESULT HandleSetVolume(
        ULONG               ulId, 
        BOOL                fSet, 
        LPVOID              pbBuffer, 
        PULONG              pcbBuffer);

    HRESULT HandleSetBoost(
        ULONG               ulId, 
        BOOL                fSet, 
        LPVOID              pbBuffer, 
        PULONG              pcbBuffer);
        
	HRESULT HandleReverb(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer);
	HRESULT HandleEffects(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer);
    HRESULT HandleGetSampleRate(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer);
    static GENERICPROPERTY m_aProperty[];
    static const int m_nProperty;
    //static GENERICPROPERTY *FindPropertyItem(REFGUID rguid, ULONG ulId);
    GENERICPROPERTY *FindPropertyItem(REFGUID rguid, ULONG ulId);
};

// Class factory
//
// Common to emulation/WDM.
// 
class CDirectMusicSynthFactory : public IClassFactory
{
public:
	// IUnknown
    //
	virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
	virtual STDMETHODIMP_(ULONG) AddRef();
	virtual STDMETHODIMP_(ULONG) Release();

	// Interface IClassFactory
    //
	virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
	virtual STDMETHODIMP LockServer(BOOL bLock); 

	// Constructor
    //
	CDirectMusicSynthFactory();

	// Destructor
	~CDirectMusicSynthFactory();

private:
	long m_cRef;
};



#endif //__UMSYNTH_H_
