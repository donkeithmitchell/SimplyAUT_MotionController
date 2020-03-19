#pragma once


// CDialogMag dialog
class CMotionControl;
class CLaserControl;
class CMagControl;


class CDialogMag : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogMag)

public:
	CDialogMag(CMotionControl&, CLaserControl&, CMagControl&, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogMag();

	void Create(CWnd*);
	void EnableControls();
	void Init(CWnd* pParent, UINT nMsg);
	BOOL CheckVisibleTab() { return TRUE; }
	int  GetMagStatus(int nStat);

	HICON	m_hIcon;
	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;

	CBitmap	m_bitmapDisconnect;
	CBitmap	m_bitmapConnect;

	CMotionControl& m_motionControl;
	CLaserControl& m_laserControl;
	CMagControl& m_magControl;

	afx_msg void OnSize(UINT nFlag, int cx, int cy);
	//	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnUserUpdateDialog(WPARAM, LPARAM);

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MAG };
#endif
	enum { WM_USER_UPDATE_DIALOG = WM_USER + 1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CString m_szMagVersion;
	CButton m_butMagEnable;
	CButton m_butMagOn;
	afx_msg void OnClickedCheckMagEnable();
	CString m_szEncoderCount;
	CString m_szRGBValue;
	afx_msg void OnClickedButtonEncoderClear();
};
