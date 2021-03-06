// StaticLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "StaticLaser.h"
#include "LaserControl.h"
#include "MagController.h"
#include "afxdialogex.h"
#include "mmsystem.h"

static double PI = 4 * atan(1.0); 

#ifdef _DEBUG_TIMING_
clock_t g_LaserOnPaintTime = 0;
int g_LaserOnPaintCount = 0;
#endif

#define DISP_MARGIN 5

static UINT PlaySoundThread(void* param)
{
	CStaticLaser* this2 = (CStaticLaser*)param;
	return this2->PlaySoundThread();
}

// CStaticLaser window
// this window draws the laser profile, weld cap offset and scan progress report

IMPLEMENT_DYNAMIC(CStaticLaser, CWnd)

CStaticLaser::CStaticLaser(CMotionControl& motion, CLaserControl& laser, CMagControl& mag, CWeldNavigation& nav, const double& fLength)
	: CWnd()
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_fScanLength(fLength)
	, m_weldNavigation(nav)
	, m_ptMouse(INT_MAX, INT_MAX)
	, m_bCentreWeld(TRUE)
	, m_bPlayOffsetSound(FALSE)
{
	m_fHomeAng = -PI / 2.0;
	m_fHomeAngDiff = 0;
	m_disp_width_factor = 1;
	m_disp_height_factor = 1;
	m_disp_height_min = 0;
	m_disp_height_max = SENSOR_HEIGHT;
	m_disp_rect = CRect(0, 0, 0, 0);
	m_rgbSum = 0;
	m_rgbCount = 0;
	m_hPlaySoundThread = NULL;
	m_gap = FLT_MAX;
}

CStaticLaser::~CStaticLaser()
{
	m_bPlayOffsetSound = FALSE;
	if (m_hPlaySoundThread)
		WaitForSingleObject(m_hPlaySoundThread, INFINITE);
}


void CStaticLaser::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStaticLaser, CWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_MESSAGE(WM_USER_PLAY_FINISHEFD, OnUserMessagePlayFinished)
	ON_COMMAND_RANGE(ID_POPUP_TOGGLELASER, ID_POPUP_TOGGLELASER, OnMenu)
	ON_COMMAND_RANGE(ID_POPUP_CENTREWELD, ID_POPUP_CENTREWELD, OnMenu)
	ON_COMMAND_RANGE(ID_POPUP_PLAYOFFSETSOUND, ID_POPUP_PLAYOFFSETSOUND, OnMenu)
END_MESSAGE_MAP()


// CStaticLaser message handlers
void CStaticLaser::Create(CWnd* pParent)
{
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0,0, 0,0), pParent, 1, NULL);
	ASSERT(m_hWnd != NULL);

	EnableWindow(TRUE);
	InvalidateRgn(NULL);
}

void CStaticLaser::OnPaint()
{
	clock_t t1 = clock();

	CRect rect;
	CPaintDC dc(this);
	GetClientRect(&rect);

	// use bitblt so faster, and so that do not see drawing
	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	ASSERT(memDC.m_hDC);

	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());

	CBitmap* pBitmap = memDC.SelectObject(&bitmap);
	DrawCrawlerLocation(&memDC);
	DrawLaserProfile(&memDC);
	////	DrawRGBProfile(&memDC);
	DrawLaserOffset(&memDC);
	//	m_wndLaserProfile.InvalidateRgn(NULL);
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);

#ifdef _DEBUG_TIMING_
	g_LaserOnPaintTime += clock() - t1;
	g_LaserOnPaintCount++;
#endif
}

void CStaticLaser::OnSize(UINT nFlag, int cx, int cy)
{
	CWnd::OnSize(nFlag, cx, cy);

	GetLaserRect(&m_disp_rect);
	m_disp_width_factor = ((double)m_disp_rect.Width()) / (double)SENSOR_WIDTH;
}

CPoint CStaticLaser::GetScreenPixel(double x, double y)
{
	CPoint ret;
	ret.x = m_disp_rect.left + int(x * m_disp_width_factor + 0.5);
	ret.y = m_disp_rect.bottom - int((y - m_disp_height_min) * m_disp_height_factor + 0.5);

	return ret;
}

