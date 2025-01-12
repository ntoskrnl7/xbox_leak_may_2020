//
// Xbox Xbcp Tool
//
// [colint]
//

#include "stdafx.h"		// our standard AFX include
#pragma hdrstop
#include "toolxbcp.h"	// our local header file

#ifdef XBCP

#include <bldapi.h>
#include <bldguid.h>

IMPLEMENT_DYNAMIC(CXbcpTool, CSchmoozeTool)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CXbcpTool::CXbcpTool() : CSchmoozeTool()
{
	// tool name
	m_nIDName = IDS_XBCP_TOOL;
	m_nIDUIName = IDS_XBCP_TOOL_UI;

	// tool exe name and input file set
	m_strToolExeName = _TEXT("xbecopy.exe");
	m_strToolInput = _TEXT("*.xbe");
	m_strToolPrefix = _TEXT("XBCP");

	// deferred tool
	m_fDeferredTool = TRUE;

	// Olympus 53: DON'T perform action if able
	ASSERT(m_fPerformIfAble == FALSE);
}

///////////////////////////////////////////////////////////////////////////////
// Return the list of files generated by the mecr tool
BOOL CXbcpTool::GenerateOutput(UINT type, CActionSlobList & lstActions, CErrorContext & EC)
{
	POSITION posAction = lstActions.GetHeadPosition();
	while (posAction != (POSITION)NULL)
	{
		CActionSlob * pAction = (CActionSlob *)lstActions.GetNext(posAction);
		CProjItem * pItem = pAction->m_pItem;

		CPath * pPath;
		if ((pPath = pItem->GetProject()->GetTargetFileName()) == (CPath *)NULL)
			continue;

		int i;
		VERIFY(pItem->GetProject()->GetIntProp(P_DeferredMecr, i));
		if (i == dabNotDeferred)
		{
			// write out to .trg to makefile if mfile is not deferred
			CPath pathName;
			pathName = *pPath;
			pathName.ChangeExtension(_TEXT("TRG"));

			if (!pAction->AddOutput((const CPath *)&pathName))
				ASSERT(FALSE);
		}
		delete pPath;
	}

	return TRUE; // success
}

UINT CXbcpTool::PerformBuildActions(UINT type, UINT stage, CActionSlobList & lstActions,
									  DWORD aob, CErrorContext & EC)
{
	UINT act = ACT_Complete;
    BOOL fCreate = stage == TOB_Stage && type != TOB_Clean;
    BOOL fClean = stage == TOB_Pre && type == TOB_Clean;

	act = CBuildTool::PerformBuildActions(type, stage, lstActions, aob, EC);

	// finished building browser database
	if ((fCreate || fClean) && act != ACT_Error)
	{
		// set timestamp on dummy .TRG output on *our* filesystem
		POSITION posAction = lstActions.GetHeadPosition();
		while (posAction != (POSITION)NULL)
		{
			CActionSlob * pAction = (CActionSlob *)lstActions.GetNext(posAction);
			
			CFileRegSet * pSet = pAction->GetOutput();
			FileRegHandle frh;
			pSet->InitFrhEnum();
			while ((frh = pSet->NextFrh()) != (FileRegHandle)NULL) {
                if(fCreate) {
				    HANDLE hdle = ::CreateFile
									    ((const TCHAR *)*pAction->m_pregistry->GetRegEntry(frh)->GetFilePath(),
									     GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, 0L);

				    if (hdle != INVALID_HANDLE_VALUE)
					    VERIFY(::CloseHandle(hdle));
                }
                if(fClean)
                    DeleteFile((const TCHAR *)*pAction->m_pregistry->GetRegEntry(frh)->GetFilePath());
#ifdef REFCOUNT_WORK
				frh->ReleaseFRHRef();
#endif
			}
		}
	}

	return act;
}

BOOL CXbcpTool::GetCommandOptions(CActionSlob * pAction, CString & strOptions)
{
 	g_prjoptengine.SetOptionHandler(pAction->m_pTool->GetOptionHandler());
	g_prjoptengine.SetPropertyBag(pAction->m_pItem);
	if (!g_prjoptengine.GenerateString(strOptions))
		return FALSE;

	return TRUE;	// ok
}

