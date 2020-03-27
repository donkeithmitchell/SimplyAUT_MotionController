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
	CStaticLaser(CLaserControl&, CMagControl&, const Profile&, const LASER_MEASURES&, const double*);

	virtual ~CStaticLaser();
	void Init(CWnd*, UINT);

	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);
	void DrawRGBProfile(CDC*);
	void DrawCrawlerLocation(CDC*);
	void DrawLaserOffset(CDC*);
	void SetCrawlerLocation(CPoint pt);
	double GetPipeCircumference();
	double GetCrawlerLocation();
	int		GetMagStatus(int);
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
	void   GetLaserMeasurment(LASER_MEASURES* meas) {*meas = m_measure2;	}
	void   ResetLaserOffsetList() { m_laserPos.SetSize(0); }

	enum{TIMER_GET_MEASUREMENT=0, TIMER_GET_TEMPERATURE};

	CWnd*	m_pParent;
	UINT	m_nMsg;
	double	m_fHomeAng;

	const Profile&			m_profile;
	const LASER_MEASURES&   m_measure2;
	const double*	m_hitBuffer;
//	Measurement 	m_measure1;
//	int				m_profile_count;
//	int				m_image_count;
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
	CArray<CDoublePoint, CDoublePoint> m_laserPos;

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
