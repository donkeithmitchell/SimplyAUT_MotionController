#pragma once
#include "SLSDef.h"

// CDialogLaser dialog
class CMotionControl;
class CLaserControl;

#define FRAME_HEIGHT	1280
#define FRAME_WIDTH		1024
#define DISP_MARGIN_HEIGHT  5
#define DISP_MARGIN_VERT  5
#define DISP_WIDTH 300
#define DISP_HEIGHT 400
#define DISP_HEIGHT_FACTOR  ((float)DISP_HEIGHT / (float)FRAME_HEIGHT)
#define DISP_WIDTH_FACTOR ((float)DISP_WIDTH / (float)FRAME_WIDTH)

// thjese are constants so do not need to be in the clasas
// they will not vary from instanc ew to instance




class CStaticLaserProfile : public CWnd
{
	DECLARE_DYNAMIC(CStaticLaserProfile)

public:
	CStaticLaserProfile(CLaserControl&, COLORREF bgColour);   // standard constructor
	virtual ~CStaticLaserProfile();
	void Init(CWnd*, UINT);

	void Create(CWnd* pParent);
	void DrawLaserProfile(CDC*);

	CWnd* m_pParent;
	UINT	m_nMsg;

	Profile			m_profile;
	Measurement		m_measure;

	CLaserControl& m_laserControl;
	CString		m_jointPos_str;
	CString		m_gapVal_str;
	CString		m_edgesPos_str;
	CString		m_mismVal_str;
	int			m_profile_count;
	int         m_image_count;
	BOOL		m_valid_edges;
	BOOL		m_valid_joint_pos;
	COLORREF    m_bgColour;

	// Dialog Data
#ifdef AFX_DESIGN_TIME
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()
};