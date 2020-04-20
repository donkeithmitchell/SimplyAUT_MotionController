// StaticLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "StaticMag.h"
#include "LaserControl.h"
#include "MagController.h"
#include "afxdialogex.h"

static double PI = 4 * atan(1.0); 

// CStaticMag dialog

IMPLEMENT_DYNAMIC(CStaticMag, CWnd)

CStaticMag::CStaticMag(CMagControl& mag, const BOOL& bScanLaser, const BOOL& bLackLine, const double& length)
	: CWnd()
	, m_magControl(mag)
	, m_bScanLaser(bScanLaser)
	, m_bSeekBlackLine(bLackLine)
	, m_fCalibrationLength(length)
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_disp_rect = CRect(0, 0, 0, 0);
}

CStaticMag::~CStaticMag()
{
}

void CStaticMag::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}



void CStaticMag::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStaticMag, CWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


// CStaticMag message handlers
void CStaticMag::Create(CWnd* pParent)
{
	m_pParent = pParent;
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0,0, 0,0), pParent, 1, NULL);
	ASSERT(m_hWnd != NULL);

	EnableWindow(TRUE);
	InvalidateRgn(NULL);
}

void CStaticMag::OnPaint()
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
	DrawRGBProfile(&memDC);
	//	m_wndLaserProfile.InvalidateRgn(NULL);
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
}

void CStaticMag::OnSize(UINT nFlag, int cx, int cy)
{
	CWnd::OnSize(nFlag, cx, cy);

	GetRGBRect(&m_disp_rect);
}


void CStaticMag::GetRGBRect(CRect* rect)
{
	GetClientRect(rect);

	CDC* pDC = GetDC();
	CSize sz = pDC->GetTextExtent("8888");
	int cy = rect->Height();
	rect->bottom -= 3*sz.cy/2;
	rect->top += sz.cy;
	rect->left += sz.cx + 5;
	rect->right -= sz.cy;
	ReleaseDC(pDC);
}

