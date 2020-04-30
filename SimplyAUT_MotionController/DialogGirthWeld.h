#pragma once
#include "StaticLaser.h"
#include "WeldNavigation.h"
#include "SimplyAUT_MotionController.h"
#include "StaticMag.h"
#include "button.h"

// CDialogGirthWeld dialog

class CMotionControl;
class CLaserControl;
class CMagControl;
struct NAVIGATION_PID;
class CDialogGirthWeld : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogGirthWeld)

public:
	CDialogGirthWeld(CMotionControl&, CLaserControl&, CMagControl&, GALIL_STATE& nState, NAVIGATION_PID& pid, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogGirthWeld();

	virtual BOOL OnInitDialog();
	void	Create(CWnd* pParent);
	void	SetButtonBitmaps();
	BOOL	CheckIfToRunOrStop(GALIL_STATE);
	BOOL	CheckParameters();
//	void    SetLaserStatus(LASER_STATUS nStatus);
	void	EnableControls();
	void    RunMotors();
	void	Init(CWnd* pParent, UINT nMsg);
	void    ShowMotorPosition();
	void    ShowRGBStatus();
	BOOL	CheckVisibleTab();
	UINT    ThreadStopMotors(void);
	UINT	ThreadGoToHome(void);
	UINT	ThreadRunScan();
	UINT	ThreadAbortScan();
	void	SendDebugMessage(const CString&);
	void	SendErrorMessage(const char*);
	void    ShowLaserTemperature();
	void    ShowLaserStatus();
	void	SetNoteRunTime(BOOL);
//	void    StartNotingRGBData(BOOL);
	void	StartNotingMotorSpeed(BOOL);
	void    StartNavigation(int nSteer, int start_pos, int end_pos, double fSpeed);
	void    StartReadMagStatus(BOOL);
	void    StartMeasuringLaser(BOOL);
	void    GetLaserProfile();
	void	NoteIfMotorsRunning();
	BOOL	SetSlewSpeed(double fSpeed, double fAccel);
	BOOL	SetSlewSpeed(const double fSpeed[4]);
	double  GetSlewSpeed(const char* axis);
	void    StopMotors(BOOL);
	double  GetRGBSum();
	BOOL    GoToPosition(double pos);
	BOOL    SetMotorJogging(int dir);
	BOOL    GoToPosition2(double left, double right);
	BOOL    ResetEncoderCount();
	double	GetAvgMotorPosition();
	double  GetEncoderDistance();
	BOOL    DefinePositions(double);
	BOOL    WaitForMotorsToStop();
	BOOL	AreMotorsRunning();
	BOOL    WaitForMotorsToStart();
	double  GetLeftRightOffset()const;
	void    FormatLeftRightOffset(double);
	void    NoteRunTime();
	void    NoteRGBSum();
	double	GetDestinationPosition();
	void    InformRecordingSW(int record, int from=0, int to=0);
	void	NoteRGBCalibration();
	double	GetCalibrationValue();
	BOOL	SeekStartLine();
	BOOL    CalibrateCircumference();
	BOOL    StartNewScan();
	double	GetMotorSpeed();
	double	GetMotorAccel();
	double  GetAccelDistance()const;
	double	GetDistanceToBuffer()const;
	void	UpdateScanFileList();
	void    Serialize(CArchive& ar);
	void    ResetParameters();
	void	EnableMagSwitchControl(BOOL bEnableMAG);
	LRESULT OnUserFinished(CWinThread**);

	enum { WM_USER_SCAN_FINISHED= WM_USER+1, WM_USER_ABORT_FINISHED, WM_USER_STATIC, WM_WELD_NAVIGATION, WM_MOTION_CONTROL, WM_MAG_STOP_SEEK, WM_ARE_MOTORS_RUNNING
	};
	enum { MC_SET_SLEW_SPEED_ACCEL = 0, MC_SET_SLEW_SPEED4, MC_GOTO_POSITION, MC_RESET_ENCODER, MC_DEFINE_POSITION, 
		MC_GET_SLEW_SPEED, MC_GET_RGB_SUM, MC_STOP_MOTORS, MC_GET_AVG_POS, MC_GET_ENC_DIST, MC_GOTO_POSITION2, MC_MOTOR_JOGGING};
	enum TIMER_GW { TIMER_SHOW_MOTOR_SPEEDS = 0, TIMER_LASER_TEMPERATURE, TIMER_LASER_STATUS1, TIMER_RGB_STATUS, TIMER_RUN_TIME, 
		TIMER_NOTE_RGB, TIMER_GET_LASER_PROFILE, TIMER_ARE_MOTORS_RUNNING, TIMER_NOTE_CALIBRATION	};
	enum { STATUS_GETLOCATION=0, STATUS_SHOWLASERSTATUS};
	enum{ NAVIGATE_SEND_DEBUG_MSG=0, NAVIGATE_SET_MOTOR_SPEED, NAVIGATE_SET_MOTOR_DECEL, NAVIGATE_STOP_MOTORS, NAVIGATE_LR_DIFFEENCE, NAVIGATE_GET_AVG_SPEED
	};

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_GIRTHWELD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	GALIL_STATE&	m_nGalilState;
	CMotionControl& m_motionControl;
	CLaserControl&	m_laserControl;
	CMagControl&	m_magControl;
	CWinThread*		m_pThreadScan;
	CWinThread*		m_pThreadAbort;
	GALIL_STATE		m_nGaililStateBackup;
	CWeldNavigation m_weldNavigation;

	UINT	m_nMsg;
	CWnd*	m_pParent;

	int     m_nCalibratingRGB;
	int     m_nTimerCount;
	int     m_rgbLast;
	int     m_lastCapPix;
	clock_t	m_nRunStart;

	CBitmap	m_bitmapPause;
	CBitmap	m_bitmapGoRight;
//	CBitmap	m_bitmapGoLeft;
	CBitmap	m_bitmapGoDown;
	CBitmap	m_bitmapGoUp;
	CBitmap	m_bitmapStop;
	CBitmap	m_bitmapLaserOff;		// blank
	CBitmap	m_bitmapLaserOK;		// Green
	CBitmap	m_bitmapLaserError;		// red
	CBitmap	m_bitmapLaserHot;		// magenta
	CBitmap	m_bitmapLaserLoading;	// blue
	CBitmap	m_bitmapDisconnect;	// blue
	CBitmap	m_bitmapConnect;	// blue

	CStatic		    m_staticLaser;
	CStatic		    m_staticMag;
	CStaticLaser	m_wndLaser;
	CStaticMag		m_wndMag;

	CSpinButtonCtrl m_spinLROffset;
	CSpinButtonCtrl m_spinScanCirc;
	CSpinButtonCtrl m_spinScanDist;
	CSpinButtonCtrl m_spinScanOverlap;
	CSpinButtonCtrl m_spinScanSpeed;
	CSpinButtonCtrl m_spinScanAccel;
	CSpinButtonCtrl m_spinPredrive;
	CSpinButtonCtrl m_spinSeekStart;

	CSliderCtrl		m_sliderSteer;

	CString		m_szLROffset;
	CString		m_szScannedDist;
	CString		m_szHomeDist;
	CString		m_szLaserEdge;
	CString		m_szLaserJoint;
	CString		m_szTempBoard;
	CString		m_szTempLaser;
	CString		m_szTempSensor;
	CString		m_szRunTime;
	CString		m_szRGBLinePresent;

	double	m_fDestinationPosition;
	double  m_fScanStartPos;
	double m_fScanCirc;
	double m_fDistToScan;
//	double m_fDistScanned;
	double m_fScanOverlap;
	double m_fScanLength;
	double m_fMotorScanSpeed;
	double m_fMotorScanAccel;
	double m_fSeekAndStartAtLine;
	double m_fPredriveDistance;

	CButton m_buttonPause;
	CButton m_buttonManual;
	CButton m_buttonZeroHome;
	CButton m_buttonGoHome;
	CButton m_buttonBack;
	CButton m_buttonFwd;
	CButton m_buttonLaserStatus;
	CButton m_butMagOn;

	BOOL    m_bResumeScan;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	BOOL	m_bPaused;
	BOOL    m_bAborted;
	BOOL    m_bScanning;
	BOOL	m_nScanType;
	BOOL	m_bStartScanAtHomePos;
	BOOL	m_bReturnToStart;
	BOOL	m_bSeekAndStartAtLine;
	BOOL	m_bSeekStartLineInReverse;
	BOOL	m_bSeekWithLaser;
	BOOL	m_bPredrive;
	BOOL	m_bSeekBlackLine;
	BOOL	m_bCalibrate;
	BOOL m_bEnableMAG;

//	CBrush	m_brRed;
//	CBrush	m_brGreen;
//	CBrush	m_brBlue;
//	CBrush	m_brMagenta;

	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	afx_msg LRESULT OnUserScanFinished(WPARAM, LPARAM);
	afx_msg LRESULT OnUserAbortFinished(WPARAM, LPARAM);
	afx_msg LRESULT OnUserStaticParameter(WPARAM, LPARAM);
	afx_msg LRESULT OnUserWeldNavigation(WPARAM, LPARAM);
	afx_msg LRESULT OnUserMotionControl(WPARAM, LPARAM);
	afx_msg LRESULT OnUserMagStopSeek(WPARAM, LPARAM);
	afx_msg LRESULT OnUserAreMotorsRunning(WPARAM, LPARAM);

	afx_msg void OnDeltaposSpinLrOffset(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinScanCirc(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinScanDist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinScanOverlap(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg void OnClickedButtonPause();
	afx_msg void OnClickedButtonScan();
	afx_msg void OnClickedButtonFwd();
	afx_msg void OnClickedButtonBack();
	afx_msg void OnPaint();

	afx_msg void OnClickedButtonZeroHome();
	afx_msg void OnClickedButtonGoHome();
	afx_msg void OnRadioScanType();
	afx_msg void OnClickedCheckGoToHome();
	afx_msg void OnClickedCheckReturnToStart();
	afx_msg void OnChangeEditLrOffset();
	afx_msg void OnClickedCheckSeekStartLine();
	afx_msg void OnDeltaposSpinScanSpeed(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinAccel2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReleasedcaptureSliderSteer(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDeltaposSpinPredrive(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinSeekStart(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickedCheckPredrive();
	afx_msg void OnClickedCheckCalibrate();
	afx_msg void OnClickedCheckEnableMag();
};
