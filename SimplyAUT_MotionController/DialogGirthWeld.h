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
class CDialogGirthWeld : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogGirthWeld)

public:
	CDialogGirthWeld(CMotionControl&, CLaserControl&, CMagControl&, GALIL_STATE& nState, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogGirthWeld();

	virtual BOOL OnInitDialog();
	void	Create(CWnd* pParent);
	void	SetButtonBitmaps();
	BOOL	CheckIfToRunOrStop(GALIL_STATE);
	BOOL	CheckParameters();
//	void    SetLaserStatus(LASER_STATUS nStatus);
	void	EnableControls();
	void    RunMotors();
	double  GetRequestedMotorSpeed(double& accel);
	void	Init(CWnd* pParent, UINT nMsg);
	void	ShowMotorSpeeds();
	void    ShowMotorPosition();
	void    ShowRGBStatus();
	BOOL	CheckVisibleTab();
	LRESULT UserSteer(BOOL bRight, BOOL bDown);
	UINT    ThreadStopMotors(void);
	UINT	ThreadGoToHome(void);
	UINT	ThreadRunScan();
	void	SendDebugMessage(const CString&);
	void	SendErrorMessage(const CString&);
	void    ShowLaserTemperature();
	void    ShowLaserStatus();
	void	SetNoteRunTime(BOOL);
//	void    StartNotingRGBData(BOOL);
	void	StartNotingMotorSpeed(BOOL);
	void    StartSteeringMotors(BOOL);
	void    StartReadMagStatus(BOOL);
	void    StartMeasuringLaser(BOOL);
	void    GetLaserProfile();
	void	NoteIfMotorsRunning();
	BOOL	SetSlewSpeed(double fSpeed);
	double  GetSlewSpeed(const char* axis);
	void    StopMotors();
	double  GetRGBSum();
	BOOL    GoToPosition(double pos);
	double	GetAvgMotorPosition();
	BOOL    ZeroPositions();
	BOOL    WaitForMotorsToStop();
	BOOL    WaitForMotorsToStart();
	double  GetLeftRightOffset()const;
	void    FormatLeftRightOffset(double);
	void    NoteRunTime();
	void    NoteRGBSum();
	double	GetDestinationPosition();
	void    StartRecording(int);
	void	NoteRGBCalibration();
	double	GetCalibrationValue();

	enum { WM_STEER_LEFT = WM_USER + 1, WM_STEER_RIGHT, WM_STOPMOTOR_FINISHED, WM_USER_STATIC, WM_WELD_NAVIGATION, WM_MOTION_CONTROL};
	enum { MC_SET_SLEW_SPEED = 0, MC_GOTO_POSITION, MC_ZERO_POSITION, MC_GET_SLEW_SPEED, MC_GET_RGB_SUM, MC_STOP_MOTORS, MC_GET_AVG_POS };
	enum TIMER_GW { TIMER_SHOW_MOTOR_SPEEDS = 0, TIMER_LASER_TEMPERATURE, TIMER_LASER_STATUS1, TIMER_RGB_STATUS, TIMER_RUN_TIME, 
		TIMER_NOTE_RGB, TIMER_GET_LASER_PROFILE, TIMER_ARE_MOTORS_RUNNING, TIMER_NOTE_CALIBRATION	};
	enum { STATUS_GETLOCATION=0, STATUS_SHOWLASERSTATUS};
	enum{ NAVIGATE_SEND_DEBUG_MSG=0, NAVIGATE_SET_MOTOR_SPEED};

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_GIRTHWELD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	afx_msg LRESULT OnUserSteerLeft(WPARAM, LPARAM);
	afx_msg LRESULT OnUserSteerRight(WPARAM, LPARAM);
	afx_msg LRESULT OnUserStopMotorFinished(WPARAM, LPARAM);
	afx_msg LRESULT OnUserStaticParameter(WPARAM, LPARAM);
	afx_msg LRESULT OnUserWeldNavigation(WPARAM, LPARAM);
	afx_msg LRESULT OnUserMotionControl(WPARAM, LPARAM);

	GALIL_STATE&	m_nGalilState;
	CMotionControl& m_motionControl;
	CLaserControl&	m_laserControl;
	CMagControl&	m_magControl;
	HANDLE			m_hThreadRunMotors;
	GALIL_STATE		m_nGaililStateBackup;
	CWeldNavigation m_weldNavigation;

	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	BOOL	m_bPaused;
	BOOL    m_bAborted;
	BOOL    m_bScanning;
	int     m_nCalibratingRGB;
	BOOL    m_bResumeScan;
	int     m_nTimerCount;
	double  m_fMotorSpeed;
	double	m_fDestinationPosition;
	double  m_fMotorAccel;
	clock_t	m_nRunStart;
	int     m_rgbLast;

	CBitmap	m_bitmapPause;
	CBitmap	m_bitmapGoRight;
	CBitmap	m_bitmapGoLeft;
	CBitmap	m_bitmapGoDown;
	CBitmap	m_bitmapGoUp;
	CBitmap	m_bitmapStop;

	CBitmap	m_bitmapLaserOff;		// blank
	CBitmap	m_bitmapLaserOK;		// Green
	CBitmap	m_bitmapLaserError;		// red
	CBitmap	m_bitmapLaserHot;		// magenta
	CBitmap	m_bitmapLaserLoading;	// blue

	CStatic		    m_staticLaser;
	CStatic		    m_staticMag;
	CStaticLaser	m_wndLaser;
	CStaticMag		m_wndMag;


	BOOL			m_nScanType;
	CSpinButtonCtrl m_spinLROffset;
	CSpinButtonCtrl m_spinScanCirc;
	CSpinButtonCtrl m_spinScanDist;
	CSpinButtonCtrl m_spinScanOverlap;

	CString m_szLROffset;
	CString m_szScanCirc;
	CString m_szDistToScan;
	CString m_szScannedDist;

	double  m_fScanStartPos;
	CString m_szHomeDist;
	CString m_szScanOverlap;

	CString   m_szLaserEdge[3];
	CString   m_szLaserJoint;

	double m_fScanCirc;
	double m_fDistToScan;
	double m_fDistScanned;
	double m_fScanOverlap;
	double m_fScanLength;

	CButton m_buttonPause;
	CButton m_buttonManual;
	CButton m_buttonZeroHome;
	CButton m_buttonGoHome;
	CButton m_buttonBack;
	CMyButton m_buttonLeft;
	CMyButton m_buttonRight;
	CButton m_buttonFwd;
	CButton m_buttonLaserStatus;

	BOOL m_bStartScanAtHomePos;
	BOOL m_bReturnToStart;
	BOOL m_bSeekAndStartAtLine;
	BOOL m_bSeekStartLineInReverse;
	double m_fSeekAndStartAtLine;

	CBrush	m_brRed;
	CBrush	m_brGreen;
	CBrush	m_brBlue;
	CBrush	m_brMagenta;

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
	afx_msg void OnClickedButtonLeft();
	afx_msg void OnClickedButtonRight();
	afx_msg void OnPaint();

	afx_msg void OnClickedButtonZeroHome();
	afx_msg void OnClickedButtonGoHome();
	afx_msg void OnRadioScanType();
	afx_msg void OnClickedCheckGoToHome();
	afx_msg void OnClickedCheckReturnToStart();
	afx_msg void OnStnClickedStaticTempBoard();
	afx_msg void OnChangeEditLrOffset();
	afx_msg void OnClickedCheckSeekStartLine();

	CString m_szTempBoard;
	CString m_szTempLaser;
	CString m_szTempSensor;
	CString m_szRunTime;
	CString m_szRGBValue;
	CString m_szRGBLinePresent;
	CString m_szRGBCalibration;
};
