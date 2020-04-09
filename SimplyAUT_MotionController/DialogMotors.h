#pragma once
#include "SimplyAUT_MotionController.h"


// CDialogMotors dialog

class CMotionControl;
class CMagControl;
class CDialogMotors : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogMotors)

public:
	CDialogMotors(CMotionControl&, CMagControl& mag, const GALIL_STATE&, CWnd* pParent = nullptr);   // standard constructor
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
	void	Init(CWnd* pParent, UINT nMsg);

	const GALIL_STATE&	m_nGalilState;
	CMotionControl&		m_motionControl;
	CMagControl&		m_magControl;

	CString m_szMotorA;
	CString m_szMotorB;
	CString m_szMotorC;
	CString m_szMotorD;

	UINT	m_nMsg;
	CWnd*	m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
};
