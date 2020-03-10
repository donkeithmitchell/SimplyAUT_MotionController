#pragma once


// CDialogMotors dialog

class CMotionControl;
class CDialogMotors : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogMotors)

public:
	CDialogMotors(CMotionControl&, const GALIL_STATE&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogMotors();

	virtual BOOL OnInitDialog();
	void Create(CWnd* pParent);
	void EnableControls();
	double GetMotorSpeed();
	double GetMotorAccel();
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
	afx_msg void OnDeltaposSpinScanSpeed(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinScanAccel(NMHDR* pNMHDR, LRESULT* pResult);
	void	EbableControls();
	void	Init(CWnd* pParent, UINT nMsg);

	const GALIL_STATE&	m_nGalilState;
	CMotionControl&		m_motionControl;

	CSpinButtonCtrl m_spinScanSpeed;
	CSpinButtonCtrl m_spinScanAccel;

	CString m_szMotorSpeed;
	CString m_szMotorAccel;

	double m_fMotorSpeed;
	double m_fMotorAccel;

	CString m_szMotorA;
	CString m_szMotorB;
	CString m_szMotorC;
	CString m_szMotorD;

	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
};
