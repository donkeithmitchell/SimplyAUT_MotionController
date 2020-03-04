// DialogStatus.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogStatus.h"
#include "afxdialogex.h"
#include "resource.h"

// CDialogStatus dialog

IMPLEMENT_DYNAMIC(CDialogStatus, CDialogEx)

CDialogStatus::CDialogStatus(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_STATUS, pParent)
{

}

CDialogStatus::~CDialogStatus()
{
}

void CDialogStatus::Create(CWnd * pParent)
{
	CDialogEx::Create(IDD_DIALOG_MOTORS, pParent);
	ShowWindow(SW_HIDE);
}


void CDialogStatus::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogStatus, CDialogEx)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CDialogStatus message handlers
BOOL CDialogStatus::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here

	m_bInit = TRUE;
	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogStatus::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

	GetDlgItem(IDC_EDIT_STATUS)->MoveWindow(2, 2, cx - 45, cy - 4);

}


