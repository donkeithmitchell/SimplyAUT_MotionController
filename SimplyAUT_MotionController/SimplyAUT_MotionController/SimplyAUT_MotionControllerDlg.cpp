
// SimplyAUT_MotionControllerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "afxdialogex.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSimplyAUTMotionControllerDlg dialog
// this dialog contains all the other dialogs in a tab
// with _DEBUG there are exstra dialog shown
CSimplyAUTMotionControllerDlg::CSimplyAUTMotionControllerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SIMPLYAUT_MOTIONCONTROLLER_DIALOG, pParent)
	, m_dlgConnect(m_motionControl, m_laserControl, m_magControl)
	, m_dlgMotors(m_motionControl, m_magControl, m_galil_state)
	, m_dlgGirthWeld(m_motionControl, m_laserControl, m_magControl, m_galil_state, m_pid)
	, m_dlgLaser(m_motionControl, m_laserControl)
	, m_dlgNavigation(m_pid)
	, m_dlgMag(m_motionControl, m_laserControl, m_magControl)
	, m_szErrorMsg(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_nSel = 0;
	m_galil_state = GALIL_IDLE;

	m_motionControl.Init(this, WM_DEGUG_MSG);
	m_laserControl.Init(this, WM_DEGUG_MSG);
	m_magControl.Init(this, WM_DEGUG_MSG);

	m_dlgConnect.Init(this, WM_DEGUG_MSG);
	m_dlgGirthWeld.Init(this, WM_DEGUG_MSG);
	m_dlgMotors.Init(this, WM_DEGUG_MSG);
	m_dlgFiles.Init(this, WM_DEGUG_MSG);
#ifdef _DEBUG_TIMING_
	m_dlgNavigation.Init(this, WM_DEGUG_MSG);
	m_dlgLaser.Init(this, WM_DEGUG_MSG);
	m_dlgMag.Init(this, WM_DEGUG_MSG);
	m_dlgStatus.Init(this, WM_DEGUG_MSG);
#endif
}

void CSimplyAUTMotionControllerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_tabControl);
	DDX_Text(pDX, IDC_EDIT_STATUS, m_szErrorMsg);
}

BEGIN_MESSAGE_MAP(CSimplyAUTMotionControllerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CSimplyAUTMotionControllerDlg::OnSelchangeTab1)
	ON_MESSAGE(WM_DEGUG_MSG, OnUserDebugMessage)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_RESET_STATUS, &CSimplyAUTMotionControllerDlg::OnClickedButtonResetStatus)
END_MESSAGE_MAP()


// CSimplyAUTMotionControllerDlg message handlers

BOOL CSimplyAUTMotionControllerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_tabControl.InsertItem(TAB_CONNECT, CString("Connect"));
	m_tabControl.InsertItem(TAB_MOTORS, CString("Motors"));
	m_tabControl.InsertItem(TAB_SCAN, CString("Scan"));
	m_tabControl.InsertItem(TAB_FILES, CString("Files"));

	// these are for debug purposes only
#ifdef _DEBUG_TIMING_
	m_tabControl.InsertItem(TAB_NAVIGATION, CString("Navigation"));
	m_tabControl.InsertItem(TAB_LASER, CString("Laser"));
	m_tabControl.InsertItem(TAB_MAG, CString("Mag"));
	m_tabControl.InsertItem(TAB_STATUS, CString("Status"));
#endif
	m_tabControl.SetCurSel(m_nSel);

	m_dlgConnect.Create(&m_tabControl);
	m_dlgMotors.Create(&m_tabControl);
	m_dlgGirthWeld.Create(&m_tabControl);
	m_dlgFiles.Create(&m_tabControl);
#ifdef _DEBUG_TIMING_
	m_dlgNavigation.Create(&m_tabControl);
	m_dlgLaser.Create(&m_tabControl);
	m_dlgMag.Create(&m_tabControl);
	m_dlgStatus.Create(&m_tabControl);
#endif

	GetDlgItem(IDC_BUTTON_RESET_STATUS)->ShowWindow(SW_HIDE);

	m_bInit = TRUE;
	PostMessage(WM_SIZE);
	StartReadMagStatus(TRUE);
	OnSelchangeTab2();

	Serialize(FALSE);

	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL); // 911
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// will turn this off while navigating
void CSimplyAUTMotionControllerDlg::StartReadMagStatus(BOOL bOn)
{
	if (bOn)
		SetTimer(TIMER_GET_MAG_STATUS, 500, NULL);
	else
		KillTimer(TIMER_GET_MAG_STATUS);
}


void CSimplyAUTMotionControllerDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	switch (nIDEvent)
	{
	case TIMER_GET_MAG_STATUS:
		if( m_magControl.IsConnected() )
			m_magControl.GetMagStatus();
		break;
	}
}

