// DialogGirthWeld.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "DialogGirthWeld.h"
#include "afxdialogex.h"
#include "MotionControl.h"
#include "LaserControl.h"
#include "MagController.h"
#include "resource.h"
#include "time.h"

static const int LASER_BLINK = 500;
#define MAX_TEMPERATURE 50

// CDialogGirthWeld dialog

static UINT ThreadStopMotors(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadStopMotors();
}

static UINT ThreadWaitForMotorsToStop(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadWaitForMotorsToStop();
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

CDialogGirthWeld::CDialogGirthWeld(CMotionControl& motion, CLaserControl& laser, CMagControl& mag, GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_GIRTHWELD, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_wndLaser(laser, mag, m_profile, m_measure2, m_hitBuffer)
	, m_nGalilState(nState)

	, m_szLROffset(_T("0.0"))
	, m_szScanCirc(_T("1000.0"))
	, m_szDistToScan(_T("1000.0"))
	, m_szScanOverlap(_T("50.0"))

	, m_fLROffset(0.0)
	, m_fScanCirc(1000.0)
	, m_fDistToScan(1000.0)
	, m_fDistScanned(10.0)
	, m_fScanOverlap(50.0)

	, m_nScanType(FALSE)
	, m_bReturnToHome(FALSE)
	, m_bReturnToStart(FALSE)

	, m_szScannedDist(_T("0.0 mm"))
	, m_szHomeDist(_T("0.0 mm"))
	, m_bSeekReverse(FALSE)
	, m_szTempBoard(_T(""))
	, m_szTempLaser(_T(""))
	, m_szTempSensor(_T(""))
	, m_szRunTime(_T(""))
	, m_szSteeringGapDist(_T(""))
	, m_szSteeringGapVel(_T(""))
	, m_szSteeringLRDiff(_T(""))
	, m_szSteeringGapAccel(_T(""))
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_bPaused = FALSE;
	m_bResumeScan = FALSE;
	m_nTimerCount = 0;
	m_pParent = NULL;
	m_nMsg = 0;
	m_hThreadRunMotors = NULL;
	m_fMotorSpeed = 0;
	m_fMotorAccel = 0;
	m_nGaililStateBackup = GALIL_IDLE;
	m_fScanStart = FLT_MAX;

	m_szLaserEdge[0] = _T("---");
	m_szLaserEdge[1] = _T("---");
	m_szLaserEdge[2] = _T("---");
	m_szLaserJoint = _T("---");

	memset(m_hitBuffer, 0x0, sizeof(m_hitBuffer));
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

	DDX_Text(pDX, IDC_STATIC_LASER_DS, m_szLaserEdge[0]);
	DDX_Text(pDX, IDC_STATIC_LASER_US, m_szLaserEdge[1]);
	DDX_Text(pDX, IDC_STATIC_LASER_DIFF, m_szLaserEdge[2]);
	DDX_Text(pDX, IDC_STATIC_JOINT_LOCN, m_szLaserJoint);
	DDX_Check(pDX, IDC_CHECK_AUTO_REVERSE, m_bSeekReverse);

	DDX_Text(pDX, IDC_STATIC_TEMP_BOARD, m_szTempBoard);
	DDX_Text(pDX, IDC_STATIC_TEMP_LASER, m_szTempLaser);
	DDX_Text(pDX, IDC_STATIC_TEMP_SENSOR, m_szTempSensor);
	DDX_Text(pDX, IDC_STATIC_RUN_TIME, m_szRunTime);
	DDX_Text(pDX, IDC_STATIC_STEERING_GAP, m_szSteeringGapDist);
	DDX_Text(pDX, IDC_STATIC_STEERING_GAP_VEL, m_szSteeringGapVel);
	DDX_Text(pDX, IDC_STATIC_STEERING_LR_DIFF, m_szSteeringLRDiff);
	DDX_Text(pDX, IDC_STATIC_STEERING_GAP_ACCEL, m_szSteeringGapAccel);
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
	ON_MESSAGE(WM_STOPMOTOR_FINISHED,		&CDialogGirthWeld::OnUserStopMotorFinished)
	ON_MESSAGE(WM_USER_STATIC,				&CDialogGirthWeld::OnUserStaticParameter)
	ON_MESSAGE(WM_WELD_NAVIGATION,			&CDialogGirthWeld::OnUserWeldNavigation)
	ON_MESSAGE(WM_MOTION_CONTROL,			&CDialogGirthWeld::OnUserMotionControl)
	
	ON_STN_CLICKED(IDC_STATIC_TEMP_BOARD, &CDialogGirthWeld::OnStnClickedStaticTempBoard)
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
	m_weldNavigation.Init(this, WM_WELD_NAVIGATION);

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
#define TURN_TIME 20
#define TURN_FRACTION 0.85
	// set a timer to perform the steer
	// slowly change the left right speeds to the desired
	m_motionControl.SetLastManoeuvrePosition();
//	m_motionControl.SteerMotors(m_fMotorSpeed, bDown, bDown ? 1.0 : 0.8);
//	return 0;

	// ramp up to the desired rate over about 500 ms
	for (int i = 0; i < TURN_TIME; i += 20)
	{
		// in 25 steps, adjust the rate from 1.00 -> +/-0.8
		double fraction = ((i + 20.0) / TURN_TIME) * (1.0- TURN_FRACTION); // 0 -> 0.2
		double rate = (bDown) ? 1.0 - fraction : TURN_FRACTION + fraction;

		if( !bDown )
			m_motionControl.SteerMotors(m_fMotorSpeed, bRight, rate);
		else
			m_motionControl.SteerMotors(m_fMotorSpeed, bRight, rate);
		Sleep(500 / 20);
	}
	return 0L;
}

void CDialogGirthWeld::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_GIRTHWELD, pParent);
	ShowWindow(SW_HIDE);
}



