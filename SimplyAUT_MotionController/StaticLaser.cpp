// StaticLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "StaticLaser.h"
#include "LaserControl.h"
#include "afxdialogex.h"

static double PI = 3.14159265358979323846;


// CStaticLaser dialog

IMPLEMENT_DYNAMIC(CStaticLaser, CWnd)

CStaticLaser::CStaticLaser(CLaserControl& laser)
	: CWnd()
	, m_laserControl(laser)
	, m_ptMouse(INT_MAX, INT_MAX)
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_fHomeAng = 0;
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
		CBrush brush2(RGB(0, 0, 0));
		CBrush* pBrush2 = pDC->SelectObject(&brush2);
		pDC->Ellipse(x1 - 5, y1 - 5, x1 + 5, y1 + 5);
	}


	pDC->SelectObject(pBrush1);
	pDC->SelectObject(pPen);
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


// draw the profile returned by the laser within the above circle
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

double CStaticLaser::GetPipeCircumference()
{
	if (m_pParent && ::IsWindow(m_pParent->m_hWnd))
	{
		double circ = 0;
		m_pParent->SendMessage(m_nMsg, STATUS_GET_CIRC, (WPARAM)&circ);
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
		m_pParent->SendMessage(m_nMsg, STATUS_GETLOCATION, (WPARAM)&loc);
		return loc;
	}
	else
		return FLT_MAX;
}


