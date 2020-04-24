#pragma once


// CDialogNavigation

struct NAVIGATION_PID;

class CDialogNavigation : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogNavigation)

public:
	CDialogNavigation(NAVIGATION_PID& pid, CWnd* pParent=NULL);
	virtual ~CDialogNavigation();

	void	Create(CWnd* pParent);
	BOOL	CheckVisibleTab();
	void	EnableControls();
	void	Init(CWnd* pParent, UINT nMsg);
	void	Serialize(CArchive& ar) {}

protected:
	NAVIGATION_PID& m_pid;
	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL m_bInit;
	BOOL m_bCheck;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnButtonReset();
	afx_msg void OnClickNavType();
	DECLARE_MESSAGE_MAP()
};


