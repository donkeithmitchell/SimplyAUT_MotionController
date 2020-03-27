// StaticLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "StaticLaser.h"
#include "LaserControl.h"
#include "MagController.h"
#include "afxdialogex.h"

static double PI = 4 * atan(1.0); 
#define DISP_MARGIN 5

// CStaticLaser dialog

IMPLEMENT_DYNAMIC(CStaticLaser, CWnd)

CStaticLaser::CStaticLaser(CLaserControl& laser, CMagControl& mag, const Profile& prof, const LASER_MEASURES& meas, const double* buffer)
	: CWnd()
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_profile(prof)
	, m_measure2(meas)
	, m_hitBuffer(buffer)
//	, m_wndLaserProfile(laser, ::GetSysColor(COLOR_3DFACE))
	, m_ptMouse(INT_MAX, INT_MAX)
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_fHomeAng = 0;
//	m_profile_count = 0;
//	m_image_count = 0;
	m_disp_width_factor = 1;
	m_disp_height_factor = 1;
	m_disp_height_min = 0;
	m_disp_height_max = SENSOR_HEIGHT;
	m_disp_rect = CRect(0, 0, 0, 0);
	m_rgbSum = 0;
	m_rgbCount = 0;
}

CStaticLaser::~CStaticLaser()
{
}

void CStaticLaser::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}



void CStaticLaser::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStaticLaser, CWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND_RANGE(ID_POPUP_SETLOCATION, ID_POPUP_SETLOCATION,OnMenu)
END_MESSAGE_MAP()


// CStaticLaser message handlers
void CStaticLaser::Create(CWnd* pParent)
{
	m_pParent = pParent;
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0,0, 0,0), pParent, 1, NULL);
	ASSERT(m_hWnd != NULL);

//	m_wndLaserProfile.Create(this);
//	m_wndLaserProfile.Init(this, 0);

	EnableWindow(TRUE);
	InvalidateRgn(NULL);
}

void CStaticLaser::OnPaint()
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
	DrawCrawlerLocation(&memDC);
	DrawLaserProfile(&memDC);
	DrawRGBProfile(&memDC);
	DrawLaserOffset(&memDC);
	//	m_wndLaserProfile.InvalidateRgn(NULL);
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
}

void CStaticLaser::OnSize(UINT nFlag, int cx, int cy)
{
	CWnd::OnSize(nFlag, cx, cy);

	GetLaserRect(&m_disp_rect);
	m_disp_width_factor = ((double)m_disp_rect.Width()) / (double)SENSOR_WIDTH;
//	m_disp_height_factor = ((double)m_disp_rect.Height()) / (double)(SENSOR_HEIGHT); // mut set when get data
}

CPoint CStaticLaser::GetScreenPixel(double x, double y)
{
	CPoint ret;
	ret.x = m_disp_rect.left + int(x * m_disp_width_factor + 0.5);
	ret.y = m_disp_rect.bottom - int((y- m_disp_height_min) * m_disp_height_factor + 0.5);

	return ret;
}

// draw a circle with the current location of the crawler noted on the circle
// also note the SET home location and the line detected by the RGB sensor
void CStaticLaser::DrawCrawlerLocation(CDC* pDC)
{
	CRect rect1;
	GetClientRect(&rect1);

	CBrush brush(::GetSysColor(COLOR_3DFACE));
	pDC->FillRect(&rect1, &brush);

	// draw thew pipe
	CGdiObject* pBrush1 = pDC->SelectStockObject(HOLLOW_BRUSH);
	CPen pen(PS_SOLID, 0, RGB(0, 0, 0));
	CPen* pPen = pDC->SelectObject(&pen);

	CRect rect2;
	int radius1 = GetPipeRect(&rect2);
	pDC->Ellipse(&rect2);

	// now note the location of the crawler
	int circum1 = (int)(2 * PI * radius1 + 0.5);


	// now draw the crawler at an angle determine by the start angle and the progression
	double circum2 = GetPipeCircumference(); // mm
	double location2 = GetCrawlerLocation(); // mm
	if (location2 != FLT_MAX)
	{
		// the frac tion of the way around the pipe
		// less the homne angle
		double ang = m_fHomeAng - (2 * PI * location2 / circum2) - PI / 2;

		// get this location
		int x0 = (rect2.right + rect2.left) / 2;
		int y0 = (rect2.top + rect2.bottom) / 2;

		int x1 = (int)(x0 + radius1 * cos(ang) + 0.5);
		int y1 = (int)(y0 + radius1 * sin(ang) + 0.5);

		// draw a filled dot at this location
		// red if magnets not engaged, else greenm
		int nMagOn = GetMagStatus(MAG_IND_MAG_ON);
		CBrush brush2(nMagOn==1 ? RGB(0, 255, 0) : RGB(255, 0, 0));
		CPen pen2(PS_SOLID, 0, (nMagOn==1 ? RGB(0, 255, 0) : RGB(255, 0, 0)));
		CPen * pPen2 = pDC->SelectObject(&pen2);
		CBrush* pBrush2 = pDC->SelectObject(&brush2);
		pDC->Ellipse(x1 - 5, y1 - 5, x1 + 5, y1 + 5);
	}


	//pDC->SelectObject(pBrush1);
	//pDC->SelectObject(pPen);
}


