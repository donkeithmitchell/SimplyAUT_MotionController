// DialogNewProject.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogNewProject.h"
#include "afxdialogex.h"


// CDialogNewProject dialog

IMPLEMENT_DYNAMIC(CDialogNewProject, CDialogEx)

CDialogNewProject::CDialogNewProject(CString szProject, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_NEW_FILE, pParent)
	, m_szProject(szProject)
{

}

CDialogNewProject::~CDialogNewProject()
{
}

void CDialogNewProject::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PROJECT, m_szProject);
}

void CDialogNewProject::OnOK()
{
	CDialogEx::OnOK();
}


BEGIN_MESSAGE_MAP(CDialogNewProject, CDialogEx)
END_MESSAGE_MAP()


// CDialogNewProject message handlers
