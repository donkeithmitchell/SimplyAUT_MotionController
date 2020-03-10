#pragma once


// CStaticLaser dialog
class CLaserControl;
class CStaticLaser : public CWnd
{
	DECLARE_DYNAMIC(CStaticLaser)

public:
	CStaticLaser(CLaserControl&);   // standard constructor
	virtual ~CStaticLaser();
	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);
	void DrawCrawlerLocation(CDC*);

	enum{TIMER_GET_MEASUREMENT=0, TIMER_GET_TEMPERATURE};

	CWnd* m_pParent;
	CLaserControl& m_laserControl;

// Dialog Data
#ifdef AFX_DESIGN_TIME
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()
};