void CStaticLaser::SetCrawlerLocation(CPoint pt)
{
	// get the angle of pt from the centre
	CRect rect;
	GetPipeRect(&rect);

	// get the angle from the centre and add 90 deg
	double x0 = (rect.left + rect.right) / 2.0;
	double y0 = (rect.top + rect.bottom) / 2.0;

	// will draw thew crawler from this position on
	m_fHomeAng = atan2((double)(pt.y) - y0, (double)(pt.x) - x0) + PI / 2;

	InvalidateRgn(NULL);
}

void CStaticLaser::GetLaserRect(CRect* rect)
{
	// positioon in the bottom half and in the centre half of that
	GetPipeRect(rect);

	// push in from the pipe circle
	int x1 = rect->left + rect->Width() / 6;
	int x2 = rect->right - rect->Width() / 6;
	int y1 = rect->top + rect->Height() / 8;
	int y2 = (rect->bottom + rect->top) / 2;
	rect->SetRect(x1, y1, x2, y2);
}


void CStaticLaser::GetOffsetRect(CRect* rect)
{
	// positioon in the bottom half and in the centre half of that
	CRect client;
	GetClientRect(&client);
	GetPipeRect(rect);

	// push in from the pipe circle
	int x1 = client.left;
	int x2 = client.right;
	int y1 = rect->bottom + 2;
	int y2 = client.bottom;

	rect->SetRect(x1, y1, x2, y2);
}



int CStaticLaser::GetPipeRect(CRect* rect)
{
	GetClientRect(rect);
	int x0 = (rect->left + rect->right) / 2;
	int y0 = (rect->top + rect->bottom) / 2;

	int radius1 = min(rect->Width(), rect->Height()) / 2;

	if (rect->Width() > rect->Height())
	{
		rect->right = x0 + radius1;
		rect->left = x0 - radius1;
	}
	else
	{

		rect->top = y0 - radius1;
		rect->bottom = y0 + radius1;
	}
	return radius1;
}


void CStaticLaser::GetRGBRect(CRect* rect)
{
	// positioon in the bottom half and in the centre half of that
	GetClientRect(rect);
	int x1 = rect->left + rect->Width() / 4;
	int x2 = rect->right - rect->Width() / 4;
	int y1 = (rect->bottom + rect->top) / 2;
	int y2 = rect->bottom - rect->Height() / 8;
	rect->SetRect(x1, y1, x2, y2);
}

double CStaticLaser::GetAverageRGBValue()
{
	return (m_rgbCount < 0) ? (double)m_rgbSum / (double)m_rgbCount : 0;
}