// draw a circle with the current location of the crawler noted on the circle
// also note the SET home location and the line detected by the RGB sensor
// this acts as a progress report for a scan
// 911 the actual position can be set, but not required
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
	double radius2 = m_fScanLength / (2 * PI);
	double scale = (double)radius1 / radius2;
	pDC->Ellipse(&rect2);

	// get this location
	int x0 = (rect2.right + rect2.left) / 2;
	int y0 = (rect2.top + rect2.bottom) / 2;

	// now note the location of the crawler
	int circum1 = (int)(2 * PI * radius1 + 0.5);
	int nMagOn = m_magControl.GetMagStatus(MAG_IND_MAG_ON);

	// note where home is
#ifdef _DEBUG_TIMING_
	int x3 = (int)(x0 + (radius1) * sin(PI / 2 - m_fHomeAng) + 0.5);
	int y3 = (int)(y0 + (radius1) * cos(PI / 2 - m_fHomeAng) + 0.5);

	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->SetBkMode(TRANSPARENT);
	CSize sz = pDC->GetTextExtent("H");
	pDC->TextOutA(x3 - sz.cx / 2, y3 - sz.cy / 2, "H");
#else
	CPen pen3(PS_SOLID, 2, RGB(0, 0, 0));
	pDC->SelectObject(&pen3);
	pDC->MoveTo(x0, rect2.top - 5);
	pDC->LineTo(x0, rect2.top + 5);
	pDC->MoveTo(x0, rect2.bottom - 5);
	pDC->LineTo(x0, rect2.bottom + 5);
	pDC->MoveTo(rect2.left-5, y0);
	pDC->LineTo(rect2.left+5, y0);
	pDC->MoveTo(rect2.right-5, y0);
	pDC->LineTo(rect2.right+5, y0);
#endif

	// now draw the crawler at an angle determine by the start angle and the progression
	double location2 = m_motionControl.GetAvgMotorPosition();
	if (location2 != FLT_MAX)
	{
		double ang = (m_fScanLength  <= 1.0 ) ? -PI/2.0 : -(2 * PI * location2 / m_fScanLength) - PI / 2 + m_fHomeAngDiff;

		int x1 = (int)(x0 + radius1 * cos(ang) + 0.5);
		int y1 = (int)(y0 + radius1 * sin(ang) + 0.5);

		// draw a filled dot at this location
		// red if magnets not engaged, else greenm
		CBrush brush2(nMagOn == 1 ? RGB(0, 255, 0) : RGB(255, 0, 0));
		CPen pen2(PS_SOLID, 0, (nMagOn == 1 ? RGB(0, 255, 0) : RGB(255, 0, 0)));
		CPen* pPen2 = pDC->SelectObject(&pen2);
		CBrush* pBrush2 = pDC->SelectObject(&brush2);
		pDC->Ellipse(x1 - 5, y1 - 5, x1 + 5, y1 + 5);
	}
}

// the popup menu and double clicking can toggle the laser on/off
void CStaticLaser::ToggleLaserOn()
{
	m_laserControl.TurnLaserOn(!m_laserControl.IsLaserOn());
	InvalidateRgn(NULL);
}

// this is added to set the green dot in the correct location
// not documented as not rewuired 911
void CStaticLaser::SetCrawlerLocation(CPoint pt)
{
	// get the angle of pt from the centre
	CRect rect;
	GetPipeRect(&rect);

	// get the angle from the centre and add 90 deg
	double x0 = ((double)rect.left + (double)rect.right) / 2.0;
	double y0 = ((double)rect.top + (double)rect.bottom) / 2.0;


	// this is the angle that the dot is now at
	double location2 = m_motionControl.GetAvgMotorPosition();
	double fCurrAng = -(2 * PI * location2 / m_fScanLength) - PI / 2;

	// this is the angle that wish to draw at
	m_fHomeAng = atan2((double)(pt.y) - y0, (double)(pt.x) - x0); //  +PI / 2;

	// note the difference in the two
	m_fHomeAngDiff = m_fHomeAng - fCurrAng;

	InvalidateRgn(NULL);
}

// this is the portion of the window where to place the laser profile
void CStaticLaser::GetLaserRect(CRect* rect)
{
	// positioon in the bottom half and in the centre half of that
	GetPipeRect(rect);

	// push in from the pipe circle
	int x1 = rect->left + rect->Width() / 10;
	int x2 = rect->right - rect->Width() / 10;
	int y1 = rect->top + rect->Height() / 10;
	int y2 = rect->top + 3*rect->Height()/4;
	rect->SetRect(x1, y1, x2, y2);
}

