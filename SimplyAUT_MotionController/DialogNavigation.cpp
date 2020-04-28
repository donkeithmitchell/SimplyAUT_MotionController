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
	ON_BN_CLICKED(IDC_BUTTON_CALC_PI, &CDialogNavigation::OnButtonCalcPI)
	ON_BN_CLICKED(IDC_BUTTON_CALC_PID2, &CDialogNavigation::OnButtonCalcPID)
	ON_BN_CLICKED(IDC_BUTTON_CALC_ENABLE, &CDialogNavigation::OnButtonCalcEnable)
	
	ON_BN_CLICKED(IDC_RADIO1, &CDialogNavigation::OnClickNavType)
	ON_BN_CLICKED(IDC_RADIO2, &CDialogNavigation::OnClickNavType)
	ON_EN_CHANGE(IDC_EDIT_NAV_P, &CDialogNavigation::OnEditChangePID)
	ON_EN_CHANGE(IDC_EDIT_NAV_I, &CDialogNavigation::OnEditChangePID)
	ON_EN_CHANGE(IDC_EDIT_NAV_D, &CDialogNavigation::OnEditChangePID)
END_MESSAGE_MAP()

void CDialogNavigation::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO1, m_pid.nav_type);

	DDX_Text(pDX, IDC_EDIT_NAV_P, m_pid.P);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.P, 0.1, 100.0);
	DDX_Text(pDX, IDC_EDIT_NAV_I, m_pid.I);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.I, 0, 1.0);
	DDX_Text(pDX, IDC_EDIT_NAV_D, m_pid.D);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.D, 0, 2000);
	DDX_Text(pDX, IDC_EDIT_NAV_D_LEN, m_pid.D_LEN);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.D_LEN, 5, 20);
	DDX_Text(pDX, IDC_EDIT_NAV_PIVOT, m_pid.pivot);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.pivot, 0.1, 0.9);
	DDX_Text(pDX, IDC_EDIT_NAV_DELAY, m_pid.turn_time);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.turn_time, 5, 1000);
	DDX_Text(pDX, IDC_EDIT_NAV_MAX_TURN, m_pid.max_turn);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.max_turn, 0.5, MIN_TURN_RATE);

	if (!pDX->m_bSaveAndValidate)
		m_szPID_Tu.Format("%.1f", m_pid.Tu);

	DDX_Text(pDX, IDC_STATIC_NAV_TU, m_szPID_Tu);
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
	GetDlgItem(IDC_EDIT_NAV_P)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_I)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_D)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_PIVOT)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_DELAY)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_BUTTON_CALC_PI)->EnableWindow(m_pid.nav_type == 0 && m_pid.P == NAVIGATION_P_OSCILLATE && m_pid.I == 0 && m_pid.D == 0 && m_pid.Tu > 0);
	GetDlgItem(IDC_BUTTON_CALC_PID2)->EnableWindow(m_pid.nav_type == 0 && m_pid.P == NAVIGATION_P_OSCILLATE && m_pid.I == 0 && m_pid.D == 0 && m_pid.Tu > 0);
	GetDlgItem(IDC_BUTTON_CALC_ENABLE)->EnableWindow(m_pid.nav_type == 0);
	UpdateData(FALSE);
}

void CDialogNavigation::OnEditChangePID()
{
	CString szP, szI, szD;
	GetDlgItem(IDC_EDIT_NAV_P)->GetWindowText(szP);
	GetDlgItem(IDC_EDIT_NAV_I)->GetWindowText(szP);
	GetDlgItem(IDC_EDIT_NAV_D)->GetWindowText(szP);

	GetDlgItem(IDC_BUTTON_CALC_PI)->EnableWindow(m_pid.nav_type == 0 && atof(szP) == NAVIGATION_P_OSCILLATE && atof(szI) == 0 && atof(szD) == 0 && m_pid.Tu > 0);
	GetDlgItem(IDC_BUTTON_CALC_PID2)->EnableWindow(m_pid.nav_type == 0 && atof(szP) == NAVIGATION_P_OSCILLATE && atof(szI) == 0 && atof(szD) == 0 && m_pid.Tu > 0);
}


// CDialogLaser message handlers
void CDialogNavigation::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}

void	CDialogNavigation::Serialize(CArchive& ar) 
{
	const int MASK = 0xCDCDCDCD;
	int mask = MASK;
	if (ar.IsStoring())
	{
		UpdateData(TRUE);
		ar << m_pid.nav_type;
		ar << m_pid.P;
		ar << m_pid.I;
		ar << m_pid.D;
		ar << m_pid.max_turn;
		ar << m_pid.pivot;
		ar << m_pid.D_LEN;
		ar << m_pid.Tu;
		ar << mask;
	}
	else
	{
#ifdef _DEBUG_TIMING_
		try
		{
			ar >> m_pid.nav_type;
			ar >> m_pid.P;
			ar >> m_pid.I;
			ar >> m_pid.D;
			ar >> m_pid.max_turn;
			ar >> m_pid.pivot;
			ar >> m_pid.D_LEN;
			ar >> m_pid.Tu;
			ar >> mask;
		}
		catch (CArchiveException * e1)
		{
			m_pid.Reset();
			e1->Delete();

		}
		if (mask != MASK)
			m_pid.Reset();
#else
		m_pid.Reset();
#endif
		UpdateData(FALSE);
	}

}



// CDialogNavigation message handlers

void CDialogNavigation::OnButtonCalcEnable()
{
	UpdateData(TRUE);

	m_pid.Reset();
	m_pid.P = NAVIGATION_P_OSCILLATE;
	m_pid.I = 0.0;
	m_pid.D = 0.0;
	UpdateData(FALSE);
	EnableControls();
}


void CDialogNavigation::OnButtonCalcPI()
{
	UpdateData(TRUE);

	if (m_pid.Tu > 0)
	{
		double Ku = NAVIGATION_P_OSCILLATE;
		double Kp = Ku * 0.45;
		double Ki = 0.54 * Ku / m_pid.Tu;
		double Kd = 0.0;

		m_pid.P = Kp;
		m_pid.I = Ki;
		m_pid.D = Kd;
	}
	UpdateData(FALSE);
}

void CDialogNavigation::OnButtonCalcPID()
{
	UpdateData(TRUE);

	if (m_pid.Tu > 0)
	{
		double Ku = NAVIGATION_P_OSCILLATE;
		double Kp = Ku * 0.60;
		double Ki = 1.20 * Ku / m_pid.Tu;
		double Kd = 3.0 * Ku * m_pid.Tu / 40.0;

		m_pid.P = Kp;
		m_pid.I = Ki;
		m_pid.D = Kd;
	}
	UpdateData(FALSE);
}


void CDialogNavigation::OnButtonReset()
{
	// TODO: Add your command handler code here
	UpdateData(TRUE);
	m_pid.Reset();
	UpdateData(FALSE);
	EnableControls();
}

void CDialogNavigation::OnClickNavType()
{
	UpdateData(TRUE);
	EnableControls();
}


