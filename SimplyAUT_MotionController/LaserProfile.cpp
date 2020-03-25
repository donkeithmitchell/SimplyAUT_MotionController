#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogLaser.h"
#include "laserControl.h"
#include "LaserProfile.h"
#include "SLS_Comms.h"
#include "afxdialogex.h"
#include "misc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DISP_MARGIN  5

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
	m_valid_edges = FALSE;
	m_valid_joint_pos = FALSE;
	m_jointPos_str = _T("");
	m_mismVal_str = _T("");
	m_edgesPos_str = _T("");
	m_gapVal_str = _T("");
	m_ROI_stage = 0;
	m_ROI_str = _T("");
	m_disp_width_factor = 1;
	m_disp_height_factor = 1;
	m_disp_height = 0;
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
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CStaticLaserProfile message handlers
void CStaticLaserProfile::Create(CWnd* pParent)
{
	m_pParent = pParent;
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), pParent, 2, NULL);
	ASSERT(m_hWnd != NULL);

	SetTimer(1, 250, NULL);
	EnableWindow(TRUE);
	InvalidateRgn(NULL);
	PostMessage(WM_SIZE);
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

void CStaticLaserProfile::OnRoiButton()
{
	if (m_ROI_stage == 0)
	{
		m_ROI_str = "Lower Left";
		m_ROI_stage = 1;
	}
	else
	{
		CRect rect_roi(0, 0, SENSOR_WIDTH-1, SENSOR_HEIGHT-1);
		m_laserControl.SetCameraRoi(rect_roi);
		//		SLSSetCameraRoi(rect_roi.left, rect_roi.top, rect_roi.right, rect_roi.bottom);
		m_ROI_str.Format("(%d,%d)(%d,%d)", (int)rect_roi.left, (int)rect_roi.top, (int)rect_roi.right, (int)rect_roi.bottom);
	}
	m_pParent->UpdateData(FALSE);
}

void CStaticLaserProfile::ResetROI()
{
	m_ROI_stage = 0;

	CRect rect_roi(0, 0, SENSOR_WIDTH - 1, SENSOR_HEIGHT - 1);
	m_laserControl.SetCameraRoi(rect_roi);
	m_ROI_str.Format("(%d,%d)(%d,%d)", (int)rect_roi.left, (int)rect_roi.top, (int)rect_roi.right, (int)rect_roi.bottom);
	m_pParent->UpdateData(FALSE);
}

CPoint CStaticLaserProfile::GetDataPoint(CPoint pixel)
{
	CPoint ret;
	ret.x = (int)((double)(pixel.x - DISP_MARGIN) / m_disp_width_factor + 0.5);
	ret.y = (int)((double)(m_disp_height + DISP_MARGIN - pixel.y) / m_disp_height_factor + 0.5);

	ret.x = max(ret.x, 0);
	ret.x = min(ret.x, SENSOR_WIDTH - 1);
	ret.y = max(ret.y, 0);
	ret.y = min(ret.y, SENSOR_HEIGHT - 1);
	return ret;
}

CPoint CStaticLaserProfile::GetScreenPixel(double x, double y)
{
	CPoint ret;
	ret.x = DISP_MARGIN + int(x * m_disp_width_factor + 0.5);
	ret.y = DISP_MARGIN + m_disp_height - int(y * m_disp_height_factor + 0.5);

	return ret;
}


void CStaticLaserProfile::OnSize(UINT nFlag, int cx, int cy)
{
	CWnd::OnSize(nFlag, cx, cy);
	CRect rect;
	GetClientRect(&rect);

	m_disp_width_factor  = ((double)rect.Width()  - 2 * DISP_MARGIN) / (double)SENSOR_WIDTH;
	m_disp_height_factor = ((double)rect.Height() - 2 * DISP_MARGIN) / (double)SENSOR_HEIGHT;
	m_disp_height = rect.Height();
}


void CStaticLaserProfile::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(&rect);

	if ((point.x > DISP_MARGIN) && (point.x < rect.Width()- DISP_MARGIN) &&
		(point.y > DISP_MARGIN) && (point.y < (rect.Height()- DISP_MARGIN)))
	{
		CPoint pixel = GetDataPoint(point);

		CRect rect_roi;
		m_laserControl.GetCameraRoi(rect_roi);

		if (m_ROI_stage == 1)
		{
			rect_roi.left = pixel.x;
			rect_roi.top  = pixel.y;
			m_ROI_str = "Upper Right";
			m_ROI_stage = 2;

		}
		else
			if (m_ROI_stage == 2)
			{
				if ((pixel.x > rect_roi.left) && (pixel.y > rect_roi.top))
				{
					rect_roi.right = pixel.x;
					rect_roi.bottom = pixel.y;
				}
				// resst to the default
				else
				{
					rect_roi.left = rect_roi.top = 0;
					rect_roi.right = SENSOR_WIDTH-1;
					rect_roi.bottom = SENSOR_HEIGHT-1;
				}
				m_ROI_str.Format("(%d,%d)(%d,%d)", (int)rect_roi.left, (int)rect_roi.top, (int)rect_roi.right, (int)rect_roi.bottom);
				m_ROI_stage = 0;
				m_laserControl.SetCameraRoi(rect_roi);
			}
		m_pParent->UpdateData(FALSE);
	}

	CWnd::OnLButtonDown(nFlags, point);
}


