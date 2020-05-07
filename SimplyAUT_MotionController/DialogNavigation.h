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
	void	OnButtonCalcPID(int);

protected:
	NAVIGATION_PID& m_pid;
	CStaticNavigation m_staticNavigation;
	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL m_bInit;
	BOOL m_bCheck;
	CSpinButtonCtrl m_spinTu;
	CSpinButtonCtrl m_spinPhz;
	enum{PID_P=1, PID_PI, PID_PD, PID_PID, PID_CRIT, PID_CALIB};

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	BOOL OnInitDialog();
	afx_msg void OnButtonReset();
	afx_msg void OnButtonCalcPI() { OnButtonCalcPID(PID_PI); }
	afx_msg void OnButtonCalcPD() { OnButtonCalcPID(PID_PD); }
	afx_msg void OnButtonCalcPID() { OnButtonCalcPID(PID_PID); }
	afx_msg void OnButtonCalcCrit() { OnButtonCalcPID(PID_CRIT); }
	afx_msg void OnButtonCalcEnable() { OnButtonCalcPID(PID_CALIB); }
	
	afx_msg void OnClickSimulate();
	afx_msg void OnEditChangePID();
	afx_msg void OnButtonSimulation();
	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	afx_msg void OnDeltaposSpinTu(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinPhz(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};


