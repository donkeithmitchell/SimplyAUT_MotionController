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
#include "define.h"

// these are used to note timing of various functions
#ifdef _DEBUG_TIMING_
static clock_t g_GetLaserProfileTime = 0;
static int g_GetLaserProfileCount = 0;
static clock_t g_CDialogGirthWeldOnTimerTime = 0;
static int g_CDialogGirthWeldOnTimerCount = 0;
static clock_t g_ShowMotorPositionTime = 0;
static int g_ShowMotorPositionCount = 0;
static clock_t g_ShowLaserTemperatureTime = 0;
static int g_ShowLaserTemperatureCount = 0;
static clock_t g_ShowLaserStatusTime = 0;
static int g_ShowLaserStatusCount = 0;
static clock_t g_ShowRGBStatusTime = 0;
static int g_ShowRGBStatusCount = 0;
static clock_t g_NoteSteeringTime = 0;
static int g_NoteSteeringCount = 0;
static clock_t g_NoteRunTimeTime = 0;
static int g_NoteRunTimeCount = 0;
static clock_t g_NoteRGBTime = 0;
static int g_NoteRGBCount = 0;
static clock_t g_GetSlewSpeedTime = 0;
static int g_GetSlewSpeedCount = 0;
static clock_t g_NoteIfMotorsRunningTime = 0;
static int g_NoteIfMotorsRunningCount =0;

static FILE* g_fp1 = NULL;
#endif

// used to determine the action of the MAG calibration graph
#define CALIBRATE_RGB_NOT      0
#define CALIBRATE_RGB_SCANNING 1
#define CALIBRATE_RGB_RETURN   2

//#define LASER_BLINK				500

// CDialogGirthWeld dialog

// the motors have deceleration time, thus wait for them to stop in a thread
// used by the manual drive buttons
static UINT ThreadStopMotors(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadStopMotors();
}

// 
static UINT ThreadGoToHome(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadGoToHome();
}

static UINT ThreadRunScan(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadRunScan();
}


static UINT ThreadAbortScan(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadAbortScan();
}

IMPLEMENT_DYNAMIC(CDialogGirthWeld, CDialogEx)

CDialogGirthWeld::CDialogGirthWeld(CMotionControl& motion, CLaserControl& laser, CMagControl& mag, GALIL_STATE& nState, NAVIGATION_PID& pid, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_GIRTHWELD, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_weldNavigation(motion,laser, pid)
	, m_wndLaser(motion, laser, mag, m_weldNavigation, m_fScanLength)
	, m_wndMag(mag, m_bSeekWithLaser, m_bSeekBlackLine, m_fSeekAndStartAtLine)
	, m_nGalilState(nState)
	, m_szScannedDist(_T("0.0 mm"))
	, m_szHomeDist(_T("0.0 mm"))
	, m_szTempBoard(_T(""))
	, m_szTempLaser(_T(""))
	, m_szTempSensor(_T(""))
	, m_szRunTime(_T(""))
	, m_szRGBLinePresent(_T(""))
	, m_bPredrive(FALSE)
	, m_bCalibrate(FALSE)
	, m_bEnableMAG(TRUE)
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_bPaused = FALSE;
	m_bAborted = FALSE;
	m_nCalibratingRGB = CALIBRATE_RGB_NOT;
	m_bScanning = FALSE;
	m_bResumeScan = FALSE;
	m_nTimerCount = 0;
	m_pParent = NULL;
	m_nMsg = 0;
	m_pThreadScan = NULL;
	m_pThreadAbort = NULL;
	m_fDestinationPosition = 0;
	m_nGaililStateBackup = GALIL_IDLE;
	m_fScanStartPos = FLT_MAX;
	m_rgbLast = 0;
	m_lastCapPix = -1;
	m_bSeekBlackLine = FALSE;

	m_szLaserEdge = _T("---");
	m_szLaserJoint = _T("---");

	ResetParameters();
}

CDialogGirthWeld::~CDialogGirthWeld()
{
}

// used to pass messages to the parent
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
	DDX_Control(pDX, IDC_SPIN_SPEED, m_spinScanSpeed);
	DDX_Control(pDX, IDC_SPIN_ACCEL2, m_spinScanAccel);
	DDX_Control(pDX, IDC_SPIN_PREDRIVE, m_spinPredrive);
	DDX_Control(pDX, IDC_SPIN_SEEK_START, m_spinSeekStart);
	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_buttonPause);
	DDX_Control(pDX, IDC_BUTTON_MANUAL, m_buttonManual);
	DDX_Control(pDX, IDC_BUTTON_BACK, m_buttonBack);
	DDX_Control(pDX, IDC_BUTTON_FWD, m_buttonFwd);
	DDX_Control(pDX, IDC_BUTTON_ZERO_HOME, m_buttonZeroHome);
	DDX_Control(pDX, IDC_BUTTON_GO_HOME, m_buttonGoHome);
	DDX_Control(pDX, IDC_BUTTON_LASER_STATUS, m_buttonLaserStatus);
	DDX_Control(pDX, IDC_SLIDER_STEER, m_sliderSteer);
	DDX_Control(pDX, IDC_STATIC_LASER, m_staticLaser);
	DDX_Control(pDX, IDC_STATIC_MAG, m_staticMag);
	DDX_Control(pDX, IDC_BUTTON_MAG_ON, m_butMagOn);

	DDX_Check(pDX, IDC_CHECK_MAG_ENABLE, m_bEnableMAG);
	DDX_Check(pDX, IDC_CHECK_PREDRIVE, m_bPredrive);
	DDX_Check(pDX, IDC_CHECK_SEEK_BLACK_LINE, m_bSeekBlackLine);
	DDX_Check(pDX, IDC_CHECK_AUTOHOME, m_bStartScanAtHomePos);
	DDX_Check(pDX, IDC_CHECK_RETURNTOSTART, m_bReturnToStart);
	DDX_Check(pDX, IDC_CHECK_CALIBRATE, m_bCalibrate);
	DDX_Check(pDX, IDC_CHECK_SEEK_START_REVERSE, m_bSeekStartLineInReverse);
	DDX_Check(pDX, IDC_CHECK_SEEK_WITH_LASER, m_bSeekWithLaser);
	DDX_Check(pDX, IDC_CHECK_SEEK_START_LINE, m_bSeekAndStartAtLine);

	DDX_Radio(pDX, IDC_RADIO_CIRC, m_nScanType);

	DDX_Text(pDX, IDC_STATIC_HOMEDIST, m_szHomeDist);
	DDX_Text(pDX, IDC_STATIC_SCANNEDDIST, m_szScannedDist);
	DDX_Text(pDX, IDC_STATIC_LASER_EDGE, m_szLaserEdge);
	DDX_Text(pDX, IDC_STATIC_JOINT_LOCN, m_szLaserJoint);
	DDX_Text(pDX, IDC_STATIC_TEMP_BOARD, m_szTempBoard);
	DDX_Text(pDX, IDC_STATIC_TEMP_LASER, m_szTempLaser);
	DDX_Text(pDX, IDC_STATIC_TEMP_SENSOR, m_szTempSensor);
	DDX_Text(pDX, IDC_STATIC_RUN_TIME, m_szRunTime);

	// the min,max definitions are in define.h
	DDX_Text(pDX, IDC_EDIT_LR_OFFSET, m_szLROffset);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, GetLeftRightOffset(), -MAX_LASER_OFFSET, MAX_LASER_OFFSET);

	DDX_Text(pDX, IDC_EDIT_CIRC, m_fScanCirc);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fScanCirc, MIN_PIPE_CIRCUMFEENCE, MAX_PIPE_CIRCUMFEENCE);

	DDX_Text(pDX, IDC_EDIT_DIST, m_fDistToScan);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fDistToScan, MIN_SCAN_DISTANCE, MAX_SCAN_DISTANCE);

	DDX_Text(pDX, IDC_EDIT_OVERLAP, m_fScanOverlap);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fScanOverlap, MIN_SCAN_OVERLAP, MAX_SCAN_OVERLAP);

	DDX_Text(pDX, IDC_EDIT_SPEED, m_fMotorScanSpeed);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fMotorScanSpeed, MIN_MOTOR_SPEED, MAX_MOTOR_SPEED);

	DDX_Text(pDX, IDC_EDIT_ACCEL2, m_fMotorScanAccel);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fMotorScanAccel, MIN_MOTOR_ACCEL, MAX_MOTOR_ACCEL);

	DDX_Text(pDX, IDC_EDIT_PREDRIVE, m_fPredriveDistance);
	if (m_bCheck && m_bPredrive)
		DDV_MinMaxDouble(pDX, m_fPredriveDistance, MIN_PREDRIVE_DIST, MAX_PREDRIVE_DIST);

	if (pDX->m_bSaveAndValidate)
	{
		int delay = max((int)(1/*mm*/ * 1000.0 / m_fMotorScanSpeed + 0.5), 1);
		SetTimer(TIMER_GET_LASER_PROFILE, delay, NULL);

		m_fScanLength = (m_nScanType == 0) ? m_fScanCirc + m_fScanOverlap : m_fDistToScan;
	}

	DDX_Text(pDX, IDC_EDIT_SEEK_START_LINE, m_fSeekAndStartAtLine);
	if (m_bCheck && m_bSeekAndStartAtLine)
		DDV_MinMaxDouble(pDX, m_fSeekAndStartAtLine, MIN_SEEK_START_LINE_DIST, max(m_fScanLength, MAX_SEEK_START_LINE_DIST));
}


BEGIN_MESSAGE_MAP(CDialogGirthWeld, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_WM_HSCROLL()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LR_OFFSET, &CDialogGirthWeld::OnDeltaposSpinLrOffset)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_CIRC, &CDialogGirthWeld::OnDeltaposSpinScanCirc)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_DIST, &CDialogGirthWeld::OnDeltaposSpinScanDist)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_OVERLAP, &CDialogGirthWeld::OnDeltaposSpinScanOverlap)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_STEER, &CDialogGirthWeld::OnReleasedcaptureSliderSteer)


	ON_BN_CLICKED(IDC_BUTTON_PAUSE,			&CDialogGirthWeld::OnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_MANUAL,		&CDialogGirthWeld::OnClickedButtonScan)
	ON_BN_CLICKED(IDC_BUTTON_FWD,			&CDialogGirthWeld::OnClickedButtonFwd)
	ON_BN_CLICKED(IDC_BUTTON_BACK,			&CDialogGirthWeld::OnClickedButtonBack)
	ON_BN_CLICKED(IDC_BUTTON_ZERO_HOME,		&CDialogGirthWeld::OnClickedButtonZeroHome)
	ON_BN_CLICKED(IDC_BUTTON_GO_HOME,		&CDialogGirthWeld::OnClickedButtonGoHome)
	ON_BN_CLICKED(IDC_CHECK_RETURNTOSTART,	&CDialogGirthWeld::OnClickedCheckReturnToStart)
	ON_BN_CLICKED(IDC_CHECK_AUTOHOME,		&CDialogGirthWeld::OnClickedCheckGoToHome)
	ON_BN_CLICKED(IDC_CHECK_SEEK_START_LINE, &CDialogGirthWeld::OnClickedCheckSeekStartLine)
	ON_BN_CLICKED(IDC_CHECK_PREDRIVE,		&CDialogGirthWeld::OnClickedCheckPredrive)
	ON_BN_CLICKED(IDC_CHECK_CALIBRATE, &CDialogGirthWeld::OnClickedCheckCalibrate)
	ON_BN_CLICKED(IDC_CHECK_MAG_ENABLE, &CDialogGirthWeld::OnClickedCheckEnableMag)

	ON_COMMAND(IDC_RADIO_CIRC,				&CDialogGirthWeld::OnRadioScanType)
	ON_COMMAND(IDC_RADIO_DIST,				&CDialogGirthWeld::OnRadioScanType)

	ON_MESSAGE(WM_USER_SCAN_FINISHED,		&CDialogGirthWeld::OnUserScanFinished)
	ON_MESSAGE(WM_USER_ABORT_FINISHED,		&CDialogGirthWeld::OnUserAbortFinished)
	ON_MESSAGE(WM_USER_STATIC,				&CDialogGirthWeld::OnUserStaticParameter)
	ON_MESSAGE(WM_WELD_NAVIGATION,			&CDialogGirthWeld::OnUserWeldNavigation)
	ON_MESSAGE(WM_MOTION_CONTROL,			&CDialogGirthWeld::OnUserMotionControl)
	ON_MESSAGE(WM_MAG_STOP_SEEK,			&CDialogGirthWeld::OnUserMagStopSeek)
	ON_MESSAGE(WM_ARE_MOTORS_RUNNING,		&CDialogGirthWeld::OnUserAreMotorsRunning)
	
	ON_EN_CHANGE(IDC_EDIT_LR_OFFSET, &CDialogGirthWeld::OnChangeEditLrOffset)

	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_SPEED, &CDialogGirthWeld::OnDeltaposSpinScanSpeed)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ACCEL2, &CDialogGirthWeld::OnDeltaposSpinAccel2)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_PREDRIVE, &CDialogGirthWeld::OnDeltaposSpinPredrive)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_SEEK_START, &CDialogGirthWeld::OnDeltaposSpinSeekStart)
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
	m_spinScanSpeed.SetRange(0, UD_MAXVAL);
	m_spinScanAccel.SetRange(0, UD_MAXVAL);
	m_spinPredrive.SetRange(0, UD_MAXVAL);
	m_spinSeekStart.SetRange(0, UD_MAXVAL);

	// the slider is used to manually steer the unit
	// if let go of the slier, it auto positions to the centre (0)
	// set the range as -100 -> 100 percent of a maximum turn
	m_sliderSteer.SetRange(-100, 100);
	m_sliderSteer.SetTicFreq(10);
	m_sliderSteer.SetPos(0);

	// disconnect is a red dot, connect is a green dot
	// these indicate if the magnets are engaged
	m_bitmapDisconnect.LoadBitmap(IDB_BITMAP_DISCONNECT);
	m_bitmapConnect.LoadBitmap(IDB_BITMAP_CONNECT);
	HBITMAP hBitmap1 = (HBITMAP)m_bitmapDisconnect.GetSafeHandle();
	m_butMagOn.SetBitmap(hBitmap1);

	// will chage controls bitmaps depending on the state
	m_bitmapPause.LoadBitmap(IDB_BITMAP_PAUSE); // mif paused, this becomes a continue arrow
	m_bitmapGoRight.LoadBitmap(IDB_BITMAP_GO_RIGHT); // pause becomes a right arrow for continuje if paused
