#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogLaser.h"
#include "laserControl.h"
#include "LaserProfile.h"
#include "SLS_Comms.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//////////////////////////////////////////
IMPLEMENT_DYNAMIC(CStaticLaserProfile, CWnd)

CStaticLaserProfile::CStaticLaserProfile(CLaserControl& laser, COLORREF bgColour)
	: CWnd()
	, m_laserControl(laser)
	, m_bgColour(bgColour)
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_profile_count = 0;
	m_image_count = 0;
	m_valid_edges = FALSE;
	m_valid_joint_pos = FALSE;
	m_jointPos_str = _T("");
	m_mismVal_str = _T("");
	m_edgesPos_str = _T("");
	m_gapVal_str = _T("");
}

CStaticLaserProfile::~CStaticLaserProfile()
{
}

void CStaticLaserProfile::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}



void CStaticLaserProfile::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStaticLaserProfile, CWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CStaticLaserProfile message handlers
void CStaticLaserProfile::Create(CWnd* pParent)
{
	m_pParent = pParent;
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), pParent, 2, NULL);
	ASSERT(m_hWnd != NULL);

	SetTimer(1, 50, NULL);
	EnableWindow(TRUE);
	InvalidateRgn(NULL);
}

void CStaticLaserProfile::OnPaint()
{
	CRect rect;
	CPaintDC dc(this);

	GetClientRect(&rect);

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	ASSERT(memDC.m_hDC);

	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());

	CBitmap* pBitmap = memDC.SelectObject(&bitmap);
	DrawLaserProfile(&memDC);
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
}


void CStaticLaserProfile::DrawLaserProfile(CDC* pDC)
{
	int i, x, y;
	CRect rect;
	GetClientRect(&rect);


	// Draw the background of the laser display
	CBrush black_brush;
	CPen PenPrimaryEdge(PS_SOLID, 2, RGB(10, 10, 250));
	CPen PenSecondaryEdge(PS_SOLID, 1, RGB(250, 250, 10));
	CPen PenTrackingPoint(PS_SOLID, 2, RGB(10, 200, 10));

	black_brush.CreateSolidBrush(m_bgColour);
	pDC->FillRect(&rect, &black_brush);


	if (m_laserControl.IsLaserOn())
	{
		// Display Profile
		for (i = 0; i < FRAME_WIDTH; i++)
		{
			x = DISP_MARGIN_HEIGHT + int(float(i) * DISP_WIDTH_FACTOR);
			y = DISP_MARGIN_VERT + DISP_HEIGHT - int(float(m_profile.hits[i].pos1) * DISP_HEIGHT_FACTOR);
			pDC->SetPixel(x, y, RGB(250, 50, 50));
		}
		if (m_image_count)
		{
			pDC->SelectObject(&PenPrimaryEdge);
			x = DISP_MARGIN_HEIGHT + int(m_measure.mp[1].x * DISP_WIDTH_FACTOR);
			y = DISP_MARGIN_VERT + DISP_HEIGHT - int(m_measure.mp[1].y * DISP_HEIGHT_FACTOR);
			pDC->MoveTo(x, y - 3);
			pDC->LineTo(x, y + 3);

			x = DISP_MARGIN_HEIGHT + int(m_measure.mp[2].x * DISP_WIDTH_FACTOR);
			y = DISP_MARGIN_VERT + DISP_HEIGHT - int(m_measure.mp[2].y * DISP_HEIGHT_FACTOR);
			pDC->MoveTo(x, y - 3);
			pDC->LineTo(x, y + 3);
		}
		// Display Tracking Point
		if (m_valid_joint_pos)
		{
			x = DISP_MARGIN_HEIGHT + int(m_measure.mp[0].x * DISP_WIDTH_FACTOR);
			y = DISP_MARGIN_VERT + DISP_HEIGHT - int(m_measure.mp[0].y * DISP_HEIGHT_FACTOR);
			pDC->SelectObject(&PenTrackingPoint);
			pDC->MoveTo(x - 7, y);
			pDC->LineTo(x + 7, y);
			pDC->MoveTo(x, y - 7);
			pDC->LineTo(x, y + 7);
		}
	}
}

void CStaticLaserProfile::OnTimer(UINT_PTR nIDEvent)
{
	CString tstr;

	double h_mm = 0.0, v_mm = 0.0;
	double h2_mm = 0.0, v2_mm = 0.0;

	if (!IsWindowVisible())
		return;


	if (m_laserControl.IsConnected() && m_laserControl.IsLaserOn() && m_laserControl.GetLaserMeasurment(m_measure))
	{
		if (m_measure.mp[0].status == 0)
		{
			m_laserControl.ConvPixelToMm((int)m_measure.mp[0].x, (int)m_measure.mp[0].y, h_mm, v_mm);
			m_jointPos_str.Format("(%.1f,%.1f)", h_mm, v_mm);
			m_valid_joint_pos = true;
		}
		else
		{
			m_jointPos_str = "-----";
			m_valid_joint_pos = false;
		}
		if ((m_measure.mp[1].status == 0) || (m_measure.mp[2].status == 0))
		{
			m_laserControl.ConvPixelToMm((int)m_measure.mp[1].x, (int)m_measure.mp[1].y, h_mm, v_mm);
			m_laserControl.ConvPixelToMm((int)m_measure.mp[2].x, (int)m_measure.mp[2].y, h2_mm, v2_mm);
			m_edgesPos_str.Format("%+5.1f,%+5.1f %+5.1f,%+5.1f", h_mm, v_mm, h2_mm, v2_mm);
			m_image_count = true;
		}
		else
		{
			m_edgesPos_str = "-----";
			m_image_count = false;
		}
		if (m_measure.mv[0].status == 0)
		{
			m_gapVal_str.Format("%+5.1f", m_measure.mv[0].val);
		}
		else
		{
			m_gapVal_str = "-----";
		}
		if (m_measure.mv[1].status == 0)
		{
			m_mismVal_str.Format("%+5.1f", m_measure.mv[1].val);
		}
		else
		{
			m_mismVal_str = "-----";
		}
		if (m_laserControl.GetProfile(m_profile))
		{
			m_image_count++;
		}
		m_profile_count = m_image_count;

		if (m_profile_count % 10 == 0)
			m_pParent->PostMessageA(m_nMsg);
	}

	InvalidateRgn(NULL);
	CWnd::OnTimer(nIDEvent);
}
