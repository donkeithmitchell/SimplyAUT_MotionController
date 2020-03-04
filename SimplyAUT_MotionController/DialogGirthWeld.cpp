// DialogGirthWeld.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogGirthWeld.h"
#include "afxdialogex.h"
#include "resource.h"

// CDialogGirthWeld dialog

IMPLEMENT_DYNAMIC(CDialogGirthWeld, CDialogEx)

CDialogGirthWeld::CDialogGirthWeld(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_GIRTHWELD, pParent)
	, m_szSpeed(_T("10.0"))
	, m_szLROffset(_T("0.0"))
	, m_szCirc(_T("100.0"))

	, m_fSpeed(10.0)
	, m_fLROffset(0.0)
	, m_fCirc(100.0)
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_bPaused = FALSE;
	m_galil_state = GALIL_IDLE;
}

CDialogGirthWeld::~CDialogGirthWeld()
{
}

void CDialogGirthWeld::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN_SPEED, m_spinSpeed);
	DDX_Control(pDX, IDC_SPIN_LR_OFFSET, m_spinLROffset);
	DDX_Control(pDX, IDC_SPIN_CIRC, m_spinCirc);

	DDX_Text(pDX, IDC_EDIT_SPEED, m_szSpeed);
	DDX_Text(pDX, IDC_EDIT_SPEED, m_fSpeed);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fSpeed, 1.0, 100.0);
	}
	DDX_Text(pDX, IDC_EDIT_LR_OFFSET, m_szLROffset);
	DDX_Text(pDX, IDC_EDIT_LR_OFFSET, m_fLROffset);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fLROffset, 0.0, 10.0);
	}
	DDX_Text(pDX, IDC_EDIT_CIRC, m_szCirc);
	DDX_Text(pDX, IDC_EDIT_CIRC, m_fCirc);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fCirc, 100.0, 10000.0);
	}
	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_buttonPause);
	DDX_Control(pDX, IDC_BUTTON_CALIB, m_buttonCalib);
	DDX_Control(pDX, IDC_BUTTON_MANUAL, m_buttonManual);
	DDX_Control(pDX, IDC_BUTTON_AUTO, m_buttonAuto);
	DDX_Control(pDX, IDC_BUTTON_BACK, m_buttonBack);
	DDX_Control(pDX, IDC_BUTTON_FWD, m_buttonFwd);
	DDX_Control(pDX, IDC_STATIC_LASER, m_staticLaser);
	DDX_Control(pDX, IDC_BUTTON_SET_HOME, m_buttonSetHome);
	DDX_Control(pDX, IDC_BUTTON_GO_HOME, m_buttonGoHome);
}


BEGIN_MESSAGE_MAP(CDialogGirthWeld, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_SPEED, &CDialogGirthWeld::OnDeltaposSpinSpeed)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LR_OFFSET, &CDialogGirthWeld::OnDeltaposSpinLrOffset)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_CIRC, &CDialogGirthWeld::OnDeltaposSpinCirc)

	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CDialogGirthWeld::OnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_CALIB, &CDialogGirthWeld::OnClickedButtonCalib)
	ON_BN_CLICKED(IDC_BUTTON_MANUAL, &CDialogGirthWeld::OnClickedButtonManual)
	ON_BN_CLICKED(IDC_BUTTON_AUTO, &CDialogGirthWeld::OnClickedButtonAuto)
	ON_BN_CLICKED(IDC_BUTTON_FWD, &CDialogGirthWeld::OnClickedButtonFwd)
	ON_BN_CLICKED(IDC_BUTTON_BACK, &CDialogGirthWeld::OnClickedButtonBack)
	ON_BN_CLICKED(IDC_BUTTON_SET_HOME, &CDialogGirthWeld::OnClickedButtonSetHome)
	ON_BN_CLICKED(IDC_BUTTON_GO_HOME, &CDialogGirthWeld::OnClickedButtonGoHome)
END_MESSAGE_MAP()


