// DialogNavigation.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogNavigation.h"
#include "WeldNavigation.h"
#include "define.h"


// CDialogNavigation

IMPLEMENT_DYNAMIC(CDialogNavigation, CDialogEx)

CDialogNavigation::CDialogNavigation(NAVIGATION_PID& pid, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_NAVIGATION, pParent)
	, m_pid(pid)
{
	m_pParent = pParent;
	m_nMsg = 0;
	m_bCheck = FALSE;
	m_bInit = FALSE;
}

CDialogNavigation::~CDialogNavigation()
{
}


BEGIN_MESSAGE_MAP(CDialogNavigation, CWnd)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CDialogNavigation::OnButtonReset)
END_MESSAGE_MAP()

void CDialogNavigation::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_NAV_P, m_pid.P);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.P, 0.1, 10.0);
	DDX_Text(pDX, IDC_EDIT_NAV_I, m_pid.I);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.I, 0, 1.0);
	DDX_Text(pDX, IDC_EDIT_NAV_D, m_pid.D);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.D, 100, 2000);
	DDX_Text(pDX, IDC_EDIT_NAV_PIVOT, m_pid.pivot);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.pivot, 0.1, 0.9);
	DDX_Text(pDX, IDC_EDIT_NAV_DELAY, m_pid.turn_time);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.turn_time, 5, 1000);
}

// do not want to check control values on every call to UpdateData(TRUE)
// only when about to scan, etc.
BOOL CDialogNavigation::CheckVisibleTab()
{
	m_bCheck = TRUE; // enable DDV calls
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret;
}


void CDialogNavigation::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_NAVIGATION, pParent);
	ShowWindow(SW_HIDE);
}

void CDialogNavigation::EnableControls()
{
}


// CDialogLaser message handlers
void CDialogNavigation::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}



// CDialogNavigation message handlers


void CDialogNavigation::OnButtonReset()
{
	// TODO: Add your command handler code here
	UpdateData(TRUE);
	m_pid.Reset();
	UpdateData(FALSE);
}