// the laser offset is the line drawen below the laser profile
// showing the offset from the weld-cap by crawler position
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


// this is the location to draw the scan progrtess (dircle which emulates a pipe)
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

// not used at this time
// but was for a graph of the RGB values
// 911 getting them is too slow
// consider adding back in if fix the time to get the RGB sum
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

// part of drawing the RGB values
double CStaticLaser::GetAverageRGBValue()
{
	return (m_rgbCount < 0) ? (double)m_rgbSum / (double)m_rgbCount : 0;
}

// 911 not used at this time, as too slow to get during a scan
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
	BOOL bPresent = m_magControl.GetMagStatus(MAG_IND_RGB_LINE) == 1;
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

	double scaleX = (double)rect.Width() / 250.0;
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

// keep a list of the RGB values so can display them (not used at this time)
int CStaticLaser::AddRGBData(const int& value)
{
	if (value != INT_MAX)
	{
		// for now
		m_rgbData.Add(value);
		m_rgbSum += value;
		m_rgbCount++;

		while (m_rgbData.GetSize() > 250)
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
// enable the user to visually see if the crawler is staying close to the weld
void CStaticLaser::DrawLaserOffset(CDC* pDC)
{
	if (!m_laserControl.IsLaserOn())
		return;

	// build an array of offsets and position
	LASER_POS lastPos = m_weldNavigation.GetLastNotedPosition(0);

	m_critLaserPos1.Lock();
	if (lastPos.gap_filt1 != FLT_MAX)
		m_laserPos1.Add(CDoublePoint(lastPos.measures.measure_pos_mm, lastPos.gap_filt1));

	if (m_laserPos1.GetSize() == 0)
	{
		m_critLaserPos1.Unlock();
		return;
	}

	double minGap = FLT_MAX;
	double maxGap = -FLT_MAX;
	double minPos = FLT_MAX;
	double maxPos = -FLT_MAX;

	for (int i = 0; i < m_laserPos1.GetSize(); ++i)
	{
		minGap = min(minGap, m_laserPos1[i].y);
		maxGap = max(maxGap, m_laserPos1[i].y);

		minPos = min(minPos, m_laserPos1[i].x);
		maxPos = max(maxPos, m_laserPos1[i].x);
	}

	CRect rect;
	GetOffsetRect(&rect);

	if (minPos == maxPos || rect.Width() <= 0 || rect.Height() <= 0)
	{
		m_critLaserPos1.Unlock();
		return;
	}

	minGap = min(minGap, -1);
	maxGap = max(maxGap, 1);

	double scaleX = (double)rect.Width() / m_fScanLength; //  (maxPos - minPos);
	double scaleY = (double)rect.Height() / (maxGap - minGap);

	// only draw one segment per 2 pixels
	// average the hits if greater resolution
	int cx = m_disp_rect.Width() / 2;
	int smooth = ((int)m_fScanLength + cx - 1) / cx;

	CPen pen(PS_SOLID, 0, RGB(250, 10, 10));
	pDC->SelectObject(&pen);
	int init = 1;
	for (int i = 0; i < m_laserPos1.GetSize(); i += smooth)
	{
		double sum1 = 0;
		double sum2 = 0;
		int cnt = 0;
		for (int j = 0; j < smooth && i + j < m_laserPos1.GetSize(); ++j)
		{
			sum1 += m_laserPos1[i + j].x;
			sum2 += m_laserPos1[i + j].y;
			cnt++;
		}
		if( cnt > 0 )
		{
			int x = rect.left + (int)((sum1/cnt - minPos) * scaleX + 0.5);
			int y = rect.bottom - (int)((sum2/cnt - minGap) * scaleY + 0.5);
			(init) ? pDC->MoveTo(x, y) : pDC->LineTo(x, y);
			init = 0;
		}
	}

	m_critLaserPos1.Unlock();
}


// draw the profile returned by the laser within the above circle
void CStaticLaser::DrawLaserProfile(CDC* pDC)
{
	// if the laswr if OFF, then just use the saved profile
	BOOL bOn = m_laserControl.IsLaserOn();
	if( bOn )
		GetLaserProfile();

	MT_Hits_Pos hits[SENSOR_WIDTH];
	double hitBuffer[SENSOR_WIDTH];
	LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();
	m_laserControl.GetLaserHits(hits, hitBuffer, SENSOR_WIDTH);

	CPen PenBlack(PS_SOLID, 0, RGB(0, 0, 0));
	COLORREF colourCrawler = RGB(10, 250, 250);
	CPen PenCrawler(PS_DASHDOT, 1, colourCrawler);
	CBrush BrushCrawler(colourCrawler);

	pDC->SelectObject(&PenBlack);
	pDC->MoveTo(m_disp_rect.left, m_disp_rect.bottom);
	pDC->LineTo(m_disp_rect.right, m_disp_rect.bottom);

	if (!bOn)
	{
		pDC->SetTextColor(RGB(255, 0, 0));
		pDC->SetBkMode(TRANSPARENT);
		CString text("Laser Off");
		CSize sz = pDC->GetTextExtent(text);
		pDC->TextOutA((m_disp_rect.left + m_disp_rect.right) / 2 - sz.cx / 2, m_disp_rect.bottom + sz.cy/2, text);
	}

	// fit the lasewr display to the screen
	m_disp_height_max = 0;
	m_disp_height_min = SENSOR_HEIGHT;
	for (int i = 0; i < SENSOR_WIDTH; i++)
	{
		MT_Hits_Pos val = hits[i];
		if (val>= 1 && val <= SENSOR_HEIGHT-1)
		{
			m_disp_height_min = min(m_disp_height_min, (double)val);
			m_disp_height_max = max(m_disp_height_max, (double)val);
		}
	}
//	m_disp_width_factor = ((double)m_disp_rect.Width()) / (double)SENSOR_WIDTH; // set in OnSize()
	m_disp_height_factor = ((double)m_disp_rect.Height()) / (m_disp_height_max- m_disp_height_min);

	// measure2.weld_cap_pix2.x is shifted to the centre (i.e the weld does not move, the crawler does)
	// the popup menu can disable this
	int shift = m_bCentreWeld ? (int)(SENSOR_WIDTH / 2 - measure2.weld_cap_pix2.x) : 0;

	// only draw one segment per 2 pixels
	// average the hits if greater resolution
	int cx = m_disp_rect.Width() / 2;
	int smooth = (SENSOR_WIDTH + cx - 1) / cx;

	int init = 1;
	for (int i = 0; i < SENSOR_WIDTH; i += smooth)
	{
		double sum1 = 0;
		double sum2 = 0;
		int cnt2 = 0;
		for (int j = 0; j < smooth && i + j < SENSOR_WIDTH; ++j)
		{
			MT_Hits_Pos val = hits[i + j];
			if (val >= 1 && val < SENSOR_HEIGHT)
			{
				sum1 += (double)i + (double)j;
				sum2 += (double)val;
				cnt2++;
			}
		}
		if (cnt2 > 0)
		{
			// MioveTo(), LineTo() is much faster than SetPixel()
			// as scatter has been filtered out, no need tfor SetPixel()
			CPoint pt = GetScreenPixel((double)(sum1/cnt2 + shift), sum2/cnt2);
			(init) ? pDC->MoveTo(pt) : pDC->LineTo(pt); // thbuis is faster, scatter has been removed regardless
			init = 0;
//			pDC->SetPixel(pt, RGB(250, 50, 50)); // was better when showed scatter
		}

	}

	// draw a dot to represent the scanner
	pDC->SelectObject(&PenCrawler);
	pDC->SetROP2(R2_COPYPEN);
	pDC->SelectObject(&BrushCrawler);
	pDC->SelectObject(&PenCrawler);
	double y1 = max(measure2.GetDnSideWeldHeight(), measure2.GetUpSideWeldHeight());
	double y2 = (y1 + measure2.weld_cap_pix2.y) / 2;
	CPoint pt1 = GetScreenPixel(SENSOR_WIDTH-measure2.weld_cap_pix2.x + shift, y2);
	pDC->Ellipse(pt1.x - 5, pt1.y - 5, pt1.x + 5, pt1.y + 5);
	pDC->MoveTo(pt1.x, pt1.y - 10);
	pDC->LineTo(pt1.x, pt1.y + 10);

	// now annotate the gap to the opposite side equal height of the craw3erl dot
	CString text;
	double h1_mm, v1_mm;
	if (m_laserControl.ConvPixelToMm((int)measure2.weld_cap_pix2.x, (int)measure2.weld_cap_pix2.y, h1_mm, v1_mm))
	{
		text.Format("%.1f", fabs(h1_mm));
		pDC->SetTextColor(RGB(10, 10, 10));
		pDC->SetBkMode(TRANSPARENT);

		CSize sz = pDC->GetTextExtent(text);

		int x0 = (m_disp_rect.left + m_disp_rect.right) / 2;
		if (pt1.x > x0 / 2)
			pDC->TextOutA((m_disp_rect.left + x0) / 2 - sz.cx, (m_disp_rect.bottom + m_disp_rect.top) / 2, text);
		else
			pDC->TextOutA((x0 + m_disp_rect.right) / 2, (m_disp_rect.bottom + m_disp_rect.top) / 2, text);
	}

	// draw a vertical line at the weld centre
	// the values are in (mm) not laser pixels
	CPoint pt = GetScreenPixel(measure2.weld_cap_pix2.x+shift, measure2.weld_cap_pix2.y);
	CPen penWeldCentre(PS_SOLID, 0, RGB(10, 255, 10));
	pDC->SelectObject(&penWeldCentre);
	pDC->MoveTo(pt.x, m_disp_rect.bottom);
	pDC->LineTo(pt.x, m_disp_rect.top);

	CPoint pt11 = GetScreenPixel((double)((double)measure2.weld_left_pix+ (double)shift) / 2.0, measure2.GetDnSideStartHeight());
	CPoint pt12 = GetScreenPixel((double)((double)measure2.weld_left_pix+ (double)shift), measure2.GetDnSideWeldHeight());
	CPen PenPrimaryEdge(PS_SOLID, 1, RGB(10, 10, 250));
	pDC->SelectObject(&PenPrimaryEdge);
	pDC->MoveTo(pt11.x, pt11.y);
	pDC->LineTo(pt12.x, pt12.y);

	CPoint pt21 = GetScreenPixel((double)((double)measure2.weld_right_pix+ (double)shift), measure2.GetUpSideWeldHeight());
	CPoint pt22 = GetScreenPixel((double)((double)measure2.weld_right_pix+ (double)shift + SENSOR_WIDTH) / 2.0, measure2.GetUpSideEndHeight());
	pDC->MoveTo(pt21.x, pt21.y);
	pDC->LineTo(pt22.x, pt22.y);
}

// 911 these timers are not set at this time
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
	case TIMER_PLAYSOUND:
		PlayOffsetSound();
		break;
	}
}

