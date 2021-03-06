// DialogLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogLaser.h"
#include "laserControl.h"
#include "LaserProfile.h"
#include "MotionControl.h"
#include "SLS_Comms.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDialogLaser dialog
// this dialog is only used in _DEBUG at this time (911)

IMPLEMENT_DYNAMIC(CDialogLaser, CDialogEx)

CDialogLaser::CDialogLaser(CMotionControl& motion, CLaserControl& laser, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LASER, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_wndProfile(laser, m_bShiftToCentre, m_bShowRawData, RGB(10, 10, 10))
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_bInit = FALSE;
	m_bCheck = FALSE;
	ResetParameters();
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CDialogLaser::UpdateMenu(CMenu* pMenu)
{
	ASSERT(pMenu);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STATE;

	mii.fState = MFS_DEFAULT;
	mii.fType |= MFT_RADIOCHECK;
	if (m_motionControl.AreMotorsRunning())
		mii.fState |= MFS_DISABLED;

	mii.fState = m_laserControl.m_bForceWeld ? MFS_CHECKED : MFS_UNCHECKED;
	pMenu->SetMenuItemInfoA(ID_LASER_FORCE_CAP, &mii, FALSE);

	mii.fState = m_laserControl.m_bForceGap ? MFS_CHECKED : MFS_UNCHECKED;
	pMenu->SetMenuItemInfoA(ID_LASER_FORCE_GAP, &mii, FALSE);
}

void CDialogLaser::ResetParameters()
{
	m_bShiftToCentre = TRUE;
	m_bShowRawData = FALSE;
	m_bAutoLaser = FALSE;
	m_LaserPower = 0;
	m_CameraShutter = 0;
}

void CDialogLaser::Serialize(CArchive& ar)
{
	const int MASK = 0xCDCDCDCD;
	int mask = MASK;

	if (ar.IsStoring())
	{
		UpdateData(TRUE);
		ar << m_bShiftToCentre;
		ar << m_bShowRawData;
		ar << m_bAutoLaser;
		ar << m_LaserPower;
		ar << m_CameraShutter;
		ar << m_laserControl.m_bFW_Gap;
		ar << m_laserControl.m_bFW_Weld;
		ar << m_laserControl.m_bForceGap;
		ar << m_laserControl.m_bForceWeld;
		ar << mask;
	}
	else
	{
		try
		{
			ar >> m_bShiftToCentre;
			ar >> m_bShowRawData;
			ar >> m_bAutoLaser;
			ar >> m_LaserPower;
			ar >> m_CameraShutter;
			ar >> m_laserControl.m_bFW_Gap;
			ar >> m_laserControl.m_bFW_Weld;
			ar >> m_laserControl.m_bForceGap;
			ar >> m_laserControl.m_bForceWeld;
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


CDialogLaser::~CDialogLaser()
{
}

void CDialogLaser::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_LASER, pParent);
	ShowWindow(SW_HIDE);
}


void CDialogLaser::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_AUTOLASER_CHECK, m_bAutoLaser);
	DDX_Control(pDX, IDC_LASERPOWER_SLIDER, m_laser_power_sld);
	DDX_Control(pDX, IDC_SHUTTER_SLIDER, m_camera_shutter_sld);
	DDX_Control(pDX, IDC_SENSORVERSION_EDIT, m_sensor_version);
	DDX_Control(pDX, IDC_TEMPERATURE_EDIT, m_temperature_edit);
	DDX_Text(pDX, IDC_LASERPOWER_EDIT, m_LaserPower);
	DDX_Text(pDX, IDC_SHUTTER_EDIT, m_CameraShutter);
	DDX_Check(pDX, IDC_CHECK_SHIFT_TO_CENTRE, m_bShiftToCentre);
	DDX_Check(pDX, IDC_CHECK_SHOW_RAW_DATA, m_bShowRawData);
	DDX_Check(pDX, IDC_CHECK_FW_GAP, m_laserControl.m_bFW_Gap);
	DDX_Check(pDX, IDC_CHECK_FW_WELD, m_laserControl.m_bFW_Weld);
	DDX_Check(pDX, IDC_CHECK_FORCE_GAP, m_laserControl.m_bForceGap);
	DDX_Check(pDX, IDC_CHECK_FORCE_WELD, m_laserControl.m_bForceWeld);
}


