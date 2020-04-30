#pragma once
#include "SLSDef.h"
#include "LaserControl.h"

// CDialogLaser dialog
class CMotionControl;
class CLaserControl;
struct LASER_MEASURES;


// thjese are constants so do not need to be in the clasas
// they will not vary from instanc ew to instance


class CStaticLaserProfile : public CWnd
{
	DECLARE_DYNAMIC(CStaticLaserProfile)

public:
	CStaticLaserProfile(CLaserControl&, const BOOL& rShift, const BOOL& rRaw, COLORREF bgColour);   // standard constructor
	virtual ~CStaticLaserProfile();
	void Init(CWnd*, UINT);

	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);
	void OnRoiButton();
	CPoint GetDataPoint(CPoint);
	CPoint GetScreenPixel(double x, double y);
	void	ResetROI();


	CWnd* m_pParent;
	UINT	m_nMsg;
	const BOOL&		m_rShiftToCentre;
	const BOOL&		m_rShowRawData;

	CLaserControl& m_laserControl;
//	CString		m_jointPos_str;
//	CString		m_gapVal_str;
//	CString		m_edgesPos_str;
//	CString		m_mismVal_str;
	int			m_profile_count;
//	BOOL		m_valid_edges;
//	BOOL		m_valid_joint_pos;
	COLORREF    m_bgColour;
	CString		m_ROI_str;
	int			m_ROI_stage;
	double		m_disp_width_factor;
	double		m_disp_height_factor;
	int			m_disp_height;

	// Dialog Data
#ifdef AFX_DESIGN_TIME
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nFlag, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};