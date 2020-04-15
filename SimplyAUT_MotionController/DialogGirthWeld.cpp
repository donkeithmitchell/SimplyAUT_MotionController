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

#ifdef _DEBUG_TIMING
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

#define CALIBRATE_RGB_NOT      0
#define CALIBRATE_RGB_SCANNING 1
#define CALIBRATE_RGB_RETURN   2
#define LASER_BLINK				500

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

static UINT ThreadRunScan(LPVOID param)
{
	CDialogGirthWeld* this2 = (CDialogGirthWeld*)param;
	return this2->ThreadRunScan();
}

IMPLEMENT_DYNAMIC(CDialogGirthWeld, CDialogEx)

CDialogGirthWeld::CDialogGirthWeld(CMotionControl& motion, CLaserControl& laser, CMagControl& mag, GALIL_STATE& nState, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_GIRTHWELD, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_weldNavigation(motion,laser)
	, m_wndLaser(motion, laser, mag, m_weldNavigation, m_fScanLength)
	, m_wndMag(mag, m_bSeekStartLineInReverse, m_bSeekWithLaser, m_fSeekAndStartAtLine)
	, m_nGalilState(nState)
	, m_szScannedDist(_T("0.0 mm"))
	, m_szHomeDist(_T("0.0 mm"))
	, m_szTempBoard(_T(""))
	, m_szTempLaser(_T(""))
	, m_szTempSensor(_T(""))
	, m_szRunTime(_T(""))
	, m_szRGBLinePresent(_T(""))
	, m_szRGBValue(_T(""))
	, m_szRGBCalibration(_T(""))
	, m_bPredrive(FALSE)
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
	m_hThreadRunMotors = NULL;
	m_fDestinationPosition = 0;
	m_nGaililStateBackup = GALIL_IDLE;
	m_fScanStartPos = FLT_MAX;
	m_rgbLast = 0;
	m_lastCapPix = -1;

	m_szLaserEdge[0] = _T("---");
	m_szLaserEdge[1] = _T("---");
	m_szLaserEdge[2] = _T("---");
	m_szLaserJoint = _T("---");

	ResetParameters();
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
	DDX_Control(pDX, IDC_SPIN_SPEED, m_spinScanSpeed);
	DDX_Control(pDX, IDC_SPIN_ACCEL2, m_spinScanAccel);
	DDX_Control(pDX, IDC_SPIN_PREDRIVE, m_spinPredrive);
	DDX_Control(pDX, IDC_SPIN_SEEK_START, m_spinSeekStart);

	DDX_Text(pDX, IDC_EDIT_LR_OFFSET, m_szLROffset);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, GetLeftRightOffset(), -10.0, 10.0);

	DDX_Text(pDX, IDC_EDIT_CIRC, m_fScanCirc);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fScanCirc, 100.0, 10000.0);

	DDX_Text(pDX, IDC_EDIT_DIST, m_fDistToScan);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fDistToScan, 10.0, 10000.0);

	DDX_Text(pDX, IDC_EDIT_OVERLAP, m_fScanOverlap);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fScanOverlap, 0.0, 100.0);

	DDX_Text(pDX, IDC_EDIT_SPEED, m_fMotorScanSpeed);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fMotorScanSpeed, 1.0, 100.0);

	DDX_Text(pDX, IDC_EDIT_ACCEL2, m_fMotorScanAccel);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_fMotorScanAccel, 1.0, 100.0);

	DDX_Check(pDX, IDC_CHECK_PREDRIVE, m_bPredrive);
	DDX_Text(pDX, IDC_EDIT_PREDRIVE, m_fPredriveDistance);
	if (m_bCheck && m_bPredrive)
		DDV_MinMaxDouble(pDX, m_fPredriveDistance, 0.0, 1000.0);

	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_buttonPause);
	DDX_Control(pDX, IDC_BUTTON_MANUAL, m_buttonManual);
	DDX_Control(pDX, IDC_BUTTON_BACK, m_buttonBack);
	DDX_Control(pDX, IDC_BUTTON_FWD, m_buttonFwd);
	DDX_Control(pDX, IDC_BUTTON_ZERO_HOME, m_buttonZeroHome);
	DDX_Control(pDX, IDC_BUTTON_GO_HOME, m_buttonGoHome);
	DDX_Control(pDX, IDC_BUTTON_LASER_STATUS, m_buttonLaserStatus);

	DDX_Control(pDX, IDC_SLIDER_STEER, m_sliderSteer);


	DDX_Radio(pDX, IDC_RADIO_CIRC, m_nScanType);
	DDX_Check(pDX, IDC_CHECK_AUTOHOME, m_bStartScanAtHomePos);
	DDX_Check(pDX, IDC_CHECK_RETURNTOSTART, m_bReturnToStart);

	DDX_Control(pDX, IDC_STATIC_LASER, m_staticLaser);
	DDX_Control(pDX, IDC_STATIC_MAG, m_staticMag);
	DDX_Text(pDX, IDC_STATIC_HOMEDIST, m_szHomeDist);
	DDX_Text(pDX, IDC_STATIC_SCANNEDDIST, m_szScannedDist);

	DDX_Text(pDX, IDC_STATIC_LASER_DS, m_szLaserEdge[0]);
	DDX_Text(pDX, IDC_STATIC_LASER_US, m_szLaserEdge[1]);
	DDX_Text(pDX, IDC_STATIC_LASER_DIFF, m_szLaserEdge[2]);
	DDX_Text(pDX, IDC_STATIC_JOINT_LOCN, m_szLaserJoint);
	DDX_Check(pDX, IDC_CHECK_SEEK_START_REVERSE, m_bSeekStartLineInReverse);
	DDX_Check(pDX, IDC_CHECK_SEEK_WITH_LASER, m_bSeekWithLaser);
	DDX_Check(pDX, IDC_CHECK_SEEK_START_LINE, m_bSeekAndStartAtLine);

	DDX_Text(pDX, IDC_STATIC_TEMP_BOARD, m_szTempBoard);
	DDX_Text(pDX, IDC_STATIC_TEMP_LASER, m_szTempLaser);
	DDX_Text(pDX, IDC_STATIC_TEMP_SENSOR, m_szTempSensor);
	DDX_Text(pDX, IDC_STATIC_RUN_TIME, m_szRunTime);
	DDX_Text(pDX, IDC_STATIC_RGB_VALUE, m_szRGBValue);
	DDX_Text(pDX, IDC_STATIC_RGB_LINE, m_szRGBLinePresent);
	DDX_Text(pDX, IDC_STATIC_RGB_CALIBRATION, m_szRGBCalibration);

	if (pDX->m_bSaveAndValidate)
	{
		int delay = max((int)(1/*mm*/ * 1000.0 / m_fMotorScanSpeed + 0.5), 1);
		SetTimer(TIMER_GET_LASER_PROFILE, delay, NULL);

		m_fScanLength = (m_nScanType == 0) ? m_fScanCirc + m_fScanOverlap : m_fDistToScan;
	}

	DDX_Text(pDX, IDC_EDIT_SEEK_START_LINE, m_fSeekAndStartAtLine);
	if (m_bCheck && m_bSeekAndStartAtLine)
		DDV_MinMaxDouble(pDX, m_fSeekAndStartAtLine, 100.0, max(m_fScanLength, 100));
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

	ON_COMMAND(IDC_RADIO_CIRC,				&CDialogGirthWeld::OnRadioScanType)
	ON_COMMAND(IDC_RADIO_DIST,				&CDialogGirthWeld::OnRadioScanType)
	ON_MESSAGE(WM_STOPMOTOR_FINISHED,		&CDialogGirthWeld::OnUserStopMotorFinished)
	ON_MESSAGE(WM_USER_STATIC,				&CDialogGirthWeld::OnUserStaticParameter)
	ON_MESSAGE(WM_WELD_NAVIGATION,			&CDialogGirthWeld::OnUserWeldNavigation)
	ON_MESSAGE(WM_MOTION_CONTROL,			&CDialogGirthWeld::OnUserMotionControl)
	ON_MESSAGE(WM_MAG_STOP_SEEK,			&CDialogGirthWeld::OnUserMagStopSeek)
	ON_MESSAGE(WM_ARE_MOTORS_RUNNING,		&CDialogGirthWeld::OnUserAreMotorsRunning)
	
	ON_STN_CLICKED(IDC_STATIC_TEMP_BOARD, &CDialogGirthWeld::OnStnClickedStaticTempBoard)
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

	m_sliderSteer.SetRange(-100, 100);
	m_sliderSteer.SetTicFreq(10);
	m_sliderSteer.SetPos(0);

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
	m_wndMag.Create(&m_staticMag);

	m_wndMag.Init(this, WM_MAG_STOP_SEEK);
	m_weldNavigation.Init(this, WM_WELD_NAVIGATION);

	m_bInit = TRUE;
	SetButtonBitmaps();

