// DialogGirthWeld.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogGirthWeld.h"
#include "afxdialogex.h"
#include "MotionControl.h"
#include "LaserControl.h"
#include "MagController.h"
#include "resource.h"

static const int LASER_BLINK = 500;

// CDialogGirthWeld dialog

static UINT ThreadStopMotors(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadStopMotors();
}

static UINT ThreadGoToHome(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadGoToHome();
}

static UINT ThreadRunRestart(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadRunManual(FALSE);
}

static UINT ThreadRunManual(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadRunManual(TRUE);
}

IMPLEMENT_DYNAMIC(CDialogGirthWeld, CDialogEx)

CDialogGirthWeld::CDialogGirthWeld(CMotionControl& motion, CLaserControl& laser, CMagController& mag, GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_GIRTHWELD, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_wndLaser(laser)
	, m_nGalilState(nState)

	, m_szLROffset(_T("0.0"))
	, m_szScanCirc(_T("100.0"))
	, m_szDistToScan(_T("100.0"))
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

	, m_szScannedDist(_T("0.0 mm"))
	, m_szHomeDist(_T("0.0 mm"))
	, m_bSeekReverse(FALSE)
	, m_szTempBoard(_T(""))
	, m_szTempLaser(_T(""))
	, m_szTempSensor(_T(""))
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_bPaused = FALSE;
	m_bResumed = FALSE;
	m_nTimerCount = 0;
	m_pParent = NULL;
	m_nMsg = 0;
	m_hThreadRunMotors = NULL;
	m_fMotorSpeed = 0;
	m_fMotorAccel = 0;
	m_nGaililStateBackup = GALIL_IDLE;
	m_fScanStart = FLT_MAX;
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
	DDX_Control(pDX, IDC_SPIN_LR_OFFSET, m_spinLROffset);
	DDX_Control(pDX, IDC_SPIN_CIRC, m_spinScanCirc);
	DDX_Control(pDX, IDC_SPIN_DIST, m_spinScanDist);
	DDX_Control(pDX, IDC_SPIN_OVERLAP, m_spinScanOverlap);

	DDX_Text(pDX, IDC_EDIT_LR_OFFSET, m_szLROffset);
	DDX_Text(pDX, IDC_EDIT_LR_OFFSET, m_fLROffset);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fLROffset, 0.0, 10.0);
	}
	DDX_Text(pDX, IDC_EDIT_CIRC, m_szScanCirc);
	DDX_Text(pDX, IDC_EDIT_CIRC, m_fScanCirc);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fScanCirc, 100.0, 10000.0);
	}
	DDX_Text(pDX, IDC_EDIT_DIST, m_szDistToScan);
	DDX_Text(pDX, IDC_EDIT_DIST, m_fDistToScan);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fDistToScan, 10.0, 10000.0);
	}

	DDX_Text(pDX, IDC_EDIT_OVERLAP, m_szScanOverlap);
	DDX_Text(pDX, IDC_EDIT_OVERLAP, m_fScanOverlap);
	if (m_bCheck)
	{
		DDV_MinMaxDouble(pDX, m_fScanOverlap, 0.0, 100.0);
	}

	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_buttonPause);
	DDX_Control(pDX, IDC_BUTTON_CALIB, m_buttonCalib);
	DDX_Control(pDX, IDC_BUTTON_MANUAL, m_buttonManual);
	DDX_Control(pDX, IDC_BUTTON_AUTO, m_buttonAuto);
	DDX_Control(pDX, IDC_BUTTON_BACK, m_buttonBack);
	DDX_Control(pDX, IDC_BUTTON_LEFT, m_buttonLeft);
	DDX_Control(pDX, IDC_BUTTON_RIGHT, m_buttonRight);
	DDX_Control(pDX, IDC_BUTTON_FWD, m_buttonFwd);
	DDX_Control(pDX, IDC_BUTTON_ZERO_HOME, m_buttonZeroHome);
	DDX_Control(pDX, IDC_BUTTON_GO_HOME, m_buttonGoHome);
	DDX_Control(pDX, IDC_BUTTON_LASER_STATUS, m_buttonLaserStatus);

	DDX_Radio(pDX, IDC_RADIO_CIRC, m_nScanType);
	DDX_Check(pDX, IDC_CHECK_AUTOHOME, m_bReturnToHome);
	DDX_Check(pDX, IDC_CHECK_RETURNTOSTART, m_bReturnToStart);

	DDX_Control(pDX, IDC_STATIC_LASER, m_staticLaser);
	DDX_Text(pDX, IDC_STATIC_HOMEDIST, m_szHomeDist);
	DDX_Text(pDX, IDC_STATIC_SCANNEDDIST, m_szScannedDist);

	DDX_Text(pDX, IDC_STATIC_LASER_US, m_szLaserHiLowUS);
	DDX_Text(pDX, IDC_STATIC_LASER_DS, m_szLaserHiLowDS);
	DDX_Text(pDX, IDC_STATIC_LASER_DIFF, m_szLaserHiLowDiff);
	DDX_Text(pDX, IDC_STATIC_CAP_HEIGHT, m_szLaserCapHeight);
	DDX_Check(pDX, IDC_CHECK_AUTO_REVERSE, m_bSeekReverse);

	DDX_Text(pDX, IDC_STATIC_TEMP_BOARD, m_szTempBoard);
	DDX_Text(pDX, IDC_STATIC_TEMP_LASER, m_szTempLaser);
	DDX_Text(pDX, IDC_STATIC_TEMP_SENSOR, m_szTempSensor);
}


