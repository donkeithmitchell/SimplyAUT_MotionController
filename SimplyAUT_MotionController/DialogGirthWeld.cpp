// DialogGirthWeld.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogGirthWeld.h"
#include "afxdialogex.h"
#include "MotionControl.h"
#include "resource.h"

// CDialogGirthWeld dialog

IMPLEMENT_DYNAMIC(CDialogGirthWeld, CDialogEx)

CDialogGirthWeld::CDialogGirthWeld(CMotionControl& motion, GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_GIRTHWELD, pParent)
	, m_motionControl(motion)
	, m_nGalilState(nState)

	, m_szLROffset(_T("0.0"))
	, m_szScanCirc(_T("100.0"))
	, m_szDistToScan(_T("100.0"))
	, m_szDistScanned(_T("0.0"))
	, m_szScanOverlap(_T("50.0"))

	, m_fLROffset(0.0)
	, m_fScanCirc(100.0)
	, m_fDistToScan(100.0)
	, m_fDistScanned(100.0)
	, m_fScanOverlap(50.0)

	, m_szLaserHiLowUS(_T("0.0"))
	, m_szLaserHiLowDS(_T("0.0"))
	, m_szLaserHiLowDiff(_T("0.0"))
	, m_szLaserCapHeight(_T("0.0"))

	, m_nScanType(FALSE)
	, m_bReturnToHome(FALSE)
	, m_bReturnToStart(FALSE)
	, m_szHomeDist(_T("0.0 mm"))
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_bPaused = FALSE;
	m_nTimerCount = 0;
}

CDialogGirthWeld::~CDialogGirthWeld()
{
}

void CDialogGirthWeld::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN_LR_OFFSET,	m_spinLROffset);
	DDX_Control(pDX, IDC_SPIN_CIRC,			m_spinScanCirc);
	DDX_Control(pDX, IDC_SPIN_DIST,			m_spinScanDist);
	DDX_Control(pDX, IDC_SPIN_OVERLAP,		m_spinScanOverlap);

	DDX_Text(pDX, IDC_EDIT_LR_OFFSET,		m_szLROffset);
	DDX_Text(pDX, IDC_EDIT_LR_OFFSET,		m_fLROffset);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fLROffset, 0.0, 10.0);
	}
	DDX_Text(pDX, IDC_EDIT_CIRC,			m_szScanCirc);
	DDX_Text(pDX, IDC_EDIT_CIRC,			m_fScanCirc);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fScanCirc, 100.0, 10000.0);
	}
	DDX_Text(pDX, IDC_EDIT_DIST,			m_szDistToScan);
	DDX_Text(pDX, IDC_EDIT_DIST,			m_fDistToScan);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fDistToScan, 10.0, 10000.0);
	}

	DDX_Text(pDX, IDC_EDIT_OVERLAP,			m_szScanOverlap);
	DDX_Text(pDX, IDC_EDIT_OVERLAP,			m_fScanOverlap);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fScanOverlap, 0.0, 100.0);
	}

	DDX_Control(pDX, IDC_BUTTON_PAUSE,		m_buttonPause);
	DDX_Control(pDX, IDC_BUTTON_CALIB,		m_buttonCalib);
	DDX_Control(pDX, IDC_BUTTON_MANUAL,		m_buttonManual);
	DDX_Control(pDX, IDC_BUTTON_AUTO,		m_buttonAuto);
	DDX_Control(pDX, IDC_BUTTON_BACK,		m_buttonBack);
	DDX_Control(pDX, IDC_BUTTON_FWD,		m_buttonFwd);
	DDX_Control(pDX, IDC_STATIC_LASER,		m_staticLaser);
	DDX_Control(pDX, IDC_BUTTON_SET_HOME,	m_buttonSetHome);
	DDX_Control(pDX, IDC_BUTTON_GO_HOME,	m_buttonGoHome);
	DDX_Control(pDX, IDC_BUTTON_LASER_STATUS, m_buttonLaserStatus);

	DDX_Radio(pDX, IDC_RADIO_CIRC,			m_nScanType);
	DDX_Check(pDX, IDC_CHECK_AUTOHOME,		m_bReturnToHome);
	DDX_Check(pDX, IDC_CHECK_RETURNTOSTART,	m_bReturnToStart);

	DDX_Text(pDX, IDC_STATIC_HOMEDIST,		m_szHomeDist);
	DDX_Text(pDX, IDC_STATIC_SCANNEDDIST,	m_szDistScanned);

	DDX_Text(pDX, IDC_STATIC_LASER_US,		m_szLaserHiLowUS);
	DDX_Text(pDX, IDC_STATIC_LASER_DS,		m_szLaserHiLowDS);
	DDX_Text(pDX, IDC_STATIC_LASER_DIFF,	m_szLaserHiLowDiff);
	DDX_Text(pDX, IDC_STATIC_CAP_HEIGHT,	m_szLaserCapHeight);
}