//	SetLaserStatus(TIMER_LASER_OFF);

	SetTimer(TIMER_LASER_STATUS1, 500, NULL); //  status
	SetTimer(TIMER_LASER_TEMPERATURE, 500, NULL); // temperature
	SetTimer(TIMER_RGB_STATUS, 250, NULL); // will avoid reads of registyer if navigating
	SetTimer(TIMER_NOTE_RGB, 250, NULL);
	SetTimer(TIMER_ARE_MOTORS_RUNNING, 100, NULL);

	int delay = max((int)(1/*mm*/ * 1000.0 / m_fMotorScanSpeed + 0.5), 1);
	SetTimer(TIMER_GET_LASER_PROFILE, delay, NULL);

	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogGirthWeld::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_GIRTHWELD, pParent);
	ShowWindow(SW_HIDE);
}

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
		ar << m_fDistScanned;
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
		ar << mask;
	}
	else
	{
		try
		{
			ar >> m_szLROffset;
			ar >> m_fScanCirc;
			ar >> m_fDistToScan;
			ar >> m_fDistScanned;
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
	m_fScanCirc = 1000;
	m_fDistToScan = 1000;
	m_fDistScanned = 10;
	m_fScanOverlap = 50;
	m_nScanType = FALSE;
	m_bStartScanAtHomePos = FALSE;
	m_bReturnToStart = FALSE;
	m_bSeekAndStartAtLine = FALSE;
	m_fSeekAndStartAtLine = 0;
	m_bSeekStartLineInReverse = FALSE;
	m_bSeekWithLaser = FALSE;
	m_fMotorScanSpeed = 50;
	m_fMotorScanAccel = 25;
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
// set a timer at 500 ms to cycle between off and on colours to show blink
void CDialogGirthWeld::OnTimer(UINT_PTR nIDEvent)
{
	clock_t t1 = clock();

	switch (nIDEvent)
	{
	case TIMER_SHOW_MOTOR_SPEEDS: // only every 500 ms
////	ShowMotorSpeeds(); // 911 (this should be handled by the motor dialog)
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
#ifdef _DEBUG_TIMING
	g_CDialogGirthWeldOnTimerTime += clock() - t1;;
	g_CDialogGirthWeldOnTimerCount++;
#endif
}


double CDialogGirthWeld::GetCalibrationValue()
{
	if (m_bSeekWithLaser)
	{
		if (m_laserControl.GetProfile(10))
		{
			m_laserControl.CalcLaserMeasures(0.0, NULL, -1);
			double avg_side_height = (m_laserControl.m_measure2.weld_left_height_mm + m_laserControl.m_measure2.weld_right_height_mm) / 2;
			double weld_cap_height = m_laserControl.m_measure2.weld_cap_mm.y;

			return -weld_cap_height;
//			return avg_side_height - weld_cap_height; // the sides will not be well calculated
		}
		else
			return FLT_MAX;
	}
	else
		return m_magControl.GetRGBSum();
}


void CDialogGirthWeld::NoteRGBCalibration()
{
	CRect rect;
	double pos = m_motionControl.GetAvgMotorPosition();

	if (m_nCalibratingRGB == CALIBRATE_RGB_SCANNING)
	{
		double val = GetCalibrationValue();
		m_magControl.NoteRGBCalibration(pos, val, (int)(m_fSeekAndStartAtLine + 0.5));
	}
	else if(m_nCalibratingRGB == CALIBRATE_RGB_RETURN )
		m_magControl.SetRGBCalibrationPos(pos);

	m_wndMag.InvalidateRgn(NULL);
}

void CDialogGirthWeld::NoteRunTime()
{
	clock_t t1 = clock();
	long elapsed = (long)(t1 - m_nRunStart);
	m_szRunTime.Format("%d.%d sec", elapsed / 1000, elapsed % 10);

	GetDlgItem(IDC_STATIC_RUN_TIME)->SetWindowText(m_szRunTime);
#ifdef _DEBUG_TIMING
	g_NoteRunTimeTime += clock() - t1;
	g_NoteRunTimeCount++;
#endif
}

void CDialogGirthWeld::NoteIfMotorsRunning()
{
	clock_t t1 = clock();
	m_motionControl.NoteIfMotorsRunning();
#ifdef _DEBUG_TIMING
	g_NoteIfMotorsRunningTime += clock() - t1;
	g_NoteIfMotorsRunningCount++;
#endif
}


void CDialogGirthWeld::GetLaserProfile()
{
	static clock_t tim0 = clock();
	static clock_t tim1 = 0;
	static int nLaserOn1 = -1;
	clock_t t1 = clock();

	int nLaserOn2 = m_laserControl.IsLaserOn();
	if (nLaserOn1 != nLaserOn2)
	{
		m_wndLaser.InvalidateRgn(NULL);
		nLaserOn1 = nLaserOn2;
	}

	if (!nLaserOn2)
	{
		m_lastCapPix = -1;
		return;
	}


	if( !m_laserControl.GetProfile(10))
		return;

	double accel, velocity4[4];
	double pos = m_motionControl.GetAvgMotorPosition();
	velocity4[0] = m_motionControl.GetMotorSpeed("A", accel);
	velocity4[1] = m_motionControl.GetMotorSpeed("B", accel);
	velocity4[2] = m_motionControl.GetMotorSpeed("C", accel);
	velocity4[3] = m_motionControl.GetMotorSpeed("D", accel);

	m_lastCapPix = m_laserControl.CalcLaserMeasures(pos, velocity4, m_lastCapPix);
#ifdef _DEBUG_TIMING
	if( g_fp1 )
		fprintf(g_fp1, "%d\t%.1f\n", clock()-tim0, pos);
#endif

	// only invalidate the laser static about 4 times per second
	// this is acqwuired much more often
	clock_t tim2 = clock();
	if (tim1 == 0 || tim2 - tim1 > 250) // 4 times per seconmd max.
	{
		tim1 = tim2;
		m_wndLaser.InvalidateRgn(NULL);
	}
#ifdef _DEBUG_TIMING
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

	// check if the magnets have changed state
	// calling this too often will cause the controls to flicker
	static int bMagEngaged = -1;
	BOOL bMag = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;
	if (bMag != bMagEngaged)
	{
		bMagEngaged = bMag;
		SetButtonBitmaps(); // 911
	}
#ifdef _DEBUG_TIMING
	g_ShowLaserStatusTime += clock() - t1;
	g_ShowLaserStatusCount++;
#endif
}
	
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

#ifdef _DEBUG_TIMING
	g_ShowLaserTemperatureTime += clock() - t1;
	g_ShowLaserTemperatureCount++;
#endif
}

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
	if (!m_weldNavigation.IsNavigating())
	{
		clock_t t1 = clock();
		m_rgbLast = m_magControl.GetRGBSum();
		m_wndLaser.AddRGBData(m_rgbLast);
#ifdef _DEBUG_TIMING
		g_NoteRGBTime += clock() - t1;
		g_NoteRGBCount++;
	}
#endif
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

	if (!m_weldNavigation.IsNavigating())
	{
		int colour = m_magControl.GetRGBSum();
		if (colour != INT_MAX)
			m_szRGBValue.Format("%d", colour);
		else
			m_szRGBValue.Format(_T(""));

		int cal = m_magControl.GetRGBCalibrationValue();
		if (cal != INT_MAX)
			m_szRGBCalibration.Format("%d", cal);
		else
			m_szRGBCalibration.Format(_T(""));
	}


	GetDlgItem(IDC_STATIC_RGB_CALIBRATION)->SetWindowTextA(m_szRGBCalibration);
	GetDlgItem(IDC_STATIC_RGB_VALUE)->SetWindowTextA(m_szRGBValue);
	GetDlgItem(IDC_STATIC_RGB_LINE)->SetWindowTextA(m_szRGBLinePresent);
#ifdef _DEBUG_TIMING
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
#ifdef _DEBUG_TIMING
	g_ShowMotorPositionTime += clock() - t1;
	g_ShowMotorPositionCount++;
#endif
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


void CDialogGirthWeld::OnDeltaposSpinPredrive(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fPredriveDistance += (inc > 0) ? 10 : -10;
	m_fPredriveDistance = min(max(m_fPredriveDistance, 0), 500);
	UpdateData(FALSE);
	*pResult = 0;
}


void CDialogGirthWeld::OnDeltaposSpinSeekStart(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	int inc = pNMUpDown->iDelta;
	UpdateData(TRUE);
	m_fSeekAndStartAtLine += (inc > 0) ? 10 : -10;
	m_fSeekAndStartAtLine = min(max(m_fSeekAndStartAtLine, 0), 500);
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
	offset = min(max(offset, -10.0), 10);
	FormatLeftRightOffset(offset);
	UpdateData(FALSE);

	*pResult = 0;
}


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


void CDialogGirthWeld::SendErrorMessage(const CString& msg)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)&msg);
}