// this will be called approximately every mm
// so add the next location on each call to a buffer
// preset the length of the buffer for the distance being run
/*
double CDialogGirthWeld::GetFilteredLaserPosition(double& gap_vel)
{
	// now LP filter to a seperate buffer
	int nSize = (int)m_posLaser.GetSize();
	double gap = (nSize > 0) ? m_posLaser[nSize-1].gap_filt : 0;

	// now note the velocity 
	return gap;
}
*/
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

	case TIMER_GET_LASER_PROFILE:
	{
		GetLaserProfile();
		m_wndLaser.InvalidateRgn(NULL);
		break;
	}

//	case TIMER_STEER_LEFT:
//		break;
//	case TIMER_STEER_RIGHT:
//		break;

	case TIMER_RUN_TIME:
	{
		clock_t t2 = clock();
		long elapsed = (long)(t2 - m_nRunStart);
		m_szRunTime.Format("%d.%d sec", elapsed/1000, elapsed%10);

		GetDlgItem(IDC_STATIC_RUN_TIME)->SetWindowText(m_szRunTime);
		break;
	}
	case TIMER_NOTE_RGB:
	{
//		RGB_DATA rgb;
//		rgb.sum = m_magControl.GetRGBValues(rgb.red, rgb.green, rgb.blue);
		int sum = m_magControl.GetRGBSum();
		m_wndLaser.AddRGBData(sum);
		break;
	}
	case TIMER_NOTE_STEERING:
	{
		NoteSteering();
		break;
	}
	default:
		return;
	}
}

void CDialogGirthWeld::GetLaserProfile()
{
	if (m_laserControl.GetProfile(m_profile))
	{
		m_laserControl.CalcLaserMeasures(m_profile.hits, m_measure2, m_motionControl.GetAvgMotorPosition(), m_hitBuffer);
	}
}

