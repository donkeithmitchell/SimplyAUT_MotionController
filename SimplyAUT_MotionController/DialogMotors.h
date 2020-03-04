#pragma once


// CDialogMotors dialog

class CDialogMotors : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogMotors)

public:
	CDialogMotors(CWnd* pParent = nullptr);   // standard constructor
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

	CString m_szMotorA;
	CString m_szMotorB;
	CString m_szMotorC;
	CString m_szMotorD;

	BOOL	m_bInit;
	BOOL	m_bCheck;
};