//	m_bitmapGoLeft.LoadBitmap(IDB_BITMAP_GO_LEFT);
	m_bitmapGoDown.LoadBitmap(IDB_BITMAP_GO_DOWN); // drive forward/back use these bitmaps
	m_bitmapGoUp.LoadBitmap(IDB_BITMAP_GO_UP);
	m_bitmapStop.LoadBitmap(IDB_BITMAP_STOP); // when driving, replace the bitmap clicked with tjhis

	// these are used to display the laser status
	// not fully implemented at this time (911)
	m_bitmapLaserOff.LoadBitmap(IDB_BITMAP_LASER_OFF); // 
	m_bitmapLaserOK.LoadBitmap(IDB_BITMAP_LASER_OK);
	m_bitmapLaserError.LoadBitmap(IDB_BITMAP_LASER_ERROR);
	m_bitmapLaserLoading.LoadBitmap(IDB_BITMAP_LASER_LOADING);
	m_bitmapLaserHot.LoadBitmap(IDB_BITMAP_LASER_HOT);

//	m_brRed.CreateSolidBrush(RGB(255, 0, 0));
//	m_brGreen.CreateSolidBrush(RGB(0, 255, 0));
//	m_brBlue.CreateSolidBrush(RGB(0255, 0, 255));
//	m_brMagenta.CreateSolidBrush(RGB(255, 0, 255));

	// statis used to display the laser and the MAG values during calibration and seek start lijne
	m_wndLaser.Create(&m_staticLaser);
	m_wndMag.Create(&m_staticMag);

	// used to pass messages back to this dialog
	m_wndMag.Init(this, WM_MAG_STOP_SEEK);
	m_weldNavigation.Init(this, WM_WELD_NAVIGATION);

	// note that initialized, this sdaves checking a control tosee if it has been created
	m_bInit = TRUE;
	SetButtonBitmaps();

//	SetLaserStatus(TIMER_LASER_OFF);

	SetTimer(TIMER_LASER_STATUS1, TIMER_LASER_STATUS1_INTERVAL, NULL); //  show the laser status as well as the weld cap parmeters evewry 500 ms if the laser is on
	SetTimer(TIMER_LASER_TEMPERATURE, TIMER_LASER_TEMPERATURE_INTERVAL, NULL); // note the laser temperature every 500 ms if the laser is on
	SetTimer(TIMER_RGB_STATUS, TIMER_RGB_STATUS_INTERVAL, NULL); // this only uses the saved status in the MAG controller
	SetTimer(TIMER_NOTE_RGB, TIMER_NOTE_RGB_INTERVAL, NULL); // if not navigating, this notes the RGB value, each read of the register takes about 200 ms. If seeking a line, this must be as fast as poossible
	SetTimer(TIMER_ARE_MOTORS_RUNNING, TIMER_ARE_MOTORS_RUNNING_INTERVAL, NULL); // threads can not talk to the motor controller directly, so get main thread to note

	// try to get the laser profile every 1 mm
	int delay = max((int)(TIMER_GET_LASER_PROFILE_INTERVAL/*mm*/ * 1000.0 / m_fMotorScanSpeed + 0.5), 1);
	SetTimer(TIMER_GET_LASER_PROFILE, delay, NULL);

	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogGirthWeld::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_GIRTHWELD, pParent);
	ShowWindow(SW_HIDE);
}

// serialize user inputs
// if new values added or removed, the mask will failt and thjey will be defaulted and not left invalid
void CDialogGirthWeld::Serialize(CArchive& ar)
{
	const int MASK = 0xCDCDCDCD;
	int mask = MASK;
	if (ar.IsStoring())
	{
		UpdateData(TRUE);
		ar << m_szLROffset;
		ar << m_fScanCirc;
		ar << m_fDistToScan;
//		ar << m_fDistScanned;
		ar << m_fScanOverlap;
		ar << m_nScanType;
		ar << m_bStartScanAtHomePos;
		ar << m_bReturnToStart;
		ar << m_bSeekAndStartAtLine;
		ar << m_fSeekAndStartAtLine;
		ar << m_bSeekStartLineInReverse;
		ar << m_bSeekWithLaser;
		ar << m_fMotorScanSpeed;
		ar << m_fMotorScanAccel;
		ar << m_fPredriveDistance;
		ar << m_bPredrive;
		ar << m_bSeekBlackLine;
		ar << m_bCalibrate;
		ar << mask;
	}
	else
	{
		try
		{
			ar >> m_szLROffset;
			ar >> m_fScanCirc;
			ar >> m_fDistToScan;
//			ar >> m_fDistScanned;
			ar >> m_fScanOverlap;
			ar >> m_nScanType;
			ar >> m_bStartScanAtHomePos;
			ar >> m_bReturnToStart;
			ar >> m_bSeekAndStartAtLine;
			ar >> m_fSeekAndStartAtLine;
			ar >> m_bSeekStartLineInReverse;
			ar >> m_bSeekWithLaser;
			ar >> m_fMotorScanSpeed;
			ar >> m_fMotorScanAccel;
			ar >> m_fPredriveDistance;
			ar >> m_bPredrive;
			ar >> m_bSeekBlackLine;
			ar >> m_bCalibrate;
			ar >> mask;
		}
		catch (CArchiveException * e1)
		{
			ResetParameters();
			e1->Delete();

		}
		if (mask != MASK)
			ResetParameters();

		UpdateData(FALSE);
	}
}

void CDialogGirthWeld::ResetParameters()
{
	m_szLROffset = _T("0.0");
	m_fScanCirc = DFLT_PIPE_CIRCUMFERENCE;
	m_fDistToScan = DFLT_SCAN_DISTANCE;
//	m_fDistScanned = 10;
	m_fScanOverlap = DFLT_SCAN_OVERLAP;
	m_nScanType = SCAN_TYPE_CIRCUMFERENCE;
	m_bStartScanAtHomePos = FALSE;
	m_bReturnToStart = FALSE;
	m_bSeekAndStartAtLine = FALSE;
	m_fSeekAndStartAtLine = 0;
	m_bSeekStartLineInReverse = FALSE;
	m_bSeekWithLaser = FALSE;
	m_fMotorScanSpeed = DFLT_MOTOR_SPEED;
	m_fMotorScanAccel = DFLT_MOTOR_ACCELERATION;
	m_fPredriveDistance = 0;
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
// use timers to poll various values, as well as diaplay them
// note that times can be called during Sleep() time, causing code to be re-entrant
void CDialogGirthWeld::OnTimer(UINT_PTR nIDEvent)
{
	clock_t t1 = clock();

	switch (nIDEvent)
	{
	case TIMER_SHOW_MOTOR_SPEEDS: // only every 500 ms
		ShowMotorPosition();
		break;

	case TIMER_LASER_TEMPERATURE: // only every 500 ms
		ShowLaserTemperature();
		break;
	case TIMER_LASER_STATUS1:
		ShowLaserStatus();
		break;
	case TIMER_RGB_STATUS:
		ShowRGBStatus();
		break;

	case TIMER_GET_LASER_PROFILE: // once per mm of travel
		GetLaserProfile();
		break;

	case TIMER_ARE_MOTORS_RUNNING:
		NoteIfMotorsRunning();
		break;

	case TIMER_RUN_TIME:
		NoteRunTime();
		break;

	case TIMER_NOTE_RGB:
		NoteRGBSum();
		break;

	case TIMER_NOTE_CALIBRATION:
		NoteRGBCalibration();
		break;

	default:
		break;
	}
#ifdef _DEBUG_TIMING_
	g_CDialogGirthWeldOnTimerTime += clock() - t1;;
	g_CDialogGirthWeldOnTimerCount++;
#endif
}

// this is used to find either a start line or a weld bump
// for now the reading of the RGB value is slow (200 ms) and if crawler too fast will miss it
double CDialogGirthWeld::GetCalibrationValue()
{
	if (m_bSeekWithLaser)
	{
		if (m_laserControl.GetProfile()) // 
		{
			m_laserControl.CalcLaserMeasures(0.0, NULL, -1);
			LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();
			double avg_side_height = (measure2.weld_left_height_mm + measure2.weld_right_height_mm) / 2;
			double weld_cap_height = measure2.weld_cap_mm.y;

			return -weld_cap_height; // the weld cap height is always -Ve, want to show as a positive value
//			return avg_side_height - weld_cap_height; // the sides will not be well calculated
		}
		else
			return FLT_MAX;
	}
	else
		return m_magControl.GetRGBSum(); // thjis is read from the MAG register, and takes about 200 ms (911)
}

// get eiother the RGB sum or the weld cap height for calibration
void CDialogGirthWeld::NoteRGBCalibration()
{
	CRect rect;
	double pos = m_motionControl.GetAvgMotorPosition();

	// while searching for the start location add the value to a list to be display
	if (m_nCalibratingRGB == CALIBRATE_RGB_SCANNING)
	{
		double val = GetCalibrationValue();
		m_magControl.NoteRGBCalibration(pos, val, (int)(m_fSeekAndStartAtLine + 0.5));

	}

	// while drving to the detected start location, not the current crawler location
	else if(m_nCalibratingRGB == CALIBRATE_RGB_RETURN )
		m_magControl.SetRGBCalibrationPos(pos);

	// redraw the statis each time added to
	m_wndMag.InvalidateRgn(NULL);
}

// note how long a scan has bveen running
void CDialogGirthWeld::NoteRunTime()
{
	clock_t t1 = clock();
	long elapsed = (long)(t1 - m_nRunStart);
	m_szRunTime.Format("%d.%d sec", elapsed / 1000, elapsed % 10);

	// UpdateData(FALSE) would cause all controls to be updated
	// faster to update only the control in question
	GetDlgItem(IDC_STATIC_RUN_TIME)->SetWindowText(m_szRunTime);
#ifdef _DEBUG_TIMING_
	g_NoteRunTimeTime += clock() - t1;
	g_NoteRunTimeCount++;
#endif
}

// let the main thread check every 1200 ms if the motorts are running and sdave to a BOOL that all threads can look at
void CDialogGirthWeld::NoteIfMotorsRunning()
{
	clock_t t1 = clock();
	m_motionControl.NoteIfMotorsRunning();
#ifdef _DEBUG_TIMING_
	g_NoteIfMotorsRunningTime += clock() - t1;
	g_NoteIfMotorsRunningCount++;
#endif
}

// get the current laser profile and sdave to a membger variable of the laser control;
void CDialogGirthWeld::GetLaserProfile()
{
	static clock_t tim0 = clock();
	static clock_t tim1 = 0;
	static int nLaserOn1 = -1;
	clock_t t1 = clock();

	// check if the laser on has changed and invalidate the laser static if it has
	int nLaserOn2 = m_laserControl.IsLaserOn();
	if (nLaserOn1 != nLaserOn2)
	{
		m_wndLaser.InvalidateRgn(NULL);
		nLaserOn1 = nLaserOn2;
	}

	// no profgile to get if not on
	// m_lastCapPix not being used at this time (911)
	// it was an attempt to window ehere to find the lasewr cap
	if (!nLaserOn2)
	{
		m_lastCapPix = -1;
		return;
	}

	// try to get the lasewr profile
	// this sometimes fails, so will try 10 times
	if( !m_laserControl.GetProfile())
		return;

	// while not part of the laser, these values are used in conjunction with the profgile, so save with it
	double accel, velocity4[4];
	double pos = m_motionControl.GetAvgMotorPosition();
	velocity4[0] = m_motionControl.GetMotorSpeed("A", accel);
	velocity4[1] = m_motionControl.GetMotorSpeed("B", accel);
	velocity4[2] = m_motionControl.GetMotorSpeed("C", accel);
	velocity4[3] = m_motionControl.GetMotorSpeed("D", accel);

	// calcualte the vari9ous laser measures
	// offset, height, side height etc.
	// m_lastCapPix is not used at this tyime (911)
	m_lastCapPix = m_laserControl.CalcLaserMeasures(pos, velocity4, m_lastCapPix);
#ifdef _DEBUG_TIMING_
	if( g_fp1 )
		fprintf(g_fp1, "%d\t%.1f\n", clock()-tim0, pos);
#endif

	// only invalidate the laser static about 4 times per second
	// this is acqwuired much too often to draw every timne
	clock_t tim2 = clock();
	if (tim1 == 0 || tim2 - tim1 > 250) // 4 times per seconmd max.
	{
		tim1 = tim2;
		m_wndLaser.InvalidateRgn(NULL);
	}
#ifdef _DEBUG_TIMING_
	g_GetLaserProfileTime += (clock() - t1);
	g_GetLaserProfileCount++;
#endif
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
		m_motionControl.SetSlewSpeed(m_fMotorScanSpeed / rate, m_fMotorScanSpeed * rate, m_fMotorScanSpeed * rate, m_fMotorScanSpeed / rate);
	}
	else if (gap > 0.25) // slow down A & D
	{
		GetDlgItem(IDC_STATIC_LEFT)->SetWindowText("Left");
		GetDlgItem(IDC_STATIC_RIGHT)->SetWindowText(str);
		m_motionControl.SetSlewSpeed(m_fMotorScanSpeed * rate, m_fMotorScanSpeed / rate, m_fMotorScanSpeed / rate, m_fMotorScanSpeed * rate);
	}
	else // all the same
	{
		GetDlgItem(IDC_STATIC_LEFT)->SetWindowText("Left");
		GetDlgItem(IDC_STATIC_RIGHT)->SetWindowText("Right");
		m_motionControl.SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanSpeed, m_fMotorScanSpeed, m_fMotorScanSpeed);
	}
}
*/