BEGIN_MESSAGE_MAP(CDialogGirthWeld, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
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
	ON_MESSAGE(WM_USER_STATIC,				&CDialogGirthWeld::OnUserStaticParameter)
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
	m_wndLaser.Init(this, WM_USER_STATIC);
	m_buttonLeft.Init(this, WM_STEER_LEFT);
	m_buttonRight.Init(this, WM_STEER_RIGHT);

	m_bInit = TRUE;
	SetButtonBitmaps();

//	SetLaserStatus(TIMER_LASER_OFF);
	SetTimer(TIMER_LASER_STATUS, 500, NULL); // temperature and status

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
/*
void CDialogGirthWeld::SetLaserStatus(LASER_STATUS nStatus)
{
	m_nTimerCount = 0;
	SetTimer(nStatus, LASER_BLINK, NULL); // this will default the laser statusd
}
*/
// set a timer at 500 ms to cycle between off and on colours to show blink
void CDialogGirthWeld::OnTimer(UINT_PTR nIDEvent)
{
	// if sewtting to steady, then kill the timer after setting
	switch (nIDEvent)
	{
	case TIMER_SHOW_MOTOR_SPEEDS:
		ShowMotorSpeeds();
		ShowMotorPosition();
		break;
	case TIMER_LASER_STATUS:
		ShowLaserTemperature();
		ShowLaserStatus();
		break;

//	case TIMER_STEER_LEFT:
//		break;
//	case TIMER_STEER_RIGHT:
//		break;
	default:
		return;
	}
}

void CDialogGirthWeld::ShowLaserStatus()
{
	HBITMAP hBitmap = NULL;

	SensorStatus SensorStatus;
	if( !m_laserControl.GetLaserStatus(SensorStatus) )
		hBitmap = (HBITMAP)m_bitmapLaserOff.GetSafeHandle();
	else if (SensorStatus.LaserStatus == 1)
		hBitmap = (HBITMAP)m_bitmapLaserOK.GetSafeHandle(); // ok (grren steady)
	else if (SensorStatus.LaserStatus == 2)
		hBitmap = (HBITMAP)m_bitmapLaserError.GetSafeHandle();// HW error (red steady)
	else if (SensorStatus.LaserStatus == 3)
		hBitmap = (HBITMAP)m_bitmapLaserHot.GetSafeHandle();// OK HOT (magenta steady)
	else if (SensorStatus.LaserStatus == 4)
		hBitmap = (m_nTimerCount++ % 2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserOK.GetSafeHandle();// sending data OK (green blink)
	else if (SensorStatus.LaserStatus == 5)
		hBitmap = (m_nTimerCount++ % 2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserHot.GetSafeHandle();// sending data HOT (magenta blink) 
	else if (SensorStatus.LaserStatus == 6)
		hBitmap = (m_nTimerCount++ % 2 == 0) ? (HBITMAP)m_bitmapLaserOff.GetSafeHandle() : (HBITMAP)m_bitmapLaserLoading.GetSafeHandle();// sending data HOT (magenta blink) 

	if( hBitmap)
		m_buttonLaserStatus.SetBitmap(hBitmap);
}
	
void CDialogGirthWeld::ShowLaserTemperature()
{
	UpdateData(TRUE);

	// note if the temperature has changeds, do not want to call UpdateData(FALSE) too often
	CString szTemp1 = m_szTempBoard;
	CString szTemp2 = m_szTempLaser;
	CString szTemp3 = m_szTempSensor;

	LASER_TEMPERATURE temp;
	if (m_laserControl.GetLaserTemperature(temp))
	{
		m_szTempBoard.Format("%.0f C", temp.BoardTemperature);
		m_szTempLaser.Format("%.0f C", temp.LaserTemperature);
		m_szTempSensor.Format("%.0f C", temp.SensorTemperature);
	}
	else
		m_szTempBoard = m_szTempLaser = m_szTempSensor = _T("Off");

	if (szTemp1.Compare(m_szTempBoard) != 0 || szTemp2.Compare(m_szTempLaser) != 0 || szTemp2.Compare(m_szTempSensor) != 0)
		UpdateData(FALSE);
}

void CDialogGirthWeld::ShowMotorSpeeds()
{
	m_pParent->SendMessage(m_nMsg, MSG_SHOW_MOTOR_SPEEDS, 0);
}

double CDialogGirthWeld::GetMaximumMotorPosition()
{
	double posA = m_motionControl.GetMotorPosition("A");
	double posB = m_motionControl.GetMotorPosition("B");
	double posC = m_motionControl.GetMotorPosition("C");
	double posD = m_motionControl.GetMotorPosition("D");


	if (posA == FLT_MAX || posB == FLT_MAX || posC == FLT_MAX || posD == FLT_MAX)
		return FLT_MAX;
	else
	{
		// these positions may be pos or neg
		double maxPos = max(max(max(posA, posB), posC), posD);
		double minPos = min(min(min(posA, posB), posC), posD);
		if (fabs(maxPos) > fabs(minPos))
			return maxPos;
		else
			return minPos;
	}

}

// show the positoon as the average of the four motors
// the motor position is the home ;osition
// the scanned position may not start at home, thus may vary
void CDialogGirthWeld::ShowMotorPosition()
{
	UpdateData(TRUE);
	double dist1 = atof(m_szHomeDist);
	double dist2 = atof(m_szScannedDist);

	double pos = GetMaximumMotorPosition();
	if (pos == FLT_MAX)
	{
		m_szHomeDist.Format("");
		m_szScannedDist.Format("");
	}
	else
	{
		m_szHomeDist.Format("%.1f", pos);
		if(m_fScanStart != FLT_MAX )
			m_szScannedDist.Format("%.1f", pos - m_fScanStart);
		else
			m_szScannedDist.Format("");
	}

	// the scanned distance requires the start location to be noted
	m_wndLaser.InvalidateRgn(NULL);
	UpdateData(FALSE);
}

BOOL CDialogGirthWeld::CheckVisibleTab()
{
	m_bCheck = TRUE;
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret;
}


HBRUSH CDialogGirthWeld::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_TEMP_BOARD)
	{
		//set the static text color to red     
		SensorStatus SensorStatus;
		m_laserControl.GetLaserStatus(SensorStatus);

		pDC->SetTextColor(RGB(255, 0, 0));
//		pDC->SelectObject(&m_brRed);
	}

	// TODO: Return a different brush if the default is not desired   
	return hbr;
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


void CDialogGirthWeld::SendDebugMessage(CString msg)
{
	if (m_pParent && ::IsWindow(m_pParent->m_hWnd) && m_nMsg != 0)
	{
		m_pParent->SendMessage(m_nMsg, MSG_SEND_DEBUGMSG, (WPARAM)&msg);
	}
}

void CDialogGirthWeld::SetButtonBitmaps()
{
	// TODO: Add your control notification handler code here
	SendDebugMessage("SetButtonBitmaps");

	BOOL bGalil = m_motionControl.IsConnected();
	BOOL bMag = m_magControl.AreWheelsEngaged();

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
	m_buttonGoHome.SetBitmap(m_nGalilState == GALIL_GOHOME ? hBitmapStop : hBitmapRight);

	// set to 
	m_buttonCalib.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_CALIB || m_nGalilState == GALIL_IDLE));
	m_buttonManual.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_MANUAL || m_nGalilState == GALIL_IDLE));
	m_buttonAuto.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_AUTO || m_nGalilState == GALIL_IDLE));
	m_buttonPause.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_CALIB || m_nGalilState == GALIL_MANUAL || m_nGalilState == GALIL_AUTO)); // && !(m_bPaused && m_hThreadRunMotors != NULL) );
	m_buttonGoHome.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_GOHOME || m_nGalilState == GALIL_IDLE) );
	m_buttonZeroHome.EnableWindow(bGalil && bMag && m_nGalilState == GALIL_IDLE );

	m_buttonBack.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_BACK || m_bPaused));
	m_buttonFwd.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_FWD || m_bPaused));
	m_buttonLeft.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_FWD || m_nGalilState == GALIL_BACK) && !m_bPaused);
	m_buttonRight.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_FWD || m_nGalilState == GALIL_BACK) && !m_bPaused);

	GetDlgItem(IDC_EDIT_LR_OFFSET)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_CIRC)->EnableWindow(m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC);
	GetDlgItem(IDC_EDIT_OVERLAP)->EnableWindow(m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC);
	GetDlgItem(IDC_EDIT_DIST)->EnableWindow(m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_DIST);
	GetDlgItem(IDC_RADIO_CIRC)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_RADIO_DIST)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_AUTOHOME)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_RETURNTOSTART)->EnableWindow(m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_AUTO_REVERSE)->EnableWindow(m_nGalilState == GALIL_IDLE);


	GetDlgItem(IDC_STATIC_PAUSE)->SetWindowText(m_bPaused ? _T("Resume" : _T("Pause")));
}


