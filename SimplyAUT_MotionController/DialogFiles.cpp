// DialogStatus.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogFiles.h"
#include "afxdialogex.h"
#include "resource.h"


// CDialogFiles dialog

IMPLEMENT_DYNAMIC(CDialogFiles, CDialogEx)

CDialogFiles::CDialogFiles(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_FILES, pParent)
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_bInit = FALSE;
	m_bCheck = FALSE;
}

CDialogFiles::~CDialogFiles()
{
}

void CDialogFiles::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}



void CDialogFiles::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FILES, m_listFiles);
}


BEGIN_MESSAGE_MAP(CDialogFiles, CDialogEx)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CDialogFiles message handlers
// CDialogFiles message handlers
BOOL CDialogFiles::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here

	m_bInit = TRUE;

	LV_COLUMN listColumn;
	listColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	listColumn.cx = 0;
	listColumn.iSubItem = 0;
	listColumn.pszText = "#";	
	listColumn.fmt = LVCFMT_CENTER;
	m_listFiles.InsertColumn(0, &listColumn);

	listColumn.iSubItem = 1;
	listColumn.pszText = "File";
	listColumn.fmt = LVCFMT_LEFT;
	m_listFiles.InsertColumn(1, &listColumn);

	PostMessage(WM_SIZE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}


// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogFiles::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

	m_listFiles.MoveWindow(2, 2, cx - 4, cy - 4);

	m_listFiles.GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

	CDC* pDC = GetDC();
	CSize sz = pDC->GetTextExtent("8888");
	ReleaseDC(pDC);
	m_listFiles.SetColumnWidth(0, sz.cx);
	m_listFiles.SetColumnWidth(1, cx -sz.cx);
}

void CDialogFiles::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_FILES, pParent);
	ShowWindow(SW_HIDE);
}

void CDialogFiles::UpdateFileList()
{
	char buffer[MAX_PATH];
	LV_ITEM listItem;
	CString path;

	m_listFiles.DeleteAllItems();

	if (::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, buffer) != S_OK)
		return;

	path.Format("%s\\SimplyUTFiles", buffer);
	DWORD attrib = GetFileAttributes(buffer);
	if (attrib == INVALID_FILE_ATTRIBUTES)
	{
		// format the path, and insure that it exists
		if (!::CreateDirectory(path, NULL))
			return;
	}

	CFileFind find;
	path.Format("%s\\SimplyUTFiles\\*.txt", buffer);
	if( !find.FindFile(path) )
		return;

	listItem.mask = LVIF_TEXT;
	listItem.iSubItem = 0;
	listItem.pszText = buffer;
	listItem.cchTextMax = sizeof(buffer);
	int ret = 1;
	for( int i = 0; ret != 0; ++i)
	{
		ret = find.FindNextFileA();

		listItem.iItem = i;
		sprintf_s(buffer, sizeof(buffer), "%d", i + 1);
		m_listFiles.InsertItem(&listItem);

		strncpy_s(buffer, find.GetFileName(), sizeof(buffer));
		m_listFiles.SetItemText(i, 1, buffer);
	}
}

void CDialogFiles::EnableControls()
{
	UpdateFileList();
}