// the laser status is not being set at thjis time
// this was an attempt to emulate the light on the laser itself (911)
void CDialogGirthWeld::ShowLaserStatus()
{
	clock_t t1 = clock();
	HBITMAP hBitmap = NULL;

	static unsigned short LaserStatus1 = -1;
	SensorStatus SensorStatus;
	BOOL ret = m_laserControl.GetLaserStatus(SensorStatus);

	if (!ret || SensorStatus.LaserStatus != LaserStatus1 )
	{
		if( !ret )
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

		if (hBitmap)
			m_buttonLaserStatus.SetBitmap(hBitmap);

		LaserStatus1 = SensorStatus.LaserStatus;
	}

	// note the 'X' location of the weld
	// this will likely change on every call
	CDoublePoint joint = m_wndLaser.GetJointPos();
	if (joint.IsSet())
	{
		const char szSide[] = { "LR" };
		m_szLaserJoint.Format("Off: %.1f %c, Hgt: %.1f mm", 
			fabs(joint.x), szSide[joint.x < 0],
			joint.y ); // height above the average of the sides

		// list as distance from the cap height
		m_szLaserEdge.Format("%.1f / %.1f / %.1f",
			joint.y - m_wndLaser.GetEdgePos(1).y,
			m_wndLaser.GetEdgePos(2).y,
			joint.y - m_wndLaser.GetEdgePos(0).y ); // height, so note to one decimal point
	}
	else
	{
		m_szLaserEdge = _T("");
		m_szLaserJoint = _T("");
	}

	GetDlgItem(IDC_STATIC_JOINT_LOCN)->SetWindowText(m_szLaserJoint);
	GetDlgItem(IDC_STATIC_LASER_EDGE)->SetWindowText(m_szLaserEdge);

	// check if the magnets have changed state
	// onmly set the bitmaps on a change not every time in here
	// otherwise the controls will flicker
	static int bMagEngaged = -1;
	BOOL bMag = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;
	if (bMag != bMagEngaged)
	{
		bMagEngaged = bMag;
		SetButtonBitmaps();
	}
#ifdef _DEBUG_TIMING_
	g_ShowLaserStatusTime += clock() - t1;
	g_ShowLaserStatusCount++;
#endif
}
	
// show the lasewr temperature
// only set the statidcs if the temperature has changed
void CDialogGirthWeld::ShowLaserTemperature()
{
	clock_t t1 = clock();
	// note if the temperature has changeds
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
	{
		m_szTempBoard = m_szTempLaser = m_szTempSensor = _T("Off");
	}

	if (szTemp1.Compare(m_szTempBoard) != 0)
		GetDlgItem(IDC_STATIC_TEMP_BOARD)->SetWindowText(m_szTempBoard);

	if (szTemp2.Compare(m_szTempLaser) != 0)
		GetDlgItem(IDC_STATIC_TEMP_LASER)->SetWindowText(m_szTempLaser);

	if (szTemp3.Compare(m_szTempSensor) != 0)
		GetDlgItem(IDC_STATIC_TEMP_SENSOR)->SetWindowText(m_szTempSensor);

#ifdef _DEBUG_TIMING_
	g_ShowLaserTemperatureTime += clock() - t1;
	g_ShowLaserTemperatureCount++;
#endif
}

// after every scan, a file is created with measures
// tell the file dialog to list this file
void CDialogGirthWeld::UpdateScanFileList()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_UPDATE_FILE_LIST);
}

// avoid getting RGB register data if navigating
// it takes about 200 ms, and is way too slow!
// this avoids having to enable and disable the timner
void CDialogGirthWeld::NoteRGBSum()
{
	// this takes too long if navigating, also if looking for a start line, the RGB is read seperatly
	if (!m_weldNavigation.IsNavigating() && m_nCalibratingRGB == CALIBRATE_RGB_NOT )
	{
		clock_t t1 = clock();
		m_rgbLast = m_magControl.GetRGBSum();
		m_wndLaser.AddRGBData(m_rgbLast);
#ifdef _DEBUG_TIMING_
		g_NoteRGBTime += clock() - t1;
		g_NoteRGBCount++;
#endif
	}

}

// avoid getting RGB register data if navigating
// it takes about 200 ms, and is way too slow!
// this avoids having to enable and disable the timner
// the status is in a status buffer, and does not require a read at this point
void CDialogGirthWeld::ShowRGBStatus()
{
	// the status here does not require a read
	clock_t t1 = clock();
	int bPresent = m_magControl.GetMagStatus(MAG_IND_RGB_LINE) == 1;
	m_szRGBLinePresent.Format(bPresent ? _T("Present") : _T(""));


#ifdef _DEBUG_TIMING_
	g_ShowRGBStatusTime = clock() - t1;
	g_ShowRGBStatusCount++;
#endif
}


// show the positoon as the average of the four motors
// the motor position is the home ;osition
// the scanned position may not start at home, thus may vary
void CDialogGirthWeld::ShowMotorPosition()
{
	clock_t t1 = clock();

	double dist1 = atof(m_szHomeDist);
	double dist2 = atof(m_szScannedDist);

	// if not steering, then get the encoder count
	// getting MAG status aznd registyers seems to be slow
	if (!m_weldNavigation.IsNavigating())
		m_magControl.GetMagStatus();

	// the encoder count is in a saved status array
	// so does not take any time
	double pos = m_motionControl.GetAvgMotorPosition();
	double fEncoderDistance = m_weldNavigation.IsNavigating() ? FLT_MAX : m_magControl.GetEncoderDistance();

	if (pos == FLT_MAX)
	{
		m_szHomeDist.Format("");
		m_szScannedDist.Format("");
	}
	else
	{
		// the distance from 'home'
		// ujnlioke the scan distance, allow this to be negative
		if (fEncoderDistance == FLT_MAX)
			m_szHomeDist.Format("%.1f", pos);
		else
			m_szHomeDist.Format("%.1f (%.1f)", pos, fEncoderDistance);

		// the distance that have scanned
		// is current position less the start location
		if (m_fScanStartPos == FLT_MAX || !m_bScanning || pos - m_fScanStartPos < 0 )
			m_szScannedDist.Format("");
		else
			m_szScannedDist.Format("%.1f", pos - m_fScanStartPos);
	}

	// the scanned distance requires the start location to be noted
	// this is cleaner than using UpdarteData()
	GetDlgItem(IDC_STATIC_HOMEDIST)->SetWindowTextA(m_szHomeDist);
	GetDlgItem(IDC_STATIC_SCANNEDDIST)->SetWindowTextA(m_szScannedDist);
#ifdef _DEBUG_TIMING_
	g_ShowMotorPositionTime += clock() - t1;
	g_ShowMotorPositionCount++;
#endif
}

// do not want to check control values on every call to UpdateData(TRUE)
// only when about to scan, etc.
BOOL CDialogGirthWeld::CheckVisibleTab()
{
	m_bCheck = TRUE; // enable DDV calls
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret;
}

// this will change the text in the laser temperature controls to RED if over 50 C
HBRUSH CDialogGirthWeld::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	SensorStatus SensorStatus;
	LASER_TEMPERATURE temp;
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_TEMP_BOARD)
	{
		//set the static text color to red     
		m_laserControl.GetLaserTemperature(temp);
		if( temp.BoardTemperature > MAX_LASER_TEMPERATURE )
			pDC->SetTextColor(RGB(255, 0, 0));
	}

	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_TEMP_LASER)
	{
		//set the static text color to red     
		m_laserControl.GetLaserTemperature(temp);
		m_laserControl.GetLaserStatus(SensorStatus);

		if (temp.LaserTemperature > MAX_LASER_TEMPERATURE)
			pDC->SetTextColor(RGB(255, 0, 0));
	}

	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_TEMP_SENSOR)
	{
		//set the static text color to red     
		m_laserControl.GetLaserTemperature(temp);
		m_laserControl.GetLaserStatus(SensorStatus);

		if (temp.SensorTemperature > MAX_LASER_TEMPERATURE)
			pDC->SetTextColor(RGB(255, 0, 0));
	}

	// TODO: Return a different brush if the default is not desired   
	return hbr;
}

// label the requested offset as L/R not +Ve -Ve
double CDialogGirthWeld::GetLeftRightOffset()const
{
	double val = atof(m_szLROffset);
	if (val < 0)
		return val;
	else if (m_szLROffset.Find("R") != -1)
		return -val;
	else
		return val;
}


void CDialogGirthWeld::FormatLeftRightOffset(double offset)
{
	if (offset > 0)
		m_szLROffset.Format("%.1f L", offset);
	else if (offset < 0)
		m_szLROffset.Format("%.1f R", -offset);
	else
		m_szLROffset.Format("0.0");
}

// distane to back up prior to start to enable crawler to get over cap prior to the scan
void CDialogGirthWeld::OnDeltaposSpinPredrive(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fPredriveDistance += (inc > 0) ? 10 : -10;
	m_fPredriveDistance = min(max(m_fPredriveDistance, MIN_PREDRIVE_DIST), MAX_PREDRIVE_DIST);
	UpdateData(FALSE);
	*pResult = 0;
}

// DISTANCE TO seek the staRt line
void CDialogGirthWeld::OnDeltaposSpinSeekStart(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fSeekAndStartAtLine += (inc > 0) ? 10 : -10;
	m_fSeekAndStartAtLine = min(max(m_fSeekAndStartAtLine, MIN_SEEK_START_LINE_DIST), MAX_SEEK_START_LINE_DIST);
	UpdateData(FALSE);
	*pResult = 0;
}


void CDialogGirthWeld::OnDeltaposSpinLrOffset(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	double offset = GetLeftRightOffset();
	offset += (inc > 0) ? 0.1 : -0.1;
	offset = min(max(offset, -MAX_LASER_OFFSET), MAX_LASER_OFFSET);
	FormatLeftRightOffset(offset);
	UpdateData(FALSE);

	*pResult = 0;
}