BOOL CXbcpTool::GetCommandLines
(
	CActionSlobList & lstActions,
	CPtrList &plCommandLines,
	DWORD attrib,
	CErrorContext & EC
)
{
	CString strDescription;

	POSITION posAction = lstActions.GetHeadPosition();
	while (posAction != (POSITION)NULL)
	{
		CActionSlob * pAction = (CActionSlob *)lstActions.GetNext(posAction);
 		CProjItem * pItem = pAction->Item();

		// project directory of this item
		CDir * pProjDir = &pItem->GetProject()->GetWorkspaceDir();

		CPath *pPathTarget = (CPath *)pItem->GetProject()->GetTargetFileName();

		BOOL fTargetFile = FALSE;

		// add our inputs to the command-line
		CFileRegSet * pset = (CFileRegSet *)pAction->GetInput();
		ASSERT(pset->IsKindOf(RUNTIME_CLASS(CFileRegSet)));

		FileRegHandle frhInput;
		pset->InitFrhEnum();
		CString strInput;
		while ((frhInput = pset->NextFrh()) != (FileRegHandle)NULL)
		{
			VERIFY(strDescription.LoadString(IDS_DESC_XBCPMAKING));
			const CPath * pPath = pAction->m_pregistry->GetRegEntry(frhInput)->GetFilePath();
#ifdef REFCOUNT_WORK
			frhInput->ReleaseFRHRef();
#endif
			CString strExt = pPath->GetExtension();

			// generate tool options
			// HACK, HACK, HACK for the ole mfile tool and exe projects/external targets
 			CString strCmdLine;
			CProject * pProject = pAction->m_pItem->GetProject();
			if (pProject->IsExeProject() || pProject->IsExternalTarget())
				VERIFY(strCmdLine.LoadString(IDS_XBCP_COMMON));
			// Ok generate the tool options the proper way.
			else if (!pAction->GetCommandOptions(strCmdLine))
			{
				delete pPathTarget;
				return FALSE;
			}

			if (_tcsicmp(pPathTarget->GetFullPath(), pPath->GetFullPath()) == 0)
				fTargetFile = TRUE;

			//delete pPathTarget;
            //pPathTarget = NULL;

			// quote this input and make relative to project directory?
			if (!pPath->GetRelativeName(*pProjDir, strInput, TRUE, FALSE))
			{
				// Use absolute path, quoted
				strInput = _T('"');
				strInput += *pPath;
				strInput += _T('"');
			}

			// base file name
			CString strBaseName = pPath->GetFileName();
							
			if ( !fTargetFile ){
				continue;
			}
			strCmdLine += _T(' ');
			strCmdLine += strInput;

			// Make sure we have a remote executable name...
			if (!pItem->GetProject()->GetInitialRemoteTarget())
				return FALSE;

			// append the name of our remote target
			CString strRemoteTarget;
			if (!pItem->GetProject()->GetStrProp(P_RemoteTarget, strRemoteTarget))
				return FALSE;

			// for the primary target, use the name in remote target
			strCmdLine += _TEXT(" \"");
			strCmdLine += strRemoteTarget;
			strDescription += strRemoteTarget;
			fTargetFile = FALSE;

			strCmdLine += _T('"');

			// may create a tmp. file if cmdline too long
			if (!g_buildengine.FormCmdLine(m_strToolExeName, strCmdLine, EC))
				return FALSE;	// error

			// add this to our list of command-lines
			CCmdLine *pCmdLine = new CCmdLine;
			pCmdLine->slCommandLines.AddTail(strCmdLine);
			pCmdLine->slDescriptions.AddTail(strDescription);
			pCmdLine->nProcessor = 1;

			plCommandLines.AddTail(pCmdLine);
		}
		delete pPathTarget;

	}

	return TRUE;	// success
}

#if 0
DAB CXbcpTool::DeferredAction(CActionSlob * pAction, CString * pstrAction)
{
	int iDefer;
	if (pAction->m_pItem->GetProject()->GetIntProp(P_DeferredMecr, iDefer) != valid)
	{
		ASSERT(FALSE);	// shouldn't happen
		iDefer = 0;
	}

/*
	if (pstrAction != (CString *)NULL)
		ASSERT(FALSE);	// FUTURE:
*/

	switch (iDefer)
	{
		case 0:	return dabNotDeferred;
		case 1:	return dabDeferred;
		default: ASSERT(FALSE); return dabNotDeferred;
	}
}

BOOL CXbcpTool::SetDeferredAction(CActionSlob * pAction, DAB dab)
{
	int iDefer;
	switch (dab)
	{
		case dabNotDeferred:
			iDefer = 0; break;

		case dabDeferred:
			iDefer = 1; break;

		default:
			ASSERT(FALSE); iDefer = 0; break;
	}

	return pAction->m_pItem->GetProject()->SetIntProp(P_DeferredMecr, iDefer);
}
#endif