void CDialogGirthWeld::OnClickedButtonPause()
{
	SendDebugMessage("OnClickedButtonPause");
	m_bPaused = !m_bPaused;
	SetButtonBitmaps();

	// stop the motors, and resume
	if (m_bPaused)
	{
		SetButtonBitmaps();
		m_motionControl.StopMotors();
		m_laserControl.TurnLaserOn(FALSE);
		KillTimer(TIMER_SHOW_MOTOR_SPEEDS);
	}
	// resume the motors
	// this is similar to OnClickedButtonManual
	// except the position is not reset
	else
	{
		m_bResumed = TRUE;
		SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		m_fMotorSpeed = GetMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread
		m_hThreadRunMotors = ::AfxBeginThread(::ThreadRunRestart, (LPVOID)this)->m_hThread;
	}
	SetButtonBitmaps();
}

void CDialogGirthWeld::OnClickedButtonCalib()
{
	// TODO: Add your control notification handler code here
	SendDebugMessage("OnClickedButtonCalib");
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_CALIB))
	{
		m_bPaused = FALSE;
		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_CALIB : GALIL_IDLE;
		SetButtonBitmaps();
	}
}

// this run does NOT look for a start line
// however, it optionally backs up to the '0' position first
// spin a thread to run this
void CDialogGirthWeld::OnClickedButtonManual()
{
	// TODO: Add your control notification handler code here
	SendDebugMessage("OnClickedButtonManual");
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_MANUAL))
	{
		m_bPaused = FALSE;
		m_bResumed = FALSE;
		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_MANUAL : GALIL_IDLE;
		if(m_nGalilState == GALIL_MANUAL )
			SetButtonBitmaps();
		else
			m_buttonManual.EnableWindow(FALSE);

		SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		m_fMotorSpeed = GetMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread

		m_hThreadRunMotors = ::AfxBeginThread(::ThreadRunManual, (LPVOID)this)->m_hThread;
	}
}