BEGIN_MESSAGE_MAP(CDialogLaser, CDialogEx)
//	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LASER_BUTTON, OnLaserButton)
	ON_BN_CLICKED(IDC_AUTOLASER_CHECK, OnAutolaserCheck)
	ON_WM_HSCROLL()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_LASERPOWER_SLIDER, OnReleasedcaptureLaserpowerSlider)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SHUTTER_SLIDER, OnReleasedcaptureShutterSlider)
	ON_BN_CLICKED(IDC_ROI_BUTTON, OnRoiButton)
	ON_MESSAGE(WM_USER_UPDATE_DIALOG, OnUserUpdateDialog)
	ON_BN_CLICKED(IDC_BUTTON_ROI_RESET, &CDialogLaser::OnClickedButtonRoiReset)
	ON_BN_CLICKED(IDC_CHECK_SHIFT_TO_CENTRE, &CDialogLaser::OnClickedCheck)
	ON_BN_CLICKED(IDC_CHECK_SHOW_RAW_DATA, &CDialogLaser::OnClickedCheck)
	ON_BN_CLICKED(IDC_CHECK_FW_GAP, &CDialogLaser::OnClickedCheck)
	ON_BN_CLICKED(IDC_CHECK_FW_WELD, &CDialogLaser::OnClickedCheck)
	ON_BN_CLICKED(IDC_CHECK_FORCE_GAP, &CDialogLaser::OnClickedCheck)
	ON_BN_CLICKED(IDC_CHECK_FORCE_WELD, &CDialogLaser::OnClickedCheck)
END_MESSAGE_MAP()


// CDialogLaser message handlers
void CDialogLaser::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}

// CDialogStatus message handlers
// CDialogStatus message handlers
BOOL CDialogLaser::OnInitDialog()
{
	CRect rect;
	CDialogEx::OnInitDialog();

	// TODO: Add extra initialization here
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_wndProfile.Create( GetDlgItem(IDC_STATIC_PROFILE) );
	m_wndProfile.Init(this, WM_USER_UPDATE_DIALOG);

	// TODO: Add extra initialization here
	m_bAutoLaser = TRUE;

	m_laser_power_sld.SetRange(1, 100);
	m_laser_power_sld.SetTicFreq(10);
	m_laser_power_sld.SetPos(1);

	m_camera_shutter_sld.SetRange(1, 100);
	m_camera_shutter_sld.SetTicFreq(10);
	m_camera_shutter_sld.SetPos(1);

//	EnableControls();
	SetTimer(1, 250, NULL);

	m_bInit = TRUE;
	PostMessage(WM_SIZE);
	EnableControls();
	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDialogLaser::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}


// display the laser measures
LRESULT CDialogLaser::OnUserUpdateDialog(WPARAM, LPARAM)
{
	CString str;

	LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();

	str.Format("%.1f mm", measure2.weld_cap_mm.x);
	GetDlgItem(IDC_STATIC_WELD_OFFSET)->SetWindowTextA(str);

	// the width of the weld
	str.Format("%.1f mm", measure2.weld_right_mm - measure2.weld_left_mm);
	GetDlgItem(IDC_STATIC_WELD_WIDTH)->SetWindowTextA(str);

	// differnece in height left to right
	double diff_side_height = fabs(measure2.weld_left_height_mm - measure2.weld_right_height_mm);
	str.Format("%.1f mm", diff_side_height);
	GetDlgItem(IDC_STATIC_WELD_LR_DIFF)->SetWindowTextA(str);

	double avg_side_height = (measure2.weld_left_height_mm + measure2.weld_right_height_mm) / 2;
	str.Format("%.1f mm", fabs(measure2.weld_cap_mm.y - avg_side_height));
	GetDlgItem(IDC_STATIC_WELD_HEIGHT)->SetWindowTextA(str);

	return 0L;

}

// turn the laser on/off
void CDialogLaser::OnLaserButton()
{
	BOOL bOn = !m_laserControl.IsLaserOn();
	m_laserControl.TurnLaserOn( bOn );
	m_wndProfile.InvalidateRgn(NULL);
//	m_wndProfile.ResetROI();
	EnableControls();
}

// noite the laser temperatures
void CDialogLaser::OnTimer(UINT nIDEvent)
{
	CString tstr;

	double h_mm = 0.0, v_mm = 0.0;
	double h2_mm = 0.0, v2_mm = 0.0;


	if (!m_laserControl.IsConnected() || !IsWindowVisible())
	{
		CDialog::OnTimer(nIDEvent);
		return;
	}


	SensorStatus sensStat;
	m_laserControl.GetLaserStatus(sensStat);

	float t1 = (((float)sensStat.MainBrdTemp) / 100.0f) - 100.0f;
	float t2 = (((float)sensStat.LaserTemp) / 100.0f) - 100.0f;
	float t3 = (((float)sensStat.PsuBrdTemp) / 100.0f) - 100.0f;

	tstr.Format("%5.1f %5.1f %5.1f", t1, t2, t3);
	m_temperature_edit.SetWindowText(tstr);

	m_wndProfile.m_profile_count = m_wndProfile.m_profile_count;

	EnableControls();
	CDialog::OnTimer(nIDEvent);
}

// set if auto laser or manual
// without _DEBUG the user is stuck with auto
void CDialogLaser::OnAutolaserCheck()
{
	UpdateData(TRUE);
	if (m_bAutoLaser)
		m_laserControl.SetLaserOptions(1);
	else
	{
		m_laserControl.SetLaserOptions(0);
		m_laserControl.SetLaserIntensity(m_LaserPower);
	}
//	EnableControls();
}

