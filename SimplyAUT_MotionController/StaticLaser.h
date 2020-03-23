#pragma once
#include "DialogLaser.h"
#include "SLSDef.h"
#include "LaserControl.h"
#include "Misc.h"
/*
struct RGB_DATA
{
	RGB_DATA() { memset(this, 0x0, sizeof(RGB_DATA)); }
	int red;
	int green;
	int blue;
	int sum;
};
*/

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
	void DrawRGBProfile(CDC*);
	void DrawCrawlerLocation(CDC*);
	void SetCrawlerLocation(CPoint pt);
	double GetPipeCircumference();
	double GetCrawlerLocation();
	int   GetMagStatus(int);
	int GetPipeRect(CRect*);
	void GetLaserProfile();
	void  GetLaserRect(CRect*);
	void  GetRGBRect(CRect*);
	CDoublePoint GetJointPos() { return m_joint_pos; }
	CDoublePoint GetEdgePos(int ind) { return m_edge_pos[ind]; }
	CPoint GetScreenPixel(double x, double y);
	int    AddRGBData(const int&);
	void   ResetRGBData();
	double GetAverageRGBValue();

	enum{TIMER_GET_MEASUREMENT=0, TIMER_GET_TEMPERATURE};

	CWnd*	m_pParent;
	UINT	m_nMsg;
	double	m_fHomeAng;

	Profile			m_profile;
	Measurement 	m_measure;
	int				m_profile_count;
	int				m_image_count;
	CDoublePoint    m_joint_pos;
	CDoublePoint    m_edge_pos[3];
	double			m_disp_width_factor;
	double			m_disp_height_factor;
	CRect			m_disp_rect;
	CRect			m_roi_rect;
	CArray<int, int>  m_rgbData;
	int				m_rgbSum; // use to calculate the average
	int				m_rgbCount;

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