void CStaticLaser::DrawRGBProfile(CDC* pDC)
{
	if (!m_magControl.IsConnected())
		return;

	CRect rect;
	GetRGBRect(&rect);

	CPen PenBlack(PS_SOLID, 0, RGB(0, 0, 0));
	CPen PenBlack2(PS_DOT, 0, RGB(0, 0, 0));
	CPen PenBlack3(PS_DASHDOT, 0, RGB(0, 0, 0));
	CPen PenGreen(PS_SOLID, 0, RGB(0, 255, 0));

	pDC->SelectObject(&PenBlack);
	pDC->MoveTo(rect.left, rect.bottom);
	pDC->LineTo(rect.right, rect.bottom);

	// change this colour to iondicatge if line present
	BOOL bPresent = GetMagStatus(MAG_IND_RGB_LINE) == 1;
	pDC->SelectObject(bPresent ? &PenGreen : &PenBlack);

	pDC->MoveTo((rect.left + rect.right) / 2, rect.bottom);
	pDC->LineTo((rect.left + rect.right) / 2, rect.top);

	// draw the last 50 mm worth of colour data
	// the data will have been set into this object 
	int minVal = INT_MAX;
	int maxVal = 5;
	int len = (int)m_rgbData.GetSize();
	if (len == 0)
		return;

	for (int i = 0; i < len; ++i)
	{
		minVal = min(minVal, m_rgbData[i]);
		maxVal = max(maxVal, m_rgbData[i]);
	}

	double scaleX = (double)rect.Width() / 25.0;
	double scaleY = (double)rect.Height() / (double)(maxVal);

	// note the median value
	double avg = GetAverageRGBValue();
	pDC->SelectObject(&PenBlack2);
	int y = (int)(rect.bottom - avg * scaleY + 0.5);
	pDC->MoveTo(rect.left, y);
	pDC->LineTo(rect.right, y);

	// note the half way point from max and median (assume this shiould be calibration)
	y = (int)(rect.bottom - (minVal+avg)/2 * scaleY + 0.5);
	pDC->SelectObject(&PenBlack3);
	pDC->MoveTo(rect.left, y);
	pDC->LineTo(rect.right, y);


	pDC->SelectObject(&PenBlack);
	for (int i = 0; i < len; ++i)
	{
		int x = (int)(rect.left + i * scaleX + 0.5);
		int y = (int)(rect.bottom - m_rgbData[i] * scaleY + 0.5);
		(i == 0) ? pDC->MoveTo(x, y) : pDC->LineTo(x, y);
	}

	CString str;
	str.Format("%d", minVal);
	CSize sz = pDC->GetTextExtent(str);
	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOutA((rect.left + rect.right) / 2 - 5 * sz.cx / 4, rect.bottom - sz.cy, str);

	str.Format("%d", maxVal);
	sz = pDC->GetTextExtent(str);
	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOutA((rect.left + rect.right) / 2, rect.bottom - sz.cy, str);

	str.Format("%d", m_rgbData[len - 1]);
	sz = pDC->GetTextExtent(str);
	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOutA((rect.left + rect.right) / 2 - sz.cx / 2, rect.bottom, str);

}

int CStaticLaser::AddRGBData(const int& value)
{
	if ( value != INT_MAX)
	{
		// for now
		m_rgbData.Add(value);
		m_rgbSum += value;
		m_rgbCount++;

		while (m_rgbData.GetSize() > 25)
		{
			m_rgbSum -= m_rgbData[0];
			m_rgbCount--;
			m_rgbData.RemoveAt(0, 1);
		}
	}

	return (int)m_rgbData.GetSize();
}

void CStaticLaser::ResetRGBData()
{
	m_rgbData.SetSize(0);
}

// arecord of the laser offsets is being kept by the steering function
void CStaticLaser::DrawLaserOffset(CDC* pDC)
{
	if (!m_laserControl.IsLaserOn())
		return;

	if (m_pParent == NULL || !::IsWindow(m_pParent->m_hWnd))
		return;

	// build an array of offsets and position
	LASER_POS lastPos;
	m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::STATUS_GET_LAST_LASER_POS, (WPARAM)&lastPos);
	if (lastPos.gap_filt != FLT_MAX)
		m_laserPos.Add(CDoublePoint(lastPos.pos, lastPos.gap_filt));

	if (m_laserPos.GetSize() == 0)
		return;

	double minGap = FLT_MAX;
	double maxGap = -FLT_MAX;
	double minPos = FLT_MAX;
	double maxPos = -FLT_MAX;

	for (int i = 0; i < m_laserPos.GetSize(); ++i)
	{
		minGap = min(minGap, m_laserPos[i].y);
		maxGap = max(maxGap, m_laserPos[i].y);

		minPos = min(minPos, m_laserPos[i].x);
		maxPos = max(maxPos, m_laserPos[i].x);
	}

	if (minPos == maxPos)
		return;

	CRect rect;
	GetOffsetRect(&rect);
	if (rect.Width() <= 0 || rect.Height() <= 0)
		return;

	int scanLength = (int)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::STATUS_GET_SCAN_LENGTH);
	minGap = min(minGap, -1);
	maxGap = max(maxGap, 1);

	double scaleX = (double)rect.Width() / (double)scanLength; //  (maxPos - minPos);
	double scaleY = (double)rect.Height() / (maxGap - minGap);

	CPen pen(PS_SOLID, 0, RGB(250, 10, 10));
	pDC->SelectObject(&pen);
	for (int i = 0; i < m_laserPos.GetSize(); ++i)
	{
		int x = rect.left + (int)((m_laserPos[i].x - minPos) * scaleX + 0.5);
		int y = rect.bottom - (int)((m_laserPos[i].y - minGap) * scaleY + 0.5);
		(i == 0) ? pDC->MoveTo(x, y) : pDC->LineTo(x, y);
	}

}