void CDialogGirthWeld::SetButtonBitmaps()
{
	// TODO: Add your control notification handler code here
//	SendDebugMessage("SetButtonBitmaps"); // happenbs toio often to auto put in statuys

	BOOL bGalil = m_motionControl.IsConnected();
	BOOL bMag = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;

	HBITMAP hBitmapRight = (HBITMAP)m_bitmapGoRight.GetSafeHandle();
	HBITMAP hBitmapLeft = (HBITMAP)m_bitmapGoLeft.GetSafeHandle();
	HBITMAP hBitmapUp = (HBITMAP)m_bitmapGoUp.GetSafeHandle();
	HBITMAP hBitmapDown = (HBITMAP)m_bitmapGoDown.GetSafeHandle();
	HBITMAP hBitmapStop = (HBITMAP)m_bitmapStop.GetSafeHandle();
	HBITMAP hBitmapPause = (HBITMAP)m_bitmapPause.GetSafeHandle();

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
	int pos = (int)(GetAvgMotorPosition() + 0.5);

	// set to 
	m_buttonManual.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_SCAN || m_nGalilState == GALIL_IDLE));
	m_buttonPause.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_SCAN)); // && !(m_bPaused && m_hThreadRunMotors != NULL) );
	m_buttonGoHome.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_GOHOME || m_nGalilState == GALIL_IDLE) && pos != 0);
	m_buttonZeroHome.EnableWindow(bGalil && bMag && m_nGalilState == GALIL_IDLE && pos != 0 );
	m_sliderSteer.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_FWD || m_nGalilState == GALIL_BACK) && !m_bPaused);

	m_buttonBack.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_BACK || 
		(m_bPaused && m_nGalilState != GALIL_FWD)));
	m_buttonFwd.EnableWindow(bGalil && bMag && (m_nGalilState == GALIL_IDLE || m_nGalilState == GALIL_FWD || 
		(m_bPaused && m_nGalilState != GALIL_BACK)));

	GetDlgItem(IDC_EDIT_LR_OFFSET)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_CIRC)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC);
	GetDlgItem(IDC_EDIT_OVERLAP)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_CIRC);
	GetDlgItem(IDC_EDIT_DIST)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_nScanType == SCANTYPE_DIST);
	GetDlgItem(IDC_RADIO_CIRC)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_RADIO_DIST)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_AUTOHOME)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_RETURNTOSTART)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_SEEK_START_LINE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_CHECK_SEEK_START_REVERSE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bSeekAndStartAtLine);
	GetDlgItem(IDC_EDIT_SEEK_START_LINE)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bSeekAndStartAtLine);
	GetDlgItem(IDC_CHECK_SEEK_WITH_LASER)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE && m_bSeekAndStartAtLine);
	GetDlgItem(IDC_EDIT_PREDRIVE)->EnableWindow(bGalil && m_bPredrive && m_nGalilState == GALIL_IDLE );

	GetDlgItem(IDC_EDIT_SPEED)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);
	GetDlgItem(IDC_EDIT_ACCEL2)->EnableWindow(bGalil && m_nGalilState == GALIL_IDLE);


	GetDlgItem(IDC_STATIC_PAUSE)->SetWindowText(m_bPaused ? _T("Resume" : _T("Pause")));
}


