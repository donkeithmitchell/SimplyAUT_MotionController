#pragma once


// CDialogNavigation

struct NAVIGATION_PID;

class CStaticNavigation : public CWnd
{
	DECLARE_DYNAMIC(CStaticNavigation)

public:
	CStaticNavigation(const NAVIGATION_PID& pid, const CArray<double,double>&);

	virtual ~CStaticNavigation();

	void Create(CWnd* pParent);
	void DrawNavigationProfile(CDC*);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nFlag, int cx, int cy);

	const NAVIGATION_PID& m_pid;
	const CArray<double, double>& m_fft_data;

	DECLARE_MESSAGE_MAP()
};


class CDialogNavigation : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogNavigation)

public:
	CDialogNavigation(NAVIGATION_PID& pid, CArray<double,double>& fft_data, CWnd* pParent=NULL);
	virtual ~CDialogNavigation();

	void	Create(CWnd* pParent);
	BOOL	CheckVisibleTab();
	void	EnableControls();
	void	Init(CWnd* pParent, UINT nMsg);
	void	Serialize(CArchive& ar);

protected:
	NAVIGATION_PID& m_pid;
	CStaticNavigation m_staticNavigation;
	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL m_bInit;
	BOOL m_bCheck;
	CSpinButtonCtrl m_spinTu;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	BOOL OnInitDialog();
	afx_msg void OnButtonReset();
	afx_msg void OnButtonCalcPI();
	afx_msg void OnButtonCalcPID();
	afx_msg void OnClickNavType();
	afx_msg void OnEditChangePID();
	afx_msg void OnButtonCalcEnable();
	afx_msg void OnButtonSimulation();
	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	afx_msg void OnDeltaposSpinTu(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};