void CDialogGirthWeld::NoteSteering()
{
	if ( !m_laserControl.IsLaserOn())
		return;

	// if tim=0, then thre is no data
	// this is thread safe
	LASER_POS pos = m_weldNavigation.GetLastNotedPosition(0);
	if (pos.time_noted == 0)
		return;

	// note the fioltered gap distance
	if (pos.gap_filt == FLT_MAX)
		m_szSteeringGapDist = _T("");
	else
		m_szSteeringGapDist.Format("%.1f mm", pos.gap_filt);
	GetDlgItem(IDC_STATIC_STEERING_GAP)->SetWindowText(m_szSteeringGapDist);

	if (pos.vel_filt == FLT_MAX)
		m_szSteeringGapVel = _T("");
	else
		m_szSteeringGapVel.Format("%.1f mm/M", pos.vel_filt);
	GetDlgItem(IDC_STATIC_STEERING_GAP_VEL)->SetWindowText(m_szSteeringGapVel);

//	GetDlgItem(IDC_STATIC_STEERING_GAP_ACCEL)->SetWindowText(m_szSteeringGapAccel);

//	GetDlgItem(IDC_STATIC_STEERING_LR_DIFF)->SetWindowText(m_szSteeringLRDiff);
}
/*
void CDialogGirthWeld::SteerCrawler()
{
	if (!m_laserControl.IsLaserOn())
		return;

	// distance (Joint.x) to left (+Ve) or rigtht (-Ve)
	// low-pass fgilter the location
	double gap_vel, gap = GetFilteredLaserPosition(gap_vel);

	// adjust the rate to steer based on how far away from 0.95 (10 mm) -> 0.99 (1 mm)
	double rate = 1;
	if (gap == FLT_MAX)
		rate = 1;
	else if (gap < 0.25)
		rate = 0.999;
	else if (gap > 10.0)
		rate = 0.900;
	else
		rate = 0.900 + (10.0 - gap) / (10 - 0.25) * (0.999 - 0.900);

	CString str;
	str.Format("%.3f", rate);

	// only adjust if off by more than 1/4 mm
	if (gap < 0.25) // to tyhe right, slow down B & C
	{
		GetDlgItem(IDC_STATIC_LEFT)->SetWindowText(str);
		GetDlgItem(IDC_STATIC_RIGHT)->SetWindowText("Right");
		m_motionControl.SetSlewSpeed(m_fMotorSpeed / rate, m_fMotorSpeed * rate, m_fMotorSpeed * rate, m_fMotorSpeed / rate);
	}
	else if (gap > 0.25) // slow down A & D
	{
		GetDlgItem(IDC_STATIC_LEFT)->SetWindowText("Left");
		GetDlgItem(IDC_STATIC_RIGHT)->SetWindowText(str);
		m_motionControl.SetSlewSpeed(m_fMotorSpeed * rate, m_fMotorSpeed / rate, m_fMotorSpeed / rate, m_fMotorSpeed * rate);
	}
	else // all the same
	{
		GetDlgItem(IDC_STATIC_LEFT)->SetWindowText("Left");
		GetDlgItem(IDC_STATIC_RIGHT)->SetWindowText("Right");
		m_motionControl.SetSlewSpeed(m_fMotorSpeed, m_fMotorSpeed, m_fMotorSpeed, m_fMotorSpeed);
	}
}
*/
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

	// note the 'X' location of the weld
	CDoublePoint joint = m_wndLaser.GetJointPos();
	if (joint.IsSet())
	{
		const char szSide[] = { "LR" };
		m_szLaserJoint.Format("%.1f %c", fabs(joint.x), szSide[joint.x < 0]); // horizontal mm, so no need for decimal point
	}
	else
		m_szLaserJoint = _T("");
	GetDlgItem(IDC_STATIC_JOINT_LOCN)->SetWindowText(m_szLaserJoint);


	const UINT IDC3[] = { IDC_STATIC_LASER_DS, IDC_STATIC_LASER_US, IDC_STATIC_LASER_DIFF };

	// down and up sides heights ('Y')
	// as well as the difference
	for (int i = 0; i < 3; ++i)
	{
		CDoublePoint edge = m_wndLaser.GetEdgePos(i);
		if (edge.IsSet())
			m_szLaserEdge[i].Format("%.1f", edge.y); // height, so note to one decimal point
		else
			m_szLaserEdge[i] = _T("");
		GetDlgItem(IDC3[i])->SetWindowText(m_szLaserEdge[i]);
	}

	SetButtonBitmaps();
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
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SHOW_MOTOR_SPEEDS, 0);
}