void CDialogGirthWeld::OnClickedButtonPause()
{
	SendDebugMessage("OnClickedButtonPause");
	m_bPaused = !m_bPaused;
//	SetButtonBitmaps();

	// stop the motors, and resume
	if (m_bPaused)
	{
//		SetButtonBitmaps();

		// asking motors to stop will cause existing thread to end
		m_bResumeScan = FALSE; // set before stopping, so noted in the thread
		m_motionControl.StopMotors(TRUE);
		m_laserControl.TurnLaserOn(FALSE);
		SetButtonBitmaps();
	}
	// resume the motors
	// this is similar to OnClickedButtonScan
	// except the position is not reset
	else
	{
		m_bResumeScan = TRUE;
		m_lastCapPix = -1;
		m_laserControl.TurnLaserOn(TRUE);
		GetLaserProfile();
		StartNotingMotorSpeed(TRUE);
		m_hThreadRunMotors = ::AfxBeginThread(::ThreadRunScan, (LPVOID)this)->m_hThread;
		SetButtonBitmaps();
	}
}

// 1. check if to go to home position prior to run
double CDialogGirthWeld::GetDestinationPosition()
{
	// the nominal length to scan
	double pos = m_fScanLength;

	// if not travelling home first, then add the current position
	// else, assuem it will be zewro
	if (!m_bStartScanAtHomePos)
		pos += m_motionControl.GetAvgMotorPosition();

	return pos;
}


