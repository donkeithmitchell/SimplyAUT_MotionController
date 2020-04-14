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
class CMotionControl;
class CWeldNavigation;

class CStaticLaser : public CWnd
{
	DECLARE_DYNAMIC(CStaticLaser)

public:
	CStaticLaser(CMotionControl&, CLaserControl&, CMagControl&, CWeldNavigation&, const double&);

	virtual ~CStaticLaser();

	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);
	void DrawRGBProfile(CDC*);
	void DrawCrawlerLocation(CDC*);
	void ToggleLaserOn();
	void DrawLaserOffset(CDC*);
	void SetCrawlerLocation(CPoint pt);
	int 	GetPipeRect(CRect*);
	void	GetOffsetRect(CRect*);
	void	GetLaserProfile();
	void	GetLaserRect(CRect*);
	void  GetRGBRect(CRect*);
	CDoublePoint GetJointPos() { return m_joint_pos; }
	CDoublePoint GetEdgePos(int ind) { return m_edge_pos[ind]; }
	CPoint GetScreenPixel(double x, double y);
	int    AddRGBData(const int&);
	void   ResetRGBData();
	double GetAverageRGBValue();
	void   ResetLaserOffsetList() {	m_critLaserPos1.Lock();  m_laserPos1.SetSize(0); m_critLaserPos1.Unlock(); 	}

	enum{TIMER_GET_MEASUREMENT=0, TIMER_GET_TEMPERATURE};

	double	m_fHomeAng;
	const double&	m_fScanLength;

	CDoublePoint    m_joint_pos;
	CDoublePoint    m_edge_pos[3];
	double			m_disp_width_factor;
	double			m_disp_height_factor;
	double          m_disp_height_min;
	double          m_disp_height_max;
	CRect			m_disp_rect;
	CArray<int, int>  m_rgbData;
	int				m_rgbSum; // use to calculate the average
	int				m_rgbCount;

	CCriticalSection m_critLaserPos1;
	CArray<CDoublePoint, CDoublePoint> m_laserPos1;

//	CStaticLaserProfile m_wndLaserProfile;
	CLaserControl& m_laserControl;
	CMagControl& m_magControl;
	CMotionControl& m_motionControl;
	CWeldNavigation& m_weldNavigation;

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