// check if the laser is on and 
void CStaticLaser::PlayOffsetSound()
{
	if (!m_bPlayOffsetSound)
		return;

	LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();
	double gap1 = measure2.weld_cap_mm.x;
	if (gap1 == FLT_MAX)
		return;

	m_gap = min(fabs(gap1), 20.0);

	BOOL bMag = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;
	BOOL bOn = m_laserControl.IsLaserOn();

	if (!bMag && bOn && m_hPlaySoundThread == NULL)
		m_hPlaySoundThread = AfxBeginThread(::PlaySoundThread, this);
}

UINT CStaticLaser::PlaySoundThread()
{
	int srate = 10000;
	int tone = 300;
	CArray<char, char> wav_vector;
	CArray<float, float> trace;
	trace.SetSize(10 * srate);

	for (int j = 0; j < trace.GetSize(); ++j)
		trace[j] = (float)sin(2 * PI * tone * j / srate);

	::ConvertToWavFormat(trace, srate, 250, wav_vector);

	BOOL bPlaySound = FALSE;
	HINSTANCE hInstance = GetModuleHandle(0);
	while (m_bPlayOffsetSound)
	{
		BOOL bMag = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;
		if (bMag)
			break;

		// if tyh gap is less than 0.25 mm, then play a continuous tone
		if (m_gap > 0.5)
		{
			if(bPlaySound )
				::PlaySound(NULL, NULL, 0);

			bPlaySound = FALSE;
			// the closer the faster
			int sleep = (int)((m_gap) * 20 + 0.5);
			int freq = (int)((20 - m_gap) * 25 + 0.5);
			Beep(freq, 250);
			Sleep(sleep);
		}
		else if( !bPlaySound )
		{
			bPlaySound = TRUE;
			::PlaySound(wav_vector.GetData(), hInstance, SND_MEMORY | SND_ASYNC | SND_LOOP);
			Sleep(1);
		}
	}
	if(bPlaySound )
		::PlaySound(NULL, NULL, 0);

	PostMessage(WM_USER_PLAY_FINISHEFD);
	return 0;
}

