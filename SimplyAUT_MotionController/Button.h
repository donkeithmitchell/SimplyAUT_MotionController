#pragma once
class CMyButton : public CButton
{
//	DECLARE_DYNAMIC(CMyButton)

public:
	CMyButton();
	virtual ~CMyButton();

	void Init(CWnd*, UINT);

	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

protected:

	CWnd*	m_pParent;
	UINT	m_nMsg;

	DECLARE_MESSAGE_MAP()
};