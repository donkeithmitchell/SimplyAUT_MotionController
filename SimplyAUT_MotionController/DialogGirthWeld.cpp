// DialogGirthWeld.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogGirthWeld.h"
#include "afxdialogex.h"
#include "MotionControl.h"
#include "resource.h"

static const int LASER_BLINK = 500;

// CDialogGirthWeld dialog

static UINT ThreadStopMotors(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadStopMotors();
}

IMPLEMENT_DYNAMIC(CDialogGirthWeld, CDialogEx)

CDialogGirthWeld::CDialogGirthWeld(CMotionControl& motion, CLaserControl& laser, GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_GIRTHWELD, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_wndLaser(laser)
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
	m_pParent = NULL;
	m_nMsg = 0;
	m_hThreadStopMotors = NULL;
}

CDialogGirthWeld::~CDialogGirthWeld()
{
}

void CDialogGirthWeld::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
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
	DDX_Control(pDX, IDC_BUTTON_LEFT,		m_buttonLeft);
	DDX_Control(pDX, IDC_BUTTON_RIGHT,		m_buttonRight);
	DDX_Control(pDX, IDC_BUTTON_FWD,		m_buttonFwd);
	DDX_Control(pDX, IDC_BUTTON_ZERO_HOME,	m_buttonZeroHome);
	DDX_Control(pDX, IDC_BUTTON_GO_HOME,	m_buttonGoHome);
	DDX_Control(pDX, IDC_BUTTON_LASER_STATUS, m_buttonLaserStatus);

	DDX_Radio(pDX, IDC_RADIO_CIRC,			m_nScanType);
	DDX_Check(pDX, IDC_CHECK_AUTOHOME,		m_bReturnToHome);
	DDX_Check(pDX, IDC_CHECK_RETURNTOSTART,	m_bReturnToStart);

	DDX_Control(pDX, IDC_STATIC_LASER, m_staticLaser);
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
	ON_BN_CLICKED(IDC_BUTTON_LEFT,			&CDialogGirthWeld::OnClickedButtonLeft)
	ON_BN_CLICKED(IDC_BUTTON_RIGHT,			&CDialogGirthWeld::OnClickedButtonRight)
	ON_BN_CLICKED(IDC_BUTTON_ZERO_HOME,		&CDialogGirthWeld::OnClickedButtonZeroHome)
	ON_BN_CLICKED(IDC_BUTTON_GO_HOME,		&CDialogGirthWeld::OnClickedButtonGoHome)
	ON_BN_CLICKED(IDC_CHECK_AUTOHOME,		&CDialogGirthWeld::OnClickedCheckGoToHome)
	ON_BN_CLICKED(IDC_CHECK_RETURNTOSTART,	&CDialogGirthWeld::OnClickedCheckGoToStart)

	ON_COMMAND(IDC_RADIO_CIRC,				&CDialogGirthWeld::OnRadioScanType)
	ON_COMMAND(IDC_RADIO_DIST,				&CDialogGirthWeld::OnRadioScanType)
	ON_MESSAGE(WM_STEER_LEFT,				&CDialogGirthWeld::OnUserSteerLeft)
	ON_MESSAGE(WM_STEER_RIGHT,				&CDialogGirthWeld::OnUserSteerRight)
	ON_MESSAGE(WM_MOTORSSTOPPED,			&CDialogGirthWeld::OnUserMotorsStopped)

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
	m_bitmapGoRight.LoadBitmap(IDB_BITMAP_GO_RIGHT);
	m_bitmapGoLeft.LoadBitmap(IDB_BITMAP_GO_LEFT);
	m_bitmapGoDown.LoadBitmap(IDB_BITMAP_GO_DOWN);
	m_bitmapGoUp.LoadBitmap(IDB_BITMAP_GO_UP);
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
	m_buttonLeft.Init(this, WM_STEER_LEFT);
	m_buttonRight.Init(this, WM_STEER_RIGHT);

	m_bInit = TRUE;
	SetButtonBitmaps();

	SetLaserStatus(TIMER_LASER_OFF);

	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

LRESULT CDialogGirthWeld::OnUserSteerLeft(WPARAM wParam, LPARAM)
{
	return 	UserSteer(FALSE, wParam == 1);
}

