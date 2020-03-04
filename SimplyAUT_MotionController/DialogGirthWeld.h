#pragma once
#include "StaticLaser.h"


// CDialogGirthWeld dialog

enum GALIL_STATE {GALIL_IDLE=0, GALIL_CALIB, GALIL_MANUAL, GALIL_AUTO, GALIL_FWD, GALIL_BACK, GALIL_GOHOME};

class CDialogGirthWeld : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogGirthWeld)

public:
	CDialogGirthWeld(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogGirthWeld();

	virtual BOOL OnInitDialog();
	void	Create(CWnd* pParent);
	void	SetButtonBitmaps();
	BOOL	CheckIfToRunOrStop(GALIL_STATE);
	BOOL	CheckParameters();
	
	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_GIRTHWELD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSize(UINT nFlag, int cx, int cy);

	BOOL	m_bInit;
	BOOL	m_bCheck;
	BOOL	m_bPaused;
	GALIL_STATE m_galil_state;

	CBitmap	m_bitmapPause;
	CBitmap	m_bitmapGoHorz;
	CBitmap	m_bitmapGoVert;
	CBitmap	m_bitmapStop;

	CStatic		    m_staticLaser;
	CStaticLaser	m_wndLaser;

	CSpinButtonCtrl m_spinSpeed;
	CSpinButtonCtrl m_spinLROffset;
	CSpinButtonCtrl m_spinCirc;

	CString m_szSpeed;
	CString m_szLROffset;
	CString m_szCirc;

	double m_fSpeed;
	double m_fLROffset;
	double m_fCirc;

	afx_msg void OnDeltaposSpinSpeed(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinLrOffset(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinCirc(NMHDR* pNMHDR, LRESULT* pResult);

	CButton m_buttonPause;
	CButton m_buttonCalib;
	CButton m_buttonManual;
	CButton m_buttonAuto;
	CButton m_buttonSetHome;
	CButton m_buttonGoHome;

	afx_msg void OnClickedButtonPause();
	afx_msg void OnClickedButtonCalib();
	afx_msg void OnClickedButtonManual();
	afx_msg void OnClickedButtonAuto();
	afx_msg void OnClickedButtonFwd();
	afx_msg void OnClickedButtonBack();
	afx_msg void OnPaint();

	CButton m_buttonBack;
	CButton m_buttonFwd;
	afx_msg void OnClickedButtonSetHome();
	afx_msg void OnClickedButtonGoHome();
};