void CDialogGirthWeld::OnClickedButtonScan()
{
	SendDebugMessage("OnClickedButtonScan");
	if (CheckParameters() && CheckIfToRunOrStop(GALIL_SCAN))
	{
		// TODO: Add your control notification handler code here
		m_bPaused = FALSE;
		m_bResumeScan = FALSE;

		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_SCAN : GALIL_IDLE;
		if (m_nGalilState == GALIL_SCAN)
		{
			m_bAborted = FALSE;
			m_bScanning = TRUE;
			SetButtonBitmaps();

			m_fDestinationPosition = GetDestinationPosition();
			m_motionControl.SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanAccel);

	//		m_magControl.ResetEncoderCount();
			m_magControl.EnableMagSwitchControl(FALSE);
//			m_magControl.ResetEncoderCount();

			SetNoteRunTime(TRUE);
			StartNotingMotorSpeed(TRUE);
			StartMeasuringLaser(TRUE);
		}
		else
		{
			m_bAborted = TRUE;
			m_bScanning = FALSE;
			m_buttonManual.EnableWindow(FALSE);
		}

		m_hThreadRunMotors = ::AfxBeginThread(::ThreadRunScan, (LPVOID)this)->m_hThread;
	}
}

void CDialogGirthWeld::StartReadMagStatus(BOOL bSet)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_MAG_STATUS_ON, bSet);
}

void CDialogGirthWeld::StartSteeringMotors(int nSteer, int start_pos, int end_pos, double fSpeed)
{
#ifdef _DEBUG_TIMING
	extern clock_t g_ThreadReadSocketTime;
	extern int g_ThreadReadSocketCount;
	extern clock_t g_LaserOnPaintTime;
	extern int g_LaserOnPaintCount;
#endif

	if (nSteer)
	{
#ifdef _DEBUG_TIMING
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

		m_weldNavigation.StartSteeringMotors(nSteer, start_pos, end_pos, fSpeed, m_fMotorScanAccel, GetLeftRightOffset(), m_bScanning);
		m_wndLaser.ResetLaserOffsetList();
		m_motionControl.ResetLastManoeuvrePosition();
	}
	else
	{
		m_weldNavigation.StartSteeringMotors(0x0, 0,0,0,0,0,0);
#ifdef _DEBUG_TIMING
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
	StartReadMagStatus(nSteer == 0); 
}

void CDialogGirthWeld::StartMeasuringLaser(BOOL bSet)
{
	if (bSet)
	{
		m_laserControl.TurnLaserOn(TRUE);
		int delay = max((int)(1 * 1000.0 / m_fMotorScanSpeed + 0.5), 1);
		SetTimer(TIMER_GET_LASER_PROFILE, delay, NULL);
		m_lastCapPix = -1;
		GetLaserProfile();
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

void CDialogGirthWeld::StartNotingMotorSpeed(BOOL bSet)
{
	if (bSet)
		SetTimer(TIMER_SHOW_MOTOR_SPEEDS, 500, NULL);
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

void CDialogGirthWeld::OnClickedButtonGoHome()
{
	// TODO: Add your control notification handler code here
	SendDebugMessage("OnClickedButtonGoHome");
	if (CheckParameters() )
	{
		// savew re-enableling controls until after stop if not now starting
		m_nGalilState = m_nGaililStateBackup = (m_nGalilState == GALIL_IDLE) ? GALIL_GOHOME : GALIL_IDLE;
		if (m_nGalilState == GALIL_GOHOME)
		{
			if (GetAvgMotorPosition() != 0)
			{
				SetButtonBitmaps();

				StartNotingMotorSpeed(TRUE);
				m_motionControl.SetSlewSpeed(m_fMotorScanSpeed / 2, m_fMotorScanAccel);
				StartMeasuringLaser(TRUE);
				m_magControl.EnableMagSwitchControl(FALSE);

				int start_pos = (int)m_motionControl.GetAvgMotorPosition();

				// insure that not navigating when tell to start
				StartSteeringMotors(0x0, 0, 0, 0);
				m_motionControl.GoToPosition(0.0, FALSE);
				StartSteeringMotors(0x3, start_pos, 0, m_fMotorScanSpeed / 2);
			}
		}
		else
			StartSteeringMotors(0x0, 0, 0, 0);

		m_hThreadRunMotors = ::AfxBeginThread(::ThreadGoToHome, (LPVOID)this)->m_hThread;
	}
}



UINT CDialogGirthWeld::ThreadGoToHome()
{
	if (m_nGalilState != GALIL_GOHOME)
		StopMotors(TRUE);
	else
	{
		WaitForMotorsToStart();
		Sleep(1000);
		WaitForMotorsToStop();
	}
	PostMessage(WM_STOPMOTOR_FINISHED);
	return 0;
}



// use this to insure thaT THE MAIN  THREAD IS C ALLING IT, NOT THE USER THREASD
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
BOOL CDialogGirthWeld::SetSlewSpeed(const double fSpeed[4])
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_SET_SLEW_SPEED4, (LPARAM)fSpeed);
	}
	else
		return FALSE;
}

BOOL CDialogGirthWeld::DefinePositions(double pos)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_DEFINE_POSITION, (LPARAM)(1000.0 * pos + 0.5) );
	else
		return FALSE;
}

BOOL CDialogGirthWeld::ResetEncoderCount()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_RESET_ENCODER);
	else
		return FALSE;
}


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

BOOL CDialogGirthWeld::GoToPosition(double pos)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (BOOL)SendMessage(WM_MOTION_CONTROL, MC_GOTO_POSITION, (LPARAM)(pos * 1000 + 0.5));
	else
		return FALSE;
}

