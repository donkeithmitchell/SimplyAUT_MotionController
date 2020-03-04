#pragma once


// CDialogConnect dialog

class CDialogConnect : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogConnect)

public:
	CDialogConnect(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogConnect();

	void Create(CWnd* pParent);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CONNECT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedButtonConnect();
	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	CIPAddressCtrl m_ipLaser;
	CIPAddressCtrl m_ipGalil;
	CButton m_buttonLaser;
	CButton m_buttonRGB;
	CButton m_buttonMAG;
	CButton m_buttonGalil;

	CBitmap	m_bitmapDisconnect;
	CBitmap	m_bitmapConnect;

	BOOL	m_bInit;
	BOOL	m_bCheck;
};