const CSchmoozeTool::SchmoozeData CXbcpTool::m_XbcpSchmoozeData =
{
		_TEXT ("XBCP"),		 		//pszExeMacro;
		_TEXT ("XBCP_FLAGS"),  		//pszFlagsMacro;
		_TEXT (""),						//pszSpecialFlags;
		_TEXT ("XBCP_FILES"),			//pszConsumeMacro;
		_TEXT (""),						//pszSpecialConsume;
		_TEXT (" $(XBCP)"),			//pszExeMacroEx;
		_TEXT (" $(XBCP_FLAGS)"),   	//pszFlagsMacroEx;
		_TEXT (""),			//pszSpecialFlagsEx;
		_TEXT (" $(XBCP_FILES)"), 		//pszConsumeMacroEx;
		_TEXT (""),				//pszSpecialConsumeEx;
		_TEXT("trg"),					//pszProductExtensions;
		_TEXT(""),						//pszDelOnRebuildExtensions;
};

BOOL CXbcpTool::IsConsumableFile(const CPath * pPath)
{
	if (FileNameMatchesExtension (pPath, m_strToolInput))
		return TRUE;

	LPTSTR pszPathOut = NULL;
	LPBUILDSYSTEM pBldSysIFace;
	theApp.FindInterface(IID_IBuildSystem, (LPVOID FAR *)&pBldSysIFace);
	if (pBldSysIFace)
	{
		if (pBldSysIFace->IsActiveBuilderValid() == S_OK)
			pBldSysIFace->GetTargetFileName(ACTIVE_BUILDER, &pszPathOut);
		pBldSysIFace->Release();
	}
	if (pszPathOut)
	{
		BOOL bOkay = _tcsicmp(pPath->GetFullPath(), pszPathOut);
		delete [] pszPathOut;
		return bOkay;
	}
	else
		return FALSE;
}

BOOL CXbcpTool::IsProductFile(const CPath * pPath)
{
	return FileNameMatchesExtension(pPath, m_XbcpSchmoozeData.pszProductExtensions);
}

BOOL CXbcpTool::IsDelOnRebuildFile(const CPath * pPath)
{
	return FileNameMatchesExtension(pPath, m_XbcpSchmoozeData.pszDelOnRebuildExtensions);
}
///////////////////////////////////////////////////////////////////////////////
static BOOL CXbcpToolTargetCallback(DWORD dw, FileRegHandle frh)
{
	return ((CSchmoozeTool *) dw)->IsProductFile(
				g_FileRegistry.GetRegEntry(frh)->GetFilePath()
			);
}

BOOL CXbcpTool::DoWriteBuildRule(CActionSlob * pAction)
{
	CString strTargets, strBuildLine;
	const SchmoozeData& sd = GetMacs();
	ASSERT (PresentInActiveConfig ( pAction->m_pItem ));
	
	TRY
	{
        /* First see whether we can fill the XBCP_DEST macro */
        CString strXbcpDest("XBCP_DEST");
		CString strRemoteTarget;

		LPBUILDSYSTEM pBldSysIFace;
		VERIFY(SUCCEEDED(theApp.FindInterface(IID_IBuildSystem, (LPVOID FAR *)&pBldSysIFace)));
		if (pBldSysIFace->IsActiveBuilderValid() == S_OK)
			pBldSysIFace->GetRemoteTargetFileName(ACTIVE_BUILDER, strRemoteTarget);
		pBldSysIFace->Release();
        if(!strRemoteTarget.IsEmpty()) {
            CString strOut = '"' + strRemoteTarget + '"';
            m_pMakWriter->WriteMacro(strXbcpDest, strOut);
        }

        CObList lstItems; lstItems.AddHead(pAction->m_pItem);
		MakeQuotedString(
						pAction->GetOutput(),
						strTargets,
						m_pBaseDir,
						TRUE,
						NULL,
						NULL,
                        NULL, FALSE,
                        &lstItems, FALSE
						);

		strBuildLine = (strBuildLine + sd.pszSpecialConsumeEx) + sd.pszConsumeMacroEx;
		m_pMakWriter->WriteDesc ( strTargets, strBuildLine);

		strBuildLine = CString (sd.pszExeMacroEx) + sd.pszFlagsMacroEx + sd.pszSpecialFlagsEx + sd.pszConsumeMacroEx + _TEXT(" $(") + strXbcpDest + _TEXT(")\r\n" );
		m_pMakWriter->Indent ();
		m_pMakWriter->WriteString (strBuildLine);
		m_pMakWriter->EndLine ();
		m_pMakWriter->EndLine ();
	}
	CATCH (CException, e)
	{
		strBuildLine.Empty (); strTargets.Empty();
		THROW_LAST ();
	}	
	END_CATCH

	return TRUE;
}

#endif // XBCP