double  CDialogGirthWeld::GetSlewSpeed(const char* axis)
{
	clock_t t1 = clock();
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		double ret = SendMessage(WM_MOTION_CONTROL, MC_GET_SLEW_SPEED, axis[0] - 'A') / 1000.0;
#ifdef _DEBUG_TIMING
		g_GetSlewSpeedTime += clock() - t1;
		g_GetSlewSpeedCount++;
#endif
		return ret;
	}
	else
		return FLT_MAX;
}

void CDialogGirthWeld::StopMotors(BOOL bWait)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		SendMessage(WM_MOTION_CONTROL, MC_STOP_MOTORS, (LPARAM)bWait);
}

double CDialogGirthWeld::GetAvgMotorPosition()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return (double)(SendMessage(WM_MOTION_CONTROL, MC_GET_AVG_POS) / 1000.0);
	else
		return FLT_MAX;
}

double  CDialogGirthWeld::GetRGBSum()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		return SendMessage(WM_MOTION_CONTROL, MC_GET_RGB_SUM) / 1000.0;
	else
		return FLT_MAX;
}

// can't use SEndMessage to wait for motors to stop
// so simply poll the motor speeds untill all are zero
// this is called by the thread
BOOL CDialogGirthWeld::WaitForMotorsToStop()
{
	Sleep(100);
	for (int i=0; i < 100 && AreMotorsRunning(); ++i)
		Sleep(10);

	return !AreMotorsRunning();
}

LRESULT CDialogGirthWeld::OnUserAreMotorsRunning(WPARAM, LPARAM)
{
	// consider going back to noted value in OnTimer()
	return (LRESULT)m_motionControl.AreMotorsRunning(); //911
}

BOOL CDialogGirthWeld::AreMotorsRunning()
{
	return (BOOL)SendMessage(WM_ARE_MOTORS_RUNNING);
}


BOOL CDialogGirthWeld::WaitForMotorsToStart()
{
	Sleep(100);
	for (int i=0; i < 100 && !AreMotorsRunning(); ++i)
		Sleep(10);

	return AreMotorsRunning();
}

LRESULT CDialogGirthWeld::OnUserMagStopSeek(WPARAM wParam, LPARAM lParam)
{
	if (m_nCalibratingRGB == CALIBRATE_RGB_SCANNING)
		m_motionControl.StopMotors(TRUE/*wait*/);

	return 0L;
}

// this will always be the start up thread
LRESULT CDialogGirthWeld::OnUserMotionControl(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case MC_SET_SLEW_SPEED_ACCEL:
	{
		int iAccel = lParam & 0xFFFF;
		int iSpeed = (lParam >> 16) & 0xFFFF;
		double fAccel = iAccel / 10.0;
		double fSpeed = iSpeed / 10.0;
		return (LRESULT)m_motionControl.SetSlewSpeed(fSpeed, fAccel);
	}
	case MC_SET_SLEW_SPEED4:
	{
		const double* fSpeed = (double*)lParam;
		return (LRESULT)m_motionControl.SetSlewSpeed(fSpeed[0], fSpeed[1], fSpeed[2], fSpeed[3]);
	}
	case MC_RESET_ENCODER:
			return (LRESULT)m_magControl.ResetEncoderCount();
		case MC_GOTO_POSITION:
			return (LRESULT)m_motionControl.GoToPosition(lParam / 1000.0, FALSE/*dont wait*/);
		case MC_GOTO_POSITION2:
		{
			const double* position = (double*)lParam;
			return (LRESULT)m_motionControl.GoToPosition2(position[0], position[1]);
		}
		case MC_GET_AVG_POS:
			return (LRESULT)(1000 * m_motionControl.GetAvgMotorPosition() + 0.5);
		case MC_DEFINE_POSITION:
			m_motionControl.DefinePositions( lParam / 1000.0 );
			return 1L;
		case MC_GET_RGB_SUM:
			return (LRESULT)(1000 * m_magControl.GetRGBSum() + 0.5);
		case MC_STOP_MOTORS:
			m_motionControl.StopMotors( (BOOL)lParam);
			return 0L;
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

BOOL CDialogGirthWeld::SeekStartLine()
{
	CString str;
	// wherte is the crawler now
	DefinePositions(0);
	double pos0 = 0; //  GetAvgMotorPosition();

	// reset any previously set calibration
	m_magControl.ResetRGBCalibration();

	// start noting the RGB data VS position
	m_nCalibratingRGB = CALIBRATE_RGB_SCANNING;
	SetTimer(TIMER_NOTE_CALIBRATION, 10, NULL);
	SendMessage(WM_SIZE); // replace the laser window with the calibration window

	// noite the rtequested motor speed, and use 1/2 of that for navigation
	int start_pos = 0;
	int end_pos = (int)(m_bSeekStartLineInReverse ? -m_fSeekAndStartAtLine : m_fSeekAndStartAtLine);

	SetSlewSpeed(m_fMotorScanSpeed / 4, m_fMotorScanAccel);
	StartSteeringMotors(0x3, start_pos, end_pos, m_fMotorScanSpeed /4);

	// now drive for the desired length, and wait for the motors to stop
	// as navigating to the weld, give a buffer to the end position
	Sleep(100);
	GoToPosition(end_pos + GetAccelDistance());
	if (WaitForMotorsToStart())
		WaitForMotorsToStop();

	if (m_bAborted || !m_magControl.CalculateRGBCalibration(m_bSeekWithLaser))
		return FALSE;

	// note where the start line is
	m_wndMag.InvalidateRgn(NULL);
	double pos2 = m_magControl.GetRGBCalibration().pos; // line found here
	double pos3 = GetAvgMotorPosition(); // where at now
	str.Format("Start Line Found at (%.1f mm)", pos2);
	if (AfxMessageBox(str, MB_OKCANCEL) != IDOK)
		return FALSE;

	// if seeking with the laser, no need to return to the ho9me position, unless starting there
	SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanAccel);
	StartSteeringMotors(0x0, 0, 0, 0);

	m_nCalibratingRGB = CALIBRATE_RGB_RETURN;
	if (1 || !m_bSeekWithLaser) // 911
	{
		// now go to this position
		// show the position on the graph
		// don't navigate during this move as nbackwards navigation is not yet working 911
		GoToPosition(pos2);
		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
		if (m_bAborted)
			return FALSE;
	
		if (AfxMessageBox("Start Scan", MB_OKCANCEL) != IDOK)
			return FALSE;
		
		// now set this as the new zero poisition
		DefinePositions(0);
		ResetEncoderCount();
	}

	// set the current position to be pos3-pos2
	else
	{
		DefinePositions(pos3-pos2);

		// 911, will need to tell recording what the start encoder count should be
	}


	// now back up by the accewleration distance
	if (m_bPredrive && m_fPredriveDistance > 0)
	{
		GoToPosition(-m_fPredriveDistance);
		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
	}

	m_nCalibratingRGB = CALIBRATE_RGB_NOT;
	m_fScanStartPos = 0; // will be starting at zero
	return !m_bAborted;
}