// draw the profile returned by the laser within the above circle
void CStaticLaser::DrawLaserProfile(CDC* pDC)
{
	if (!m_laserControl.IsLaserOn())
		return;

	GetLaserProfile();


	CPen PenBlack(PS_SOLID, 0, RGB(0, 0, 0));
	COLORREF colourCrawler = RGB(10, 250, 250);
	CPen PenCrawler(PS_DASHDOT, 1, colourCrawler);
	CBrush BrushCrawler(colourCrawler);

	pDC->SelectObject(&PenBlack);
	pDC->MoveTo(m_disp_rect.left, m_disp_rect.bottom);
	pDC->LineTo(m_disp_rect.right, m_disp_rect.bottom);

	// Draw the background of the laser display
//	CPen PenSecondaryEdge(PS_SOLID, 1, RGB(250, 250, 10));
//	CPen PenTrackingPoint(PS_SOLID, 2, RGB(10, 200, 10));

	CPen hits(PS_SOLID, 0, RGB(250, 50, 50));
	pDC->SelectObject(&hits);

	// fit the lasewr display to the screen
	m_disp_height_max = 0;
	m_disp_height_min = SENSOR_HEIGHT;
	for (int i = 0; i <= SENSOR_WIDTH; i++)
	{
		double val = m_hitBuffer[i];
//		int val = m_profile.hits[i].pos1;
		if (val>= 1 && val <= SENSOR_HEIGHT-1)
		{
			m_disp_height_min = min(m_disp_height_min, val);
			m_disp_height_max = max(m_disp_height_max, val);
		}
	}
//	m_disp_width_factor = ((double)m_disp_rect.Width()) / (double)SENSOR_WIDTH; // set in OnSize()
	m_disp_height_factor = ((double)m_disp_rect.Height()) / (m_disp_height_max- m_disp_height_min);

	// m_measure2.weld_cap_pix.x is shifted to the centre (i.e the weld does not move, the crawler does)
	int shift = (int)(SENSOR_WIDTH / 2 - m_measure2.weld_cap_pix.x);

	for (int i = 0; i <= SENSOR_WIDTH; i++)
	{
		double val = m_hitBuffer[i];
//		int val = m_profile.hits[i].pos1;
		if (val >= 1 && val <= SENSOR_HEIGHT - 1)
		{
			CPoint pt = GetScreenPixel(i+shift, val);
			pDC->SetPixel(pt, RGB(250, 50, 50));
		}
	}

	// draw a dot to represent the scanner
	pDC->SelectObject(&PenCrawler);
	pDC->SetROP2(R2_COPYPEN);
	pDC->SelectObject(&BrushCrawler);
	pDC->SelectObject(&PenCrawler);
	double y1 = max(m_measure2.GetDnSideWeldHeight(), m_measure2.GetUpSideWeldHeight());
	double y2 = (y1 + m_measure2.weld_cap_pix.y) / 2;
	CPoint pt1 = GetScreenPixel(SENSOR_WIDTH-m_measure2.weld_cap_pix.x + shift, y2);
	pDC->Ellipse(pt1.x - 5, pt1.y - 5, pt1.x + 5, pt1.y + 5);
	pDC->MoveTo(pt1.x, pt1.y - 10);
	pDC->LineTo(pt1.x, pt1.y + 10);

	// draw a vertical line at the weld centre
	// the values are in (mm) not laser pixels
	CPoint pt = GetScreenPixel(m_measure2.weld_cap_pix.x+shift, m_measure2.weld_cap_pix.y);
	CPen penWeldCentre(PS_SOLID, 0, RGB(10, 255, 10));
	pDC->SelectObject(&penWeldCentre);
	pDC->MoveTo(pt.x, m_disp_rect.bottom);
	pDC->LineTo(pt.x, m_disp_rect.top);

	CPoint pt11 = GetScreenPixel((m_measure2.weld_left+shift) / 2, m_measure2.GetDnSideStartHeight());
	CPoint pt12 = GetScreenPixel(m_measure2.weld_left+shift, m_measure2.GetDnSideWeldHeight());
	CPen PenPrimaryEdge(PS_SOLID, 1, RGB(10, 10, 250));
	pDC->SelectObject(&PenPrimaryEdge);
	pDC->MoveTo(pt11.x, pt11.y);
	pDC->LineTo(pt12.x, pt12.y);

	CPoint pt21 = GetScreenPixel(m_measure2.weld_right+shift, m_measure2.GetUpSideWeldHeight());
	CPoint pt22 = GetScreenPixel((m_measure2.weld_right+shift + SENSOR_WIDTH) / 2, m_measure2.GetUpSideEndHeight());
	pDC->MoveTo(pt21.x, pt21.y);
	pDC->LineTo(pt22.x, pt22.y);
/*
	if ( m_edge_pos[0].IsSet())
	{
		pDC->SelectObject(&PenPrimaryEdge);
		CPoint pt = GetScreenPixel(m_measure1.mp[1].x, m_measure1.mp[1].y);
		pDC->MoveTo(pt.x, pt.y - 3);
		pDC->LineTo(pt.x, pt.y + 3);
	}

	if ( m_edge_pos[1].IsSet())
	{
		pDC->SelectObject(&PenPrimaryEdge);
		CPoint pt = GetScreenPixel(m_measure1.mp[2].x, m_measure1.mp[2].y);
		pDC->MoveTo(pt.x, pt.y - 3);
		pDC->LineTo(pt.x, pt.y + 3);
	}
	// Display Tracking Point
	if ( m_joint_pos.IsSet())
	{
		CPoint pt = GetScreenPixel(m_measure1.mp[0].x, m_measure1.mp[0].y);
		pDC->SelectObject(&PenTrackingPoint);
		//		pDC->MoveTo(pt.x - 7, pt.y);
		//		pDC->LineTo(pt.x + 7, pt.y);
		//		pDC->MoveTo(pt.x, pt.y - 7);
		//		pDC->LineTo(pt.x, pt.y + 7);
		pDC->MoveTo(pt.x, m_disp_rect.top);
		pDC->LineTo(pt.x, m_disp_rect.bottom);

		// also draw a line in the centre the full width of the view
		CRect rect;
		GetPipeRect(&rect);
		pDC->SelectObject(&PenBlack);
		pDC->MoveTo((rect.left + rect.right) / 2, rect.top);
		pDC->LineTo((rect.left + rect.right) / 2, rect.bottom);
	}
	*/
}