void CSimplyAUTMotionControllerDlg::Serialize(BOOL bSave)
{
	TCHAR szBuff[512];
	CFile file1;

	char my_documents[MAX_PATH];
	HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	if (result != S_OK)
		return;

	// get a list of the existing files
	CString path;
	path.Format("%s\\SimplyAUTFiles\\Serialize.txt", my_documents);

	if (file1.Open(path, bSave ? CFile::modeCreate | CFile::modeWrite : CFile::modeRead))
	{
		CArchive ar(&file1, bSave ? CArchive::store : CArchive::load, 512, szBuff);
		Serialize(ar);
	}
}

void CSimplyAUTMotionControllerDlg::OnOK()
{
	Serialize(TRUE);
	EndDialog(IDOK);
}

void CSimplyAUTMotionControllerDlg::OnCancel()
{
	Serialize(TRUE);
	EndDialog(IDCANCEL);
}

void CSimplyAUTMotionControllerDlg::Serialize(CArchive& ar)
{
	UpdateData(TRUE);
	m_dlgConnect.Serialize(ar);
	m_dlgGirthWeld.Serialize(ar);
#ifdef _DEBUG_TIMING_
	m_dlgMag.Serialize(ar);
	m_dlgLaser.Serialize(ar);
	m_dlgNavigation.Serialize(ar);
#endif
	UpdateData(FALSE);
}
LRESULT CSimplyAUTMotionControllerDlg::OnUserDebugMessage(WPARAM wParam, LPARAM lParam)
{
	if (m_bInit)
		switch (wParam)
		{
		case MSG_SEND_DEBUGMSG: // receivbing address to a CString
		{
			const CString* pMsg = (CString*)lParam;
			// now pass this to the status window
			m_dlgStatus.AppendDebugMessage(*pMsg);
			break;
		}
		case MSG_ERROR_MSG: // receivbing address to a CString
		{
			const char* str = (char*)lParam;
			AppendErrorMessage(str);
			break;
		}
		case MSG_SETBITMAPS: // enable the various controlds
		{
			m_dlgConnect.EnableControls();
			m_dlgMotors.EnableControls();
			m_dlgGirthWeld.EnableControls();
			m_dlgFiles.EnableControls();
#ifdef _DEBUG_TIMING_
			m_dlgNavigation.EnableControls();
			m_dlgLaser.EnableControls();
			m_dlgMag.EnableControls();
			m_dlgStatus.EnableControls();
#endif
			break;
		}
		case MSG_GETSCANSPEED:
		{
			double* pSpeed = (double*)lParam;
			*pSpeed = m_dlgGirthWeld.GetMotorSpeed();
			break;
		}
		case MSG_GETACCEL:
		{
			double* pAccel = (double*)lParam;
			*pAccel = m_dlgGirthWeld.GetMotorAccel();
			break;
		}
		case MSG_MAG_STATUS_ON:
			StartReadMagStatus(lParam);
			break;
		case MSG_UPDATE_FILE_LIST:
			m_dlgFiles.UpdateFileList();
			break;
		}

	return 0;
}


void CSimplyAUTMotionControllerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSimplyAUTMotionControllerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSimplyAUTMotionControllerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSimplyAUTMotionControllerDlg::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	CDialogEx::OnSize(nFlag, cx, cy);

	if (!m_bInit)
		return;

	BOOL bStatus = m_tabControl.GetCurSel() == TAB_STATUS;
	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

	GetDlgItem(IDC_BUTTON_RESET_STATUS)->GetClientRect(&rect);
	int cx2 = rect.Width();
	int cy2 = rect.Height();

	GetDlgItem(IDC_EDIT_STATUS)->GetClientRect(&rect);
	int cx1 = rect.Width();
	int cy1 = 3 * cy2; //  rect.Height();

	GetDlgItem(IDC_EDIT_STATUS)->MoveWindow(2, cy - cy1 - 2, cx - 4, cy1);
	GetDlgItem(IDC_BUTTON_RESET_STATUS)->MoveWindow(cx - cx2 - 2, cy - cy2 - 2, cx2, cy2);

	m_tabControl.MoveWindow(2, 2, cx - 4, cy - (bStatus ? cy2 : cy1) - 6);
	m_tabControl.GetClientRect(&rect);
	int cx3 = rect.Width();
	int cy3 = rect.Height();

	m_dlgConnect.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgMotors.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgGirthWeld.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgFiles.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
#ifdef _DEBUG_TIMING_
	m_dlgNavigation.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgLaser.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgMag.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgStatus.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
#endif
}