// if change the L/R offset valuje manually, put the L/R indicator back in
void CDialogGirthWeld::OnChangeEditLrOffset()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	if (!m_bInit)
		return;

	UpdateData(TRUE);
	double offset = GetLeftRightOffset();
	FormatLeftRightOffset(offset);
	UpdateData(FALSE);
}


void CDialogGirthWeld::OnDeltaposSpinScanCirc(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fScanCirc += (inc > 0) ? 1 : -1;
	m_fScanCirc = min(max(m_fScanCirc, MIN_PIPE_CIRCUMFEENCE), MAX_PIPE_CIRCUMFEENCE);
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
	m_fDistToScan = min(max(m_fDistToScan, MIN_SCAN_DISTANCE), MAX_SCAN_DISTANCE);
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
	m_fScanOverlap = min(max(m_fScanOverlap, MIN_SCAN_OVERLAP), MAX_SCAN_OVERLAP);
	UpdateData(FALSE);

	*pResult = 0;
}

// this is used in _DEBG to check what is happending
void CDialogGirthWeld::SendDebugMessage(const CString& msg)
{
#ifdef _DEBUG_TIMING_
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
#endif
}

// used to indicate errors
// better then using AfxMessagewBox()
void CDialogGirthWeld::SendErrorMessage(const char* msg)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)msg);
}

// enable/disable and set bitmap
void CDialogGirthWeld::SetButtonBitmaps()
{
	// TODO: Add your control notification handler code here
//	SendDebugMessage("SetButtonBitmaps"); // happenbs toio often to auto put in statuys

	BOOL bGalil = m_motionControl.IsConnected();
	BOOL bMag = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;
	int bMagOn = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;

	HBITMAP hBitmapRight = (HBITMAP)m_bitmapGoRight.GetSafeHandle();
//	HBITMAP hBitmapLeft = (HBITMAP)m_bitmapGoLeft.GetSafeHandle();
	HBITMAP hBitmapUp = (HBITMAP)m_bitmapGoUp.GetSafeHandle();
	HBITMAP hBitmapDown = (HBITMAP)m_bitmapGoDown.GetSafeHandle();
	HBITMAP hBitmapStop = (HBITMAP)m_bitmapStop.GetSafeHandle();
	HBITMAP hBitmapPause = (HBITMAP)m_bitmapPause.GetSafeHandle();
	HBITMAP hBitmapDisconnect = (HBITMAP)m_bitmapDisconnect.GetSafeHandle();
	HBITMAP hBitmapConnect = (HBITMAP)m_bitmapConnect.GetSafeHandle();

	m_butMagOn.SetBitmap((bMagOn == 1) ? hBitmapConnect : hBitmapDisconnect);
	m_buttonPause.SetBitmap(m_bPaused ? hBitmapRight : hBitmapPause);
	m_buttonManual.SetBitmap(m_bPaused ? hBitmapRight : hBitmapPause);


	// set to stop if running
	m_buttonManual.SetBitmap((m_nGalilState == GALIL_SCAN) ? hBitmapStop : hBitmapRight);
	// m_buttonGoHome.SetBitmap(m_nGalilState == GALIL_GOHOME ? hBitmapStop : hBitmapHorz);

	// fowd and back are either vertical or stop
	m_buttonFwd.SetBitmap(m_nGalilState == GALIL_FWD ? hBitmapStop : hBitmapUp);
	m_buttonBack.SetBitmap(m_nGalilState == GALIL_BACK ? hBitmapStop : hBitmapDown);
	m_buttonGoHome.SetBitmap(m_nGalilState == GALIL_GOHOME ? hBitmapStop : hBitmapRight);

	// disasble buttonbs if within a mm
	int pos1 = (int)(GetAvgMotorPosition() + 0.5);
	int pos2 = m_magControl.GetMagStatus(MAG_IND_ENC_CNT);
	BOOL bNotHome = pos1 != 0 || pos2 != 0;

	// set to 
	m_buttonManual.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_SCAN || m_nGalilState == GALIL_IDLE));
	m_buttonPause.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_SCAN)); // && !(m_bPaused && m_hThreadRunMotors != NULL) );
	m_buttonGoHome.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_GOHOME || m_nGalilState == GALIL_IDLE) && bNotHome);
	m_buttonZeroHome.EnableWindow(bGalil && bMag && m_nGalilState == GALIL_IDLE && bNotHome );
	m_sliderSteer.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_FWD || m_nGalilState == GALIL_BACK) && !m_bPaused);

	m_buttonBack.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_BACK || 
		(m_bPaused && m_nGalilState != GALIL_FWD)));
	m_buttonFwd.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_FWD || 
		(m_bPaused && m_nGalilState != GALIL_BACK)));

	GetDlgItem(IDC_CHECK_CALIBRATE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_LR_OFFSET)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_CIRC)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC);
	GetDlgItem(IDC_EDIT_OVERLAP)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC && !m_bCalibrate);
	GetDlgItem(IDC_EDIT_DIST)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_DIST);
	GetDlgItem(IDC_RADIO_CIRC)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && !m_bCalibrate);
	GetDlgItem(IDC_RADIO_DIST)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && !m_bCalibrate);
	GetDlgItem(IDC_CHECK_AUTOHOME)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && !m_bCalibrate);
	GetDlgItem(IDC_CHECK_RETURNTOSTART)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_SEEK_START_LINE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && !m_bCalibrate);
	GetDlgItem(IDC_CHECK_SEEK_START_REVERSE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bSeekAndStartAtLine);
	GetDlgItem(IDC_EDIT_SEEK_START_LINE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bSeekAndStartAtLine);
	GetDlgItem(IDC_CHECK_SEEK_WITH_LASER)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bSeekAndStartAtLine);
	GetDlgItem(IDC_CHECK_SEEK_BLACK_LINE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bSeekAndStartAtLine && !m_bSeekWithLaser);
	GetDlgItem(IDC_CHECK_MAG_ENABLE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);

	
	GetDlgItem(IDC_CHECK_PREDRIVE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && !m_bCalibrate);
	GetDlgItem(IDC_EDIT_PREDRIVE)->EnableWindow(bGalil && m_bPredrive && m_nGalilState == GALIL_IDLE && !m_bCalibrate);

	GetDlgItem(IDC_EDIT_SPEED)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_ACCEL2)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);


	GetDlgItem(IDC_STATIC_PAUSE)->SetWindowText(m_bPaused ? _T("Resume" : _T("Pause")));
}

// during a scan can click the pause button
// this will stop the motors, enable the Fwd/Back buttobns (but not steer)
// user can manually drive Fwd/Back, resume or cancel
void CDialogGirthWeld::OnClickedButtonPause()
{
	SendDebugMessage("OnClickedButtonPause");
	m_bPaused = !m_bPaused;

	// stop the motors, and resume
	if (m_bPaused)
	{
		// asking motors to stop will cause existing thread to end
		m_bResumeScan = FALSE; // set before stopping, so noted in the thread
		m_motionControl.StopMotors(TRUE); // this is main thread so call directly
		m_laserControl.TurnLaserOn(FALSE); // if stopped turn off laser, can always turn on manually
		SetButtonBitmaps(); // enable/disable controls as well as setr bitmaps
	}
	// resume the motors
	// this is similar to OnClickedButtonScan
	// except the position is not reset
	else
	{
		m_bResumeScan = TRUE; // need this to indicate that resuming not starting a new scan
		m_lastCapPix = -1;
		m_laserControl.TurnLaserOn(TRUE); // isnsure that the laser is on
		GetLaserProfile(); // get a first laser oprofile
		StartNotingMotorSpeed(TRUE);

		// all scanning is done in this thread
		// m_bPause, m_bResume, etc. indicate what it is to do
		m_pThreadScan = ::AfxMyBeginThread(::ThreadRunScan, (LPVOID)this);
		SetButtonBitmaps();
	}
}

// 1. check if to go to home position prior to run
// will tel the motors where to travel to, may not start at zero, so add starting position to the length desired
double CDialogGirthWeld::GetDestinationPosition()
{
	// the nominal length to scan
	double pos = m_fScanLength; // this is either the requested distamnce or circumgerence + overlap

	// if not travelling home first, then add the current position
	// else, assuem it will be zewro
	if (!m_bStartScanAtHomePos)
		pos += m_motionControl.GetAvgMotorPosition();

	return pos;
}

// when scanning, the MAG switch is disableed on the unit and reenabled at the end of the scan
// also indeicate that disabled in a control
void CDialogGirthWeld::EnableMagSwitchControl(BOOL bEnableMAG)
{
	m_bEnableMAG = bEnableMAG;
	m_magControl.EnableMagSwitchControl(m_bEnableMAG);
	CButton* pButton = (CButton*)GetDlgItem(IDC_CHECK_MAG_ENABLE); // un-check if disabvled
	pButton->SetCheck(bEnableMAG);
	SetButtonBitmaps();
}

// the scan button has been clicked
// this may be either to start a scan based on various other control values, or to abort a current scan
void CDialogGirthWeld::OnClickedButtonScan()
{
	// TODO: Add your control notification handler code here
// double check if to scan or abort
	SendDebugMessage("OnClickedButtonScan");
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_SCAN))
	{
		m_bPaused = FALSE;
		m_bResumeScan = FALSE;

		// change the state from scan to/from idel
		//
		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_SCAN : GALIL_IDLE;

		// changed to scan, so with to start a new scan
		if (m_nGalilState == GALIL_SCAN)
		{
			// reset the abort flag
			m_bAborted = FALSE;
			m_bScanning = TRUE;
			SetButtonBitmaps();

			// note the destination postiion abnd set the morot speeds
			m_fDestinationPosition = GetDestinationPosition();
			m_motionControl.SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanAccel);

			// disable the MAG switch so can't accidently drop it during a scan
			EnableMagSwitchControl(FALSE);

			// start noting tyhe run time as from this point on
			SetNoteRunTime(TRUE);

			// note the motor speed to the motors diaolog
			StartNotingMotorSpeed(TRUE);

			// srtart measuring the various laser measures to be used by navigation
			StartMeasuringLaser(TRUE);
			m_pThreadScan = ::AfxMyBeginThread(::ThreadRunScan, (LPVOID)this);
		}

		// changed to idel so wish to abort a current scan
		else
		{
			// niote that aborted anbd no longer scanning
			m_bAborted = TRUE;
			m_bScanning = FALSE;
			m_buttonManual.EnableWindow(FALSE);
			m_pThreadAbort = ::AfxMyBeginThread(::ThreadAbortScan, (LPVOID)this);
		}

		// this thread will enact what required
		// do in a scan so don't pause the main thread
	}
}

// tell the parent dialoog that to either start or stop reading the MAG statusd
void CDialogGirthWeld::StartReadMagStatus(BOOL bSet)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_MAG_STATUS_ON, bSet);
}