// CDialogGirthWeld message handlers
BOOL CDialogGirthWeld::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here
	m_spinSpeed.SetRange(0, UD_MAXVAL);
	m_spinLROffset.SetRange(0, UD_MAXVAL);
	m_spinCirc.SetRange(0, UD_MAXVAL);

	m_bitmapPause.LoadBitmapW(IDB_BITMAP_PAUSE);
	m_bitmapGoHorz.LoadBitmapW(IDB_BITMAP_GO_HORZ);
	m_bitmapGoVert.LoadBitmapW(IDB_BITMAP_GO_VERT);
	m_bitmapStop.LoadBitmapW(IDB_BITMAP_STOP);

	m_wndLaser.Create(&m_staticLaser);

	m_bInit = TRUE;
	SetButtonBitmaps();
	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogGirthWeld::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_GIRTHWELD, pParent);
	ShowWindow(SW_HIDE);
}


void CDialogGirthWeld::OnDeltaposSpinSpeed(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fSpeed += (inc > 0) ? 0.1 : -0.1;
	m_fSpeed = min(max(m_fSpeed, 0.1), 100);
	UpdateData(FALSE);

	*pResult = 0;
}


void CDialogGirthWeld::OnDeltaposSpinLrOffset(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fLROffset += (inc > 0) ? 0.1 : -0.1;
	m_fLROffset = min(max(m_fLROffset, 0.0), 10);
	UpdateData(FALSE);

	*pResult = 0;
}

void CDialogGirthWeld::OnDeltaposSpinCirc(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fCirc += (inc > 0) ? 1 : -1;
	m_fCirc = min(max(m_fCirc, 100.0), 10000);
	UpdateData(FALSE);

	*pResult = 0;
}


void CDialogGirthWeld::SetButtonBitmaps()
{
	// TODO: Add your control notification handler code here
	HBITMAP hBitmap1 = !m_bPaused ? (HBITMAP)m_bitmapPause.GetSafeHandle() : (HBITMAP)m_bitmapGoHorz.GetSafeHandle();
	m_buttonPause.SetBitmap(hBitmap1);

	m_buttonCalib.SetBitmap(hBitmap1);
	m_buttonManual.SetBitmap(hBitmap1);
	m_buttonAuto.SetBitmap(hBitmap1);

	// these are set to stop if clicked on
	HBITMAP hBitmapHorz = (HBITMAP)m_bitmapGoHorz.GetSafeHandle();
	HBITMAP hBitmapVert = (HBITMAP)m_bitmapGoVert.GetSafeHandle();
	HBITMAP hBitmapStop = (HBITMAP)m_bitmapStop.GetSafeHandle();
	HBITMAP hBitmapPause = (HBITMAP)m_bitmapPause.GetSafeHandle();

	// set to stop if running
	m_buttonCalib.SetBitmap((m_galil_state == GALIL_CALIB) ? hBitmapStop : hBitmapHorz);
	m_buttonManual.SetBitmap((m_galil_state == GALIL_MANUAL) ? hBitmapStop : hBitmapHorz);
	m_buttonAuto.SetBitmap((m_galil_state == GALIL_AUTO) ? hBitmapStop : hBitmapHorz);
	m_buttonGoHome.SetBitmap(m_galil_state == GALIL_GOHOME ? hBitmapStop : hBitmapHorz);

	// fowd and back are either vertical or stop
	m_buttonBack.SetBitmap(m_galil_state == GALIL_BACK ? hBitmapStop : hBitmapVert);
	m_buttonFwd.SetBitmap(m_galil_state == GALIL_FWD ? hBitmapStop : hBitmapVert);

	// set to 
	m_buttonCalib.EnableWindow(m_galil_state == GALIL_CALIB || m_galil_state == GALIL_IDLE);
	m_buttonManual.EnableWindow(m_galil_state == GALIL_MANUAL || m_galil_state == GALIL_IDLE);
	m_buttonAuto.EnableWindow(m_galil_state == GALIL_AUTO || m_galil_state == GALIL_IDLE);
	m_buttonPause.EnableWindow(m_galil_state == GALIL_CALIB || m_galil_state == GALIL_MANUAL || m_galil_state == GALIL_AUTO);
	m_buttonGoHome.EnableWindow(m_galil_state == GALIL_GOHOME || m_galil_state == GALIL_IDLE);
	m_buttonSetHome.EnableWindow(m_galil_state == GALIL_IDLE);

	m_buttonBack.EnableWindow(m_galil_state == GALIL_IDLE || m_galil_state == GALIL_BACK || m_bPaused);
	m_buttonFwd.EnableWindow(m_galil_state == GALIL_IDLE || m_galil_state == GALIL_FWD || m_bPaused);

	GetDlgItem(IDC_EDIT_SPEED)->EnableWindow(m_galil_state == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_LR_OFFSET)->EnableWindow(m_galil_state == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_CIRC)->EnableWindow(m_galil_state == GALIL_IDLE);
	GetDlgItem(IDC_STATIC_PAUSE)->SetWindowText(m_bPaused ? _T("Resume" : _T("Pause")));
}


