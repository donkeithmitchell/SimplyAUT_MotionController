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

CStaticMag::CStaticMag(CMagControl& mag, const double& length)
	: CWnd()
	, m_magControl(mag)
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

	GetRGBRect(&rect);

	// draw a scale on the bottom
	int y1 = rect.bottom;
	double pos1 = m_magControl.GetRGBCalibrationData(0).x;

	CPen pen1(PS_SOLID, 0, RGB(250, 250, 250));
	pDC->SelectObject(&pen1);

	pDC->SetTextColor(RGB(250, 250, 250));
	pDC->SetBkMode(TRANSPARENT);

	double maxVal = 0;
	for (int i = 0; i < len; ++i)
		maxVal = max(m_magControl.GetRGBCalibrationData(i).y, maxVal);

	// round this out to 20
	int rem = (int)maxVal % 20;
	maxVal += (rem > 0) ? (20 - rem) : 0;

	// scale to fit the run length
	// scale from 0 -> 100
	double disp_width_factor = ((double)m_disp_rect.Width()) / m_fCalibrationLength;
	double disp_height_factor = ((double)m_disp_rect.Height()) / maxVal;


	pDC->MoveTo(rect.left, y1);
	pDC->LineTo(rect.right, y1);
	int tick = 10;
	int nticks = (int)m_fCalibrationLength / tick;
	while (nticks > 10)
	{
		tick *= 2;
		nticks = (int)m_fCalibrationLength / tick;
	}

	for (double pos = 0; pos < m_fCalibrationLength; pos += tick)
	{
		int x = (int)(rect.left + (pos)*disp_width_factor + 0.5);
		pDC->MoveTo(x, y1 - 5);
		pDC->LineTo(x, y1 + 5);

		str.Format("%.0f", pos);
		CSize sz = pDC->GetTextExtent(str);
		pDC->TextOut(x - sz.cx / 2, y1, str);
	}

	pDC->MoveTo(rect.left, rect.bottom);
	pDC->LineTo(rect.left, rect.top);
	tick = 5;
	nticks = (int)maxVal / tick;
	while (nticks > 5)
	{
		tick *= 2;
		nticks = (int)maxVal / tick;
	}

	for (int rgb = 0; rgb <= maxVal; rgb += tick)
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
		int x = (int)(rect.left + (m_magControl.GetRGBCalibrationData(i).x - pos1) * disp_width_factor + 0.5);
		int y = (int)(rect.bottom - m_magControl.GetRGBCalibrationData(i).y * disp_height_factor + 0.5);
		(i == 0) ? pDC->MoveTo(x, y) : pDC->LineTo(x, y);
	}

	// draw a vertical line at thew noted minimum
	double pos2 = m_magControl.GetRGBCalibration().pos;
	if (pos2 != FLT_MAX)
	{
		CPen pen3(PS_DASH, 0, RGB(10, 250, 10));
		pDC->SelectObject(&pen3);
		
		int x = (int)(rect.left + (pos2 - pos1) * disp_width_factor + 0.5);
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

		int x = (int)(rect.left + (pos3 - pos1) * disp_width_factor + 0.5);
		int y = (int)(rect.bottom - median *disp_height_factor + 0.5);
		pDC->Ellipse(x - radius, y - radius, x + radius, y + radius);
	}

}

void CStaticMag::OnTimer(UINT_PTR nIDEvent)
{
}
