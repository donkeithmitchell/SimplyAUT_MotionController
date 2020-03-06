#pragma once


// CDialogMotors dialog

class CDialogMotors : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogMotors)

public:
	CDialogMotors(const GALIL_STATE&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogMotors();

	virtual BOOL OnInitDialog();
	void Create(CWnd* pParent);

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
	void EbableControls();

	const GALIL_STATE& m_nGalilState;

	CSpinButtonCtrl m_spinScanSpeed;
	CSpinButtonCtrl m_spinScanAccel;

	CString m_szScanSpeed;
	CString m_szScanAccel;

	double m_fScanSpeed;
	double m_fScanAccel;

	CString m_szMotorA;
	CString m_szMotorB;
	CString m_szMotorC;
	CString m_szMotorD;

	BOOL	m_bInit;
	BOOL	m_bCheck;
};