// want to set the GoToPosition() distance further than that actually desired
// this way can issue a 'ATOP' before deceleration
// the motors will otherwise travel the exact same distance, even if don't want to
double CDialogGirthWeld::GetDistanceToBuffer()const
{
	double accel_dist = GetAccelDistance();
	return accel_dist + 50;
}


// 1. check if to go to home position prior to run
UINT CDialogGirthWeld::ThreadRunScan()
{
	CString str;
	// have requested to stop

	int from_mm = 0;
	int to_mm = (int)(m_fDestinationPosition + 0.5);

	if (m_bAborted)
	{
		m_bScanning = FALSE;
		m_bResumeScan = FALSE;
		m_nCalibratingRGB = CALIBRATE_RGB_NOT;
		StopMotors(TRUE);
		KillTimer(TIMER_NOTE_CALIBRATION);
		WaitForMotorsToStop();
		StartSteeringMotors(0x0, 0,0, 0);
		InformRecordingSW(-1); // indicate that aborted
		SendMessage(WM_SIZE); // replace the laser window with the calibration window
		PostMessage(WM_STOPMOTOR_FINISHED);
	}

	// have requested to resume the scan from current location
	// the destination location has not changed
	// the navigation was naot stopped during pause
	// slo no need to resume
	else if (m_bResumeScan)
	{
		GoToPosition(m_fDestinationPosition + GetDistanceToBuffer());
		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
		if (!m_bPaused)
			PostMessage(WM_STOPMOTOR_FINISHED);
	}

	// starting a new scan
	else
	{
		// optionally go to the home positionb
		if (m_bStartScanAtHomePos)
		{
			m_fScanStartPos = 0; // wil;l be starting at zero
			m_bScanning = FALSE;

			// go to the home position and resewt the encoder
			GoToPosition(0);
			if (WaitForMotorsToStart())
				WaitForMotorsToStop();
			if (m_bAborted)
				return ThreadRunScan();

			ResetEncoderCount();
			// go to the acceleration distaznce behind home
			// tell the recording S/W to start a zero
			from_mm = 0;
			to_mm = (int)(m_fDestinationPosition + 0.5);
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
			BOOL bSeek = SeekStartLine();
			KillTimer(TIMER_NOTE_CALIBRATION);
			m_nCalibratingRGB = CALIBRATE_RGB_NOT;
			SendMessage(WM_SIZE); // replace the laser window with the calibration window
			
			if( !bSeek )
			{
				m_bAborted = TRUE;
				return ThreadRunScan();
			}
			from_mm = 0;
			to_mm = (int)(m_fDestinationPosition + 0.5);
		}

		// just record from where are
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

		if (m_bAborted)
			return ThreadRunScan();

		// signal the recording S/W to start recording
		// may have already been navigating at a different speed, so restart it
		m_bScanning = TRUE;
		SetSlewSpeed(m_fMotorScanSpeed, m_fMotorScanAccel);
		StartSteeringMotors(0x0, 0, 0, 0);
		StartSteeringMotors(0x3, from_mm, to_mm, m_fMotorScanSpeed);

		InformRecordingSW(1, from_mm, to_mm);
		GoToPosition(m_fDestinationPosition + GetDistanceToBuffer());
		if (WaitForMotorsToStart())
			WaitForMotorsToStop();
		if (m_bAborted)
			return ThreadRunScan();

		// if paused have not finished recording
		// if aborted, then will finish above
		/////////////////////////////////////////
		if (!m_bPaused && !m_bAborted )
		{
			// update thje files list
			StartSteeringMotors(0x0, 0, 0, 0);
			m_weldNavigation.WriteScanFile();
			UpdateScanFileList();
			m_bScanning = FALSE;
			InformRecordingSW(0); // normal termination

			// check to see if to return to start position now (start noit home
			if (m_bReturnToStart)
			{
				// return to the start locatioon (less the accdeeration backup)
				GoToPosition(m_fScanStartPos);
				if (WaitForMotorsToStart())
					WaitForMotorsToStop();
				if (m_bAborted)
					return ThreadRunScan();
			}
			PostMessage(WM_STOPMOTOR_FINISHED);
		}
	}

	return 0L;
}

// nStart
// 1: starting
// 0: ended
// -1: aborted
void CDialogGirthWeld::InformRecordingSW(BOOL record, int from/*= 0*/, int to/* = 0*/)
{
	int xx = 1; // 911
}


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