void CDialogGirthWeld::OnClickedButtonPause()
{
	m_bPaused = !m_bPaused;
	SetButtonBitmaps();
}

void CDialogGirthWeld::OnClickedButtonCalib()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_CALIB))
	{
		m_galil_state = (m_galil_state == GALIL_IDLE) ? GALIL_CALIB : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonManual()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_MANUAL))
	{
		m_galil_state = (m_galil_state == GALIL_IDLE) ? GALIL_MANUAL : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonAuto()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_AUTO))
	{
		m_galil_state = (m_galil_state == GALIL_IDLE) ? GALIL_AUTO : GALIL_IDLE;
		SetButtonBitmaps();
	}
}
void CDialogGirthWeld::OnClickedButtonGoHome()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_GOHOME))
	{
		m_galil_state = (m_galil_state == GALIL_IDLE) ? GALIL_GOHOME : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonBack()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_BACK))
	{
		m_galil_state = (m_galil_state == GALIL_IDLE) ? GALIL_BACK : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonFwd()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_FWD))
	{
		m_galil_state = (m_galil_state == GALIL_IDLE) ? GALIL_FWD : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonSetHome()
{
	// TODO: Add your control notification handler code here
}


BOOL CDialogGirthWeld::CheckParameters()
{

	// if currently idel, then check if have valid parameters before proceeding
	if (m_galil_state == GALIL_IDLE)
	{
		m_bCheck = TRUE;
		int ret = UpdateData(TRUE);
		m_bCheck = FALSE;
		return ret;
	}
	else
		return TRUE;
}

BOOL CDialogGirthWeld::CheckIfToRunOrStop(GALIL_STATE nState)
{
	int ret = IDOK;
	switch (m_galil_state)
	{
	// if now idle, wait until the recording APP is ready to record
	case GALIL_IDLE:
	{
		switch (nState)
		{
		case GALIL_CALIB:
				ret = AfxMessageBox(_T("Begin Calibration Run"), MB_OKCANCEL);
				break;
		case GALIL_MANUAL:
			ret = AfxMessageBox(_T("Begin Manual Run"), MB_OKCANCEL);
			break;
		case GALIL_AUTO:
			ret = AfxMessageBox(_T("Begin Auto Run"), MB_OKCANCEL);
			break;
		case GALIL_GOHOME:
			ret = AfxMessageBox(_T("Return to Home"), MB_OKCANCEL);
			break;
		}
		break;
	}
	case GALIL_CALIB:
		ret = AfxMessageBox(_T("Stop the Calibration Run"), MB_OKCANCEL);
		break;
	case GALIL_MANUAL:
		ret = AfxMessageBox(_T("Stop the Manual Run"), MB_OKCANCEL);
		break;
	case GALIL_AUTO:
		ret = AfxMessageBox(_T("Stop the Auto Run"), MB_OKCANCEL);
		break;
	case GALIL_GOHOME:
		ret = AfxMessageBox(_T("Stop the Return to Home"), MB_OKCANCEL);
		break;
	}

	return (ret == IDOK);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDialogGirthWeld::OnPaint()
{
	CDialogEx::OnPaint();
}


// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogGirthWeld::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

	m_staticLaser.GetClientRect(&rect);
	int cx1 = rect.Width();
	int cy1 = rect.Height();
	m_wndLaser.MoveWindow(0, 0, cx1, cy1);

}