BEGIN_MESSAGE_MAP(CDialogGirthWeld, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
//	ON_WM_CTLCOLOR()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LR_OFFSET, &CDialogGirthWeld::OnDeltaposSpinLrOffset)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_CIRC, &CDialogGirthWeld::OnDeltaposSpinScanCirc)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_DIST, &CDialogGirthWeld::OnDeltaposSpinScanDist)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_OVERLAP, &CDialogGirthWeld::OnDeltaposSpinScanOverlap)

	ON_BN_CLICKED(IDC_BUTTON_PAUSE,			&CDialogGirthWeld::OnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_CALIB,			&CDialogGirthWeld::OnClickedButtonCalib)
	ON_BN_CLICKED(IDC_BUTTON_MANUAL,		&CDialogGirthWeld::OnClickedButtonManual)
	ON_BN_CLICKED(IDC_BUTTON_AUTO,			&CDialogGirthWeld::OnClickedButtonAuto)
	ON_BN_CLICKED(IDC_BUTTON_FWD,			&CDialogGirthWeld::OnClickedButtonFwd)
	ON_BN_CLICKED(IDC_BUTTON_BACK,			&CDialogGirthWeld::OnClickedButtonBack)
	ON_BN_CLICKED(IDC_BUTTON_SET_HOME,		&CDialogGirthWeld::OnClickedButtonSetHome)
	ON_BN_CLICKED(IDC_BUTTON_GO_HOME,		&CDialogGirthWeld::OnClickedButtonGoHome)
	ON_BN_CLICKED(IDC_CHECK_AUTOHOME,		&CDialogGirthWeld::OnClickedCheckGoToHome)
	ON_BN_CLICKED(IDC_CHECK_RETURNTOSTART,	&CDialogGirthWeld::OnClickedCheckGoToStart)

	ON_COMMAND(IDC_RADIO_CIRC,				&CDialogGirthWeld::OnRadioScanType)
	ON_COMMAND(IDC_RADIO_DIST,				&CDialogGirthWeld::OnRadioScanType)
END_MESSAGE_MAP()


// CDialogGirthWeld message handlers
BOOL CDialogGirthWeld::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here
	m_spinLROffset.SetRange(0, UD_MAXVAL);
	m_spinScanCirc.SetRange(0, UD_MAXVAL);
	m_spinScanDist.SetRange(0, UD_MAXVAL);
	m_spinScanOverlap.SetRange(0, UD_MAXVAL);

	m_bitmapPause.LoadBitmap(IDB_BITMAP_PAUSE);
	m_bitmapGoHorz.LoadBitmap(IDB_BITMAP_GO_HORZ);
	m_bitmapGoVert.LoadBitmap(IDB_BITMAP_GO_VERT);
	m_bitmapStop.LoadBitmap(IDB_BITMAP_STOP);

	m_bitmapLaserOff.LoadBitmap(IDB_BITMAP_LASER_OFF);
	m_bitmapLaserOK.LoadBitmap(IDB_BITMAP_LASER_OK);
	m_bitmapLaserError.LoadBitmap(IDB_BITMAP_LASER_ERROR);
	m_bitmapLaserLoading.LoadBitmap(IDB_BITMAP_LASER_LOADING);
	m_bitmapLaserHot.LoadBitmap(IDB_BITMAP_LASER_HOT);

	m_brRed.CreateSolidBrush(RGB(255, 0, 0));
	m_brGreen.CreateSolidBrush(RGB(0, 255, 0));
	m_brBlue.CreateSolidBrush(RGB(0255, 0, 255));
	m_brMagenta.CreateSolidBrush(RGB(255, 0, 255));

	m_wndLaser.Create(&m_staticLaser);

	m_bInit = TRUE;
	SetButtonBitmaps();

	SetLaserStatus(LASER_OFF);

	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogGirthWeld::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_GIRTHWELD, pParent);
	ShowWindow(SW_HIDE);
}

