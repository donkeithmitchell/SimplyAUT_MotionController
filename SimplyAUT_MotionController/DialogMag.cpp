// DialogLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "DialogMag.h"
#include "laserControl.h"
#include "magController.h"
#include "SLS_Comms.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDialogMag dialog

IMPLEMENT_DYNAMIC(CDialogMag, CDialogEx)

CDialogMag::CDialogMag(CMotionControl& motion, CLaserControl& laser, CMagControl& mag, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LASER, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_szMagVersion(_T(""))
	, m_szEncoderCount(_T(""))
	, m_szRGBValue(_T(""))
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_bInit = FALSE;
	m_bCheck = FALSE;

	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

CDialogMag::~CDialogMag()
{
}

void CDialogMag::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_MAG, pParent);
	ShowWindow(SW_HIDE);
}


void CDialogMag::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_MAG_VERSION, m_szMagVersion);
	DDX_Control(pDX, IDC_CHECK_MAG_ENABLE, m_butMagEnable);
	DDX_Control(pDX, IDC_BUTTON_MAG_ON, m_butMagOn);
	DDX_Text(pDX, IDC_STATIC_MAG_ENCODER, m_szEncoderCount);
	DDX_Text(pDX, IDC_STATIC_MAG_RGB, m_szRGBValue);
}


BEGIN_MESSAGE_MAP(CDialogMag, CDialogEx)
	//	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_USER_UPDATE_DIALOG, OnUserUpdateDialog)
	ON_BN_CLICKED(IDC_CHECK_MAG_ENABLE, &CDialogMag::OnClickedCheckMagEnable)
	ON_BN_CLICKED(IDC_BUTTON_MAG_CLEAR, &CDialogMag::OnClickedButtonEncoderClear)
END_MESSAGE_MAP()


// CDialogMag message handlers
void CDialogMag::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}

// CDialogStatus message handlers
// CDialogStatus message handlers
BOOL CDialogMag::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	// TODO: Add extra initialization here
	m_bitmapDisconnect.LoadBitmap(IDB_BITMAP_DISCONNECT);
	m_bitmapConnect.LoadBitmap(IDB_BITMAP_CONNECT);
	HBITMAP hBitmap1 = (HBITMAP)m_bitmapDisconnect.GetSafeHandle();
	m_butMagOn.SetBitmap(hBitmap1);

	//	EnableControls();
	SetTimer(1, 500, NULL);

	m_bInit = TRUE;
	PostMessage(WM_SIZE);

	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDialogMag::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}

LRESULT CDialogMag::OnUserUpdateDialog(WPARAM, LPARAM)
{
	CString str;


	return 0L;

}

int CDialogMag::GetMagStatus(int nStat)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		int nStatus = INT_MAX;
		m_pParent->SendMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_GET_MAG_STATUS + nStat, (WPARAM)(&nStatus));
		return nStatus;
	}
	else
		return INT_MAX;
}

void CDialogMag::OnTimer(UINT nIDEvent)
{
	// get the status on a t6ime, then only reference this status
	if (m_laserControl.IsConnected()&& IsWindowVisible())
		{
		BOOL bConnected = m_magControl.IsConnected();
		GetDlgItem(IDC_CHECK_MAG_ENABLE)->EnableWindow(bConnected);
		GetDlgItem(IDC_BUTTON_MAG_CLEAR)->EnableWindow(bConnected);


		int nEncoderCount = GetMagStatus(MAG_IND_ENC_CNT);
		if (nEncoderCount != INT_MAX)
			m_szEncoderCount.Format("%d", nEncoderCount);
		else
			m_szEncoderCount.Format("***");
		GetDlgItem(IDC_STATIC_MAG_ENCODER)->SetWindowText(m_szEncoderCount);

		int red, green, blue;
		if (m_magControl.GetRGBValues(red, green, blue))
		{
			m_szRGBValue.Format("(%d,%d,%d)", red, green, blue);
		}
		else
			m_szRGBValue.Format(_T(""));

		int bLocked = GetMagStatus(MAG_IND_MAG_LOCKOUT) == 1;
		m_butMagEnable.SetCheck(bLocked == 0);

		GetDlgItem(IDC_STATIC_MAG_RGB)->SetWindowText(m_szRGBValue);


		// if this changes from the last call, then pass on to the scan tab
		HBITMAP hBitmap1 = (HBITMAP)m_bitmapConnect.GetSafeHandle();
		HBITMAP hBitmap2 = (HBITMAP)m_bitmapDisconnect.GetSafeHandle();

		int bMagOn = GetMagStatus(MAG_IND_MAG_ON) == 1;
		m_butMagOn.SetBitmap((bMagOn == 1) ? hBitmap1 : hBitmap2);

		// on a change this will cause the p[arenrt to update all tabs
		static int sLastMagOn = INT_MAX;
		if (bMagOn != sLastMagOn && m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		{
			sLastMagOn = bMagOn;
			m_pParent->PostMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SETBITMAPS);
		}

	}

	CDialog::OnTimer(nIDEvent);
}




// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogMag::OnSize(UINT nFlag, int cx, int cy)
{
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

}

static CString FormatVersionDate(int nDate)
{
	CString str;

	int day = nDate % 100;
	int month = (nDate / 100) % 100;
	int year = (nDate / 10000);

	str.Format("%d/%d/%d", year, month, day);
	return str;
}

void CDialogMag::EnableControls()
{
	BOOL bConnected = m_magControl.IsConnected();
	GetDlgItem(IDC_CHECK_MAG_ENABLE)->EnableWindow(bConnected);
	GetDlgItem(IDC_BUTTON_MAG_CLEAR)->EnableWindow(bConnected);

	int version[5];
	if (m_magControl.GetMagVersion(version))
	{
		m_szMagVersion.Format("HW:%d, FW: %d.%d.%d, %s", version[0], version[1], version[2], version[3], FormatVersionDate(version[4]));
		GetDlgItem(IDC_STATIC_MAG_VERSION)->SetWindowTextA(m_szMagVersion);
	}

	int bLocked = GetMagStatus(MAG_IND_MAG_LOCKOUT) == 1;
	m_butMagEnable.SetCheck(bLocked == 0);
}







void CDialogMag::OnClickedCheckMagEnable()
{
	// TODO: Add your control notification handler code here
	BOOL bEnabled = m_butMagEnable.GetCheck();
	BOOL ret = m_magControl.EnableMagSwitchControl(bEnabled);
}


void CDialogMag::OnClickedButtonEncoderClear()
{
	// TODO: Add your control notification handler code here
	m_magControl.ResetEncoderCount();
}
