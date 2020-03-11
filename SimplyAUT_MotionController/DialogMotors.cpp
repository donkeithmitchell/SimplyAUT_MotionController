// DialogMotors.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogMotors.h"
#include "afxdialogex.h"
#include "Gclib2.h"
#include "MotionControl.h"
#include "resource.h"

// CDialogMotors dialog

IMPLEMENT_DYNAMIC(CDialogMotors, CDialogEx)

CDialogMotors::CDialogMotors(CMotionControl& motion, const GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MOTORS, pParent)
	, m_motionControl(motion)
	, m_nGalilState(nState)

	, m_szMotorSpeed(_T("50.0"))
	, m_szMotorAccel(_T("25.0"))
	, m_fMotorSpeed(50.0)
	, m_fMotorAccel(25.0)

	, m_szMotorA(_T(""))
	, m_szMotorB(_T(""))
	, m_szMotorC(_T(""))
	, m_szMotorD(_T(""))
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_bInit = FALSE;
	m_bCheck = FALSE;
}

CDialogMotors::~CDialogMotors()
{
}

void CDialogMotors::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}



void CDialogMotors::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN_SPEED, m_spinScanSpeed);
	DDX_Control(pDX, IDC_SPIN_ACCEL, m_spinScanAccel);

	DDX_Text(pDX, IDC_EDIT_SPEED, m_szMotorSpeed);
	DDX_Text(pDX, IDC_EDIT_SPEED, m_fMotorSpeed);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fMotorSpeed, 1.0, 100.0);
	}

	DDX_Text(pDX, IDC_EDIT_ACCEL, m_szMotorAccel);
	DDX_Text(pDX, IDC_EDIT_ACCEL, m_fMotorAccel);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fMotorAccel, 1.0, 100.0);
	}

	DDX_Text(pDX, IDC_STATIC_MOTOR_A, m_szMotorA);
	DDX_Text(pDX, IDC_STATIC_MOTOR_B, m_szMotorB);
	DDX_Text(pDX, IDC_STATIC_MOTOR_C, m_szMotorC);
	DDX_Text(pDX, IDC_STATIC_MOTOR_D, m_szMotorD);
}


BEGIN_MESSAGE_MAP(CDialogMotors, CDialogEx)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_SPEED, &CDialogMotors::OnDeltaposSpinScanSpeed)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ACCEL, &CDialogMotors::OnDeltaposSpinScanAccel)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogMotors::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

}

BOOL CDialogMotors::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here
	m_spinScanSpeed.SetRange(0, UD_MAXVAL);
	m_spinScanAccel.SetRange(0, UD_MAXVAL);

	m_bInit = TRUE;
	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogMotors::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_MOTORS, pParent);
	ShowWindow(SW_HIDE);
}


void CDialogMotors::OnDeltaposSpinScanSpeed(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fMotorSpeed += (inc > 0) ? 0.1 : -0.1;
	m_fMotorSpeed = min(max(m_fMotorSpeed, 0.1), 100);
	UpdateData(FALSE);

	if (m_motionControl.AreMotorsRunning())
		m_motionControl.SetMotorJogging(m_fMotorSpeed, m_fMotorAccel);

	*pResult = 0;
}

void CDialogMotors::OnDeltaposSpinScanAccel(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fMotorAccel += (inc > 0) ? 0.1 : -0.1;
	m_fMotorAccel = min(max(m_fMotorAccel, 0.1), 100);
	UpdateData(FALSE);

	if (m_motionControl.AreMotorsRunning())
		m_motionControl.SetMotorJogging(m_fMotorSpeed, m_fMotorAccel);

	*pResult = 0;
}

void CDialogMotors::EbableControls()
{
	GetDlgItem(IDC_EDIT_SPEED)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_ACCEL)->EnableWindow(m_nGalilState == GALIL_IDLE);

}

void CDialogMotors::EnableControls()
{
}

BOOL CDialogMotors::CheckVisibleTab()
{
	m_bCheck = TRUE;
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret;
}

double CDialogMotors::GetMotorSpeed()
{
	m_bCheck = TRUE;
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret ? m_fMotorSpeed : FLT_MAX;
}
double CDialogMotors::GetMotorAccel()
{
	m_bCheck = TRUE;
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret ? m_fMotorAccel : FLT_MAX;
}

void CDialogMotors::ShowMotorSpeeds()
{
	UpdateData(TRUE);

	double fAA, fSA = m_motionControl.GetMotorSpeed("A", fAA);
	double fAB, fSB = m_motionControl.GetMotorSpeed("B", fAB);
	double fAC, fSC = m_motionControl.GetMotorSpeed("C", fAC);
	double fAD, fSD = m_motionControl.GetMotorSpeed("D", fAD);

	m_szMotorA.Format("%.1f (%.1f)", fSA, fAA);
	m_szMotorB.Format("%.1f (%.1f)", fSB, fAB);
	m_szMotorC.Format("%.1f (%.1f)", fSC, fAC);
	m_szMotorD.Format("%.1f (%.1f)", fSD, fAD);
	UpdateData(FALSE);
}


