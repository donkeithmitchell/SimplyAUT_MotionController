// DialogMotors.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogMotors.h"
#include "afxdialogex.h"
#include "Gclib2.h"
#include "MotionControl.h"
#include "MagController.h"
#include "resource.h"

// CDialogMotors dialog
// display the motor speeds and positions

IMPLEMENT_DYNAMIC(CDialogMotors, CDialogEx)

CDialogMotors::CDialogMotors(CMotionControl& motion, CMagControl& mag, const int& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MOTORS, pParent)
	, m_motionControl(motion)
	, m_magControl(mag)
	, m_nGalilState(nState)

	, m_szMotorA1(_T(""))
	, m_szMotorB1(_T(""))
	, m_szMotorC1(_T(""))
	, m_szMotorD1(_T(""))

	, m_szMotorA2(_T(""))
	, m_szMotorB2(_T(""))
	, m_szMotorC2(_T(""))
	, m_szMotorD2(_T(""))
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
	DDX_Text(pDX, IDC_STATIC_MOTOR_A, m_szMotorA1);
	DDX_Text(pDX, IDC_STATIC_MOTOR_B, m_szMotorB1);
	DDX_Text(pDX, IDC_STATIC_MOTOR_C, m_szMotorC1);
	DDX_Text(pDX, IDC_STATIC_MOTOR_D, m_szMotorD1);

	DDX_Text(pDX, IDC_STATIC_MOTOR_A2, m_szMotorA2);
	DDX_Text(pDX, IDC_STATIC_MOTOR_B2, m_szMotorB2);
	DDX_Text(pDX, IDC_STATIC_MOTOR_C2, m_szMotorC2);
	DDX_Text(pDX, IDC_STATIC_MOTOR_D2, m_szMotorD2);
}


BEGIN_MESSAGE_MAP(CDialogMotors, CDialogEx)
	ON_WM_SIZE()
	ON_WM_TIMER()
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

void CDialogMotors::OnTimer(UINT_PTR nIDEvent)
{
	if (IsWindowVisible() && m_motionControl.IsConnected() )
	{
		ShowMotorSpeeds();
	}
}

BOOL CDialogMotors::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here

	m_bInit = TRUE;
	PostMessage(WM_SIZE);
	SetTimer(1, 250, NULL);
	EnableControls();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogMotors::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_MOTORS, pParent);
	ShowWindow(SW_HIDE);
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


void CDialogMotors::ShowMotorSpeeds()
{
	UpdateData(TRUE);

	double fAA, fSA = m_motionControl.GetMotorSpeed("A", fAA);
	double fAB, fSB = m_motionControl.GetMotorSpeed("B", fAB);
	double fAC, fSC = m_motionControl.GetMotorSpeed("C", fAC);
	double fAD, fSD = m_motionControl.GetMotorSpeed("D", fAD);

	double fPosA = m_motionControl.GetMotorPosition("A");
	double fPosB = m_motionControl.GetMotorPosition("B");
	double fPosC = m_motionControl.GetMotorPosition("C");
	double fPosD = m_motionControl.GetMotorPosition("D");

	m_szMotorA1.Format("%5.1f", fSA);
	m_szMotorB1.Format("%5.1f", fSB);
	m_szMotorC1.Format("%5.1f", fSC);
	m_szMotorD1.Format("%5.1f", fSD);

	m_szMotorA2.Format("%4.0f", fPosA);
	m_szMotorB2.Format("%4.0f", fPosB);
	m_szMotorC2.Format("%4.0f", fPosC);
	m_szMotorD2.Format("%4.0f", fPosD);

	UpdateData(FALSE);
}