LRESULT CStaticLaser::OnUserMessagePlayFinished(WPARAM, LPARAM)
{
	m_hPlaySoundThread = NULL;
	return 0L;
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

	// noter the last measured laser values
	LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();
	if (measure2.status == 0)
	{
		m_joint_pos.x = measure2.weld_cap_mm.x;
		m_joint_pos.y = (measure2.weld_right_height_mm + measure2.weld_left_height_mm) / 2 - measure2.weld_cap_mm.y;

		m_edge_pos[0].x = measure2.weld_left_mm;
		m_edge_pos[0].y = measure2.weld_left_height_mm;

		m_edge_pos[1].x = measure2.weld_right_mm;
		m_edge_pos[1].y = measure2.weld_right_height_mm;

		m_edge_pos[2].x = m_edge_pos[1].x - m_edge_pos[0].x;
		m_edge_pos[2].y = m_edge_pos[1].y - m_edge_pos[0].y;
	}
	else
	{
		m_joint_pos.Reset();
		m_edge_pos[0].Reset();
		m_edge_pos[1].Reset();
		m_edge_pos[2].Reset();
	}
}

void CStaticLaser::OnMouseMove(UINT nFlags, CPoint pt)
{
	CWnd::OnMouseMove(nFlags, pt);
#ifdef _DEBUG_TIMING_
	if (nFlags & MK_LBUTTON)
		SetCrawlerLocation(pt);
#endif
}
void CStaticLaser::OnLButtonDown(UINT nFlags, CPoint pt)
{
	CWnd::OnLButtonDown(nFlags, pt);
}


