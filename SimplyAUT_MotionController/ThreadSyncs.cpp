#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "ThreadSyncs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMyWinThread::CMyWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam) : 
	CWinThread(pfnThreadProc, pParam)
	{
	m_dwCreateFlagsFast = 0;
	}

CMyWinThread::CMyWinThread() : CWinThread()
	{
	m_dwCreateFlagsFast = 0;
	}

CMyWinThread::~CMyWinThread()
	{
	if( m_hThread )
		CloseHandle(m_hThread);

	m_hThread = NULL;
	}

#ifdef AFX_CORE1_SEG
#pragma code_seg(AFX_CORE1_SEG)
#endif

// here only bevcause can't see it in program files
struct _AFX_THREAD_STARTUP
{
	// following are "in" parameters to thread startup
	_AFX_THREAD_STATE* pThreadState;    // thread state of parent thread
	CWinThread* pThread;    // CWinThread3 for new thread
	DWORD dwCreateFlags;    // thread creation flags
	_PNH pfnNewHandler;     // new handler for new thread

	HANDLE hEvent;          // event triggered after success/non-success
	HANDLE hEvent2;         // event triggered after thread is resumed

	// strictly "out" -- set after hEvent is triggered
	BOOL bError;    // TRUE if error during startup
};

// here so can set to critical prior to getting started
BOOL CMyWinThread::CreateThread(DWORD dwCreateFlags, UINT nStackSize,
	LPSECURITY_ATTRIBUTES lpSecurityAttrs)
	{
	extern UINT APIENTRY _AfxThreadEntry(void* pParam);
#ifndef _MT
	dwCreateFlags;
	nStackSize;
	lpSecurityAttrs;

	return FALSE;
#else
	ASSERT(m_hThread == NULL);  // already created?

	// setup startup structure for thread initialization
	_AFX_THREAD_STARTUP startup; memset(&startup, 0, sizeof(startup));
	startup.pThreadState = AfxGetThreadState();
	startup.pThread = this;
	startup.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	startup.hEvent2 = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	startup.dwCreateFlags = dwCreateFlags;
	if (startup.hEvent == NULL || startup.hEvent2 == NULL)
	{
		TRACE0("Warning: CreateEvent failed in CWinThread3::CreateThread.\n");
		if (startup.hEvent != NULL)
			::CloseHandle(startup.hEvent);
		if (startup.hEvent2 != NULL)
			::CloseHandle(startup.hEvent2);
		return FALSE;
	}

	// create the thread (it may or may not start to run)
	m_hThread = (HANDLE)_beginthreadex(lpSecurityAttrs, nStackSize, 
		&_AfxThreadEntry, &startup, dwCreateFlags | CREATE_SUSPENDED, (UINT*)&m_nThreadID);
	if (m_hThread == NULL)
		return FALSE;

	// this is the only actual change in all of this
	if( m_dwCreateFlagsFast & CREATE_FAST )
		::SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);

	// start the thread just for MFC initialization
	VERIFY(ResumeThread() != (DWORD)-1);

	VERIFY(::WaitForSingleObject(startup.hEvent, INFINITE) == WAIT_OBJECT_0);
	::CloseHandle(startup.hEvent);

	// if created suspended, suspend it until resume thread wakes it up
	if (dwCreateFlags & CREATE_SUSPENDED)
		VERIFY(::SuspendThread(m_hThread) != (DWORD)-1);


	// if error during startup, shut things down
	if (startup.bError)
	{
		VERIFY(::WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		::CloseHandle(startup.hEvent2);
		return FALSE;
	}

	// allow thread to continue, once resumed (it may already be resumed)
	::SetEvent(startup.hEvent2);
	return TRUE;
#endif //!_MT
}

CWinThread* AFXAPI AfxMyBeginThreadNoDelete(AFX_THREADPROC pfnThreadProc, 
	LPVOID					pParam,
	int						nPriority/*=THREAD_PRIORITY_NORMAL*/, 
	UINT					nStackSize/*=0*/, 
	DWORD					dwCreateFlags/*=0*/,
	LPSECURITY_ATTRIBUTES	lpSecurityAttrs/*=NULL*/)
	{

	return AfxMyBeginThread(pfnThreadProc, pParam, nPriority, nStackSize, dwCreateFlags|CREATE_NOAUTODELETE, lpSecurityAttrs);
	}

CWinThread* AFXAPI AfxMyBeginThread(AFX_THREADPROC pfnThreadProc, 
	LPVOID					pParam,
	int						nPriority/*=THREAD_PRIORITY_NORMAL*/, 
	UINT					nStackSize/*=0*/, 
	DWORD					dwCreateFlags/*=0*/,
	LPSECURITY_ATTRIBUTES	lpSecurityAttrs/*=NULL*/)
	{
#ifndef _MT
	pfnThreadProc;
	pParam;
	nPriority;
	nStackSize;
	dwCreateFlags;
	lpSecurityAttrs;

	return NULL;
#else
	ASSERT(pfnThreadProc != NULL);

	// changed to create a child CWindThread Object
	// optionally remove the auto-delete
	// remove the bit incase it affects CreateThread() 
	CMyWinThread* pThread = new CMyWinThread(pfnThreadProc, pParam);
	pThread->m_dwCreateFlagsFast = dwCreateFlags;

	// now that noted remove these new bits
	dwCreateFlags &= ~(CREATE_NOAUTODELETE|CREATE_FAST);

	// remove the auto-delete, and remove bit incase used by system
	if( pThread->m_dwCreateFlagsFast & CREATE_NOAUTODELETE )
		pThread->m_bAutoDelete = FALSE;

	ASSERT_VALID(pThread);

	if (!pThread->CreateThread(dwCreateFlags|CREATE_SUSPENDED, nStackSize,
		lpSecurityAttrs))
		{
		pThread->Delete();
		return NULL;
		}

	VERIFY(pThread->SetThreadPriority(nPriority));
	if (!(dwCreateFlags & CREATE_SUSPENDED))
		VERIFY(pThread->ResumeThread() != (DWORD)-1);

	return pThread;
#endif //!_MT)
	}