void CStaticMag::DrawRGBProfile(CDC* pDC)
{
	CString str;
	CRect rect, client;
	GetClientRect(&client);
	int cx = client.Width();
	int cy = client.Height();

	if (!m_magControl.IsConnected())
		return;

	int len = (int)m_magControl.GetRGBCalibrationDataSize();
	if (len == 0)
		return;

	CDoublePoint pt1 = m_magControl.GetRGBCalibrationData(0);
	CDoublePoint pt2 = m_magControl.GetRGBCalibrationData(len-1);
	BOOL bScanReverse = (pt1.x > pt2.x);

	// get a copy of the data to filter
	/////////////////////////////////////
	CArray<double, double> X2, Y2;
	X2.SetSize(len);
	Y2.SetSize(len);
	for (int i = 0; i < len; ++i)
	{
		CDoublePoint pt = m_magControl.GetRGBCalibrationData(i);
		X2[i] = pt.x;
		Y2[i] = pt.y;
	}

	// high-cut the response
	// only if using the laser, it is very spiky
	// also filter with a +/- 1 median (i.e. over 3 samples), this removed any laser simgle sample spikes
	CIIR_Filter filter(len);
	if (len >= 5 && m_bScanLaser )
	{
		filter.MedianFilter(Y2.GetData(), 1);
		filter.AveragingFilter(Y2.GetData(), 2);
	}

	GetRGBRect(&rect);

	// draw a scale on the bottom
	int y1 = rect.bottom;
	double pos1 = bScanReverse ? X2[0] : 0;

	CPen pen1(PS_SOLID, 0, RGB(250, 250, 250));
	pDC->SelectObject(&pen1);

	pDC->SetTextColor(RGB(250, 250, 250));
	pDC->SetBkMode(TRANSPARENT);

	double minX = FLT_MAX;
	double maxX = -FLT_MAX;
	double minY = 0;
	double maxY = 0;
	for (int i = 0; i < len; ++i)
	{
		minX = min(minX, X2[i]);
		maxX = max(maxX, X2[i]);
		minY = min(minY, Y2[i]);
		maxY = max(maxY, Y2[i]);
	}

	// round this out to 20
	minX = (minX > 0) ? (int)(minX + 0.5) : (int)(minX - 0.5);
	int rem = (int)fabs(minX) % 20;
	minX -= (rem > 0) ? (rem) : 0;

	maxX = (maxX > 0) ? (int)(maxX + 0.5) : (int)(maxX - 0.5);
	rem = (int)fabs(maxX) % 20;
	maxX += (rem > 0) ? (20 - rem) : 0;

	// round this out to 20
	minY = (minY > 0) ? (int)(minY + 0.5) : (int)(minY - 0.5);
	rem = (int)minY % 20;
	minY -= (rem > 0) ? (rem) : 0;

	// round this out to 20
	maxY = (maxY > 0) ? (int)(maxY + 0.5) : (int)(maxY - 0.5);
	rem = (int)maxY % 20;
	maxY += (rem > 0) ? (20 - rem) : 0;

	// scale to fit the run length
	// scale from 0 -> 100
	if(bScanReverse)
		minX = min(minX, maxX - m_fCalibrationLength);
	else
		maxX = max(maxX, minX + m_fCalibrationLength);

	double disp_width_factor = ((double)m_disp_rect.Width()) / (maxX-minX);
	double disp_height_factor = ((double)m_disp_rect.Height()) / (maxY-minY);


	pDC->MoveTo(rect.left, y1);
	pDC->LineTo(rect.right, y1);
	int tick = 10;
	int nticks = (int)(maxX-minX) / tick;
	while (nticks > 10)
	{
		tick *= 2;
		nticks = (int)(maxX-minX) / tick;
	}

	for (double pos = minX; pos <= maxX; pos += tick)
	{
		int x = (int)(rect.left + (pos-minX)*disp_width_factor + 0.5);
		pDC->MoveTo(x, y1 - 5);
		pDC->LineTo(x, y1 + 5);

		str.Format("%.0f", pos);
		CSize sz = pDC->GetTextExtent(str);
		pDC->TextOut(x - sz.cx / 2, y1, str);
	}

	pDC->MoveTo(rect.left, rect.bottom);
	pDC->LineTo(rect.left, rect.top);
	tick = 5;
	nticks = (int)maxY / tick;
	while (nticks > 5)
	{
		tick *= 2;
		nticks = (int)maxY / tick;
	}

	for (int rgb = 0; rgb <= maxY; rgb += tick)
	{
		int y = (int)(rect.bottom - rgb * disp_height_factor + 0.5);

		pDC->MoveTo(rect.left-5, y);
		pDC->LineTo(rect.left+5, y);

		str.Format("%d", rgb);
		CSize sz = pDC->GetTextExtent(str);
		pDC->TextOut(rect.left - sz.cx - 5, y-sz.cy/2, str);
	}

	CPen pen2(PS_SOLID, 0, RGB(10, 250, 10));
	pDC->SelectObject(&pen2);
	pDC->SetBkMode(TRANSPARENT);

	for (int i = 0; i < len; ++i)
	{
		int x = (int)(rect.left + (X2[i]-minX - pos1) * disp_width_factor + 0.5);
		int y = (int)(rect.bottom - (Y2[i]-minY) * disp_height_factor + 0.5);
		(i == 0) ? pDC->MoveTo(x, y) : pDC->LineTo(x, y);
	}

	// draw a vertical line at thew noted minimum
	double pos2 = m_magControl.GetRGBCalibration().pos;
	if (pos2 != FLT_MAX)
	{
		CPen pen3(PS_DASH, 0, RGB(10, 250, 10));
		pDC->SelectObject(&pen3);
		
		int x = (int)(rect.left + (pos2-minX - pos1) * disp_width_factor + 0.5);
		int y = (int)(rect.bottom - m_magControl.GetRGBCalibration().rgb * disp_height_factor + 0.5);

		pDC->MoveTo(x, rect.bottom);
		pDC->LineTo(x, rect.top);

		pDC->MoveTo(rect.left, y);
		pDC->LineTo(rect.right, y);
	}

	// note the locatiopn of the crawler
	double pos3 = m_magControl.GetRGBCalibrationPos();
	double median = m_magControl.GetRGBCalibration().median;

	if( pos3 != FLT_MAX && median != FLT_MAX )
	{
		int radius = 5;
		CPen pen4(PS_SOLID, 0, RGB(10, 255, 255));
		pDC->SelectObject(&pen4);
		CBrush brush4( RGB(10, 255, 255));
		pDC->SelectObject(&brush4);

		int x = (int)(rect.left + (pos3-minX - pos1) * disp_width_factor + 0.5);
		int y = (int)(rect.bottom - median *disp_height_factor + 0.5);
		pDC->Ellipse(x - radius, y - radius, x + radius, y + radius);
	}

}

void CStaticMag::OnTimer(UINT_PTR nIDEvent)
{
	CWnd::OnTimer(nIDEvent);
}

// this indicates that start liune seen in graph and to stop look,ing
void CStaticMag::OnLButtonDblClk(UINT nFlags, CPoint pt)
{
	CWnd::OnLButtonDblClk(nFlags, pt);
	if (m_pParent != NULL && IsWindow(m_pParent->m_hWnd))
		m_pParent->PostMessageA(m_nMsg, 0);
}