void CDialogGirthWeld::OnClickedButtonAuto()
{
	// TODO: Add your control notification handler code here
	SendDebugMessage("OnClickedButtonAuto");
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_AUTO))
	{
		m_bPaused = FALSE;
		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_AUTO : GALIL_IDLE;
		SetButtonBitmaps();
	}
}
void CDialogGirthWeld::OnClickedButtonGoHome()
{
	// TODO: Add your control notification handler code here
	SendDebugMessage("OnClickedButtonGoHome");
	if (CheckParameters() ) // && CheckIfToRunOrStop(GALIL_GOHOME)) 
	{
		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_GOHOME : GALIL_IDLE;
		SetButtonBitmaps();
		SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		m_fMotorSpeed = GetMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread
		m_hThreadRunMotors = ::AfxBeginThread(::ThreadGoToHome, (LPVOID)this)->m_hThread;
	}
}

// 1. check if to go to home position prior to run
UINT CDialogGirthWeld::ThreadRunManual(BOOL reset_pos)
{
	// check if to go home first
	double pos = (m_nScanType == 0) ? m_fScanCirc + m_fScanOverlap : m_fDistToScan;
	if (m_bReturnToHome)
		m_motionControl.GoToPosition(0.0, m_fMotorSpeed, m_fMotorAccel);
	else if( !m_bResumed )
		pos += atof(m_szHomeDist);

	// check if the run was aborted
	if (m_nGalilState == GALIL_MANUAL)
	{
		// check that the laser is on
		m_laserControl.TurnLaserOn(TRUE);

		// set the current location as zero
		if (reset_pos)
		{
			// optionally return to the home poisitioin beforee scanning
			if(m_bReturnToHome )
				m_motionControl.ZeroPositions();

			// always set the default speed to all motors
			m_motionControl.SetSlewSpeed(m_fMotorSpeed);
			m_fScanStart = FLT_MAX;
		}

		// calculate the length of the run
		// it will be currently at zero, so just go to the length
		// if did not goto home first, may need to add the starting position to the length

		if( !m_bResumed )
			m_fScanStart = atof(m_szHomeDist);
		m_motionControl.GoToPosition(pos, m_fMotorSpeed, m_fMotorAccel);
		m_laserControl.TurnLaserOn(FALSE);
	}
	else
	{
		m_bResumed = FALSE;
		m_motionControl.StopMotors();
		m_laserControl.TurnLaserOn(FALSE);

	}

	// if click pause, this thread will end
	// howev er, do not call the call back
	if( !m_bPaused )
		PostMessage(WM_MOTORSSTOPPED);

	return 0L;
}



