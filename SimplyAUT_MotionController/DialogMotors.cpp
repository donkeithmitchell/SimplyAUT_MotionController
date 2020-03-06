// DialogMotors.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogMotors.h"
#include "afxdialogex.h"
#include "Gclib2.h""
#include "resource.h"

// CDialogMotors dialog

IMPLEMENT_DYNAMIC(CDialogMotors, CDialogEx)

CDialogMotors::CDialogMotors(const GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MOTORS, pParent)
	, m_nGalilState(nState)

	, m_szScanSpeed(_T("10.0"))
	, m_szScanAccel(_T("10.0"))
	, m_fScanSpeed(10.0)
	, m_fScanAccel(10.0)

	, m_szMotorA(_T(""))
	, m_szMotorB(_T(""))
	, m_szMotorC(_T(""))
	, m_szMotorD(_T(""))
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
}

CDialogMotors::~CDialogMotors()
{
}

void CDialogMotors::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN_SPEED, m_spinScanSpeed);
	DDX_Control(pDX, IDC_SPIN_ACCEL, m_spinScanAccel);

	DDX_Text(pDX, IDC_EDIT_SPEED, m_szScanSpeed);
	DDX_Text(pDX, IDC_EDIT_SPEED, m_fScanSpeed);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fScanSpeed, 1.0, 100.0);
	}

	DDX_Text(pDX, IDC_EDIT_ACCEL, m_szScanAccel);
	DDX_Text(pDX, IDC_EDIT_ACCEL, m_szScanAccel);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fScanAccel, 1.0, 100.0);
	}

	DDX_Text(pDX, IDC_EDIT_MA, m_szMotorA);
	DDX_Text(pDX, IDC_EDIT_MB, m_szMotorB);
	DDX_Text(pDX, IDC_EDIT_MC, m_szMotorC);
	DDX_Text(pDX, IDC_EDIT_MD, m_szMotorD);
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
	m_fScanSpeed += (inc > 0) ? 0.1 : -0.1;
	m_fScanSpeed = min(max(m_fScanSpeed, 0.1), 100);
	UpdateData(FALSE);

	*pResult = 0;
}

void CDialogMotors::OnDeltaposSpinScanAccel(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fScanAccel += (inc > 0) ? 0.1 : -0.1;
	m_fScanAccel = min(max(m_fScanAccel, 0.1), 100);
	UpdateData(FALSE);

	*pResult = 0;
}

void CDialogMotors::EbableControls()
{
	GetDlgItem(IDC_EDIT_SPEED)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_ACCEL)->EnableWindow(m_nGalilState == GALIL_IDLE);

}



