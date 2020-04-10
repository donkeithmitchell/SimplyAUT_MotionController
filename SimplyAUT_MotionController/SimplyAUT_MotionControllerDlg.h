
// SimplyAUT_MotionControllerDlg.h : header file
//

#pragma once
#include "CDialogConnect.h"
#include "DialogMotors.h"
#include "DialogGirthWeld.h"
#include "DialogStatus.h"
#include "MotionControl.h"
#include "LaserControl.h"
#include "MagController.h"
#include "DialogLaser.h"
#include "DialogMag.h"
#include "DialogFiles.h"


// CSimplyAUTMotionControllerDlg dialog
class CSimplyAUTMotionControllerDlg : public CDialogEx
{
// Construction
public:
	CSimplyAUTMotionControllerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SIMPLYAUT_MOTIONCONTROLLER_DIALOG };
#endif
	enum{TAB_CONNECT=0, TAB_MOTORS, TAB_LASER, TAB_MAG, TAB_SCAN, TAB_FILES, TAB_STATUS};
	enum {MSG_SEND_DEBUGMSG = 0, MSG_ERROR_MSG, MSG_SETBITMAPS, MSG_GETSCANSPEED, MSG_GETACCEL, MSG_SHOW_MOTOR_SPEEDS, 
		MSG_MAG_STATUS_ON, MSG_UPDATE_FILE_LIST	};


	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	void OnOK();
	void OnCancel();
	void OnSelchangeTab2();
	BOOL CheckVisibleTab();
	void AppendErrorMessage(const CString& szMsg);
	void StartReadMagStatus(BOOL);
	void Serialize(BOOL bSave);
	void Serialize(CArchive& ar);

	enum { WM_DEGUG_MSG = WM_USER + 1 };
	enum{ TIMER_GET_MAG_STATUS =1 };

// Implementation
protected:
	HICON				m_hIcon;
	GALIL_STATE			m_galil_state;
	CMotionControl		m_motionControl;
	CLaserControl       m_laserControl;
	CMagControl			m_magControl;

	CDialogConnect		m_dlgConnect;
	CDialogMotors		m_dlgMotors;
	CDialogGirthWeld	m_dlgGirthWeld;
	CDialogLaser		m_dlgLaser;
	CDialogFiles        m_dlgFiles;
	CDialogStatus		m_dlgStatus;
	CDialogMag          m_dlgMag;

	BOOL	m_bInit;
	BOOL	m_bCheck;
	int		m_nSel;


	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT, int cx, int xy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnUserDebugMessage(WPARAM, LPARAM);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
public:
	CTabCtrl m_tabControl;
	CString m_szErrorMsg;
	afx_msg void OnClickedButtonResetStatus();
};