void CDialogGirthWeld::SetLaserStatus(LASER_STATUS nStatus)
{
	m_nTimerCount = 0;
	SetTimer(nStatus, LASER_BLINK, NULL); // this will default the laser statusd
}

// set a timer at 500 ms to cycle between off and on colours to show blink
void CDialogGirthWeld::OnTimer(UINT_PTR nIDEvent)
{
	// if sewtting to steady, then kill the timer after setting
	HBITMAP hBitmap = NULL;
	switch (nIDEvent)
	{
	case LASER_OFF:
		hBitmap = (HBITMAP)m_bitmapLaserOff.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // off
	case LASER_OK:
		hBitmap = (HBITMAP)m_bitmapLaserOK.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // ok (grren steady)
	case LASER_ERROR:
		hBitmap = (HBITMAP)m_bitmapLaserError.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // HW error (red steady)
	case LASER_HOT:
		hBitmap = (HBITMAP)m_bitmapLaserHot.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // OK HOT (magenta steady)
	// these are blink, so keep the time alive
	case LASER_SENDING_OK:
		m_nTimerCount++;
		hBitmap = (m_nTimerCount%2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserOK.GetSafeHandle();
		break; // sending data OK (green blink)
	case LASER_SENDING_ERROR:
		m_nTimerCount++;
		hBitmap = (m_nTimerCount % 2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserHot.GetSafeHandle();
		break; // sending data HOT (magenta blink) 
	case LASER_LOADING:
		m_nTimerCount++;
		hBitmap = (m_nTimerCount % 2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserLoading.GetSafeHandle();
		break; // saving data (blue blink)
	default:
		return;
	}
	m_buttonLaserStatus.SetBitmap(hBitmap);
}

/*
HBRUSH CDialogGirthWeld::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_LASER_STATUS)
	{
		//set the static text color to red      
	//	pDC->SetTextColor(RGB(255, 0, 0));
		pDC->SelectObject(&m_brRed);
	}

	// TODO: Return a different brush if the default is not desired   
	return hbr;
}
*/
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

void CDialogGirthWeld::OnDeltaposSpinScanCirc(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fScanCirc += (inc > 0) ? 1 : -1;
	m_fScanCirc = min(max(m_fScanCirc, 100.0), 10000);
	UpdateData(FALSE);

	*pResult = 0;
}


void CDialogGirthWeld::OnDeltaposSpinScanDist(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fDistToScan += (inc > 0) ? 1 : -1;
	m_fDistToScan = min(max(m_fDistToScan, 10.0), 10000);
	UpdateData(FALSE);

	*pResult = 0;
}


void CDialogGirthWeld::OnDeltaposSpinScanOverlap(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fScanOverlap += (inc > 0) ? 1 : -1;
	m_fScanOverlap = min(max(m_fScanOverlap, 0.0), 100);
	UpdateData(FALSE);

	*pResult = 0;
}




void CDialogGirthWeld::SetButtonBitmaps()
{
	// TODO: Add your control notification handler code here

	BOOL bGalil = m_motionControl.IsConnected();
	HBITMAP hBitmapHorz = (HBITMAP)m_bitmapGoHorz.GetSafeHandle();
	HBITMAP hBitmapVert = (HBITMAP)m_bitmapGoVert.GetSafeHandle();
	HBITMAP hBitmapStop = (HBITMAP)m_bitmapStop.GetSafeHandle();
	HBITMAP hBitmapPause = (HBITMAP)m_bitmapPause.GetSafeHandle();

	m_buttonPause.SetBitmap(m_bPaused ? hBitmapHorz : hBitmapPause);
	m_buttonCalib.SetBitmap(m_bPaused ? hBitmapHorz : hBitmapPause);
	m_buttonManual.SetBitmap(m_bPaused ? hBitmapHorz : hBitmapPause);
	m_buttonAuto.SetBitmap(m_bPaused ? hBitmapHorz : hBitmapPause);

	// set to stop if running
	m_buttonCalib.SetBitmap((m_nGalilState == GALIL_CALIB) ? hBitmapStop : hBitmapHorz);
	m_buttonManual.SetBitmap((m_nGalilState == GALIL_MANUAL) ? hBitmapStop : hBitmapHorz);
	m_buttonAuto.SetBitmap((m_nGalilState == GALIL_AUTO) ? hBitmapStop : hBitmapHorz);
	m_buttonGoHome.SetBitmap(m_nGalilState == GALIL_GOHOME ? hBitmapStop : hBitmapHorz);

	// fowd and back are either vertical or stop
	m_buttonBack.SetBitmap(m_nGalilState == GALIL_BACK ? hBitmapStop : hBitmapVert);
	m_buttonFwd.SetBitmap(m_nGalilState == GALIL_FWD ? hBitmapStop : hBitmapVert);

	// set to 
	m_buttonCalib.EnableWindow(bGalil && (m_nGalilState == GALIL_CALIB || m_nGalilState == GALIL_IDLE));
	m_buttonManual.EnableWindow(bGalil && (m_nGalilState == GALIL_MANUAL || m_nGalilState == GALIL_IDLE));
	m_buttonAuto.EnableWindow(bGalil && (m_nGalilState == GALIL_AUTO || m_nGalilState == GALIL_IDLE));
	m_buttonPause.EnableWindow(bGalil && (m_nGalilState == GALIL_CALIB || m_nGalilState == GALIL_MANUAL || m_nGalilState == GALIL_AUTO));
	m_buttonGoHome.EnableWindow(bGalil && (m_nGalilState == GALIL_GOHOME || m_nGalilState == GALIL_IDLE) && m_bReturnToHome);
	m_buttonSetHome.EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bReturnToHome);

	m_buttonBack.EnableWindow(bGalil && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_BACK || m_bPaused));
	m_buttonFwd.EnableWindow(bGalil && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_FWD || m_bPaused));

	GetDlgItem(IDC_EDIT_LR_OFFSET)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_CIRC)->EnableWindow(m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC);
	GetDlgItem(IDC_EDIT_OVERLAP)->EnableWindow(m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC);
	GetDlgItem(IDC_EDIT_DIST)->EnableWindow(m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_DIST);
	GetDlgItem(IDC_RADIO_CIRC)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_RADIO_DIST)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_AUTOHOME)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_RETURNTOSTART)->EnableWindow(m_nGalilState == GALIL_IDLE);


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
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_CALIB : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonManual()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_MANUAL))
	{
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_MANUAL : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonAuto()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_AUTO))
	{
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_AUTO : GALIL_IDLE;
		SetButtonBitmaps();
	}
}
void CDialogGirthWeld::OnClickedButtonGoHome()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_GOHOME))
	{
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_GOHOME : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonBack()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_BACK))
	{
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_BACK : GALIL_IDLE;
		SetButtonBitmaps();
	}
}


void CDialogGirthWeld::OnClickedButtonFwd()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_FWD))
	{
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_FWD : GALIL_IDLE;
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
	if (m_nGalilState == GALIL_IDLE)
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
	switch (m_nGalilState)
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






void CDialogGirthWeld::OnRadioScanType()
{
	// TODO: Add your command handler code here
	UpdateData(TRUE);
	SetButtonBitmaps();
}


void CDialogGirthWeld::OnClickedCheckGoToHome()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_bReturnToHome && m_bReturnToStart)
	{
		m_bReturnToStart = FALSE;
		UpdateData(FALSE);
	}
	SetButtonBitmaps();
}

void CDialogGirthWeld::OnClickedCheckGoToStart()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_bReturnToHome && m_bReturnToStart)
	{
		m_bReturnToHome = FALSE;
		UpdateData(FALSE);
	}
	SetButtonBitmaps();
}

void CDialogGirthWeld::EnableControls()
{
	SetButtonBitmaps();
}