void CStaticLaser::OnTimer(UINT_PTR nIDEvent)
{
	Measurement meas;
	LASER_TEMPERATURE temp;

	switch (nIDEvent)
	{
	case TIMER_GET_MEASUREMENT:
		m_laserControl.GetLaserMeasurment(meas);
		break;
	case TIMER_GET_TEMPERATURE:
		m_laserControl.GetLaserTemperature(temp);
		break;
	}
}

void CStaticLaser::GetLaserProfile()
{
	CString tstr;

	double h_mm = 0.0, v_mm = 0.0;
	double h2_mm = 0.0, v2_mm = 0.0;

	if (!IsWindowVisible())
		return;


	// ASSUMING THAT the index values are for (mp)
	// 0: weld cap position and height
	// 1: down side start and height
	// 2: up side start and height
	// 3: 

	// assuming that the index values for (mv)
	// 0: gap value
	// 1: mismatch

	if (m_measure2.status == 0)
	{
		m_joint_pos = m_measure2.weld_cap_mm;

		m_laserControl.ConvPixelToMm((int)m_measure2.weld_left, (int)m_measure2.GetDnSideWeldHeight(), m_edge_pos[0].x, m_edge_pos[0].y);
		m_laserControl.ConvPixelToMm((int)m_measure2.weld_right, (int)m_measure2.GetUpSideWeldHeight(), m_edge_pos[1].x, m_edge_pos[1].y);
		m_edge_pos[2].x = m_edge_pos[1].x - m_edge_pos[0].x;
		m_edge_pos[2].y = m_edge_pos[1].y - m_edge_pos[0].y;
	}


/*
	if ( m_laserControl.GetLaserMeasurment(m_measure1))
	{
		if (m_measure1.mp[0].status == 0)
			m_laserControl.ConvPixelToMm((int)m_measure1.mp[0].x, (int)m_measure1.mp[0].y, m_joint_pos.x, m_joint_pos.y);
		else
			m_joint_pos.Reset();

		if ((m_measure1.mp[1].status == 0) || (m_measure1.mp[2].status == 0))
		{
			m_laserControl.ConvPixelToMm((int)m_measure1.mp[1].x, (int)m_measure1.mp[1].y, m_edge_pos[0].x, m_edge_pos[0].y);
			m_laserControl.ConvPixelToMm((int)m_measure1.mp[2].x, (int)m_measure1.mp[2].y, m_edge_pos[1].x, m_edge_pos[1].y);
			m_edge_pos[2].x = 0;
			m_edge_pos[2].y = fabs(m_edge_pos[0].y - m_edge_pos[1].y);
		}
		else
		{
			m_edge_pos[0].Reset();
			m_edge_pos[1].Reset();
			m_edge_pos[2].Reset();
		}
		if (m_measure1.mv[0].status == 0)
		{
	//		m_gapVal_str.Format("%+5.1f", m_measure1.mv[0].val);
		}
		else
		{
	//		m_gapVal_str = "-----";
		}
		if (m_measure1.mv[1].status == 0)
		{
	//		m_mismVal_str.Format("%+5.1f", m_measure1.mv[1].val);
		}
		else
		{
	//		m_mismVal_str = "-----";
		}
		if ( m_laserControl.GetProfile(m_profile))
			m_image_count++;

		m_profile_count = m_image_count;

//		if ( m_profile_count % 10 == 0)
//			m_pParent->PostMessageA(m_nMsg, STATUS_SHOWLASERSTATUS, 0);

//		InvalidateRgn(NULL);
	}
	*/
}

