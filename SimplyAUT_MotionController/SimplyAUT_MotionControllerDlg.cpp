
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



CSimplyAUTMotionControllerDlg::CSimplyAUTMotionControllerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SIMPLYAUT_MOTIONCONTROLLER_DIALOG, pParent)
	, m_dlgGirthWeld(m_galil_state)
	, m_dlgMotors(m_galil_state)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_galil_state = GALIL_IDLE;
}

void CSimplyAUTMotionControllerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_tabControl);
}

BEGIN_MESSAGE_MAP(CSimplyAUTMotionControllerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CSimplyAUTMotionControllerDlg::OnSelchangeTab1)
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
	m_tabControl.InsertItem(0, CString("Connect"));
	m_tabControl.InsertItem(1, CString("Motors"));
	m_tabControl.InsertItem(2, CString("Scan"));
	m_tabControl.InsertItem(3, CString("Status"));
	m_tabControl.SetCurSel(0);

	m_dlgConnect.Create(&m_tabControl);
	m_dlgMotors.Create(&m_tabControl);
	m_dlgGirthWeld.Create(&m_tabControl);
	m_dlgStatus.Create(&m_tabControl);

	m_bInit = TRUE;
	PostMessage(WM_SIZE);
	OnSelchangeTab2();

	return TRUE;  // return TRUE  unless you set the focus to a control
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

	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

	GetDlgItem(IDOK)->GetClientRect(&rect);
	int cx1 = rect.Width();
	int cy1 = rect.Height();
	GetDlgItem(IDOK)->MoveWindow(2, cy - cy1 - 2, cx1, cy1);

	GetDlgItem(IDCANCEL)->GetClientRect(&rect);
	int cx2 = rect.Width();
	int cy2 = rect.Height();
	GetDlgItem(IDCANCEL)->MoveWindow(cx-cx1-2, cy - cy1 - 2, cx1, cy1);

	m_tabControl.MoveWindow(2, 2, cx - 4, cy - cy1 - 6);
	m_tabControl.GetClientRect(&rect);
	int cx3 = rect.Width();
	int cy3 = rect.Height();

	m_dlgConnect.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgMotors.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgGirthWeld.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
	m_dlgStatus.MoveWindow(2, 28, cx3 - 4, cy3 - 30);
}



void CSimplyAUTMotionControllerDlg::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	OnSelchangeTab2();
	*pResult = 0;
}

void CSimplyAUTMotionControllerDlg::OnSelchangeTab2()
{
	// TODO: Add your control notification handler code here
	int nSel = m_tabControl.GetCurSel();
	ASSERT(IsWindow(m_dlgConnect.m_hWnd));
	ASSERT(IsWindow(m_dlgMotors.m_hWnd));
	ASSERT(IsWindow(m_dlgGirthWeld.m_hWnd));
	ASSERT(IsWindow(m_dlgStatus.m_hWnd));

	m_dlgConnect.ShowWindow(nSel == 0 ? SW_SHOW : SW_HIDE);
	m_dlgMotors.ShowWindow(nSel == 1 ? SW_SHOW : SW_HIDE);
	m_dlgGirthWeld.ShowWindow(nSel == 2 ? SW_SHOW : SW_HIDE);
	m_dlgStatus.ShowWindow(nSel == 3 ? SW_SHOW : SW_HIDE);
}