// this starts the navigation 
// nSteer: (0x1) start therasd to note the laser measures by unit position (0x2) start thread to navigate 
// start_pos: will request to go to a position pas the thend point, 
// end_pos: navigate will stop when hit this position, do not want all motors driving to the same position at the end, as this will stop some motors before otyhers
// fSpeed: the degautl speed of the motors that will vary to manoeuvre
void CDialogGirthWeld::StartNavigation(int nSteer, int start_pos, int end_pos, double fSpeed)
{
#ifdef _DEBUG_TIMING_
	extern clock_t g_ThreadReadSocketTime;
	extern int g_ThreadReadSocketCount;
	extern clock_t g_LaserOnPaintTime;
	extern int g_LaserOnPaintCount;
#endif

	if (nSteer)
	{
#ifdef _DEBUG_TIMING_
		g_GetLaserProfileTime = 0;
		g_GetLaserProfileCount = 0;
		g_CDialogGirthWeldOnTimerTime = 0;
		g_CDialogGirthWeldOnTimerCount = 0;
		g_ShowMotorPositionTime = 0;
		g_ShowMotorPositionCount = 0;
		g_ShowLaserTemperatureTime = 0;
		g_ShowLaserTemperatureCount = 0;
		g_ShowLaserStatusTime = 0;
		g_ShowLaserStatusCount = 0;
		g_ShowRGBStatusTime = 0;
		g_ShowRGBStatusCount = 0;
		g_NoteSteeringTime = 0;
		g_NoteSteeringCount = 0;
		g_LaserOnPaintTime = 0;
		g_LaserOnPaintCount = 0;
		g_NoteRunTimeTime = 0;
		g_NoteRunTimeCount = 0;
		g_NoteRGBTime = 0;
		g_NoteRGBCount = 0;
		g_GetSlewSpeedTime = 0;
		g_GetSlewSpeedCount = 0;
		g_NoteIfMotorsRunningTime = 0;
		g_NoteIfMotorsRunningCount = 0;
		g_ThreadReadSocketTime = 0;
		g_ThreadReadSocketCount = 0;

		char my_documents[MAX_PATH];
		HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
		if (result == S_OK)
		{
			CString szFile;
			szFile.Format("%s\\GetLaserProfile.txt", my_documents);
			fopen_s(&g_fp1, szFile, "w");
		}
#endif
		// when seeking th3e startt line, may run slower
		if (fSpeed == FLT_MAX)
			fSpeed = m_fMotorScanSpeed;

		// pass this request to the navigation object
		m_weldNavigation.StartNavigation(nSteer, start_pos, end_pos, fSpeed, m_fMotorScanAccel, GetLeftRightOffset(), m_bScanning);

		// these values are used to draw the lasser offset below the laser profile
		m_wndLaser.ResetLaserOffsetList();

		// when filtering the weld cap offset values, must be careful around known manoeuvre positions
		m_motionControl.ResetLastManoeuvrePosition();
	}
	else
	{
		// end any existing therads
		m_weldNavigation.StartNavigation(0x0, 0,0,0,0,0,0);
#ifdef _DEBUG_TIMING_
		CString text;
		text.Format("CalcLaserMeasures: %.1f (%d)", g_GetLaserProfileCount ? (double)g_GetLaserProfileTime / g_GetLaserProfileCount : 0, g_GetLaserProfileTime);
		SendDebugMessage(text);
		text.Format("LaserOnPaint: %.1f (%d)", g_LaserOnPaintCount ? (double)g_LaserOnPaintTime / g_LaserOnPaintCount : 0, g_LaserOnPaintTime);
		SendDebugMessage(text);
		text.Format("CDialogGirthWeldOnTimer: %.1f (%d)", g_CDialogGirthWeldOnTimerCount ? (double)g_CDialogGirthWeldOnTimerTime / g_CDialogGirthWeldOnTimerCount : 0, g_CDialogGirthWeldOnTimerTime);
		SendDebugMessage(text);
		text.Format("ShowMotorPosition: %.1f (%d)", g_ShowMotorPositionCount ? (double)g_ShowMotorPositionTime / g_ShowMotorPositionCount : 0, g_ShowMotorPositionTime);
		SendDebugMessage(text);
		text.Format("ShowLaserTemperature: %.1f (%d)", g_ShowLaserTemperatureCount ? (double)g_ShowLaserTemperatureTime / g_ShowLaserTemperatureCount : 0, g_ShowLaserTemperatureTime);
		SendDebugMessage(text);
		text.Format("ShowLaserStatus: %.1f (%d)", g_ShowLaserStatusCount ? (double)g_ShowLaserStatusTime / g_ShowLaserStatusCount : 0, g_ShowLaserStatusTime);
		SendDebugMessage(text);
		text.Format("ShowRGBStatus: %.1f (%d)", g_ShowRGBStatusCount ? (double)g_ShowRGBStatusTime / g_ShowRGBStatusCount : 0, g_ShowRGBStatusTime);
		SendDebugMessage(text);
		text.Format("NoteSteering: %.1f (%d)", g_NoteSteeringCount ? (double)g_NoteSteeringTime / g_NoteSteeringCount : 0, g_NoteSteeringTime);
		SendDebugMessage(text);
		text.Format("NoteRunTime: %.1f (%d)", g_NoteRunTimeCount ? (double)g_NoteRunTimeTime / g_NoteRunTimeCount : 0, g_NoteRunTimeTime);
		SendDebugMessage(text);
		text.Format("NoteRGBSum: %.1f (%d)", g_NoteRGBCount ? (double)g_NoteRGBTime / g_NoteRGBCount : 0, g_NoteRGBTime);
		SendDebugMessage(text);
		text.Format("GetSlewSpeed: %.1f (%d)", g_GetSlewSpeedCount ? (double)g_GetSlewSpeedTime / g_GetSlewSpeedCount : 0, g_GetSlewSpeedTime);
		SendDebugMessage(text);
		text.Format("NoteIfMotorsRunning: %.1f (%d)", g_NoteIfMotorsRunningCount ? (double)g_NoteIfMotorsRunningTime / g_NoteIfMotorsRunningCount : 0, g_NoteIfMotorsRunningTime);
		SendDebugMessage(text);
		text.Format("ThreadReadSocket: %.1f ms (%d)", g_ThreadReadSocketCount ? (double)g_ThreadReadSocketTime / g_ThreadReadSocketCount : 0, g_ThreadReadSocketTime);
		SendDebugMessage(text);

		if (g_fp1)
		{
			fclose(g_fp1);
			g_fp1 = NULL;
		}
#endif
	}
	// 911 takes too mjuch time, will have to enable if looking for line seen flag
	// last seen to take about 300 ms per register read
	StartReadMagStatus(nSteer == 0); 
}

// want to measure the laser about every 1 mm
// thus, call this on every ndrive, as the spewed may be changed
void CDialogGirthWeld::StartMeasuringLaser(BOOL bSet)
{
	if (bSet)
	{
		m_laserControl.TurnLaserOn(TRUE);
		int delay = max((int)(TIMER_GET_LASER_PROFILE_INTERVAL * 1000.0 / m_fMotorScanSpeed + 0.5), 1);
		SetTimer(TIMER_GET_LASER_PROFILE, delay, NULL);
		m_lastCapPix = -1; // not used at this time (911)
		//GetLaserProfile();
	}
	else
	{
		m_laserControl.TurnLaserOn(FALSE);
	}
	// never kill the timer
	// if laser off, then will not do anyting
//	else
//		KillTimer(TIMER_GET_LASER_PROFILE);
}

// enable noting morot speeds to the motors dialog
void CDialogGirthWeld::StartNotingMotorSpeed(BOOL bSet)
{
	if (bSet)
		SetTimer(TIMER_SHOW_MOTOR_SPEEDS, TIMER_SHOW_MOTOR_SPEEDS_INTERVAL, NULL);
	else
		KillTimer(TIMER_SHOW_MOTOR_SPEEDS);
}
/*
void CDialogGirthWeld::StartNotingRGBData(BOOL bSet)
{
	if (bSet)
	{
		double accel,speed = GetRequestedMotorSpeed(accel); // this uses a SendMessage, and must not be called from a thread
		int delay2 = (int)(1000.0 / speed / 4.0 + 0.5); // every 0.25 mm
		SetTimer(TIMER_NOTE_RGB, delay2, NULL);
		SetTimer(TIMER_RGB_STATUS, 500, NULL);
	}
	else
	{
		KillTimer(TIMER_NOTE_RGB);
		KillTimer(TIMER_RGB_STATUS);
	}
}
*/

// have clicked the GoHom,e bnutton
// thhis will return to tyhe preset (zero) position
// it is assumed that this is used after a drive
// thus, this will drive in revferse
// it does not calculate the forward position to get to home (that would not be zero)
void CDialogGirthWeld::OnClickedButtonGoHome()
{
	// TODO: Add your control notification handler code here
	SendDebugMessage("OnClickedButtonGoHome");
	if (CheckParameters() )
	{
		// savew re-enableling controls until after stop if not now starting
		// if the state is currently GOHOME, then aborting an earlier request
		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_GOHOME : GALIL_IDLE;
		if (m_nGalilState == GALIL_GOHOME)
		{
			// if the position is currently zero, then will wait in vain for the mjotors to start
			int start_pos = (int)m_motionControl.GetAvgMotorPosition();
			if (start_pos != 0)
			{
				SetButtonBitmaps();

				// navigation is used during this manoeuvre
				// as it may be in reverse, drive at 1/2 speed (911)
				// it is also possible that manually drove porior to home, and will drive forward
				StartNotingMotorSpeed(TRUE);
				m_motionControl.SetSlewSpeed(m_fMotorScanSpeed / 2, m_fMotorScanAccel);
				StartMeasuringLaser(TRUE);
				EnableMagSwitchControl(FALSE);

				// insure that not navigating when tell to start
				StartNavigation(0x0, 0, 0, 0);

				// the HOME podsition is always zero
				m_motionControl.GoToPosition(0.0, FALSE);

				// start the navigation at 1/2 speed
				StartNavigation(0x3, start_pos, 0, m_fMotorScanSpeed / 2);
			}
		}
		// if aborting a GoHome previous request, stop mnavigation now
		else
			StartNavigation(0x0, 0, 0, 0);

		// driving and stopping take time, so sue a thread
		m_pThreadScan = ::AfxMyBeginThread(::ThreadGoToHome, (LPVOID)this);
	}
}


// use a threasd to drive to home, or even to request a s stop 
UINT CDialogGirthWeld::ThreadGoToHome()
{
	if (m_nGalilState != GALIL_GOHOME)
		StopMotors(TRUE); // TRUE; the wait to stop is in this function
	else
	{
		// if the motors started, then wait an additional 1000 ms, before checkingt if stopped
		// due to acceleration and deceleration time as well as travel time
		// this wilol take much longert than 1 sec.
		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
	}

	// tell tyhe main thread that now stoppeds, so can reset controls
	PostMessage(WM_USER_SCAN_FINISHED);
	return 0;
}



// use this to insure thaT THE MAIN  THREAD IS C ALLING IT, NOT THE USER THREASD
// as these are doubles, and can only pass LPARAM, multipoly by 10
BOOL CDialogGirthWeld::SetSlewSpeed(double fSpeed, double fAccel)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		int iSpeed = (int)(fSpeed * 10 + 0.5);
		int iAccel = (int)(fAccel * 10 + 0.5);
		LPARAM lParam = (iSpeed << 16) + iAccel;
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_SET_SLEW_SPEED_ACCEL, lParam);
	}
	else
		return FALSE;
}

// use this to insure thaT THE MAIN  THREAD IS C ALLING IT, NOT THE USER THREASD
// the above set all motors to the same speed
// this enables setting individual motors
BOOL CDialogGirthWeld::SetSlewSpeed(const double fSpeed[4])
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_SET_SLEW_SPEED4, (LPARAM)fSpeed);
	}
	else
		return FALSE;
}

// this will define the current posityion of all motors as (pos)
// call with zero to set a HOME position
BOOL CDialogGirthWeld::DefinePositions(double pos)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_DEFINE_POSITION, (LPARAM)(1000.0 * pos + 0.5) );
	else
		return FALSE;
}

// at the start of a scan, may want to reset the encoder count
// again only use the main thread to talk to the controllers
BOOL CDialogGirthWeld::ResetEncoderCount()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_RESET_ENCODER);
	else
		return FALSE;
}

// this was used in an attempt to correct the varying distance driven L/R during a manoeuvre
// this is not used at this time (911)
// thus the L/R motors are told to go to different positions
BOOL CDialogGirthWeld::GoToPosition2(double left, double right)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		double position[] = { left, right };
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_GOTO_POSITION2, (LPARAM)position);
	}
	else
		return FALSE;
}

// this tells all motors to go to a the same position
// 
BOOL CDialogGirthWeld::GoToPosition(double pos)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_GOTO_POSITION, (LPARAM)(pos * 1000 + 0.5));
	else
		return FALSE;
}

// set the motors driving at a given velocity with no end posityion
// this was used in an early attempt at navigation to avoid the motors stopping at different time
// not used at this time, as tell motors in navigation to go too far, then tell all to stop at the same time
BOOL CDialogGirthWeld::SetMotorJogging(int dir)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_MOTOR_JOGGING, (LPARAM)dir);
	else
		return FALSE;
}

// what is the current motor speed of the given axis
// the spped is a double, so will be passed as *1000
double  CDialogGirthWeld::GetSlewSpeed(const char* axis)
{
	clock_t t1 = clock();
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		double ret = SendMessage(WM_MOTION_CONTROL, MC_GET_SLEW_SPEED, axis[0] - 'A') / 1000.0;
#ifdef _DEBUG_TIMING_
		g_GetSlewSpeedTime += clock() - t1;
		g_GetSlewSpeedCount++;
#endif
		return ret;
	}
	else
		return FLT_MAX;
}

// stop all motors now
// if they are not all travelling at the same speed when this is called, they may travel different amounts during deceleration (911)
void CDialogGirthWeld::StopMotors(BOOL bWait)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		SendMessage(WM_MOTION_CONTROL, MC_STOP_MOTORS, (LPARAM)bWait);
}

// requstr the encoder distance from the MAG controller
double CDialogGirthWeld::GetEncoderDistance()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (double)(SendMessage(WM_MOTION_CONTROL, MC_GET_ENC_DIST) / 1000.0);
	else
		return FLT_MAX;
}

// get the averae motor positioon of all 4 motors
// assume that as navigate, the motors will travel an average distance, though each wheels distance will vary
double CDialogGirthWeld::GetAvgMotorPosition()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (double)(SendMessage(WM_MOTION_CONTROL, MC_GET_AVG_POS) / 1000.0);
	else
		return FLT_MAX;
}

