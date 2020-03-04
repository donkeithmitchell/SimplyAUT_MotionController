// DialogMotors.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogMotors.h"
#include "afxdialogex.h"
#include "resource.h"

// CDialogMotors dialog

IMPLEMENT_DYNAMIC(CDialogMotors, CDialogEx)

CDialogMotors::CDialogMotors(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MOTORS, pParent)
	, m_szMotorA(_T(""))
	, m_szMotorB(_T(""))
	, m_szMotorC(_T(""))
	, m_szMotorD(_T(""))
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
}

CDialogMotors::~CDialogMotors()
{
}

void CDialogMotors::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_MA, m_szMotorA);
	DDX_Text(pDX, IDC_EDIT_MB, m_szMotorB);
	DDX_Text(pDX, IDC_EDIT_MC, m_szMotorC);
	DDX_Text(pDX, IDC_EDIT_MD, m_szMotorD);
}


BEGIN_MESSAGE_MAP(CDialogMotors, CDialogEx)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogMotors::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

}

BOOL CDialogMotors::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here

	m_bInit = TRUE;
	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogMotors::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_MOTORS, pParent);
	ShowWindow(SW_HIDE);
}




