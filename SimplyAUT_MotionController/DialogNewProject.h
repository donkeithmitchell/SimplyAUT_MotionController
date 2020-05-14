#pragma once


// CDialogNewProject dialog

class CDialogNewProject : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogNewProject)

public:
	CDialogNewProject(CString szProject, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogNewProject();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_NEW_FILE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	CString m_szProject;
};
