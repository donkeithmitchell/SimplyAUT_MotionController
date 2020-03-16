#pragma once
#include "DialogLaser.h"
#include "SLSDef.h"

enum { STATUS_GET_CIRC = 0, STATUS_GETLOCATION };

// CStaticLaser dialog
class CLaserControl;
class CMagControl;
class CStaticLaser : public CWnd
{
	DECLARE_DYNAMIC(CStaticLaser)

public:
	CStaticLaser(CLaserControl&, CMagControl&);   // standard constructor
	virtual ~CStaticLaser();
	void Init(CWnd*, UINT);

	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);
	void DrawCrawlerLocation(CDC*);
	void SetCrawlerLocation(CPoint pt);
	double GetPipeCircumference();
	double GetCrawlerLocation();
	int GetPipeRect(CRect*);
	void GetLaserProfile();
	void  GetLaserRect(CRect*);

	enum{TIMER_GET_MEASUREMENT=0, TIMER_GET_TEMPERATURE};

	CWnd*	m_pParent;
	UINT	m_nMsg;
	double	m_fHomeAng;

	Profile			m_profile;
	Measurement		m_measure;
	int			m_profile_count;
	int         m_image_count;
	BOOL		m_valid_edges;
	BOOL		m_valid_joint_pos;

//	CStaticLaserProfile m_wndLaserProfile;
	CLaserControl& m_laserControl;
	CMagControl& m_magControl;
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
	afx_msg void OnSize(UINT nFlag, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};
