#pragma once


// CDialogFiles dialog

class CDialogFiles : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogFiles)

public:
	CDialogFiles(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogFiles();

	void Create(CWnd*);
	void UpdateFileList();
	void EnableControls();
	void Init(CWnd* pParent, UINT nMsg);
	BOOL CheckVisibleTab() { return TRUE; }

	UINT  m_nMsg;
	CWnd* m_pParent;
	BOOL	m_bInit;
	BOOL	m_bCheck;
	CListCtrl	m_listFiles;

		// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_FILES};
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nFlag, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};