// get the RGB sum from the MAG controller
// this takes about 200 ms, so must not call too often
// note, called by the main thread
// commented out as not used at this time
/*double  CDialogGirthWeld::GetRGBSum()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return SendMessage(WM_MOTION_CONTROL, MC_GET_RGB_SUM) / 1000.0;
	else
		return FLT_MAX;
}
*/

// due to deceleration, the motors take time to stop
// AreTheMotorsRunning() looks at a flag only and does not talk to the motors
// no need for edsxact timeing in this, so OK to pool the motor state in the main thread
// may sit in this while loop for the entire drive
BOOL CDialogGirthWeld::WaitForMotorsToStop()
{
	// add this time, to insure that the motorts are running
	// it seems to fail if don't wait long enough
	// even though have always passed wait for motors to start prior to this
	// this is likely due to looking at the polled flag indicating if running or not
	// the motors will never run for less than 500 ms regardless
	Sleep(1000);

	// check 5 times to see if stopped
	// seem to get erroneous return of motors stopped wqhen not
	for (int i = 0; i < 5; ++i)
	{
		while (AreMotorsRunning())
			Sleep(10);
	}

	return TRUE;
}

// only wait for up to 1 sec for the mnotors to start
BOOL CDialogGirthWeld::WaitForMotorsToStart()
{
	Sleep(100);
	for (int i = 0; i < 500 && !AreMotorsRunning(); ++i)
		Sleep(10);

	// return if the motors acutally started, or if timed out
	if (m_motionControl.AreTheMotorsRunning())
		return TRUE;

	SendErrorMessage("Motors did not Start");
	return FALSE;
}
// this uses a noted parameter in the motion controller
// it is set in an OnTimer()
// at this time, this is not used
// for now only the saved motor state is used
LRESULT CDialogGirthWeld::OnUserAreMotorsRunning(WPARAM, LPARAM)
{
	return (LRESULT)m_motionControl.AreMotorsRunning();
}

// not used, but a main thread to check the motor controler to see if any one motor is running
BOOL CDialogGirthWeld::AreMotorsRunning()
{
//	return m_motionControl.AreTheMotorsRunning();
	return (BOOL)SendMessage(WM_ARE_MOTORS_RUNNING);
}

// while seeking the start line
// the user can double click on the static to stop the seek, as the line may already be noted
// in thisd case, just stop the motors
// check if if RGB scanning mode first
LRESULT CDialogGirthWeld::OnUserMagStopSeek(WPARAM wParam, LPARAM lParam)
{
	if (m_nCalibratingRGB == CALIBRATE_RGB_SCANNING)
		m_motionControl.StopMotors(TRUE/*wait*/);

	return 0L;
}

// this will always be the start up thread
// this handles vaious worker thread to main thread requests
LRESULT CDialogGirthWeld::OnUserMotionControl(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	// set the acceleration and deceleration rates
	// they were passed as 16 of 32 bytes each
	case MC_SET_SLEW_SPEED_ACCEL:
	{
		int iAccel = lParam & 0xFFFF;
		int iSpeed = (lParam >> 16) & 0xFFFF;
		double fAccel = iAccel / 10.0;
		double fSpeed = iSpeed / 10.0;
		return (LRESULT)m_motionControl.SetSlewSpeed(fSpeed, fAccel);
	}
	// set the slew speed independantly for each motor
	// this will be used by navigaztion to alter L/R motor speeds
	case MC_SET_SLEW_SPEED4:
	{
		const double* fSpeed = (double*)lParam;
		return (LRESULT)m_motionControl.SetSlewSpeed(fSpeed[0], fSpeed[1], fSpeed[2], fSpeed[3]);
	}

	// set the motor joggind at the default speed andf acceleration
	// thuis as default values, they are not passed here
	case MC_MOTOR_JOGGING:
	{
		int dir = (int)lParam;
		return (LRESULT)m_motionControl.SetMotorJogging(dir*m_fMotorScanSpeed, m_fMotorScanAccel);
	}

	// resete the encoder count to zero
	case MC_RESET_ENCODER:
			return (LRESULT)m_magControl.ResetEncoderCount();

	// request all motors to go to this position
	case MC_GOTO_POSITION:
		return (LRESULT)m_motionControl.GoToPosition(lParam / 1000.0, FALSE/*dont wait*/);

	// this requests the L/R motors to go to different positions
	// 911, not used at this time
	case MC_GOTO_POSITION2:
	{
		const double* position = (double*)lParam;
		return (LRESULT)m_motionControl.GoToPosition2(position[0], position[1]);
	}

	// get the encoder distance fom the MAG board
	// this is slow (200 ms), so only get at the end of a scan, not during it
	case MC_GET_ENC_DIST:
	{
		m_magControl.GetMagStatus(); // may not be looking at the status, so get it now
		return (LRESULT)(1000 * m_magControl.GetEncoderDistance() + 0.5);
	}

	// get the average positioon of all four motors
	// assume that as navigating while each motor may travel different distances, the average is a measure of the crawler location
	case MC_GET_AVG_POS:
		return (LRESULT)(1000 * m_motionControl.GetAvgMotorPosition() + 0.5);

	// define the current position to be as given
	case MC_DEFINE_POSITION:
		m_motionControl.DefinePositions( lParam / 1000.0 );
		return 1L;

	// get the sumn of the RGB values
	// thhis is slow (200 ms) so \will not requestr during navigation
	case MC_GET_RGB_SUM:
		return (LRESULT)(1000 * m_magControl.GetRGBSum() + 0.5);

	// request all four motors to stop
	// not that if a different sppe3eds, they may not all stop at the same time (911)
	case MC_STOP_MOTORS:
		m_motionControl.StopMotors( (BOOL)lParam);
		return 0L;

	// get the current speed of the given axis
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

// this is used when the 'calibrate' check box is selected
// 1. seek the start line either forearsd or in reverse
// 2. back up to the the noted start line
// 3. define the motor positioon to be zer as well as reset the encoder distance
// 4. drive the circumference plus the seek distance above
// 5. when nearing 360 deg. slowm down if RGB used, else full speed, dispolay the RGB or cap height graph
// 6. note the centre of the start line
// 7. return to the start line in reverse
// 8 note the encoder distance as compared to the useer supplied circumference
BOOL CDialogGirthWeld::CalibrateCircumference()
{
	CString str;

	// note where is now, so that can return to thge start at the end of the scxan
	// (911) this option is not enabled at this time
	// after calibrate the scanner would be at the desired start location
	double pos = GetAvgMotorPosition();

	// go to the start line
	// this will resulot in  the motor position as zero
	if (!SeekStartLine())
		return FALSE;

	// this will cause the calibration graph to be hid, and the laser profile to be shown
	m_nCalibratingRGB = CALIBRATE_RGB_NOT;
	SendMessage(WM_SIZE);

	// at this point the scanner will be on the start line
	// turn on navigation at full speed and seek the start line again
	m_bScanning = TRUE;
	SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanAccel); // full speed
	Sleep(100);

	// scan the circumference plus the predefined seek distance for the start line
	// also scan the deceleration distance
	GoToPosition(m_fScanCirc + m_fSeekAndStartAtLine/2 + GetDistanceToBuffer());

	// will navigate to the circumference plus the seek start line distance
	// will need to pass the start line, to be able to detect it
	int from_mm = 0;
	int to_mm = (int)(m_fScanCirc + m_fSeekAndStartAtLine + 0.5);
	StartNavigation(0x3, from_mm, to_mm, m_fMotorScanSpeed);

	// while waiting for the maotors to stop
	// check if near the circumference, and then  turn the RGB back on, so that can spot the start lien again
	if (!WaitForMotorsToStart())
		return FALSE;

	// while the motors are running, check if nearing the staret line again
	for (int i = 0; m_motionControl.AreTheMotorsRunning() && !m_bAborted; ++i)
	{
		Sleep(1); // avoid tight loop
		// close to the stop line
		// no need to stop navigation if using laser
		// only the RGB slows the system down
		double pos = GetAvgMotorPosition();
		if ( pos > m_fScanCirc - m_fSeekAndStartAtLine && m_nCalibratingRGB != CALIBRATE_RGB_SCANNING )
		{
			// start noting the RGB data VS position so that can find
			// reset any existinbg values first
			if (!m_bSeekWithLaser)
			{
				StartNavigation(0x0, 0, 0, 0); // RGB register read conflicts with navigation (too slow)
				SetSlewSpeed(m_fMotorScanSpeed / 4, m_fMotorScanAccel); // 1/4 speed so don;'t miss the line
			}

			// reset the calibtation, as starting a new seek for start line
			m_magControl.ResetRGBCalibration();
			m_nCalibratingRGB = CALIBRATE_RGB_SCANNING;

			// checfk every 10 ms for the start line
			SetTimer(TIMER_NOTE_CALIBRATION, 10, NULL);
			SendMessage(WM_SIZE); // force to show the calibrate window VS laser profile
		}

		// are now 1/2 the seek distance past 360 deg, so assume have completed a full scan and stop the motors
		else if (pos > m_fScanCirc + m_fSeekAndStartAtLine/2 && m_nCalibratingRGB == CALIBRATE_RGB_SCANNING)
		{
			StopMotors(FALSE);
			WaitForMotorsToStop();
		}
	}

	// if aboted, then return now
	double pos2 = (!m_bAborted && m_magControl.CalculateRGBCalibration(m_bSeekWithLaser, m_bSeekBlackLine)) ? m_magControl.GetRGBCalibration().pos : FLT_MAX;
	str.Format("Start Line Found at (%.1f mm)", pos2);

	// tell the usert qwhere the start line was found
	// option to abort and leaqve the craewler where is
	if( pos2 == FLT_MAX || AfxMessageBox(str, MB_OKCANCEL) != IDOK)
		return FALSE;

	// note wherte the line was found
	// and return to this position
	// turn off navigation, this will be backwards and not very far
	m_nCalibratingRGB = CALIBRATE_RGB_RETURN;
	StartNavigation(0x0, 0, 0, 0);

	GoToPosition(pos2);
	if (!WaitForMotorsToStart())
		return FALSE;

	// check if cliocked abort while driving
	WaitForMotorsToStop();
	if (m_bAborted)
		return FALSE;

	// note the encoder distance
	double encoder = GetEncoderDistance();

	// tell the user the distance as compared to the circumference
	str.Format("Circumference = %.1f\nMotor Position = %.1f\nEncoder Distance = %.1f", 
		m_fScanCirc, GetAvgMotorPosition(), encoder);
	AfxMessageBox(str);

	// optionally drive back to the original start location
	// 911 niot enabled at this time
	if (m_bReturnToStart)
	{

	}

	return TRUE;
}

// seek the start line, so can scan from that point (or a given pre-drive diastance prior to it)
BOOL CDialogGirthWeld::SeekStartLine()
{
	CString str;

	// redefine where am now to zero
	DefinePositions(0);
	double pos0 = 0; //  GetAvgMotorPosition();

	// reset any previously set calibration
	// this is a list of position VS value
	m_magControl.ResetRGBCalibration();

	// start noting the RGB data VS position every 10 ms
	m_nCalibratingRGB = CALIBRATE_RGB_SCANNING;
	SetTimer(TIMER_NOTE_CALIBRATION, 10, NULL);
	SendMessage(WM_SIZE); // replace the laser window with the calibration window with m_nCalibratingRGB set

	// noite the rtequested motor speed, and use 1/4 of that for navigation
	int start_pos = 0;
	int end_pos = (int)(m_bSeekStartLineInReverse ? -m_fSeekAndStartAtLine : m_fSeekAndStartAtLine);

	// if using RGB slow to 1/4 speed so dont miss the line
	double speed = m_bSeekWithLaser ? m_fMotorScanSpeed : m_fMotorScanSpeed / 4;
	SetSlewSpeed(speed, m_fMotorScanAccel);
	StartNavigation(0x3, start_pos, end_pos, speed);

	// now drive for the desired length, and wait for the motors to stop
	// as navigating to the weld, give a buffer to the end position
	Sleep(100);
	GoToPosition(end_pos + GetAccelDistance());
	if (WaitForMotorsToStart())
		WaitForMotorsToStop();

	// check if aborted the run, or unable to find a start line
	if (m_bAborted || !m_magControl.CalculateRGBCalibration(m_bSeekWithLaser, m_bSeekBlackLine))
		return FALSE;

	// note where the start line is
	// check with operator if this is indeed the start line
	// is not, then leave crawler where is
	m_wndMag.InvalidateRgn(NULL);
	double pos2 = m_magControl.GetRGBCalibration().pos; // line found here
	double pos3 = GetAvgMotorPosition(); // where at now
	str.Format("Start Line Found at (%.1f mm)", pos2);
	if (AfxMessageBox(str, MB_OKCANCEL) != IDOK)
		return FALSE;

	// now that have the start line, can run at full speed with RGB
	// don't use navigation while driving back to the line that just passed
	SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanAccel);

	// backing up to the start line is a shiort drive, so navigation not required
	StartNavigation(0x0, 0, 0, 0);

	// always drive to the start point
	// this allows the encoder wheel to be reset and the motor position to be set to zero
	// m_nCalibratingRGB stillkeeps the calibration display
	m_nCalibratingRGB = CALIBRATE_RGB_RETURN;

	// now go to this position
	// show the position on the graph
	// don't navigate during this move as nbackwards navigation is not yet working 911
	GoToPosition(pos2);
	if (WaitForMotorsToStart())
		WaitForMotorsToStop();
	if (m_bAborted)
		return FALSE;

	// ask the user if to run the scan now that on the start line
	if (AfxMessageBox("Start Scan", MB_OKCANCEL) != IDOK)
		return FALSE;

	// now set this as the new zero poisition
	DefinePositions(0);
	ResetEncoderCount();

	// now back up by the accewleration distance
	// if calibrating,m then just start from here always
	// again a short distane, so don't use navigation
	if (!m_bCalibrate && m_bPredrive && m_fPredriveDistance > 0)
	{
		GoToPosition(-m_fPredriveDistance);
		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
	}

	// the display is shown or hid in OnSize()
	m_fScanStartPos = 0; // will be starting at zero
	return !m_bAborted;
}

