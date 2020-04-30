#pragma once
#include "DialogLaser.h"
#include "SLSDef.h"
#include "LaserControl.h"
#include "MagController.h"
#include "Misc.h"

// CStaticMag dialog
class CLaserControl;
class CMagControl;
class CStaticMag : public CWnd
{
	DECLARE_DYNAMIC(CStaticMag)

public:
	CStaticMag(CMagControl&, const BOOL&, const BOOL&, const double&);

	virtual ~CStaticMag();
	void	Init(CWnd*, UINT);
	void	Create(CWnd* pParent);
	void	DrawMagProfile(CDC*);
	void	GetOffsetRect(CRect*);
	void	GetRGBRect(CRect*);
	double	GetAverageRGBValue();
	void	DrawRGBProfile(CDC* pDC);

	CWnd*	m_pParent;
	UINT	m_nMsg;

	CRect				m_disp_rect;
	CMagControl&		m_magControl;
	const double&		m_fCalibrationLength;
	const BOOL&			m_bScanLaser;
	const BOOL&			m_bSeekBlackLine;

// Dialog Data
#ifdef AFX_DESIGN_TIME
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint pt);

	DECLARE_MESSAGE_MAP()
};
