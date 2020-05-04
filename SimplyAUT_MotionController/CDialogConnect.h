#pragma once


// CDialogConnect dialog
class CMotionControl;
class CLaserControl;
class CMagControl;

class CDialogConnect : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogConnect)

public:
	CDialogConnect(CMotionControl&, CLaserControl&, CMagControl&, CString&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogConnect();
	void Init(CWnd*, UINT);

	void	Create(CWnd* pParent);
	BOOL	CheckVisibleTab();
	void	Serialize(CArchive& ar);
	void	EnableControls();

protected:
	void	SetButtonBitmaps();
	void    ResetParameters();
	void	SendErrorMessage(const char* msg);
	void	ValidateProjectName(CDataExchange* pDX, CString szProject);

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

	afx_msg void OnClickedButtonConnect();
	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	CIPAddressCtrl m_ipLaser;
	CIPAddressCtrl m_ipGalil;
	CIPAddressCtrl m_ipMag;
	CButton m_buttonLaser;
	CButton m_buttonMAG;
	CButton m_buttonGalil;

	CBitmap	m_bitmapDisconnect;
	CBitmap	m_bitmapConnect;
	CMotionControl& m_motionControl;
	CLaserControl& m_laserControl;
	CMagControl& m_magControl;

	CString& m_szProject;
	CString m_szPort;
	BOOL	m_bInit;
	BOOL	m_bCheck;

	BYTE	m_laserIP[4], m_galilIP[4], m_magIP[4];

	UINT  m_nMsg;
	CWnd* m_pParent;
	afx_msg void OnClickedButtonReset();
};