// show the positoon as the average of the four motors
// the motor position is the home ;osition
// the scanned position may not start at home, thus may vary
void CDialogGirthWeld::ShowMotorPosition()
{
//	UpdateData(TRUE);
	double dist1 = atof(m_szHomeDist);
	double dist2 = atof(m_szScannedDist);

	double pos = m_motionControl.GetAvgMotorPosition();
	int nEncoderCount = GetMagStatus(MAG_IND_ENC_CNT);

	if (pos == FLT_MAX)
	{
		m_szHomeDist.Format("");
		m_szScannedDist.Format("");
	}
	else
	{
		if (nEncoderCount == INT_MAX)
			m_szHomeDist.Format("%.1f", pos);
		else
			m_szHomeDist.Format("%.1f (%d)", pos, nEncoderCount);

		if (m_fScanStart == FLT_MAX)
			m_szScannedDist.Format("");
		else
			m_szScannedDist.Format("%.1f", pos - m_fScanStart);
	}

	// the scanned distance requires the start location to be noted
	// this is cleaner than using UpdarteData()
	GetDlgItem(IDC_STATIC_HOMEDIST)->SetWindowTextA(m_szHomeDist);
	GetDlgItem(IDC_STATIC_SCANNEDDIST)->SetWindowTextA(m_szScannedDist);
	
//	UpdateData(FALSE);
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
	SensorStatus SensorStatus;
	LASER_TEMPERATURE temp;
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_TEMP_BOARD)
	{
		//set the static text color to red     
		m_laserControl.GetLaserTemperature(temp);
		if( temp.BoardTemperature > MAX_TEMPERATURE )
			pDC->SetTextColor(RGB(255, 0, 0));
	}

	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_TEMP_LASER)
	{
		//set the static text color to red     
		m_laserControl.GetLaserTemperature(temp);
		m_laserControl.GetLaserStatus(SensorStatus);

		if (temp.LaserTemperature > MAX_TEMPERATURE)
			pDC->SetTextColor(RGB(255, 0, 0));
	}

	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_TEMP_SENSOR)
	{
		//set the static text color to red     
		m_laserControl.GetLaserTemperature(temp);
		m_laserControl.GetLaserStatus(SensorStatus);

		if (temp.SensorTemperature > MAX_TEMPERATURE)
			pDC->SetTextColor(RGB(255, 0, 0));
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


void CDialogGirthWeld::SendDebugMessage(const CString& msg)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
}


int CDialogGirthWeld::GetMagStatus(int nStat)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		int nStatus = INT_MAX;
		m_pParent->SendMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_GET_MAG_STATUS + nStat, (WPARAM)(&nStatus));
		return nStatus;
	}
	else
		return INT_MAX;
}


void CDialogGirthWeld::SetButtonBitmaps()
{
	// TODO: Add your control notification handler code here
//	SendDebugMessage("SetButtonBitmaps"); // happenbs toio often to auto put in statuys

	BOOL bGalil = m_motionControl.IsConnected();
	BOOL bMag = GetMagStatus(MAG_IND_MAG_ON) == 1;

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
//		m_magControl.EnableMagSwitchControl(TRUE); // not on a pause
		StartNotingMotorSpeed(FALSE);
		StartNotingRGBData(FALSE);
		m_bResumeScan = FALSE;
	}
	// resume the motors
	// this is similar to OnClickedButtonManual
	// except the position is not reset
	else
	{
		m_bResumeScan = TRUE;
		m_laserControl.TurnLaserOn(TRUE);
		m_fMotorSpeed = GetRequestedMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread
		StartNotingMotorSpeed(TRUE);
		StartNotingRGBData(TRUE);
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
		m_bResumeScan = FALSE;

		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_MANUAL : GALIL_IDLE;
		if (m_nGalilState == GALIL_MANUAL)
		{
			SetButtonBitmaps();

			m_fMotorSpeed = GetRequestedMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread
			m_laserControl.TurnLaserOn(TRUE);
			m_magControl.ResetEncoderCount();
			m_magControl.EnableMagSwitchControl(FALSE);
			m_magControl.ResetEncoderCount();

			SetRunTime(0);
			// set a time to manage the steering
			// adjust the location every ( 10 mm)
			StartNotingMotorSpeed(TRUE);
			StartMeasuringLaser(TRUE);
			//		StartNotingRGBData(TRUE);
			StartSteeringMotors(0x3);
		}
		else
			m_buttonManual.EnableWindow(FALSE);

		m_hThreadRunMotors = ::AfxBeginThread(::ThreadRunManual, (LPVOID)this)->m_hThread;
	}
}

