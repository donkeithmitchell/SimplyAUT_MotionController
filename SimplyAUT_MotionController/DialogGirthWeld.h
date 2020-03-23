#pragma once
#include "StaticLaser.h"
#include "WeldNavigation.h"
#include "SimplyAUT_MotionController.h"
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
	BOOL	CheckVisibleTab();
	LRESULT UserSteer(BOOL bRight, BOOL bDown);
	UINT    ThreadStopMotors(void);
	UINT	ThreadGoToHome(void);
	UINT	ThreadRunManual(BOOL);
	void	SendDebugMessage(const CString&);
	double  GetMaximumMotorPosition();
	void    ShowLaserTemperature();
	void    ShowLaserStatus();
	int		GetMagStatus(int nStat);
	void	SetRunTime(int);
	void    NoteSteering();
	void    StartNotingRGBData(BOOL);
	void	StartNotingMotorSpeed(BOOL);
	void    StartSteeringMotors(BOOL);

	enum { WM_STEER_LEFT = WM_USER + 1, WM_STEER_RIGHT, WM_STOPMOTOR_FINISHED, WM_USER_STATIC, WM_WELD_NAVIGATION};
	enum { TIMER_SHOW_MOTOR_SPEEDS = 0, TIMER_LASER_STATUS, TIMER_RUN_TIME, TIMER_NOTE_RGB, TIMER_NOTE_STEERING};
	enum { STATUS_GET_CIRC = 0, STATUS_GETLOCATION, STATUS_SHOWLASERSTATUS, STATUS_MAG_STATUS};
	enum{ NAVIGATE_GET_MEASURE=0, NAVIGATE_SEND_DEBUG_MSG};

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

	GALIL_STATE&	m_nGalilState;
	CMotionControl& m_motionControl;
	CLaserControl&	m_laserControl;
	CMagControl& m_magControl;
	HANDLE			m_hThreadRunMotors;
	GALIL_STATE		m_nGaililStateBackup;
	CWeldNavigation m_weldNavigation;

	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	BOOL	m_bPaused;
	BOOL    m_bResumed;
	int     m_nTimerCount;
	double  m_fMotorSpeed;
	double  m_fMotorAccel;
	clock_t	m_nRunStart;

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
	CStaticLaser	m_wndLaser;

	BOOL			m_nScanType;
	CSpinButtonCtrl m_spinLROffset;
	CSpinButtonCtrl m_spinScanCirc;
	CSpinButtonCtrl m_spinScanDist;
	CSpinButtonCtrl m_spinScanOverlap;

	CString m_szLROffset;
	CString m_szScanCirc;
	CString m_szDistToScan;
	CString m_szScannedDist;

	double  m_fScanStart;
	CString m_szHomeDist;
	CString m_szScanOverlap;

	CString   m_szLaserEdge[3];
	CString   m_szLaserJoint;

	double m_fLROffset;
	double m_fScanCirc;
	double m_fDistToScan;
	double m_fDistScanned;
	double m_fScanOverlap;

	CButton m_buttonPause;
	CButton m_buttonCalib;
	CButton m_buttonManual;
	CButton m_buttonAuto;
	CButton m_buttonZeroHome;
	CButton m_buttonGoHome;
	CButton m_buttonBack;
	CMyButton m_buttonLeft;
	CMyButton m_buttonRight;
	CButton m_buttonFwd;
	CButton m_buttonLaserStatus;

	BOOL m_bReturnToHome;
	BOOL m_bReturnToStart;

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
	afx_msg void OnClickedButtonCalib();
	afx_msg void OnClickedButtonManual();
	afx_msg void OnClickedButtonAuto();
	afx_msg void OnClickedButtonFwd();
	afx_msg void OnClickedButtonBack();
	afx_msg void OnClickedButtonLeft();
	afx_msg void OnClickedButtonRight();
	afx_msg void OnPaint();

	afx_msg void OnClickedButtonZeroHome();
	afx_msg void OnClickedButtonGoHome();
	afx_msg void OnRadioScanType();
	afx_msg void OnClickedCheckGoToHome();
	afx_msg void OnClickedCheckGoToStart();
	BOOL m_bSeekReverse;
	CString m_szTempBoard;
	CString m_szTempLaser;
	CString m_szTempSensor;
	CString m_szRunTime;
	CString m_szSteeringGapDist;
	CString m_szSteeringGapVel;
	CString m_szSteeringLRDiff;
	CString m_szSteeringGapAccel;
};