void CStaticLaserProfile::DrawLaserProfile(CDC* pDC)
{
	CRect rect;
	// the rectangle to draw into
	/////////////////////////////
	GetClientRect(&rect);

	// Draw the background of the laser display
	CPen PenPrimaryEdge(PS_DASHDOT, 0, RGB(10, 10, 250));
	CPen PenSecondaryEdge(PS_DOT, 1, RGB(250, 250, 10));
	CPen PenTrackingPoint(PS_DOT, 2, RGB(10, 200, 10));

	// erase the rectangle to draw into
	//////////////////////////////////////
	CBrush brushErase(m_bgColour);
	pDC->FillRect(&rect, &brushErase);

	if (!m_laserControl.IsLaserOn())
		return;

	// draw a line in the centre to represent the craler
	pDC->SelectObject(&PenPrimaryEdge);
	pDC->SetROP2(R2_MERGEPEN);
	pDC->MoveTo((rect.left + rect.right) / 2, rect.top);
	pDC->LineTo((rect.left + rect.right) / 2, rect.bottom);

	// Display Profile as a RED line
	///////////////////////////////////
	pDC->SelectObject(&PenTrackingPoint);
	for (int i = 0; i < SENSOR_WIDTH; i++)
	{
		if (m_profile.hits[i].pos1 > 0 && m_profile.hits[i].pos1 < SENSOR_HEIGHT)
		{
			CPoint pt = GetScreenPixel(i, m_profile.hits[i].pos1);
			pDC->SetPixel(pt, RGB(250,50,50));
		}
	}

	// get a measure of the lasewr
	// show it as a line drawn top to bottom at thazt location
	//////////////////////////////////////////////////////////////
//	m_laserControl.CalcLaserMeasures(m_profile.hits, m_measure2); in ontimer

	CPoint pt = GetScreenPixel(m_measure2.weld_cap_pix.x, m_measure2.weld_cap_pix.y);
	CPen penWeldCentre(PS_DOT, 0, RGB(10, 255, 10));
	pDC->SelectObject(&penWeldCentre);
	pDC->MoveTo(pt.x, pt.y-20);
	pDC->LineTo(pt.x, pt.y+20);

	// draw the hgith of the dsown and up sides
	// alread have points at the inside of the sides, now need the outside point
	//////////////////////////////////////////////////////
	pDC->SelectObject(&PenSecondaryEdge);
	CPoint pt11 = GetScreenPixel(m_measure2.weld_left/2, m_measure2.GetDnSideStartHeight());
	CPoint pt12 = GetScreenPixel(m_measure2.weld_left, m_measure2.GetDnSideWeldHeight());
	pDC->MoveTo(pt11.x, pt11.y);
	pDC->LineTo(pt12.x, pt12.y);

	CPoint pt21 = GetScreenPixel(m_measure2.weld_right, m_measure2.GetUpSideWeldHeight());
	CPoint pt22 = GetScreenPixel((m_measure2.weld_right+SENSOR_WIDTH)/2, m_measure2.GetUpSideEndHeight());
	pDC->MoveTo(pt21.x, pt21.y);
	pDC->LineTo(pt22.x, pt22.y);


	// draw the values returned by the F/W
	/*
	if (m_valid_edges)
	{
		pDC->SelectObject(&PenPrimaryEdge);
		CPoint pt = GetScreenPixel(m_measure.mp[1].x, m_measure.mp[2].y);
		pDC->MoveTo(pt.x, pt.y - 3);
		pDC->LineTo(pt.x, pt.y + 3);

		pt = GetScreenPixel(m_measure.mp[2].x, m_measure.mp[2].y);
		pDC->MoveTo(pt.x, pt.y - 3);
		pDC->LineTo(pt.x, pt.y + 3);
	}
	// Display Tracking Point
	if (m_valid_joint_pos)
	{
		CPoint pt = GetScreenPixel(m_measure.mp[0].x, m_measure.mp[0].y);
		pDC->SelectObject(&PenTrackingPoint);
		pDC->MoveTo(pt.x - 7, pt.y);
		pDC->LineTo(pt.x + 7, pt.y);
		pDC->MoveTo(pt.x, pt.y - 7);
		pDC->LineTo(pt.x, pt.y + 7);
	}
	*/
	// while waiting for the 2nd clicki, draw a rectangle
	if (m_ROI_stage == 2)
	{
		CPen penROI(PS_DOT, 0, RGB(255, 255, 255));
		pDC->SelectStockObject(HOLLOW_BRUSH);
		pDC->SelectObject(&penROI);

		// get the lower left position as saved
		CRect rect_roi;
		m_laserControl.GetCameraRoi(rect_roi);
		CPoint bott_left = GetScreenPixel(rect_roi.left, rect_roi.top);

		CPoint top_right;
		GetCursorPos(&top_right); // screen position
		ScreenToClient(&top_right);

		pDC->Rectangle(bott_left.x, bott_left.y, top_right.x, top_right.y);
	}
}

