#pragma once
#include "LaserProfile.h"
#include "SLSDef.h"


// CDialogLaser dialog
class CMotionControl;
class CLaserControl;

class CDialogLaser : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogLaser)

public:
	CDialogLaser(CMotionControl&, CLaserControl&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogLaser();

	void Create(CWnd*);
	void EnableControls();
	void Init(CWnd* pParent, UINT nMsg);
	BOOL CheckVisibleTab() { return TRUE; }
	void	Serialize(CArchive& ar);
	void    ResetParameters();

	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	CStaticLaserProfile m_wndProfile;

	BOOL	m_bAutoLaser;
	CSliderCtrl	m_laser_power_sld;
	CSliderCtrl	m_camera_shutter_sld;
	CEdit	m_calibration_version;
	CEdit	m_sensor_version;
	CEdit	m_temperature_edit;
	int		m_LaserPower;
	int		m_CameraShutter;
	HICON	m_hIcon;

	CMotionControl& m_motionControl;
	CLaserControl& m_laserControl;

	afx_msg void OnSize(UINT nFlag, int cx, int cy);
//	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLaserButton();
	afx_msg void OnAutolaserCheck();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnReleasedcaptureLaserpowerSlider(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReleasedcaptureShutterSlider(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRoiButton();
	afx_msg LRESULT OnUserUpdateDialog(WPARAM, LPARAM);


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_LASER };
#endif
	enum { WM_USER_UPDATE_DIALOG = WM_USER + 1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedButtonRoiReset();
	BOOL m_bShiftToCentre;
	afx_msg void OnClickedCheckShiftToCentre();
	BOOL m_bShowRawData;
	afx_msg void OnClickedCheckShowRawData();
};
