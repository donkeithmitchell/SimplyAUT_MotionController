// StaticLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "StaticLaser.h"
#include "LaserControl.h"
#include "afxdialogex.h"


// CStaticLaser dialog

IMPLEMENT_DYNAMIC(CStaticLaser, CWnd)

CStaticLaser::CStaticLaser(CLaserControl& laser)
	: CWnd()
	, m_laserControl(laser)
{
	m_pParent = NULL;
}

CStaticLaser::~CStaticLaser()
{
}



void CStaticLaser::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStaticLaser, CWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CStaticLaser message handlers
void CStaticLaser::Create(CWnd* pParent)
{
	m_pParent = pParent;
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0,0, 0,0), pParent, 1, NULL);
	ASSERT(m_hWnd != NULL);

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
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
}

// draw a circle with the current location of the crawler noted on the circle
// also note the SET home location and the line detected by the RGB sensor
void CStaticLaser::DrawCrawlerLocation(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);

	CBrush brush(::GetSysColor(COLOR_3DFACE));
	pDC->FillRect(&rect, &brush);

	CGdiObject* pBrush = pDC->SelectStockObject(HOLLOW_BRUSH);
	CPen pen(PS_SOLID, 0, RGB(0, 0, 0));
	CPen* pPen = pDC->SelectObject(&pen);

	pDC->Ellipse(&rect);

	pDC->SelectObject(pBrush);
	pDC->SelectObject(pPen);
}

// draw the profile returned by the lasr within the above circle
void CStaticLaser::DrawLaserProfile(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);

	CPen pen(PS_SOLID, 0, RGB(0, 0, 0));
	CPen* pPen = pDC->SelectObject(&pen);

	// draw the horizontal axis across the middle
	int x1 = rect.left;
	int x2 = rect.right;
	int y = (rect.top + rect.bottom) / 2;

	pDC->MoveTo(x1, y);
	pDC->LineTo(x2, y);

	pDC->SelectObject(pPen);
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

