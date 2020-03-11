#pragma once
#include "StaticLaser.h"
#include "button.h"

enum LASER_STATUS {
	TIMER_LASER_OFF = 0, TIMER_LASER_OK, TIMER_LASER_ERROR, TIMER_LASER_HOT,
	TIMER_LASER_SENDING_OK, TIMER_LASER_SENDING_ERROR, TIMER_LASER_LOADING, TIMER_SHOW_MOTOR_SPEEDS,
	/*TIMER_STEER_LEFT, TIMER_STEER_RIGHT*/
};



// CDialogGirthWeld dialog

class CMotionControl;
class CLaserControl;
class CDialogGirthWeld : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogGirthWeld)

public:
	CDialogGirthWeld(CMotionControl&, CLaserControl&, GALIL_STATE& nState, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogGirthWeld();

	virtual BOOL OnInitDialog();
	void	Create(CWnd* pParent);
	void	SetButtonBitmaps();
	BOOL	CheckIfToRunOrStop(GALIL_STATE);
	BOOL	CheckParameters();
	void    SetLaserStatus(LASER_STATUS nStatus);
	void	EnableControls();
	void    RunMotors();
	double  GetMotorSpeed(double& accel);
	void	Init(CWnd* pParent, UINT nMsg);
	void	ShowMotorSpeeds();
	void    ShowMotorPosition();
	BOOL	CheckVisibleTab();
	LRESULT UserSteer(BOOL bRight, BOOL bDown);
	UINT    ThreadStopMotors(void);
	UINT	ThreadGoToHome(void);
	UINT	ThreadRunManual(BOOL);

	enum { WM_STEER_LEFT = WM_USER + 1, WM_STEER_RIGHT, WM_MOTORSSTOPPED	};

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
	afx_msg LRESULT OnUserMotorsStopped(WPARAM, LPARAM);

	GALIL_STATE&	m_nGalilState;
	CMotionControl& m_motionControl;
	CLaserControl&	m_laserControl;
	HANDLE			m_hThreadRunMotors;
	GALIL_STATE		m_nGaililStateBackup;

	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	BOOL	m_bPaused;
	int     m_nTimerCount;
	double  m_fMotorSpeed;
	double  m_fMotorAccel;

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
	CString m_szDistScanned;
	CString m_szScanOverlap;

	CString   m_szLaserHiLowUS;
	CString   m_szLaserHiLowDS;
	CString   m_szLaserHiLowDiff;
	CString   m_szLaserCapHeight;

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
	CString m_szHomeDist;

	CBrush	m_brRed;
	CBrush	m_brGreen;
	CBrush	m_brBlue;
	CBrush	m_brMagenta;

	afx_msg void OnDeltaposSpinLrOffset(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinScanCirc(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinScanDist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinScanOverlap(NMHDR* pNMHDR, LRESULT* pResult);
//	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
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
};