void CStaticLaser::OnRButtonDown(UINT nFlags, CPoint pt)
{
	CWnd::OnRButtonDown(nFlags, pt);
	CMenu menu;
	menu.LoadMenuA(IDR_MENU_POPUP);

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup);

	m_ptMouse = pt;
	ClientToScreen(&pt);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
}

void CStaticLaser::OnLButtonDblClk(UINT nFlags, CPoint pt)
{
	CWnd::OnLButtonDblClk(nFlags, pt);
}

void CStaticLaser::OnMenu(UINT nID)
{
	switch (nID)
	{
	case ID_POPUP_SETLOCATION:
		SetCrawlerLocation(m_ptMouse);
		break;
	}
}


int CStaticLaser::GetMagStatus(int nStatus)
{
	if (m_pParent && ::IsWindow(m_pParent->m_hWnd))
	{
		int stat = INT_MAX;
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::STATUS_MAG_STATUS + nStatus, (WPARAM)&stat);
		return stat;
	}
	else
		return INT_MAX;
}

double CStaticLaser::GetPipeCircumference()
{
	if (m_pParent && ::IsWindow(m_pParent->m_hWnd))
	{
		double circ = 0;
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::STATUS_GET_CIRC, (WPARAM)&circ);
		return circ;
	}
	else
		return FLT_MAX;
}

double CStaticLaser::GetCrawlerLocation()
{
	if (m_pParent && ::IsWindow(m_pParent->m_hWnd))
	{
		double loc = 0;
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::STATUS_GETLOCATION, (WPARAM)&loc);
		return loc;
	}
	else
		return FLT_MAX;
}