// show a popup menu
void CStaticLaser::OnRButtonDown(UINT nFlags, CPoint pt)
{
	CWnd::OnRButtonDown(nFlags, pt);
}

void CStaticLaser::OnLButtonDblClk(UINT nFlags, CPoint pt)
{
	CWnd::OnLButtonDblClk(nFlags, pt);
	if (!m_motionControl.IsConnected() || m_motionControl.AreMotorsRunning())
		MessageBeep(MB_ICONASTERISK);
	else
		OnMenu(ID_POPUP_TOGGLELASER);
}

void CStaticLaser::PopupMenu(UINT nID)
{
	OnMenu(nID);
}

void CStaticLaser::UpdateMenu(CMenu* pMenu)
{
	ASSERT(pMenu);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STATE;

	mii.fState = MFS_DEFAULT;
	mii.fType |= MFT_RADIOCHECK;
	mii.fState = m_laserControl.IsLaserOn() ? MFS_CHECKED : MFS_UNCHECKED;
	if (!m_motionControl.IsConnected() || m_motionControl.AreMotorsRunning())
		mii.fState |= MFS_DISABLED;
	pMenu->SetMenuItemInfoA(ID_POPUP_TOGGLELASER, &mii, FALSE);

	mii.fState = m_bCentreWeld ? MFS_CHECKED : MFS_UNCHECKED;
	pMenu->SetMenuItemInfoA(ID_POPUP_CENTREWELD, &mii, FALSE);

	// onl;y use the sound if magnetds noit on and the laser is onb
	mii.fState = m_bPlayOffsetSound ? MFS_CHECKED : MFS_UNCHECKED;
	pMenu->SetMenuItemInfoA(ID_POPUP_PLAYOFFSETSOUND, &mii, FALSE);
}

void CStaticLaser::OnMenu(UINT nID)
{
	switch (nID)
	{
	case ID_POPUP_TOGGLELASER:
	{
		BOOL bRunning = m_motionControl.AreMotorsRunning();
		if (!bRunning)
		{
			BOOL bOn = !m_laserControl.IsLaserOn();
			ToggleLaserOn();
			if (!bOn)
				m_bPlayOffsetSound = FALSE;
		}
		break;
	}
	case ID_POPUP_CENTREWELD:
		m_bCentreWeld = !m_bCentreWeld;
		break;
	case ID_POPUP_PLAYOFFSETSOUND:
		m_bPlayOffsetSound = !m_bPlayOffsetSound;
		if (m_bPlayOffsetSound)
		{
			m_gap = FLT_MAX;
			SetTimer(TIMER_PLAYSOUND, 100, NULL);
		}
		else
			KillTimer(TIMER_PLAYSOUND);
		break;
	}
}

