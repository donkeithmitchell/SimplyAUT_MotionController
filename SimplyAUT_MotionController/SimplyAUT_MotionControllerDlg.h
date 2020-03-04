
// SimplyAUT_MotionControllerDlg.h : header file
//

#pragma once
#include "CDialogConnect.h"
#include "DialogMotors.h"
#include "DialogGirthWeld.h"
#include "DialogStatus.h"


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

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void OnSelchangeTab2();


// Implementation
protected:
	HICON m_hIcon;
	CDialogConnect m_dlgConnect;
	CDialogMotors  m_dlgMotors;
	CDialogGirthWeld  m_dlgGirthWeld;
	CDialogStatus  m_dlgStatus;

	BOOL m_bInit;
	BOOL m_bCheck;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT, int cx, int xy);
	DECLARE_MESSAGE_MAP()
public:
	CTabCtrl m_tabControl;
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
};