void CSimplyAUTMotionControllerDlg::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	if (CheckVisibleTab())
		OnSelchangeTab2();
	else
		m_tabControl.SetCurSel(m_nSel); // put back as there qwere errors in n edit box

	BOOL bStatus = m_tabControl.GetCurSel() == TAB_STATUS;
	GetDlgItem(IDC_BUTTON_RESET_STATUS)->ShowWindow(bStatus ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_EDIT_STATUS)->ShowWindow(bStatus ? SW_HIDE : SW_SHOW);

	*pResult = 0;
}

BOOL CSimplyAUTMotionControllerDlg::CheckVisibleTab()
{
	switch (m_nSel)
	{
	case TAB_CONNECT: return m_dlgConnect.CheckVisibleTab();
	case TAB_MOTORS: return m_dlgMotors.CheckVisibleTab();
	case TAB_SCAN: return m_dlgConnect.CheckVisibleTab();
	case TAB_FILES: return m_dlgFiles.CheckVisibleTab();
#ifdef _DEBUG_TIMING_
	case TAB_NAVIGATION: return m_dlgNavigation.CheckVisibleTab();
	case TAB_LASER: return m_dlgLaser.CheckVisibleTab();
	case TAB_MAG: return m_dlgMag.CheckVisibleTab();
	case TAB_STATUS: return m_dlgStatus.CheckVisibleTab();
#endif
	default: return FALSE;
	}

}

void CSimplyAUTMotionControllerDlg::OnSelchangeTab2()
{
	// TODO: Add your control notification handler code here
	// if changing from the laser tab, then turn the laser off
	if (m_nSel == TAB_LASER)
		m_laserControl.TurnLaserOn(FALSE);

	m_nSel = m_tabControl.GetCurSel();
	if (m_nSel == TAB_LASER)
		m_laserControl.TurnLaserOn(TRUE);

	ASSERT(IsWindow(m_dlgConnect.m_hWnd));
	ASSERT(IsWindow(m_dlgMotors.m_hWnd));
	ASSERT(IsWindow(m_dlgGirthWeld.m_hWnd));
	ASSERT(IsWindow(m_dlgFiles.m_hWnd));
#ifdef _DEBUG_TIMING_
	ASSERT(IsWindow(m_dlgNavigation.m_hWnd));
	ASSERT(IsWindow(m_dlgLaser.m_hWnd));
	ASSERT(IsWindow(m_dlgMag.m_hWnd));
	ASSERT(IsWindow(m_dlgStatus.m_hWnd));
#endif

	m_dlgConnect.ShowWindow(m_nSel == TAB_CONNECT ? SW_SHOW : SW_HIDE);
	m_dlgMotors.ShowWindow(m_nSel == TAB_MOTORS ? SW_SHOW : SW_HIDE);
	m_dlgGirthWeld.ShowWindow(m_nSel == TAB_SCAN ? SW_SHOW : SW_HIDE);
	m_dlgFiles.ShowWindow(m_nSel == TAB_FILES ? SW_SHOW : SW_HIDE);
#ifdef _DEBUG_TIMING_
	m_dlgNavigation.ShowWindow(m_nSel == TAB_NAVIGATION ? SW_SHOW : SW_HIDE);
	m_dlgLaser.ShowWindow(m_nSel == TAB_LASER ? SW_SHOW : SW_HIDE);
	m_dlgMag.ShowWindow(m_nSel == TAB_MAG ? SW_SHOW : SW_HIDE);
	m_dlgStatus.ShowWindow(m_nSel == TAB_STATUS ? SW_SHOW : SW_HIDE);
#endif
	// this will causew the sdizing of the laser to be adjusted
	PostMessage(WM_SIZE);
	m_dlgGirthWeld.PostMessage(WM_SIZE);
}

// if this error message is the same as the previous one, do not add again
// if pass NULL, then remove any noted errors
// add the latest error message to the start, so on tiop
void CSimplyAUTMotionControllerDlg::AppendErrorMessage(const char* szMsg)
{
	UpdateData(TRUE);
	if (szMsg == NULL)
	{
		m_szErrorMsg = _T("");
		UpdateData(FALSE);
	}
	else
	{
		CString temp = _T(szMsg);
		temp = temp.TrimLeft();
		temp = temp.TrimRight();
		int len = temp.GetLength();
		if (len == 0)
			return;

		if (temp[len - 1] == '\n')
			temp = temp.Left(--len);

		// if this is identical to the last entry then do not re add
		if (m_szErrorMsg.Left(len).Compare(temp) != 0)
		{
			temp = temp + CString("\r\n") + m_szErrorMsg;
			m_szErrorMsg = temp;
			UpdateData(FALSE);
		}
	}
}

// inform the satatus dialog to reset the noted status to datye
void CSimplyAUTMotionControllerDlg::OnClickedButtonResetStatus()
{
	// TODO: Add your control notification handler code here
#ifdef _DEBUG_TIMING_
	m_dlgStatus.ResetStatus();
#endif
}