UINT CDialogGirthWeld::ThreadWaitForMotorsToStop()
{
	m_motionControl.WaitForMotorsToStop();

	// if the pause button was clicked, then the mnotors will stop before reaching desired position
	if (!m_bPaused)
		PostMessage(WM_STOPMOTOR_FINISHED);

	return 0;
}

void CDialogGirthWeld::StartReadMagStatus(BOOL bSet)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_MAG_STATUS_ON, bSet);
}

void CDialogGirthWeld::StartSteeringMotors(int nSteer)
{
	if (nSteer)
	{
		double accel, speed = GetRequestedMotorSpeed(accel); // this uses a SendMessage, and must not be called from a thread
		m_weldNavigation.StartSteeringMotors(nSteer, speed, m_fLROffset);
		m_wndLaser.ResetLaserOffsetList();
		m_motionControl.ResetLastManoeuvrePosition();
		StartReadMagStatus(nSteer == 0); // takes too mjuch time
		SetTimer(TIMER_NOTE_STEERING, 500, NULL); // this speed does not need to be the same as that used to manage the steerin g

	}
	else
	{
		KillTimer(TIMER_NOTE_STEERING);
		m_weldNavigation.StartSteeringMotors(0x0);
	}
}

void CDialogGirthWeld::StartMeasuringLaser(BOOL bSet)
{
	if (bSet)
	{
		int delay = max((int)(1/*mm*/ * 1000.0 / m_fMotorSpeed + 0.5), 1);
		SetTimer(TIMER_GET_LASER_PROFILE, delay/2, NULL);
	}
	else
		KillTimer(TIMER_GET_LASER_PROFILE);
}

void CDialogGirthWeld::StartNotingMotorSpeed(BOOL bSet)
{
	if (bSet)
		SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
	else
		KillTimer(TIMER_SHOW_MOTOR_SPEEDS);
}

void CDialogGirthWeld::StartNotingRGBData(BOOL bSet)
{
	if (bSet)
	{
		double accel,speed = GetRequestedMotorSpeed(accel); // this uses a SendMessage, and must not be called from a thread
		int delay2 = (int)(1000.0 / speed / 4.0 + 0.5); // every 0.25 mm
		SetTimer(TIMER_NOTE_RGB, delay2, NULL);
	}
	else
	{
		KillTimer(TIMER_NOTE_RGB);
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
		StartNotingMotorSpeed(TRUE);
		m_motionControl.SetSlewSpeed(m_fMotorSpeed);
		m_laserControl.TurnLaserOn(TRUE);
//		m_magControl.ResetEncoderCount(); // don't rest as want to see it return to zero also
		m_magControl.EnableMagSwitchControl(FALSE);
		m_fMotorSpeed = GetRequestedMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread
		m_motionControl.SetSlewSpeed(m_fMotorSpeed);
		m_hThreadRunMotors = ::AfxBeginThread(::ThreadGoToHome, (LPVOID)this)->m_hThread;
	}
}

// use this to insure thaT THE MAIN  THREAD IS C ALLING IT, NOT THE USER THREASD
BOOL CDialogGirthWeld::SetSlewSpeed(double speed)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_SET_SLEW_SPEED, (LPARAM)(speed*1000 + 0.5));
	else
		return FALSE;
}

BOOL CDialogGirthWeld::GoToPosition(double pos)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_GOTO_POSITION, (LPARAM)(pos * 1000 + 0.5));
	else
		return FALSE;
}

double  CDialogGirthWeld::GetSlewSpeed(const char* axis)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return SendMessage(WM_MOTION_CONTROL, MC_GET_SLEW_SPEED, axis[0] - 'A') / 1000.0;
	else
		return FLT_MAX;

}

