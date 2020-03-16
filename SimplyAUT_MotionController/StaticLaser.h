#pragma once

enum { STATUS_GET_CIRC = 0, STATUS_GETLOCATION };

// CStaticLaser dialog
class CLaserControl;
class CStaticLaser : public CWnd
{
	DECLARE_DYNAMIC(CStaticLaser)

public:
	CStaticLaser(CLaserControl&);   // standard constructor
	virtual ~CStaticLaser();
	void Init(CWnd*, UINT);

	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);
	void DrawCrawlerLocation(CDC*);
	void SetCrawlerLocation(CPoint pt);
	double GetPipeCircumference();
	double GetCrawlerLocation();
	int GetPipeRect(CRect*);

	enum{TIMER_GET_MEASUREMENT=0, TIMER_GET_TEMPERATURE};

	CWnd*	m_pParent;
	UINT	m_nMsg;
	double	m_fHomeAng;

	CLaserControl& m_laserControl;
	CPoint m_ptMouse;

// Dialog Data
#ifdef AFX_DESIGN_TIME
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint pt);
	afx_msg void OnMenu(UINT nID);

	DECLARE_MESSAGE_MAP()
};