LRESULT CDialogGirthWeld::OnUserSteerRight(WPARAM wParam, LPARAM)
{
	return 	UserSteer(TRUE, wParam == 1);
}
LRESULT CDialogGirthWeld::UserSteer(BOOL bRight, BOOL bDown)
{
	m_motionControl.SteerMotors(bRight, bDown);
	return 0L;
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
	case TIMER_LASER_OFF:
		hBitmap = (HBITMAP)m_bitmapLaserOff.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // off
	case TIMER_LASER_OK:
		hBitmap = (HBITMAP)m_bitmapLaserOK.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // ok (grren steady)
	case TIMER_LASER_ERROR:
		hBitmap = (HBITMAP)m_bitmapLaserError.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // HW error (red steady)
	case TIMER_LASER_HOT:
		hBitmap = (HBITMAP)m_bitmapLaserHot.GetSafeHandle();
		KillTimer(nIDEvent);
		break; // OK HOT (magenta steady)
	// these are blink, so keep the time alive
	case TIMER_LASER_SENDING_OK:
		m_nTimerCount++;
		hBitmap = (m_nTimerCount%2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserOK.GetSafeHandle();
		break; // sending data OK (green blink)
	case TIMER_LASER_SENDING_ERROR:
		m_nTimerCount++;
		hBitmap = (m_nTimerCount % 2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserHot.GetSafeHandle();
		break; // sending data HOT (magenta blink) 
	case TIMER_LASER_LOADING:
		m_nTimerCount++;
		hBitmap = (m_nTimerCount % 2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserLoading.GetSafeHandle();
		break; // saving data (blue blink)
	case TIMER_SHOW_MOTOR_SPEEDS:
		ShowMotorSpeeds();
		ShowMotorPosition();
		break;

//	case TIMER_STEER_LEFT:
//		break;
//	case TIMER_STEER_RIGHT:
//		break;
	default:
		return;
	}
	m_buttonLaserStatus.SetBitmap(hBitmap);
}

void CDialogGirthWeld::ShowMotorSpeeds()
{
	m_pParent->SendMessage(m_nMsg, MSG_SHOW_MOTOR_SPEEDS, 0);
}

// show the positoon as the average of the four motors
void CDialogGirthWeld::ShowMotorPosition()
{
	UpdateData(TRUE);
	double dist1 = atof(m_szDistScanned);
	double posA = m_motionControl.GetMotorPosition("A");
	double posB = m_motionControl.GetMotorPosition("B");
	double posC = m_motionControl.GetMotorPosition("C");
	double posD = m_motionControl.GetMotorPosition("D");

	if( posA == FLT_MAX || posB == FLT_MAX || posC == FLT_MAX || posD == FLT_MAX )
		m_szDistScanned.Format("****");
	else
	{
		m_szDistScanned.Format("%.1f", (posA + posB + posC + posD)/4);
	}

	UpdateData(FALSE);
}

BOOL CDialogGirthWeld::CheckVisibleTab()
{
	m_bCheck = TRUE;
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret;
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
	HBITMAP hBitmapRight = (HBITMAP)m_bitmapGoRight.GetSafeHandle();
	HBITMAP hBitmapLeft = (HBITMAP)m_bitmapGoLeft.GetSafeHandle();
	HBITMAP hBitmapUp = (HBITMAP)m_bitmapGoUp.GetSafeHandle();
	HBITMAP hBitmapDown = (HBITMAP)m_bitmapGoDown.GetSafeHandle();
	HBITMAP hBitmapStop = (HBITMAP)m_bitmapStop.GetSafeHandle();
	HBITMAP hBitmapPause = (HBITMAP)m_bitmapPause.GetSafeHandle();

	m_buttonPause.SetBitmap(m_bPaused ? hBitmapRight : hBitmapPause);
	m_buttonCalib.SetBitmap(m_bPaused ? hBitmapRight : hBitmapPause);
	m_buttonManual.SetBitmap(m_bPaused ? hBitmapRight : hBitmapPause);
	m_buttonAuto.SetBitmap(m_bPaused ? hBitmapRight : hBitmapPause);

	// set to stop if running
	m_buttonCalib.SetBitmap((m_nGalilState == GALIL_CALIB) ? hBitmapStop : hBitmapRight);
	m_buttonManual.SetBitmap((m_nGalilState == GALIL_MANUAL) ? hBitmapStop : hBitmapRight);
	m_buttonAuto.SetBitmap((m_nGalilState == GALIL_AUTO) ? hBitmapStop : hBitmapRight);
	// m_buttonGoHome.SetBitmap(m_nGalilState == GALIL_GOHOME ? hBitmapStop : hBitmapHorz);

	// fowd and back are either vertical or stop
	m_buttonFwd.SetBitmap(m_nGalilState == GALIL_FWD ? hBitmapStop : hBitmapUp);
	m_buttonBack.SetBitmap(m_nGalilState == GALIL_BACK ? hBitmapStop : hBitmapDown);
	m_buttonLeft.SetBitmap(hBitmapLeft);
	m_buttonRight.SetBitmap(hBitmapRight);

	// set to 
	m_buttonCalib.EnableWindow(bGalil && (m_nGalilState == GALIL_CALIB || m_nGalilState == GALIL_IDLE));
	m_buttonManual.EnableWindow(bGalil && (m_nGalilState == GALIL_MANUAL || m_nGalilState == GALIL_IDLE));
	m_buttonAuto.EnableWindow(bGalil && (m_nGalilState == GALIL_AUTO || m_nGalilState == GALIL_IDLE));
	m_buttonPause.EnableWindow(bGalil && (m_nGalilState == GALIL_CALIB || m_nGalilState == GALIL_MANUAL || m_nGalilState == GALIL_AUTO));
	m_buttonGoHome.EnableWindow(bGalil && (m_nGalilState == GALIL_GOHOME || m_nGalilState == GALIL_IDLE) );
	m_buttonZeroHome.EnableWindow(bGalil && m_nGalilState == GALIL_IDLE );

	m_buttonBack.EnableWindow(bGalil && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_BACK || m_bPaused));
	m_buttonFwd.EnableWindow(bGalil && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_FWD || m_bPaused));
	m_buttonLeft.EnableWindow(bGalil && (m_nGalilState == GALIL_FWD || m_nGalilState == GALIL_BACK) && !m_bPaused);
	m_buttonRight.EnableWindow(bGalil && (m_nGalilState == GALIL_FWD || m_nGalilState == GALIL_BACK) && !m_bPaused);

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
		SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		m_motionControl.GoToHomePosition();
	}
}