// can't use SEndMessage to wait for motors to stop
// so simply poll the motor speeds untill all are zero
// this is called by the thread
BOOL CDialogGirthWeld::WaitForMotorsToStop()
{
	double fSA = FLT_MAX;
	double fSB = FLT_MAX;
	double fSC = FLT_MAX;
	double fSD = FLT_MAX;
	// must bet motor stopped for at least 10 ms
	while (fSA != 0 || fSB != 0 || fSC != 0 || fSD != 0)
	{
		fSA = GetSlewSpeed("A");
		fSB = GetSlewSpeed("B");
		fSC = GetSlewSpeed("C");
		fSD = GetSlewSpeed("D");
		Sleep(1);
	}

	return TRUE;
}

BOOL CDialogGirthWeld::WaitForMotorsToStart()
{
	double fSA = 0;
	double fSB = 0;
	double fSC = 0;
	double fSD = 0;
	// must bet motor stopped for at least 10 ms
	while (fSA == 0 || fSB == 0 || fSC == 0 || fSD == 0)
	{
		fSA = GetSlewSpeed("A");
		fSB = GetSlewSpeed("B");
		fSC = GetSlewSpeed("C");
		fSD = GetSlewSpeed("D");
		Sleep(1);
	}

	return TRUE;
}

// this will always be the start up thread
LRESULT CDialogGirthWeld::OnUserMotionControl(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case MC_SET_SLEW_SPEED:
			return (LRESULT)m_motionControl.SetSlewSpeed(lParam / 1000.0);
		case MC_GOTO_POSITION:
			return (LRESULT)m_motionControl.GoToPosition(lParam / 1000.0, m_fMotorSpeed, m_fMotorAccel, FALSE);
		case MC_GET_SLEW_SPEED:
		{
			double accel;
			CString axis;
			axis.Format("%c", 'A' + lParam);
			return (LRESULT)(1000 * m_motionControl.GetMotorSpeed(axis, accel) + 0.5);
		}
	}
	return 0L;
}

// 1. check if to go to home position prior to run
UINT CDialogGirthWeld::ThreadRunManual(BOOL reset_pos)
{
	// check if to go home first
	double pos = (m_nScanType == 0) ? m_fScanCirc + m_fScanOverlap : m_fDistToScan;
	if (m_bReturnToHome)
	{
		GoToPosition(0.0);
		WaitForMotorsToStart();
		WaitForMotorsToStop();
	}

	else if (!m_bResumeScan)
		pos += atof(m_szHomeDist);

	else
		pos += m_fScanStart;

	// check if the run was aborted
	if (m_nGalilState == GALIL_MANUAL)
	{
		// check that the laser is on
		// alreadyn done in the start up thread
//		m_laserControl.TurnLaserOn(TRUE);
//		m_magControl.EnableMagSwitchControl(FALSE);

		// set the current location as zero
		if (reset_pos)
		{
			// optionally return to the home poisitioin beforee scanning
			if(m_bReturnToHome )
				m_motionControl.ZeroPositions();

			// always set the default speed to all motors
			SetSlewSpeed(m_fMotorSpeed);
			m_fScanStart = FLT_MAX;
		}

		// calculate the length of the run
		// it will be currently at zero, so just go to the length
		// if did not goto home first, may need to add the starting position to the length

		if( !m_bResumeScan )
			m_fScanStart = atof(m_szHomeDist);

		GoToPosition(pos);
		WaitForMotorsToStart();
		WaitForMotorsToStop();

		// this wilkl be done in the callback function
//		m_laserControl.TurnLaserOn(FALSE);
//		m_magControl.EnableMagSwitchControl(TRUE);
	}
	else
	{
		m_bResumeScan = FALSE;
		m_motionControl.StopMotors();
//		m_laserControl.TurnLaserOn(FALSE);
//		m_magControl.EnableMagSwitchControl(TRUE);
	}

	// if click pause, this thread will end
	// howev er, do not call the call back
	if( !m_bPaused )
		PostMessage(WM_STOPMOTOR_FINISHED);

	return 0L;
}



