#pragma once
#include "SimplyAUT_MotionController.h"


// CDialogMotors dialog

class CMotionControl;
class CMagControl;
class CDialogMotors : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogMotors)

public:
	CDialogMotors(CMotionControl&, CMagControl& mag, const int&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogMotors();

	virtual BOOL OnInitDialog();
	void Create(CWnd* pParent);
	void EnableControls();
	void ShowMotorSpeeds();
	BOOL CheckVisibleTab();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MOTORS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	void	Init(CWnd* pParent, UINT nMsg);

	const int&	m_nGalilState;
	CMotionControl&		m_motionControl;
	CMagControl&		m_magControl;

	CString m_szMotorA1;
	CString m_szMotorB1;
	CString m_szMotorC1;
	CString m_szMotorD1;

	CString m_szMotorA2;
	CString m_szMotorB2;
	CString m_szMotorC2;
	CString m_szMotorD2;

	UINT	m_nMsg;
	CWnd*	m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
};
