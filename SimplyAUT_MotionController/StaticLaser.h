#pragma once


// CStaticLaser dialog

class CStaticLaser : public CWnd
{
	DECLARE_DYNAMIC(CStaticLaser)

public:
	CStaticLaser();   // standard constructor
	virtual ~CStaticLaser();
	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);
	void DrawCrawlerLocation(CDC*);

	CWnd* m_pParent;

// Dialog Data
#ifdef AFX_DESIGN_TIME
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};
