#pragma once


// CDialogConnect dialog
class CMotionControl;
class CLaserControl;
class CMagControl;

class CDialogConnect : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogConnect)

public:
	CDialogConnect(CMotionControl&, CLaserControl&, CMagControl&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogConnect();
	void Init(CWnd*, UINT);

	void Create(CWnd* pParent);
	void SetButtonBitmaps();
	void EnableControls();
	BOOL CheckVisibleTab() { return TRUE; }

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
	CIPAddressCtrl m_ipMag;
	CButton m_buttonLaser;
	CButton m_buttonMAG;
	CButton m_buttonGalil;
	CString m_szPort;

	CBitmap	m_bitmapDisconnect;
	CBitmap	m_bitmapConnect;
	CMotionControl& m_motionControl;
	CLaserControl& m_laserControl;
	CMagControl& m_magControl;

	BOOL	m_bInit;
	BOOL	m_bCheck;

	UINT  m_nMsg;
	CWnd* m_pParent;
};
