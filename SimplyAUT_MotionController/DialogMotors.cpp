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

IMPLEMENT_DYNAMIC(CDialogMotors, CDialogEx)

CDialogMotors::CDialogMotors(CMotionControl& motion, CMagControl& mag, const GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MOTORS, pParent)
	, m_motionControl(motion)
	, m_magControl(mag)
	, m_nGalilState(nState)

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
	DDX_Text(pDX, IDC_STATIC_MOTOR_A, m_szMotorA);
	DDX_Text(pDX, IDC_STATIC_MOTOR_B, m_szMotorB);
	DDX_Text(pDX, IDC_STATIC_MOTOR_C, m_szMotorC);
	DDX_Text(pDX, IDC_STATIC_MOTOR_D, m_szMotorD);
}


BEGIN_MESSAGE_MAP(CDialogMotors, CDialogEx)
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

	m_bInit = TRUE;
	PostMessage(WM_SIZE);
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

	m_szMotorA.Format("%5.1f (%4.0f)", fSA, fPosA);
	m_szMotorB.Format("%5.1f (%4.0f)", fSB, fPosB);
	m_szMotorC.Format("%5.1f (%4.0f)", fSC, fPosC);
	m_szMotorD.Format("%5.1f (%4.0f)", fSD, fPosD);

	UpdateData(FALSE);
}


