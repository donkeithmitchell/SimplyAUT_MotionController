// CDialogConnect.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "CDialogConnect.h"
#include "afxdialogex.h"
#include "MotionControl.h"
#include "LaserControl.h"
#include "MagController.h"
#include "resource.h"


// CDialogConnect dialog

IMPLEMENT_DYNAMIC(CDialogConnect, CDialogEx)

CDialogConnect::CDialogConnect(CMotionControl& motion, CLaserControl& laser, CMagControl& mag, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CONNECT, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_nMsg(0)
	, m_pParent(NULL)
{
	m_bInit = FALSE;
	m_bCheck = FALSE;
	ResetParameters();
}

CDialogConnect::~CDialogConnect()
{
	m_motionControl.Disconnect();
	m_laserControl.Disconnect();
	m_magControl.Disconnect();
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
	DDX_Control(pDX, IDC_IPADDRESS_MAG, m_ipMag);
	DDX_Control(pDX, IDC_BUTTON_LASER, m_buttonLaser);
	DDX_Control(pDX, IDC_BUTTON_MAG, m_buttonMAG);
	DDX_Control(pDX, IDC_BUTTON_GALIL, m_buttonGalil);
	DDX_Text(pDX, IDC_EDIT_MAG_PORT, m_szPort);

	if (pDX->m_bSaveAndValidate)
	{
		m_ipGalil.GetAddress(m_galilIP[0], m_galilIP[1], m_galilIP[2], m_galilIP[3]);
		m_ipLaser.GetAddress(m_laserIP[0], m_laserIP[1], m_laserIP[2], m_laserIP[3]);
		m_ipMag.GetAddress(m_magIP[0], m_magIP[1], m_magIP[2], m_magIP[3]);
	}
	else
	{
		m_ipLaser.SetAddress(m_laserIP[0], m_laserIP[1], m_laserIP[2], m_laserIP[3]);
		m_ipGalil.SetAddress(m_galilIP[0], m_galilIP[1], m_galilIP[2], m_galilIP[3]);
		m_ipMag.SetAddress(m_magIP[0], m_magIP[1], m_magIP[2], m_magIP[3]);
	}

}


BEGIN_MESSAGE_MAP(CDialogConnect, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CDialogConnect::OnClickedButtonConnect)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CDialogConnect::OnClickedButtonReset)
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
		m_magControl.Disconnect();
	}
	else
	{
		// get the IP addresses of the lasewr and Gailil
		m_bCheck = TRUE;
		int ret = UpdateData(TRUE);
		m_bCheck = FALSE;
		if (ret)
		{
			SendErrorMessage(NULL);
			m_motionControl.Connect(m_galilIP, 0.0);
			m_laserControl.Connect(m_laserIP);
			m_magControl.Connect(m_magIP, atoi(m_szPort));

		}
	}

	// request the parent to enable all controls 
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
		m_pParent->PostMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SETBITMAPS);

//	SetButtonBitmaps(); // donwe in the above

}
void CDialogConnect::SendErrorMessage(const char* msg)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)msg);
	}
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

	m_ipLaser.SetAddress(m_laserIP[0], m_laserIP[1], m_laserIP[2], m_laserIP[3]);
	m_ipGalil.SetAddress(m_galilIP[0], m_galilIP[1], m_galilIP[2], m_galilIP[3]);
	m_ipMag.SetAddress(m_magIP[0], m_magIP[1], m_magIP[2], m_magIP[3]);

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
	BOOL bMag = m_magControl.IsConnected();

	m_buttonMAG.SetBitmap(bMag ? hBitmap2 : hBitmap1);
	m_buttonGalil.SetBitmap(bGalil ? hBitmap2 : hBitmap1);
	m_buttonLaser.SetBitmap(bLaser ? hBitmap2 : hBitmap1);

	m_ipLaser.EnableWindow(!bLaser);
	m_ipGalil.EnableWindow(!bGalil);
	m_ipMag.EnableWindow(!bMag);
	GetDlgItem(IDC_BUTTON_RESET)->EnableWindow(!bLaser && !bGalil && !bMag);

	GetDlgItem(IDC_EDIT_MAG_PORT)->EnableWindow(!bMag);


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

void CDialogConnect::Serialize(CArchive& ar)
{
	const int MASK = 0xCDCDCDCD;
	int mask = MASK;

	if (ar.IsStoring())
	{
		UpdateData(TRUE);
		ar << m_szPort;
		for (int i = 0; i < 4; ++i)
		{
			ar << m_laserIP[i];
			ar << m_galilIP[i];
			ar << m_magIP[i];
		}
		ar << mask;
	}
	else
	{
		try
		{
			ar >> m_szPort;
			for (int i = 0; i < 4; ++i)
			{
				ar >> m_laserIP[i];
				ar >> m_galilIP[i];
				ar >> m_magIP[i];
			}
			ar >> mask;
		}
		catch (CArchiveException * e1)
		{
			ResetParameters();
			e1->Delete();

		}
		if (mask != MASK)
			ResetParameters();

		UpdateData(FALSE);
	}
}

void CDialogConnect::ResetParameters()
{
	m_szPort = _T("23");
	BYTE laserIP[] = { 192, 168, 12, 101 };
	BYTE galilIP[] = { 192, 168, 1, 41 };
	BYTE magIP[] = { 192, 168, 1, 30 };

	memcpy(m_laserIP, laserIP, sizeof(laserIP));
	memcpy(m_galilIP, galilIP, sizeof(galilIP));
	memcpy(m_magIP, magIP, sizeof(magIP));
}





void CDialogConnect::OnClickedButtonReset()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	ResetParameters();
	UpdateData(FALSE);
}
