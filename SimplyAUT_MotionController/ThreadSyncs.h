#pragma once
#include <afxmt.h>         // MFC core and standard components
#include "afxwin.h"
#include <ddeml.h>  // for MSGF_DDEMGR
#include <process.h>

#define CREATE_FAST			0x40000000
#define CREATE_NOAUTODELETE 0x80000000

CWinThread* AFXAPI AfxMyBeginThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam, int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0, DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);
CWinThread* AFXAPI AfxMyBeginThreadNoDelete(AFX_THREADPROC pfnThreadProc, LPVOID pParam, int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0, DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);


// overload CreateThread()
class CMyWinThread : public CWinThread
{
	//	DECLARE_DYNAMIC(CMyWinThread)

public:
	// Constructors
	CMyWinThread();
	~CMyWinThread();
	CMyWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam);

	BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
		LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);

	DWORD	m_dwCreateFlagsFast;
};