UINT CDialogGirthWeld::ThreadGoToHome()
{
	if (m_nGalilState == GALIL_GOHOME)
	{
//		m_motionControl.SetSlewSpeed(m_fMotorSpeed); // do all this in the startup thread
//		m_laserControl.TurnLaserOn(TRUE);
//		m_magControl.EnableMagSwitchControl(FALSE);
		m_motionControl.GoToPosition(0.0, m_fMotorSpeed, m_fMotorAccel, TRUE/*wait*/);
	}
	else
	{
		m_motionControl.StopMotors();
//		m_laserControl.TurnLaserOn(FALSE);
//		m_magControl.EnableMagSwitchControl(TRUE);
	}

	// no difference in response if just ending a motor run or got to hoime
	PostMessage(WM_STOPMOTOR_FINISHED);
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
	double speed = GetRequestedMotorSpeed(accel);
	int delay2 = (int)(1000.0 / speed + 0.5); // every mm

	if (m_nGalilState == GALIL_IDLE)
	{
		// because of deceleration it takes a while for the motors to stop
		// will not return from this fuinction until they have stopped
		// thus, call this from a thread, and block starts until the motors are stopped
		m_fMotorSpeed = GetRequestedMotorSpeed(m_fMotorAccel); // this uses a SendMessage, and must not be called from a thread
		m_hThreadRunMotors = AfxBeginThread(::ThreadStopMotors, (LPVOID)this)->m_hThread;
	}
	else if (m_nGalilState == GALIL_FWD || (m_bPaused && m_nGaililStateBackup == GALIL_FWD) )
	{
		if (speed != FLT_MAX && accel != FLT_MAX)
		{
			// tezmperary code to test steering
			m_wndLaser.ResetRGBData();
			SetRunTime(0);

			if (m_motionControl.SetMotorJogging(speed, accel))
			{
				m_motionControl.ResetLastManoeuvrePosition();
				m_laserControl.TurnLaserOn(TRUE);
				m_magControl.EnableMagSwitchControl(FALSE);
				m_magControl.ResetEncoderCount();
				StartSteeringMotors(0x1); // track only
				StartNotingRGBData(TRUE);
				StartMeasuringLaser(TRUE);
				StartNotingMotorSpeed(TRUE);
			}
			else
			{
				m_nGalilState = GALIL_IDLE;
			}
			SetButtonBitmaps();
		}
	}
	else if (m_nGalilState == GALIL_BACK || (m_bPaused && m_nGaililStateBackup == GALIL_FWD))
	{
		if (speed != FLT_MAX && accel != FLT_MAX)
		{
			// tezmperary code to test steering
			m_wndLaser.ResetRGBData();
			SetRunTime(0);
			if (m_motionControl.SetMotorJogging(-speed, accel))
			{
				m_motionControl.ResetLastManoeuvrePosition();
				m_laserControl.TurnLaserOn(TRUE);
				m_magControl.EnableMagSwitchControl(FALSE);
				m_magControl.ResetEncoderCount();
				StartSteeringMotors(0x1); // track only
				StartNotingRGBData(TRUE);
				StartMeasuringLaser(TRUE);
				StartNotingMotorSpeed(TRUE);
			}
			else
			{
				m_nGalilState = GALIL_IDLE;
			}
			SetButtonBitmaps();
		}
	}
}

UINT CDialogGirthWeld::ThreadStopMotors()
{
	m_motionControl.StopMotors();
//	m_laserControl.TurnLaserOn(FALSE);				// do not call thesae from a thread
//	m_magControl.EnableMagSwitchControl(TRUE);		// call, them in the finish up call-back
	PostMessage(WM_STOPMOTOR_FINISHED);
	return 0;
}

