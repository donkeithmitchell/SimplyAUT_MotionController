#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "Button.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// this class wasx originally used to tell the parent dfialoog, that a button had the mouse clicked on it
// it is no longer used, with a slider control rep[lacing it
CMyButton::CMyButton()
	: CButton()
{
}

CMyButton::~CMyButton()
{

}

BEGIN_MESSAGE_MAP(CMyButton, CButton)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

void CMyButton::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}


void CMyButton::OnSize(UINT nFlag, int cx, int cy)
{
	CButton::OnSize(nFlag, cx,cy);
}

void CMyButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	CButton::OnLButtonDown(nFlags, point);
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->PostMessageA(m_nMsg, 1);
}

void CMyButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	CButton::OnLButtonUp(nFlags, point);
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->PostMessageA(m_nMsg, 0);
}