//		SetButtonBitmaps();
		RunMotors();
	}
}


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

	//	SetButtonBitmaps();
		RunMotors();
	}
}

void CDialogGirthWeld::RunMotors()
{
	int delay2 = (int)(1000.0 / m_fMotorScanSpeed + 0.5); // every mm

	if (m_nGalilState == GALIL_IDLE)
	{
		// because of deceleration it takes a while for the motors to stop
		// will not return from this fuinction until they have stopped
		// thus, call this from a thread, and block starts until the motors are stopped
		m_hThreadRunMotors = AfxBeginThread(::ThreadStopMotors, (LPVOID)this)->m_hThread;
	}
	else if (m_nGalilState == GALIL_FWD || (m_bPaused && m_nGaililStateBackup == GALIL_FWD) )
	{
		if (m_fMotorScanSpeed != FLT_MAX && m_fMotorScanAccel != FLT_MAX)
		{
			// tezmperary code to test steering
			m_wndLaser.ResetRGBData();
			SetNoteRunTime(TRUE);

			if (m_motionControl.SetMotorJogging(m_fMotorScanSpeed, m_fMotorScanAccel))
			{
				m_motionControl.ResetLastManoeuvrePosition();
				m_magControl.EnableMagSwitchControl(FALSE);
	//			m_magControl.ResetEncoderCount();

				// if pausewd, then leave the navigation alone
				// else just the tracking only
				if( !m_bPaused )
					StartSteeringMotors(0x1, 0,0, 0); // track only

	//			StartNotingRGBData(TRUE);
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
		if (m_fMotorScanSpeed != FLT_MAX && m_fMotorScanAccel != FLT_MAX)
		{
			// tezmperary code to test steering
			m_wndLaser.ResetRGBData();
			SetNoteRunTime(TRUE);
			if (m_motionControl.SetMotorJogging(-m_fMotorScanSpeed, m_fMotorScanAccel))
			{
				m_motionControl.ResetLastManoeuvrePosition();
				m_magControl.EnableMagSwitchControl(FALSE);
		//		m_magControl.ResetEncoderCount();

				// if pausewd, then leave the navigation alone
				// else just the tracking only
				if (!m_bPaused)
					StartSteeringMotors(0x1, 0,0, 0); // track only

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

UINT CDialogGirthWeld::ThreadStopMotors()
{
	StopMotors(TRUE);
//	m_laserControl.TurnLaserOn(FALSE);				// do not call thesae from a thread
//	m_magControl.EnableMagSwitchControl(TRUE);		// call, them in the finish up call-back
	PostMessage(WM_STOPMOTOR_FINISHED);
	return 0;
}

// these aare slow, and should be used sparingly
// talking to the motor controller must be done by the main thread
// so required here
LRESULT CDialogGirthWeld::OnUserWeldNavigation(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case NAVIGATE_LR_DIFFEENCE:
	{
		double A = m_motionControl.GetMotorPosition("A");
		double B = m_motionControl.GetMotorPosition("B");
		double C = m_motionControl.GetMotorPosition("D");
		double D = m_motionControl.GetMotorPosition("D");

		return (LRESULT)(100.0 * ((A + D) / 2 - (B + C) / 2) + 0.5);
	}
	case NAVIGATE_STOP_MOTORS:
		m_motionControl.StopMotors((BOOL)lParam);
		return 1L;
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

		case NAVIGATE_SET_MOTOR_DECEL:
		{
			double decel = lParam / 100.0;
			m_motionControl.SetSlewDeceleration(decel);
			return 1L;
		}
	}
	return 0L;
}

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
		m_fScanStartPos = FLT_MAX;
		StartSteeringMotors(0x0, 0,0, 0);
		SetNoteRunTime(FALSE); // an invalid value ends it
		ShowMotorPosition();
	}
	else
		m_nGalilState = m_nGaililStateBackup;

	SetButtonBitmaps();
	return 0L;
}

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
	UINT nIDC1[] = { IDC_STATIC_LASER, 
		IDC_SLIDER_STEER, IDC_STATIC_LEFT, IDC_STATIC_RIGHT,
		IDC_STATIC_FWD, IDC_BUTTON_FWD, IDC_STATIC_BACK, IDC_BUTTON_BACK,
		IDC_STATIC_RGB_CALIBRATION , IDC_STATIC_RGB_VALUE , IDC_STATIC_RGB_LINE , 
		IDC_STATIC1, IDC_STATIC2, IDC_STATIC3, IDC_STATIC4, 0 };

	UINT nIDC2[] = { IDC_STATIC_MAG, 0 };

	// note the original location of the statis, so can put back to 
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




void CDialogGirthWeld::OnStnClickedStaticTempBoard()
{
	// TODO: Add your control notification handler code here
}

// drive to the home (0) position before scanning
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
	m_fMotorScanSpeed = min(max(m_fMotorScanSpeed, 0.1), 100);
	UpdateData(FALSE);

	// call direct, this can not be a thread
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
	m_fMotorScanAccel = min(max(m_fMotorScanAccel, 0.1), 100);
	UpdateData(FALSE);

	*pResult = 0;
}


void CDialogGirthWeld::OnReleasedcaptureSliderSteer(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	m_sliderSteer.SetPos(0);
	m_motionControl.SteerMotors(m_fMotorScanSpeed, 0.0);
	*pResult = 0;
}



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

		double rate = abs(pos) / 100.0; //
		int sign = (pos > 0) ? -1 : 1;

		m_motionControl.SteerMotors(m_fMotorScanSpeed, sign * rate);
	}
	else
	{
		int xx = 1;
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