void CDialogGirthWeld::OnClickedButtonLeft()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_BACK))
	{
	}
}

void CDialogGirthWeld::OnClickedButtonRight()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_BACK))
	{
	}
}

void CDialogGirthWeld::OnClickedButtonBack()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_BACK))
	{
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_BACK : GALIL_IDLE;
//		SetButtonBitmaps();
		RunMotors();
	}
}


void CDialogGirthWeld::OnClickedButtonFwd()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_FWD))
	{
		m_nGalilState = (m_nGalilState == GALIL_IDLE) ? GALIL_FWD : GALIL_IDLE;
	//	SetButtonBitmaps();
		RunMotors();
	}
}

void CDialogGirthWeld::RunMotors()
{
	double accel = 0;
	double speed = GetMotorSpeed(accel);

	if (m_nGalilState == GALIL_IDLE)
	{
		// because of deceleration it takes a while for the motors to stop
		// will not return from this fuinction until they have stopped
		// thus, call this from a thread, and block starts until the motors are stopped
		m_hThreadStopMotors = AfxBeginThread(::ThreadStopMotors, (LPVOID)this)->m_hThread;
	//	m_motionControl.StopMotors();		// donw in the thread
	//	KillTimer(TIMER_SHOW_MOTOR_SPEEDS);
	}
	else if (m_nGalilState == GALIL_FWD)
	{
		if (speed != FLT_MAX && accel != FLT_MAX)
		{
			m_motionControl.SetMotorSpeed(speed, accel);
			SetButtonBitmaps();
			SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		}
	}
	else if (m_nGalilState == GALIL_BACK)
	{
		if (speed != FLT_MAX && accel != FLT_MAX)
		{
			m_motionControl.SetMotorSpeed(-speed, accel);
			SetButtonBitmaps();
			SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		}
	}
}

UINT CDialogGirthWeld::ThreadStopMotors()
{
	m_motionControl.StopMotors();
	PostMessage(WM_MOTORSSTOPPED);
	return 0;
}

LRESULT CDialogGirthWeld::OnUserMotorsStopped(WPARAM, LPARAM)
{
	::WaitForSingleObject(m_hThreadStopMotors, INFINITE);
	m_hThreadStopMotors = NULL;
	KillTimer(TIMER_SHOW_MOTOR_SPEEDS);
	SetButtonBitmaps();
	return 0L;
}

double CDialogGirthWeld::GetMotorSpeed(double& rAccel)
{
	double speed=0;
	m_pParent->SendMessage(m_nMsg, MSG_GETSCANSPEED, (LPARAM)(&speed));
	m_pParent->SendMessage(m_nMsg, MSG_GETACCEL, (LPARAM)(&rAccel));

	return speed;
}

// reset the motor positioons to zero
void CDialogGirthWeld::OnClickedButtonZeroHome()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	m_motionControl.ZeroPositions();
	m_szDistScanned = _T("0.0");
	UpdateData(FALSE);
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