// want to set the GoToPosition() distance further than that actually desired
// this way can issue a 'STOP' before deceleration
// the motors will otherwise travel the exact same distance, even if don't want to
// at the end, the L/R distance dravelled can be a number of mm different
// asking all motors to travel the same distance will cause significant turn at the end
double CDialogGirthWeld::GetDistanceToBuffer()const
{
	double accel_dist = GetAccelDistance();
	return accel_dist + 50;
}


// with eh exception of seek start line, this is all the functions for a scan
UINT CDialogGirthWeld::ThreadAbortScan()
{
	CString str;
	// have requested to stop

	int from_mm = 0;
	int to_mm = (int)(m_fDestinationPosition + 0.5);

	// 1. even called to abort a scan
	// this takes timne, so best done in a thread
	// if want to abort later in this function, just set m_bAbort and recurse
	m_bScanning = FALSE;
	m_bResumeScan = FALSE;
	m_nCalibratingRGB = CALIBRATE_RGB_NOT;
	StopMotors(TRUE);
	KillTimer(TIMER_NOTE_CALIBRATION);
	WaitForMotorsToStop();
	StartNavigation(0x0, 0, 0, 0);
	InformRecordingSW(-1); // indicate that aborted
	SendMessage(WM_SIZE); // replace the laser window with the calibration window
	PostMessage(WM_USER_ABORT_FINISHED);
	return 0L;
}


// with eh exception of seek start line, this is all the functions for a scan
UINT CDialogGirthWeld::ThreadRunScan()
{
	CString str;
	// have requested to stop

	int from_mm = 0;
	int to_mm = (int)(m_fDestinationPosition + 0.5);

	// 1. even called to abort a scan
	// this takes timne, so best done in a thread
	// if want to abort later in this function, just set m_bAbort and recurse
	if (m_bAborted)
	{
		m_bScanning = FALSE;
		m_bResumeScan = FALSE;
		m_nCalibratingRGB = CALIBRATE_RGB_NOT;
		StopMotors(TRUE);
		KillTimer(TIMER_NOTE_CALIBRATION);
		WaitForMotorsToStop();
		StartNavigation(0x0, 0,0, 0);
		InformRecordingSW(-1); // indicate that aborted
		SendMessage(WM_SIZE); // replace the laser window with the calibration window
		PostMessage(WM_USER_SCAN_FINISHED);
		return 0L;
	}

	// calibrate is not an actual scan
	else if (m_bCalibrate)
	{
		BOOL bCalibrate = CalibrateCircumference();
		m_nCalibratingRGB = CALIBRATE_RGB_NOT;
		SendMessage(WM_SIZE); // replace the laser window with the calibration window
		PostMessage(WM_USER_SCAN_FINISHED);
		return 0L;
	}

	// have requested to resume the scan from current location
	// the destination location has not changed
	// the navigation was naot stopped during pause
	// so no need to resume
	else if (m_bResumeScan)
	{
		GoToPosition(m_fDestinationPosition + GetDistanceToBuffer());
		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
		if (!m_bPaused)
			PostMessage(WM_USER_SCAN_FINISHED);
		return 0L;
	}

	// starting a new scan
	else
	{
		// optionally go to the home positionb prior to thew scxan
		if (m_bStartScanAtHomePos)
		{
			m_fScanStartPos = 0; // will be starting at zero
			m_bScanning = FALSE; // not scanning yhet

			// go to the home position and resewt the encoder
			GoToPosition(0);
			if (WaitForMotorsToStart())
				WaitForMotorsToStop();
			if (m_bAborted)
				return ThreadRunScan();

			// the motor position is zero by default, insuire the encoder is also zero
			ResetEncoderCount();

			// go to the acceleration distaznce behind home
			// tell the recording S/W to start a zero
			from_mm = 0;
			to_mm = (int)(m_fDestinationPosition + 0.5);

			// if backing up prior to the start position so can navigate to the start line prior to the scan
			if (m_bPredrive && m_fPredriveDistance)
			{
				GoToPosition(-m_fPredriveDistance);
				if (WaitForMotorsToStart())
					WaitForMotorsToStop();
				if (m_bAborted)
					return ThreadRunScan();
			}
		}
		// seek a start line, and set HOME to be at that line
		// thus start recording at zero
		else if (m_bSeekAndStartAtLine)
		{
			// this will leave the craewler on the start line
			// ooptionally it will be pre-drive prior to the start line
			// bSeek = FALSE if aborted
			BOOL bSeek = SeekStartLine();

			// =replace the calibration graph with the laser profile
			m_nCalibratingRGB = CALIBRATE_RGB_NOT;
			SendMessage(WM_SIZE);

			KillTimer(TIMER_NOTE_CALIBRATION);
			
			if( !bSeek )
			{
				m_bAborted = TRUE;
				return ThreadRunScan();
			}
			from_mm = 0;
			to_mm = (int)(m_fDestinationPosition + 0.5);
		}

		// just scan from where are
		else
		{
			// get the starting position, so can return to after the scan
			// this does not include the acceleration distance back up
			// do this before backing up the acceleration distance
			// ignore this step if already behind this location
			m_fScanStartPos = atof(m_szHomeDist);

			double pos = GetAvgMotorPosition();
			if (m_bPredrive && m_fPredriveDistance > 0 && GetAvgMotorPosition() > -m_fPredriveDistance )
			{
				GoToPosition(pos - m_fPredriveDistance);
				if (WaitForMotorsToStart())
					WaitForMotorsToStop();
				if (m_bAborted)
					return ThreadRunScan();
			}

			from_mm = (int)(pos + 0.5);
			to_mm = (int)(m_fDestinationPosition + 0.5);
		}

		// if m_bAbort = TRE, just recurse to call abort options
		if (m_bAborted)
			return ThreadRunScan();

		// signal the recording S/W to start recording
		// may have already been navigating at a different speed, so restart it
		m_bScanning = TRUE;
		SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanAccel);
		InformRecordingSW(1, from_mm, to_mm);
		Sleep(100);

		// add a deceleration distance to this
		// the navigation will stop the motors when readch to_mm regardless
		GoToPosition(m_fDestinationPosition + GetDistanceToBuffer());
		StartNavigation(0x3, from_mm, to_mm, m_fMotorScanSpeed);

		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
		if (m_bAborted)
			return ThreadRunScan();

		// if paused have not finished recording
		// if aborted, then will finish above
		// these are set by the main thread, so can change during the WaitForMotorsToStop() time
		// if apused, the Fwd/Back buttons wuill be enabled, and will re-entre this function when click resume
		// the following sasdsumes that ended normally
		if (!m_bPaused && !m_bAborted )
		{
			// update thje files list
			StartNavigation(0x0, 0, 0, 0);
			UpdateScanFileList();
			m_bScanning = FALSE;
			InformRecordingSW(0); // normal termination

			// check to see if to return to start position now (start not from home)
			// do this with navigation
			// it will be in reverse so slow the motors to 1/2 speed (911)
			if (m_bReturnToStart)
			{
				// return to the start locatioon (less the accdeeration backup)
				int start_pos = (int)(GetAvgMotorPosition() + 0.5);
				int end_pos = (int)(m_fScanStartPos + 0.5);
				SetSlewSpeed(m_fMotorScanSpeed / 2, m_fMotorScanAccel);
				StartNavigation(0x3, start_pos, end_pos, m_fMotorScanSpeed / 2);
				GoToPosition(m_fScanStartPos);
				if (WaitForMotorsToStart())
					WaitForMotorsToStop();
				if (m_bAborted)
					return ThreadRunScan();
			}
			PostMessage(WM_USER_SCAN_FINISHED);
		}
	}

	return 0L;
}

// nStart
// 1: starting
// 0: ended
// -1: aborted
// (911) this does nothing at this time, but may be reuired when integrate with recording softewarte
void CDialogGirthWeld::InformRecordingSW(BOOL record, int from/*= 0*/, int to/* = 0*/)
{
	int xx = 1; // 911
}

// clicked the manual back button
// RunMotors() will handle forward, back and stop
void CDialogGirthWeld::OnClickedButtonBack()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters())
	{
		if (!m_bPaused)
			m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_BACK : GALIL_IDLE;
		else if (m_nGalilState == GALIL_BACK)
			m_nGalilState = GALIL_IDLE;
		else
			m_nGalilState = GALIL_BACK;

		RunMotors();
	}
}

// as per mback
void CDialogGirthWeld::OnClickedButtonFwd()
{
	// TODO: Add your control notification handler code here
	if (CheckParameters())
	{
		if (!m_bPaused)
			m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_FWD : GALIL_IDLE;
		else if(m_nGalilState == GALIL_FWD )
			m_nGalilState = GALIL_IDLE;
		else
			m_nGalilState = GALIL_FWD;

		RunMotors();
	}
}

