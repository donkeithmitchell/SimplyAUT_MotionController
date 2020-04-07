#pragma once
#include "StaticMag.h"

// CDialogMag dialog
class CMotionControl;
class CLaserControl;
class CMagControl;

class CDialogMag : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogMag)

public:
	CDialogMag(CMotionControl&, CLaserControl&, CMagControl&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogMag();

	void	Create(CWnd*);
	void	EnableControls();
	void	Init(CWnd* pParent, UINT nMsg);
	BOOL	CheckVisibleTab() { return TRUE; }
	double	GetRequestedMotorSpeed(double& rAccel);
	void	StartReadMagStatus(BOOL bSet);
	UINT	ThreadWaitCalibration();
	double	GetCalibrationValue();


	HICON	m_hIcon;
	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	int 	m_nCalibrating;
	HANDLE	m_hThreadWaitCalibration;

	CBitmap	m_bitmapDisconnect;
	CBitmap	m_bitmapConnect;

	CMotionControl& m_motionControl;
	CLaserControl&	m_laserControl;
	CMagControl&	m_magControl;
	CStaticMag		m_wndMag;

	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	//	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnUserUpdateDialog(WPARAM, LPARAM);
	afx_msg LRESULT OnUserWaitCalibrationFinished(WPARAM, LPARAM);
	afx_msg LRESULT OnUserAreMotorsStopped(WPARAM, LPARAM);

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MAG };
#endif
	enum { WM_USER_UPDATE_DIALOG = WM_USER + 1 };
	enum{ TIMER_NOTE_VALUES=1, TIMER_NOTE_CALIBRATION};
	enum{ WM_ABORT_CALIBRATION_FINISHED = WM_USER+1, WM_WAIT_CALIBRATION_FINISHED, WM_ARE_MOTORS_STOPPED	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedButtonSetCalValue();
	afx_msg void OnClickedCheckMagEnable();
	afx_msg void OnClickedButtonEncoderClear();
	afx_msg void OnClickedButtonRgbCalibration();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	CString m_szMagVersion;
	CButton m_butMagEnable;
	CButton m_butMagOn;
	CString m_szRGBValue;
	CString m_szRGBCalValue;
	CString m_szRGBCalValueSet;
	CString m_szRGBLinePresent;
	CString m_szRGBEncCount;
	CStatic m_staticRGB;
	double	m_fCalibrationLength;
	CString m_szCalibrationSpeed;
	BOOL	m_bCalibrateWithLaser;
	afx_msg void OnClickedCheckUseLaser();
	BOOL m_bCalibrateReturnToStart;
	afx_msg void OnClickedCheckReturnToStart();
};