LRESULT CDialogGirthWeld::OnUserWeldNavigation(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		// this is avoid threads talking to laser and motion controller
		case NAVIGATE_GET_MEASURE:
		{
			LASER_MEASURES* pMeas = (LASER_MEASURES*)lParam;
			m_wndLaser.GetLaserMeasurment(pMeas);
			return pMeas->status == 0;
		}
		case NAVIGATE_LAST_MAN_POSITION:
		{
			double* pos = (double*)lParam;
			*pos = m_motionControl.GetLastManoeuvrePosition();
			return 1L;
		}
		case NAVIGATE_GET_MOTOR_POS:
		{
			double* pos = (double*)lParam;
			*pos = m_motionControl.GetAvgMotorPosition();
			return 1L;
		}
		case NAVIGATE_GET_MOTOR_SPEED:
		{
			double* speed = (double*)lParam;
			double accel;
			speed[0] = m_motionControl.GetMotorSpeed("A", accel);
			speed[1] = m_motionControl.GetMotorSpeed("B", accel);
			speed[2] = m_motionControl.GetMotorSpeed("C", accel);
			speed[3] = m_motionControl.GetMotorSpeed("D", accel);
			return 1L;
		}
		case NAVIGATE_SET_MOTOR_SPEED:
		{
			const double* speed = (double*)lParam;
			m_motionControl.SetLastManoeuvrePosition();
			m_motionControl.SetSlewSpeed(speed[0],speed[1],speed[2],speed[3]);
			return 1L;
		}
		case NAVIGATE_SEND_DEBUG_MSG:
		{
			const CString* pMsg = (CString*)lParam;
			SendDebugMessage(*pMsg);
			return 0L;
		}
	}
	return 0L;
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
	case STATUS_SHOWLASERSTATUS:
	//	ShowLaserStatus();
		break;
	case STATUS_GET_SCAN_LENGTH:
		return (LRESULT)((m_nScanType == 0) ? m_fScanCirc + m_fScanOverlap : m_fDistToScan);
	case STATUS_GET_LAST_LASER_POS:
	{
		LASER_POS* lastPos = (LASER_POS*)lParam;
		*lastPos = m_weldNavigation.GetLastNotedPosition(0);
		break;
	}
	case STATUS_MAG_STATUS + 0:
	case STATUS_MAG_STATUS + 1:
	case STATUS_MAG_STATUS + 2:
	case STATUS_MAG_STATUS + 3:
	case STATUS_MAG_STATUS + 4:
	case STATUS_MAG_STATUS + 5:
		if (m_pParent && IsWindow(m_pParent->m_hWnd))
		{
			int nStatus = (wParam - STATUS_MAG_STATUS);
			m_pParent->SendMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_GET_MAG_STATUS + nStatus, lParam);
		}
		else
		{
			int* pStat = (int*)lParam;
			*pStat = INT_MAX;
		}
		break;
	default:
		*param = FLT_MAX;
	}

	return 0L;
}

// this is called at the end of the tread which was qwaiting for the 
// manual run of the motors to stop
LRESULT CDialogGirthWeld::OnUserStopMotorFinished(WPARAM, LPARAM)
{
	int ret = ::WaitForSingleObject(m_hThreadRunMotors, 1000);
	if (ret != WAIT_OBJECT_0 && m_hThreadRunMotors != NULL)
	{
		::TerminateThread(m_hThreadRunMotors, 0);
	}

	m_laserControl.TurnLaserOn(FALSE); 
	m_magControl.EnableMagSwitchControl(TRUE); // the magnet servo is always disdabled during a run

	m_hThreadRunMotors = NULL;
	if (!m_bPaused)
	{
		m_nGalilState = GALIL_IDLE;
		StartMeasuringLaser(FALSE);
		StartNotingMotorSpeed(FALSE);
	}
	else
		m_nGalilState = m_nGaililStateBackup;

	// dont reset if paused, as will mcontinue scan
	if (!m_bPaused)
		m_fScanStart = FLT_MAX;

	StartSteeringMotors(0x0);
	StartNotingRGBData(FALSE);

	SetRunTime(INT_MAX); // an invalid value ends it
	SetButtonBitmaps();
	return 0L;
}

void CDialogGirthWeld::SetRunTime(int val)
{
	switch( val )
	{
	case 0:
		m_nRunStart = clock();
		SetTimer(TIMER_RUN_TIME, 100, NULL);
		break;
	case INT_MAX:
		KillTimer(TIMER_RUN_TIME);
		break;
	}

}

// this returns the speed that requested, as compared to the actual speed of the motors
// thus must get from the motor dialog
double CDialogGirthWeld::GetRequestedMotorSpeed(double& rAccel)
{
	double speed=0;
	rAccel = 0;
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_GETSCANSPEED, (LPARAM)(&speed));
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_GETACCEL, (LPARAM)(&rAccel));
	}

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
	m_wndLaser.PostMessageA(WM_SIZE);
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




void CDialogGirthWeld::OnStnClickedStaticTempBoard()
{
	// TODO: Add your control notification handler code here
}