// common function for forward,back and stop
void CDialogGirthWeld::RunMotors()
{
//	int delay2 = (int)(1000.0 / m_fMotorScanSpeed + 0.5); // every mm

	// have request the motors to stop
	// the thread will stop the motors, and wait for them to decelerate
	if (m_nGalilState == GALIL_IDLE)
	{
		// because of deceleration it takes a while for the motors to stop
		// will not return from this fuinction until they have stopped
		// thus, call this from a thread, and block starts until the motors are stopped
		m_pThreadAbort = AfxMyBeginThread(::ThreadStopMotors, (LPVOID)this);
	}

	// request to drive forward
	// if paused, then have noted a backup state
	else if (m_nGalilState == GALIL_FWD || (m_bPaused && m_nGaililStateBackup == GALIL_FWD) )
	{
		if (m_fMotorScanSpeed != FLT_MAX && m_fMotorScanAccel != FLT_MAX)
		{
			m_wndLaser.ResetRGBData();
			SetNoteRunTime(TRUE);

			// this is the main thread, so can talk to motor controllewr directly
			if (m_motionControl.SetMotorJogging(m_fMotorScanSpeed, m_fMotorScanAccel))
			{
				m_motionControl.ResetLastManoeuvrePosition();
				EnableMagSwitchControl(FALSE);

				// if pausewd, then leave the navigation alone
				// else just the tracking data
				if( !m_bPaused )
					StartNavigation(0x1, 0,0, 0); // track the crawler, but don't navigate

				// no need for this during a manual drive regardfless
	//			StartNotingRGBData(TRUE); // cannot use RGB during a scan, it takes too long (911)

				// staret calculating the various laser measures to be used by navigation
				StartMeasuringLaser(TRUE);
				StartNotingMotorSpeed(TRUE);
			}
			// if fail to set motors jogging, then just not that not
			else
			{
				m_nGalilState = GALIL_IDLE;
			}
			SetButtonBitmaps();
		}
	}

	// as per forward, but give the speed a -Ve
	else if (m_nGalilState == GALIL_BACK || (m_bPaused && m_nGaililStateBackup == GALIL_FWD))
	{
		if (m_fMotorScanSpeed != FLT_MAX && m_fMotorScanAccel != FLT_MAX)
		{
			// tezmperary code to test steering
			m_wndLaser.ResetRGBData();
			SetNoteRunTime(TRUE);
			if (m_motionControl.SetMotorJogging(-m_fMotorScanSpeed, m_fMotorScanAccel))
			{
				m_motionControl.ResetLastManoeuvrePosition();
				EnableMagSwitchControl(FALSE);

				// if pausewd, then leave the navigation alone
				// else just the tracking only
				if (!m_bPaused)
					StartNavigation(0x1, 0,0, 0); // track only

//				StartNotingRGBData(TRUE); // no navigation, so OK
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

// called when request a manmual drive to end
// the motors take time to decelerate, so best handled in a threasd
UINT CDialogGirthWeld::ThreadStopMotors()
{
	StopMotors(TRUE); // TRUE, wait for the motors to stop
//	m_laserControl.TurnLaserOn(FALSE);				// do not call thesae from a thread
//	m_magControl.EnableMagSwitchControl(TRUE);		// call, them in the finish up call-back
	PostMessage(WM_USER_ABORT_FINISHED);
	return 0;
}

// talking to the motor controller must be done by the main thread
// so required here
LRESULT CDialogGirthWeld::OnUserWeldNavigation(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	// get the difference betrween the L/R motor positions
	// not used at this time
	case NAVIGATE_LR_DIFFEENCE:
	{
		double A = m_motionControl.GetMotorPosition("A");
		double B = m_motionControl.GetMotorPosition("B");
		double C = m_motionControl.GetMotorPosition("D");
		double D = m_motionControl.GetMotorPosition("D");

		return (LRESULT)(100.0 * ((A + D) / 2 - (B + C) / 2) + 0.5);
	}
	// request the motors to stop
	// lParam = 1: wait for then to stop, lParam=0, return immediately
	case NAVIGATE_STOP_MOTORS:
		m_motionControl.StopMotors((BOOL)lParam);
		return 1L;

	// this is used by navigation to set the motor speeds
	// thus, note this as a manoeuvre
	case NAVIGATE_SET_MOTOR_SPEED:
	{
		const double* speed = (double*)lParam;
		m_motionControl.SetLastManoeuvrePosition();
		m_motionControl.SetSlewSpeed(speed[0],speed[1],speed[2],speed[3]);
		return 1L;
	}

	case NAVIGATE_GET_AVG_SPEED:
	{
		double speed = m_motionControl.GetAvgMotorSpeed();
		return (speed == FLT_MAX) ? INT_MAX : (LRESULT)(speed*1000.0 + 0.5);
	}
	// send a message to the status view used in _DEBUG
	case NAVIGATE_SEND_DEBUG_MSG:
	{
		const CString* pMsg = (CString*)lParam;
		SendDebugMessage(*pMsg);
		return 0L;
	}

	// at the end of navigation the deceleration is set to double the requested
	// as each motor may have a slightly different speed
	// want deceleration to be as fast as possible, so decelerat5ion distane varies as little as possibler
	case NAVIGATE_SET_MOTOR_DECEL:
	{
		double decel = lParam / 100.0;
		m_motionControl.SetSlewDeceleration(decel);
		return 1L;
	}
	}
	return 0L;
}

// 911 not used at this time
LRESULT CDialogGirthWeld::OnUserStaticParameter(WPARAM wParam, LPARAM lParam)
{
	double* param = (double*)lParam;
	switch (wParam)
	{
	case STATUS_GETLOCATION:
		*param = m_motionControl.GetMotorPosition("A");
		break;
	case STATUS_SHOWLASERSTATUS:
	//	ShowLaserStatus();
		break;
	default:
		*param = FLT_MAX;
	}

	return 0L;
}

// this is called at the end of the tread which was qwaiting for the 
// manual run of the motors to stop
LRESULT CDialogGirthWeld::OnUserScanFinished(WPARAM, LPARAM)
{
	return OnUserFinished(&m_pThreadScan);
}

LRESULT CDialogGirthWeld::OnUserAbortFinished(WPARAM, LPARAM)
{
	return OnUserFinished(&m_pThreadAbort);
}

LRESULT CDialogGirthWeld::OnUserFinished(CWinThread** ppThread)
{
	// this should not happen
	// not sure if TerminteThread() will actually do so
	// this ability must be included when the thread was created
	// regardless, a dangerous move 
	int ret = ::WaitForSingleObject((*ppThread)->m_hThread, 1000);
	if (ret != WAIT_OBJECT_0 )
	{
		SendErrorMessage("Thread Timed Out");
	}

	// if paused and going to resume a scan
	// then do not want to cal the following
	delete *ppThread;
	*ppThread = NULL;

	if (!m_bPaused)
	{
		// if paused, then will still want the laser on and the MAG switch disabled
		m_laserControl.TurnLaserOn(FALSE);
		EnableMagSwitchControl(TRUE); // the magnet servo is always disdabled during a run
		
		m_nGalilState = GALIL_IDLE;
		StartMeasuringLaser(FALSE);
		StartNotingMotorSpeed(FALSE);
		m_fScanStartPos = FLT_MAX;
		StartNavigation(0x0, 0,0, 0);
		SetNoteRunTime(FALSE); // an invalid value ends it
		ShowMotorPosition();
	}

	// if was in paused state of a scan, then just return the state to scan
	else
		m_nGalilState = m_nGaililStateBackup;

	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->PostMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SETBITMAPS);

	SetButtonBitmaps();
	return 0L;
}

// set a timer to note the run time
// get the current clock() time when enabled the timer
void CDialogGirthWeld::SetNoteRunTime(BOOL bSet)
{
	if (bSet)
	{
		m_nRunStart = clock();
		SetTimer(TIMER_RUN_TIME, 100, NULL);
	}
	else
		KillTimer(TIMER_RUN_TIME);
}

// reset the motor positioons to zero
void CDialogGirthWeld::OnClickedButtonZeroHome()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	SendDebugMessage("OnClickedButtonZeroHome");

	m_motionControl.DefinePositions(0);
	m_magControl.ResetEncoderCount();
	ShowMotorPosition();
	SetButtonBitmaps();

	UpdateData(FALSE);
}


BOOL CDialogGirthWeld::CheckParameters()
{
	// if currently idle, then check if have valid parameters before proceeding
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

// when scanning or aborting a scan, double check
// likewise on the return to home
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
		case GALIL_SCAN:
			ret = AfxMessageBox(_T("Begin Scan"), MB_OKCANCEL);
			break;
		case GALIL_GOHOME:
			ret = AfxMessageBox(_T("Return to Home"), MB_OKCANCEL);
			break;
		}
		break;
	}
	case GALIL_SCAN:
		ret = AfxMessageBox(_T("Abort the Scan"), MB_OKCANCEL);
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

	const UINT nIDC1[] = { IDC_STATIC_LASER, 
		IDC_SLIDER_STEER, IDC_STATIC_LEFT, IDC_STATIC_RIGHT,IDC_STATIC_FWD, IDC_BUTTON_FWD, IDC_STATIC_BACK, IDC_BUTTON_BACK, 
		IDC_STATIC_MAG_STATUS, IDC_CHECK_MAG_ENABLE, IDC_BUTTON_MAG_ON, 0 };
	const UINT nIDC2[] = { IDC_STATIC_MAG, 0 };

	for (int i = 0; nIDC1[i] != 0; ++i)
		GetDlgItem(nIDC1[i])->ShowWindow(m_nCalibratingRGB != CALIBRATE_RGB_NOT ? SW_HIDE : SW_SHOW);

	for (int i = 0; nIDC2[i] != 0; ++i)
		GetDlgItem(nIDC2[i])->ShowWindow(m_nCalibratingRGB == CALIBRATE_RGB_NOT ? SW_HIDE : SW_SHOW);

	m_wndLaser.ShowWindow(m_nCalibratingRGB != CALIBRATE_RGB_NOT ? SW_HIDE : SW_SHOW);
	m_wndMag.ShowWindow(m_nCalibratingRGB == CALIBRATE_RGB_NOT ? SW_HIDE : SW_SHOW);

	m_staticLaser.GetClientRect(&rect);
	m_wndLaser.MoveWindow(&rect);

	m_staticMag.GetClientRect(&rect);
	m_wndMag.MoveWindow(&rect);

	m_wndLaser.PostMessageA(WM_SIZE);
	m_wndMag.PostMessageA(WM_SIZE);
}


void CDialogGirthWeld::OnRadioScanType()
{
	// TODO: Add your command handler code here
	UpdateData(TRUE);
	SetButtonBitmaps();
}


void CDialogGirthWeld::EnableControls()
{
	SetButtonBitmaps();
}


// drive to the home (0) position before scanning
// this insures that m_bStartScanAtHomePos and m_bSeekAndStartAtLine cannot bve both set at the sanme time
void CDialogGirthWeld::OnClickedCheckGoToHome()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_bStartScanAtHomePos)
	{
		m_bSeekAndStartAtLine = FALSE;
		UpdateData(FALSE);
	}
	SetButtonBitmaps();
}

// seek the start line, then scan
// this insures that m_bSeekAndStartAtLine and m_bStartScanAtHomePos cannot both be set
void CDialogGirthWeld::OnClickedCheckSeekStartLine()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_bSeekAndStartAtLine)
	{
		m_bStartScanAtHomePos = FALSE;
		UpdateData(FALSE);
	}
	SetButtonBitmaps();
}

// if starting at home or on a start line option to return to start position after the scan
void CDialogGirthWeld::OnClickedCheckReturnToStart()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	SetButtonBitmaps();
}

// this will validate that the value in the control is valid
// else it will return FLT_MAX
double CDialogGirthWeld::GetMotorSpeed()
{
	m_bCheck = TRUE;
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret ? m_fMotorScanSpeed : FLT_MAX;
}

double CDialogGirthWeld::GetMotorAccel()
{
	m_bCheck = TRUE;
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret ? m_fMotorScanAccel : FLT_MAX;
}

// calculate the likely deceleration distance
// this is used to estimate how much furthjer to drive motorts to, than required
// ideally at the end of the navbigation distance, the motors will not be deceleration
double  CDialogGirthWeld::GetAccelDistance()const
{
	double accel_time = m_fMotorScanSpeed / m_fMotorScanAccel;
	double accel_dist = accel_time * m_fMotorScanSpeed / 2.0;
	return accel_dist;
}

void CDialogGirthWeld::OnDeltaposSpinScanSpeed(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fMotorScanSpeed += (inc > 0) ? 0.1 : -0.1;
	m_fMotorScanSpeed = min(max(m_fMotorScanSpeed, MIN_MOTOR_SPEED), MAX_MOTOR_SPEED);
	UpdateData(FALSE);

	// call direct, this can not be a thread
	// this means that can speed up or slow down while driving
	// however, the control is not enavbled while driving
	// so will not haoppen (911)
	if (m_motionControl.AreMotorsRunning())
		m_motionControl.SetMotorJogging(m_fMotorScanSpeed, m_fMotorScanAccel);

	*pResult = 0;
}


void CDialogGirthWeld::OnDeltaposSpinAccel2(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fMotorScanAccel += (inc > 0) ? 0.1 : -0.1;
	m_fMotorScanAccel = min(max(m_fMotorScanAccel, MIN_MOTOR_ACCEL), MAX_MOTOR_ACCEL);
	UpdateData(FALSE);

	*pResult = 0;
}

// when release the slider used for steering, return it to zero
void CDialogGirthWeld::OnReleasedcaptureSliderSteer(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	// a rate of zero will set all motors to the sasme speed
	m_sliderSteer.SetPos(0);
	m_motionControl.SteerMotors(m_fMotorScanSpeed, 0.0/*rate*/);
	*pResult = 0;
}


// this is called when the salider is slid, clicked etc.
void CDialogGirthWeld::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// Slider Controls have been adjusted
	CSliderCtrl* slider = (CSliderCtrl*)pScrollBar;

	if (slider != &m_sliderSteer )
		return;
	
	if( nSBCode == TB_THUMBTRACK)
	{
		int pos = m_sliderSteer.GetPos();
		m_motionControl.SetLastManoeuvrePosition();

		double rate = abs(pos) / 100.0; // set the rate to 0 -> 1.0, pos is -100 -> 100
		int sign = (pos > 0) ? -1 : 1;

		m_motionControl.SteerMotors(m_fMotorScanSpeed, sign * rate);
	}

	UpdateData(FALSE);
	//	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}



void CDialogGirthWeld::OnClickedCheckPredrive()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	SetButtonBitmaps();
}

// default various options if calibrating
void CDialogGirthWeld::OnClickedCheckCalibrate()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_bCalibrate)
	{
		m_nScanType = 0;
		m_bSeekAndStartAtLine = TRUE;
		m_bPredrive = FALSE;
		UpdateData(FALSE);
	}
	SetButtonBitmaps();
}

// this causes the MAG switch to be immediately enabled or dsabled
void CDialogGirthWeld::OnClickedCheckEnableMag()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	EnableMagSwitchControl(m_bEnableMAG);
}
