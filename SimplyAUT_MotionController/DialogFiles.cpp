// DialogStatus.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogFiles.h"
#include "afxdialogex.h"
#include "resource.h"

const char* g_szTitle[] = { "#", "File", "Max Offset", "Avg Offset", NULL };

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
	char buffer[MAX_PATH];
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here

	m_bInit = TRUE;

	LV_COLUMN listColumn;
	listColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	listColumn.cx = 0;
	listColumn.pszText = buffer;

	for (int i = 0; g_szTitle[i] != NULL; ++i)
	{
		strncpy_s(buffer, g_szTitle[i], sizeof(buffer));
		listColumn.iSubItem = i;
		listColumn.fmt = (i == 0) ? LVCFMT_CENTER : LVCFMT_LEFT;
		m_listFiles.InsertColumn(i, &listColumn);
	}

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
	CSize sz1 = pDC->GetTextExtent("8888");
	CSize sz2 = pDC->GetTextExtent("Max Offset");
	ReleaseDC(pDC);

	m_listFiles.SetColumnWidth(0, sz1.cx);
	m_listFiles.SetColumnWidth(1, cx - sz1.cx - 2*sz2.cx);
	m_listFiles.SetColumnWidth(2, sz2.cx);
	m_listFiles.SetColumnWidth(3, sz2.cx);
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
	for( int i = 0, j=0; ret != 0; ++i)
	{
		ret = find.FindNextFileA();

		CString szFile = find.GetFileName();
		int ind = szFile.Find("File_");
		if (ind == -1)
			continue;

		int nFileNum = atoi(szFile.Mid(ind + 5));

		listItem.iItem = j;
		sprintf_s(buffer, sizeof(buffer), "%d", nFileNum);
		m_listFiles.InsertItem(&listItem);

		strncpy_s(buffer, szFile, sizeof(buffer));
		m_listFiles.SetItemText(j, 1, buffer);

		FILE* fp = NULL;
		if (fopen_s(&fp, find.GetFilePath(), "r") == 0 && fp != NULL)
		{
			int pos;
			double capH, capW, HiLo, offset;

			fgets(buffer, sizeof(buffer), fp);
			double maxOff = 0;
			double sum = 0;
			int cnt = 0;
			while (fgets(buffer, sizeof(buffer), fp))
			{
				sscanf_s(buffer, "%d\t%lf\t%lf\t%lf\t%lf\n", &pos, &capH, &capW, &HiLo, &offset);
				maxOff = max(maxOff, fabs(offset));
				sum += fabs(offset);
				cnt++;
			}
			sprintf_s(buffer, sizeof(buffer), "%5.1f", maxOff);
			m_listFiles.SetItemText(j, 2, buffer);

			sprintf_s(buffer, sizeof(buffer), "%5.2f", cnt ? sum/cnt : 0);
			m_listFiles.SetItemText(j, 3, buffer);
			j++;
		}
	}
}

void CDialogFiles::EnableControls()
{
	UpdateFileList();
}