UINT CDialogGirthWeld::ThreadGoToHome()
{
	if (m_nGalilState == GALIL_GOHOME)
	{
		m_motionControl.SetSlewSpeed(m_fMotorSpeed);
		m_laserControl.TurnLaserOn(TRUE);
		m_motionControl.GoToPosition(0.0, m_fMotorSpeed, m_fMotorAccel);
	}
	else
	{
		m_motionControl.StopMotors();
		m_laserControl.TurnLaserOn(FALSE);
	}

	PostMessage(WM_MOTORSSTOPPED);
	return 0;
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
		if (!m_bPaused)
			m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_BACK : GALIL_IDLE;
		else if (m_nGalilState == GALIL_BACK)
			m_nGalilState = GALIL_IDLE;
		else
			m_nGalilState = GALIL_BACK;

//		SetButtonBitmaps();
		RunMotors();
	}
}


void CDialogGirthWeld::OnClickedButtonFwd()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_FWD))
	{
		if (!m_bPaused)
			m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_FWD : GALIL_IDLE;
		else if(m_nGalilState == GALIL_FWD )
			m_nGalilState = GALIL_IDLE;
		else
			m_nGalilState = GALIL_FWD;

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
		m_fMotorSpeed = GetMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread
		m_hThreadRunMotors = AfxBeginThread(::ThreadStopMotors, (LPVOID)this)->m_hThread;
	//	m_motionControl.StopMotors();		// donw in the thread
	//	KillTimer(TIMER_SHOW_MOTOR_SPEEDS);
	}
	else if (m_nGalilState == GALIL_FWD || (m_bPaused && m_nGaililStateBackup == GALIL_FWD) )
	{
		if (speed != FLT_MAX && accel != FLT_MAX)
		{
			m_laserControl.TurnLaserOn(true);
			m_motionControl.SetMotorJogging(speed, accel);
			SetButtonBitmaps();
			SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		}
	}
	else if (m_nGalilState == GALIL_BACK || (m_bPaused && m_nGaililStateBackup == GALIL_FWD))
	{
		if (speed != FLT_MAX && accel != FLT_MAX)
		{
			m_laserControl.TurnLaserOn(true);
			m_motionControl.SetMotorJogging(-speed, accel);
			SetButtonBitmaps();
			SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
		}
	}
}

UINT CDialogGirthWeld::ThreadStopMotors()
{
	m_motionControl.StopMotors();
	m_laserControl.TurnLaserOn(FALSE);
	PostMessage(WM_MOTORSSTOPPED);
	return 0;
}

LRESULT CDialogGirthWeld::OnUserStaticParameter(WPARAM wParam, LPARAM lParam)
{
	double* param = (double*)lParam;
	switch (wParam)
	{
	case STATUS_GET_CIRC:
		*param = (m_nScanType == 0) ? m_fScanCirc : FLT_MAX;
		break;
	case STATUS_GETLOCATION:
		*param = m_motionControl.GetMotorPosition("A");
		break;
	default:
		*param = FLT_MAX;
	}

	return 0L;
}

LRESULT CDialogGirthWeld::OnUserMotorsStopped(WPARAM, LPARAM)
{
	::WaitForSingleObject(m_hThreadRunMotors, INFINITE);
	m_hThreadRunMotors = NULL;
	if (!m_bPaused)
	{
		m_nGalilState = GALIL_IDLE;
		KillTimer(TIMER_SHOW_MOTOR_SPEEDS);
	}
	else
		m_nGalilState = m_nGaililStateBackup;

	// dont reset if paused, as will mcontinue scan
	if( !m_bPaused)
		m_fScanStart = FLT_MAX;

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
	SendDebugMessage("OnClickedButtonZeroHome");

	m_motionControl.ZeroPositions();
	m_szScannedDist = _T("0.0");
	m_szHomeDist = _T("0.0");
	m_wndLaser.InvalidateRgn(NULL);
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
		ret = IDOK; //  AfxMessageBox(_T("Stop the Manual Run"), MB_OKCANCEL);
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


