// CDialogConnect.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "CDialogConnect.h"
#include "afxdialogex.h"
#include "MotionControl.h"
#include "LaserControl.h"
#include "resource.h"


// CDialogConnect dialog

IMPLEMENT_DYNAMIC(CDialogConnect, CDialogEx)

CDialogConnect::CDialogConnect(CMotionControl& motion, CLaserControl& laser, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CONNECT, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
}

CDialogConnect::~CDialogConnect()
{
}

void CDialogConnect::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}


void CDialogConnect::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS_LASER, m_ipLaser);
	DDX_Control(pDX, IDC_IPADDRESS_GALIL, m_ipGalil);
	DDX_Control(pDX, IDC_BUTTON_LASER, m_buttonLaser);
	DDX_Control(pDX, IDC_BUTTON_RGB, m_buttonRGB);
	DDX_Control(pDX, IDC_BUTTON_MAG, m_buttonMAG);
	DDX_Control(pDX, IDC_BUTTON_GALIL, m_buttonGalil);
}


BEGIN_MESSAGE_MAP(CDialogConnect, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CDialogConnect::OnClickedButtonConnect)
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CDialogConnect::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_CONNECT, pParent);
	ShowWindow(SW_HIDE);
}


// CDialogConnect message handlers


// override OnClose() to insure that canot be closed
void CDialogConnect::OnCancel()
{
	// TODO: Add your control notification handler code here
}


// override OnOK() to insure that canot be closed
void CDialogConnect::OnOK()
{
	// TODO: Add your control notification handler code here
}


void CDialogConnect::OnClickedButtonConnect()
{
	// TODO: Add your control notification handler code here
	// connect if there is not a connection now, else disconnect
	if (m_motionControl.IsConnected())
	{
		m_motionControl.Disconnect();
		m_laserControl.Disconnect();
	}
	else
	{
		BYTE laser[4], galil[4];
		// get the IP addresses of the lasewr and Gailil
		m_bCheck = TRUE;
		int ret = UpdateData(TRUE);
		m_bCheck = FALSE;
		if (ret)
		{
			m_ipGalil.GetAddress(galil[0], galil[1], galil[2], galil[3]);
			m_motionControl.Connect(galil, 0.0);
			
			m_ipLaser.GetAddress(laser[0], laser[1], laser[2], laser[3]);
			m_laserControl.Connect(laser);

		}
	}

	// request the parent to enable all controls 
	m_pParent->PostMessageA(m_nMsg, MSG_SETBITMAPS);

//	SetButtonBitmaps(); // donwe in the above

}
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogConnect::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	GetClientRect(&rect);
	cx = rect.Width();
	cy = rect.Height();

	// locate the Close and Connect Buttons
	GetDlgItem(IDC_BUTTON_CONNECT)->GetWindowRect(&rect);
	int cx1 = rect.Width();
	int cy1 = rect.Height();
//	GetDlgItem(IDC_BUTTON_CONNECT)->MoveWindow(2, cy - cy1 - 2, cx1, cy1);
}

BOOL CDialogConnect::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here
	m_buttonLaser.GetWindowRect(&rect);
	int cx1 = rect.Width();
	int cy1 = rect.Height();


	m_bitmapDisconnect.LoadBitmap(IDB_BITMAP_DISCONNECT);
	m_bitmapConnect.LoadBitmap(IDB_BITMAP_CONNECT);
	SetButtonBitmaps();

	m_ipLaser.SetAddress(192, 168, 12, 101);
	m_ipGalil.SetAddress(192, 168, 1, 41);

	m_bInit = TRUE;
	PostMessage(WM_SIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogConnect::SetButtonBitmaps()
{
	HBITMAP hBitmap1 = (HBITMAP)m_bitmapDisconnect.GetSafeHandle();
	HBITMAP hBitmap2 = (HBITMAP)m_bitmapConnect.GetSafeHandle();

	BOOL bGalil = m_motionControl.IsConnected();
	BOOL bLaser = m_laserControl.IsConnected();

	m_buttonRGB.SetBitmap(hBitmap1);
	m_buttonMAG.SetBitmap(hBitmap1);
	m_buttonGalil.SetBitmap(bGalil ? hBitmap2 : hBitmap1);
	m_buttonLaser.SetBitmap(bLaser ? hBitmap2 : hBitmap1);

	if (!m_motionControl.IsConnected())
		GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("Connect"));
	else
		GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("Disconnect"));

	m_ipGalil.EnableWindow(m_motionControl.IsConnected() ? FALSE : TRUE);
}

void CDialogConnect::EnableControls()
{
	SetButtonBitmaps();
}