void CStaticLaserProfile::OnTimer(UINT_PTR nIDEvent)
{
	CString tstr;

	double h_mm = 0.0, v_mm = 0.0;
	double h2_mm = 0.0, v2_mm = 0.0;
	double gap_h_mm, gap_v_mm;

	if (!IsWindowVisible())
		return;

	InvalidateRgn(NULL);
	
	if (!m_laserControl.IsConnected() || !m_laserControl.IsLaserOn())
		return;

	if (!m_laserControl.GetProfile(m_profile))
		return;

	// will truy every 50 ms, but only draw every 500 ms
	m_profile_count++;

	m_laserControl.CalcLaserMeasures(m_profile.hits, m_measure2); 
	m_laserControl.ConvPixelToMm((int)m_measure2.weld_cap_pix.x, (int)m_measure2.weld_cap_pix.y, h_mm, v_mm);
	m_jointPos_str.Format("(%.1f,%.1f)", h_mm, v_mm);
	m_valid_joint_pos = true;

	m_laserControl.ConvPixelToMm((int)m_measure2.weld_left, (int)m_measure2.GetDnSideWeldHeight(), h_mm, v_mm);
	m_laserControl.ConvPixelToMm((int)m_measure2.weld_right, (int)m_measure2.GetUpSideWeldHeight(), h2_mm, v2_mm);
	m_edgesPos_str.Format("(%.1f,%.1f) (%.1f,%.1f)", h_mm, v_mm, h2_mm, v2_mm);
	m_valid_edges = true;

	m_laserControl.ConvPixelToMm(
		(int)(m_measure2.weld_right - m_measure2.weld_left),
		(int)(m_measure2.GetUpSideWeldHeight() - m_measure2.GetDnSideWeldHeight()), gap_h_mm, gap_v_mm);
	m_gapVal_str.Format("%.1f", -gap_h_mm);
	m_mismVal_str.Format("%.1f", gap_v_mm);

	m_pParent->PostMessageA(m_nMsg);

/*
	if (m_laserControl.IsConnected() && m_laserControl.IsLaserOn() && m_laserControl.GetLaserMeasurment(m_measure1))
	{
		if (m_measure1.mp[0].status == 0)
		{
			m_laserControl.ConvPixelToMm((int)m_measure1.mp[0].x, (int)m_measure1.mp[0].y, h_mm, v_mm);
			m_jointPos_str.Format("(%.1f,%.1f)", h_mm, v_mm);
			m_valid_joint_pos = true;
		}
		else
		{
			m_jointPos_str = "-----";
			m_valid_joint_pos = false;
		}
		if ((m_measure1.mp[1].status == 0) || (m_measure1.mp[2].status == 0))
		{
			m_laserControl.ConvPixelToMm((int)m_measure1.mp[1].x, (int)m_measure1.mp[1].y, h_mm, v_mm);
			m_laserControl.ConvPixelToMm((int)m_measure1.mp[2].x, (int)m_measure1.mp[2].y, h2_mm, v2_mm);
			m_edgesPos_str.Format("(%+5.1f,%+5.1f) (%+5.1f,%+5.1f)", h_mm, v_mm, h2_mm, v2_mm);
			m_valid_edges = true;
		}
		else
		{
			m_edgesPos_str = "-----";
			m_valid_edges = false;
		}
		if (m_measure1.mv[0].status == 0)
		{
			m_gapVal_str.Format("%+5.1f", m_measure1.mv[0].val);
		}
		else
		{
			m_gapVal_str = "-----";
		}
		if (m_measure1.mv[1].status == 0)
		{
			m_mismVal_str.Format("%+5.1f", m_measure1.mv[1].val);
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
	*/

	CWnd::OnTimer(nIDEvent);
}