// note that changing the lasert shutter speed
void CDialogLaser::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// Slider Controls have been adjusted
	CSliderCtrl* slider = (CSliderCtrl*)pScrollBar;

	if (slider == &m_laser_power_sld)
		m_LaserPower = m_laser_power_sld.GetPos();

	if (slider == &m_camera_shutter_sld)
		m_CameraShutter = m_camera_shutter_sld.GetPos();


	UpdateData(FALSE);
//	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

// don't enact until release the slider
void CDialogLaser::OnReleasedcaptureLaserpowerSlider(NMHDR* pNMHDR, LRESULT* pResult)
{
	UpdateData(TRUE);
	m_laserControl.SetLaserIntensity(m_LaserPower);
	*pResult = 0;
}

void CDialogLaser::OnReleasedcaptureShutterSlider(NMHDR* pNMHDR, LRESULT* pResult)
{
	UpdateData(TRUE);
	m_laserControl.SetCameraShutter(m_CameraShutter);
	*pResult = 0;
}

void CDialogLaser::OnRoiButton()
{
	m_wndProfile.OnRoiButton();
	GetDlgItem(IDC_ROI_EDIT)->SetWindowTextA(m_wndProfile.m_ROI_str);
}



// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogLaser::OnSize(UINT nFlag, int cx, int cy)
{
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	CRect rect;
	GetDlgItem(IDC_STATIC_PROFILE)->GetClientRect(&rect);
	int cx1 = rect.Width();
	int cy1 = rect.Height();

	m_wndProfile.MoveWindow(0,0, cx1,cy1);
}

void CDialogLaser::EnableControls()
{
	BOOL bConnected = m_laserControl.IsConnected();
	GetDlgItem(IDC_LASER_BUTTON)->EnableWindow(bConnected);

	BOOL bLaserOn = m_laserControl.IsLaserOn();
	GetDlgItem(IDC_ROI_BUTTON)->EnableWindow(bLaserOn);
	GetDlgItem(IDC_BUTTON_ROI_RESET)->EnableWindow(bConnected);
	GetDlgItem(IDC_AUTOLASER_CHECK)->EnableWindow(bConnected);
	GetDlgItem(IDC_LASERPOWER_SLIDER)->EnableWindow(bConnected && !m_bAutoLaser);
	GetDlgItem(IDC_SHUTTER_SLIDER)->EnableWindow(bConnected && !m_bAutoLaser);
	GetDlgItem(IDC_CHECK_SHIFT_TO_CENTRE)->EnableWindow(bConnected);
	GetDlgItem(IDC_CHECK_SHOW_RAW_DATA)->EnableWindow(bConnected);
	GetDlgItem(IDC_LASER_BUTTON)->SetWindowText(bLaserOn ? _T("Laser Off") : _T("LaserOn"));

	if (bConnected)
	{
		CRect rect;
		m_laserControl.GetCameraRoi(rect);

	//	m_SerialNumber = m_laserControl.GetSerialNumber();
	//	m_wndProfile.m_ROI_str.Format("%d,%d-%d,%d", rect.left, rect.top, rect.right, rect.bottom);



		CString str;
		unsigned short major, minor;
		m_laserControl.GetLaserVersion(major, minor);
		str.Format("%d.%d", (int)major, (int)minor);
		m_sensor_version.SetWindowText(str);
		GetDlgItem(IDC_ROI_EDIT)->SetWindowTextA(m_wndProfile.m_ROI_str);
	}
}

void CDialogLaser::OnClickedButtonRoiReset()
{
	// TODO: Add your control notification handler code here
	CRect rect_roi;
	rect_roi.left = rect_roi.top = 0;
	rect_roi.right = SENSOR_WIDTH - 1;
	rect_roi.bottom = SENSOR_HEIGHT - 1;

	UpdateData(TRUE);
	m_wndProfile.m_ROI_str.Format("(%d,%d)(%d,%d)", (int)rect_roi.left, (int)rect_roi.top, (int)rect_roi.right, (int)rect_roi.bottom);
	m_laserControl.SetCameraRoi(rect_roi);
	UpdateData(FALSE);
}

void CDialogLaser::OnClickedCheck()
{
	// TODO: Add your control notification handler code here
	if (m_bInit)
	{
		BOOL bForceGap = m_laserControl.m_bForceGap;
		BOOL bForceWeld = m_laserControl.m_bForceWeld;
		UpdateData(TRUE);
		if (m_laserControl.m_bForceGap && m_laserControl.m_bForceWeld)
		{
			if (m_laserControl.m_bForceGap != bForceGap)
				m_laserControl.m_bForceWeld = FALSE;
			else
				m_laserControl.m_bForceGap = FALSE;
			UpdateData(FALSE);
		}
		EnableControls();
	}
}
