// DialogLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogLaser.h"
#include "laserControl.h"
#include "LaserProfile.h"
#include "SLS_Comms.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDialogLaser dialog

IMPLEMENT_DYNAMIC(CDialogLaser, CDialogEx)

CDialogLaser::CDialogLaser(CMotionControl& motion, CLaserControl& laser, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LASER, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_wndProfile(laser, RGB(10, 10, 10))
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_bInit = FALSE;
	m_bCheck = FALSE;

	m_ROI_stage = 0;
	m_ROI_str = _T("");

	m_LaserPower = 0;
	m_CameraShutter = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

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
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_USER_UPDATE_DIALOG, OnUserUpdateDialog)
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

	// Dimension of laser Display
	m_ImageDisplayRect.SetRect(DISP_MARGIN_HEIGHT, DISP_MARGIN_VERT, DISP_MARGIN_HEIGHT + DISP_WIDTH, DISP_MARGIN_VERT + DISP_HEIGHT);

	m_laser_power_sld.SetRange(1, 100);
	m_laser_power_sld.SetTicFreq(10);
	m_laser_power_sld.SetPos(1);

	m_camera_shutter_sld.SetRange(1, 100);
	m_camera_shutter_sld.SetTicFreq(10);
	m_camera_shutter_sld.SetPos(1);

//	EnableControls();
	SetTimer(1, 25*50, NULL);

	m_bInit = TRUE;
	PostMessage(WM_SIZE);

	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDialogLaser::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}

LRESULT CDialogLaser::OnUserUpdateDialog(WPARAM, LPARAM)
{
	CString str;

	GetDlgItem(IDC_JOINTPOS_EDIT)->SetWindowTextA(m_wndProfile.m_jointPos_str);
	GetDlgItem(IDC_GAPVALUE_EDIT)->SetWindowTextA(m_wndProfile.m_gapVal_str);
	GetDlgItem(IDC_EDGESPOS_EDIT)->SetWindowTextA(m_wndProfile.m_edgesPos_str);
	GetDlgItem(IDC_MISMATCHVALUE_EDIT)->SetWindowTextA(m_wndProfile.m_mismVal_str);

	str.Format("%d", m_wndProfile.m_profile_count);
	GetDlgItem(IDC_PROFILECOUNT_EDIT)->SetWindowTextA(str);

	return 0L;

}

void CDialogLaser::OnLaserButton()
{
	m_laserControl.TurnLaserOn( !m_laserControl.IsLaserOn() );
//	EnableControls();
}

void CDialogLaser::OnTimer(UINT nIDEvent)
{
	CString tstr;

	double h_mm = 0.0, v_mm = 0.0;
	double h2_mm = 0.0, v2_mm = 0.0;


	if (!m_laserControl.IsConnected() )
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

	m_wndProfile.m_profile_count = m_wndProfile.m_image_count;

	EnableControls();
	CDialog::OnTimer(nIDEvent);
}

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

	if (m_ROI_stage == 0)
	{
		m_ROI_str = "Click on lower left corner";
		m_ROI_stage = 1;
	}
	else
	{
		CRect rect_roi(0, 0, 1023, 1279);
		m_laserControl.SetCameraRoi(rect_roi);
//		SLSSetCameraRoi(rect_roi.left, rect_roi.top, rect_roi.right, rect_roi.bottom);
		m_ROI_str.Format("%d,%d-%d,%d", (int)rect_roi.left, (int)rect_roi.top, (int)rect_roi.right, (int)rect_roi.bottom);
	}

//	EnableControls();
	GetDlgItem(IDC_ROI_EDIT)->SetWindowTextA(m_ROI_str);
}



void CDialogLaser::OnLButtonDown(UINT nFlags, CPoint point)
{

	if ((point.x > DISP_MARGIN_HEIGHT) && (point.x < (DISP_MARGIN_HEIGHT + DISP_WIDTH)) &&
		(point.y > DISP_MARGIN_VERT) && (point.y < (DISP_MARGIN_VERT + DISP_HEIGHT)))
	{
		int xpix, ypix;

		xpix = (int)((float)(point.x - DISP_MARGIN_HEIGHT) / DISP_WIDTH_FACTOR);
		ypix = (int)((float)(DISP_HEIGHT + DISP_MARGIN_VERT - point.y) / DISP_HEIGHT_FACTOR);

		if (xpix < 0)
			xpix = 0;
		if (xpix > 1023)
			xpix = 1023;
		if (ypix < 0)
			ypix = 0;
		if (ypix > 1279)
			ypix = 1279;

		CRect rect_roi;
		m_laserControl.GetCameraRoi(rect_roi);

		if (m_ROI_stage == 1)
		{
			rect_roi.left = xpix;
			rect_roi.top = ypix;
			m_ROI_str = "Click on upper right corner";
			m_ROI_stage = 2;

		}
		else
			if (m_ROI_stage == 2)
			{
				if ((xpix > rect_roi.left) && (ypix > rect_roi.top))
				{
					rect_roi.right = xpix;
					rect_roi.bottom = ypix;
	//				SLSSetCameraRoi(rect_roi.left, rect_roi.top, rect_roi.right, rect_roi.bottom);
					m_ROI_str.Format("%d,%d-%d,%d", (int)rect_roi.left, (int)rect_roi.top, (int)rect_roi.right, (int)rect_roi.bottom);
				}
				else
				{
					rect_roi.left = rect_roi.top = 0;
					rect_roi.right = 1023;
					rect_roi.bottom = 1279;

	//				SLSSetCameraRoi(rect_roi.left, rect_roi.top, rect_roi.right, rect_roi.bottom);
					m_ROI_str.Format("%d,%d-%d,%d", (int)rect_roi.left, (int)rect_roi.top, (int)rect_roi.right, (int)rect_roi.bottom);
				}
				m_ROI_stage = 0;
			}
		GetDlgItem(IDC_ROI_EDIT)->SetWindowTextA(m_ROI_str);
		m_laserControl.SetCameraRoi(rect_roi);
	}

	CDialog::OnLButtonDown(nFlags, point);
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

	m_wndProfile.MoveWindow(0,0, cx,cy);
}

void CDialogLaser::EnableControls()
{
	BOOL bConnected = m_laserControl.IsConnected();
	GetDlgItem(IDC_LASER_BUTTON)->EnableWindow(bConnected);

	BOOL bLaserOn = m_laserControl.IsLaserOn();
	GetDlgItem(IDC_ROI_BUTTON)->EnableWindow(bLaserOn);
	GetDlgItem(IDC_AUTOLASER_CHECK)->EnableWindow(bLaserOn);
	GetDlgItem(IDC_LASERPOWER_SLIDER)->EnableWindow(bLaserOn && !m_bAutoLaser);
	GetDlgItem(IDC_SHUTTER_SLIDER)->EnableWindow(bLaserOn && !m_bAutoLaser);


	if (bConnected)
	{
		CRect rect;
		m_laserControl.GetCameraRoi(rect);

	//	m_SerialNumber = m_laserControl.GetSerialNumber();
		m_ROI_str.Format("%d,%d-%d,%d", rect.left, rect.top, rect.right, rect.bottom);

		CString str;
		unsigned short major, minor;
		m_laserControl.GetLaserVersion(major, minor);
		str.Format("%d.%d", (int)major, (int)minor);
		m_sensor_version.SetWindowText(str);
		GetDlgItem(IDC_ROI_EDIT)->SetWindowTextA(m_ROI_str);
	}
}




