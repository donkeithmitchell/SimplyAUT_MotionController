#pragma once


// CDialogStatus dialog

class CDialogStatus : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogStatus)

public:
	CDialogStatus(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogStatus();

	void Create(CWnd*);
	void AppendDebugMessage(const CString&);
	void EnableControls();
	void Init(CWnd* pParent, UINT nMsg);
	BOOL CheckVisibleTab() { return TRUE; }
	void ResetStatus();

	afx_msg void OnSize(UINT nFlag, int cx, int cy);

	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	CString m_szStatus;

		// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_STATUS };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
