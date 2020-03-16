// DialogLaser.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogLaser.h"
#include "laserControl.h"
#include "SLS_Comms.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define FRAME_HEIGHT	1280
#define FRAME_WIDTH		1024


//static BOOL fast_scan = FALSE;
//static BOOL sensor_initialised = FALSE;

const int g_disp_margin_h = 5;
const int g_disp_margin_v = 5;
const int g_disp_width = 300;
const int g_disp_height = 400;

static Profile	g_profile;
static Measurement g_measure;


static float	g_disp_height_factor,g_disp_width_factor;
static int		g_image_count = 0;

//float	laser_temperature = -1.0f;
//int __stdcall callback_fct(int val);

//int msgs_received = 0;

static bool	g_valid_joint_pos = false;
static bool	g_valid_edges = false;


// CDialogLaser dialog

IMPLEMENT_DYNAMIC(CDialogLaser, CDialogEx)

CDialogLaser::CDialogLaser(CMotionControl& motion, CLaserControl& laser, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LASER, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_wndProfile(laser)
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
//	DDX_Text(pDX, IDC_PROFILECOUNT_EDIT, m_profile_count);
//	DDX_Text(pDX, IDC_JOINTPOS_EDIT, m_jointPos_str);
//	DDX_Text(pDX, IDC_MISMATCHVALUE_EDIT, m_mismVal_str);
//	DDX_Text(pDX, IDC_EDGESPOS_EDIT, m_edgesPos_str);
//	DDX_Text(pDX, IDC_GAPVALUE_EDIT, m_gapVal_str);
//	DDX_Text(pDX, IDC_ROI_EDIT, m_ROI_str);
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
	m_ImageDisplayRect.SetRect(g_disp_margin_h, g_disp_margin_v, g_disp_margin_h + g_disp_width, g_disp_margin_v + g_disp_height);

	g_disp_height_factor = (float)g_disp_height / (float)FRAME_HEIGHT;
	g_disp_width_factor = (float)g_disp_width / (float)FRAME_WIDTH;


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
/*
void CDialogLaser::OnPaint()
{
	int i, x, y;

	CPaintDC dc(this); // device context for painting
	// Draw the background of the laser display
	CBrush black_brush;
	CPen PenPrimaryEdge(PS_SOLID, 2, RGB(10, 10, 250));
	CPen PenSecondaryEdge(PS_SOLID, 1, RGB(250, 250, 10));
	CPen PenTrackingPoint(PS_SOLID, 2, RGB(10, 200, 10));
	black_brush.CreateSolidBrush(RGB(10, 10, 10));
	dc.FillRect(&m_ImageDisplayRect, &black_brush);


	if (m_laserControl.IsLaserOn())
	{
		// Display Profile
		for (i = 0; i < FRAME_WIDTH; i++)
		{
			x = g_disp_margin_h + int(float(i) * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(float(g_profile.hits[i].pos1) * g_disp_height_factor);
			dc.SetPixel(x, y, RGB(250, 50, 50));
		}
		if (g_valid_edges)
		{
			dc.SelectObject(&PenPrimaryEdge);
			x = g_disp_margin_h + int(g_measure.mp[1].x * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(g_measure.mp[1].y * g_disp_height_factor);
			dc.MoveTo(x, y - 3);
			dc.LineTo(x, y + 3);

			x = g_disp_margin_h + int(g_measure.mp[2].x * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(g_measure.mp[2].y * g_disp_height_factor);
			dc.MoveTo(x, y - 3);
			dc.LineTo(x, y + 3);
		}
		// Display Tracking Point
		if (g_valid_joint_pos)
		{
			x = g_disp_margin_h + int(g_measure.mp[0].x * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(g_measure.mp[0].y * g_disp_height_factor);
			dc.SelectObject(&PenTrackingPoint);
			dc.MoveTo(x - 7, y);
			dc.LineTo(x + 7, y);
			dc.MoveTo(x, y - 7);
			dc.LineTo(x, y + 7);
		}
	}

	CDialog::OnPaint();
}
*/
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
	static int count_comm_err = 0;
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

	m_wndProfile.m_profile_count = g_image_count;

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

	if ((point.x > g_disp_margin_h) && (point.x < (g_disp_margin_h + g_disp_width)) &&
		(point.y > g_disp_margin_v) && (point.y < (g_disp_margin_v + g_disp_height)))
	{
		int xpix, ypix;

		xpix = (int)((float)(point.x - g_disp_margin_h) / g_disp_width_factor);
		ypix = (int)((float)(g_disp_height + g_disp_margin_v - point.y) / g_disp_height_factor);

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

//////////////////////////////////////////
IMPLEMENT_DYNAMIC(CStaticLaserProfile, CWnd)

CStaticLaserProfile::CStaticLaserProfile(CLaserControl& laser)
	: CWnd()
	, m_laserControl(laser)
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_profile_count = 0;
	m_jointPos_str = _T("");
	m_mismVal_str = _T("");
	m_edgesPos_str = _T("");
	m_gapVal_str = _T("");
}

CStaticLaserProfile::~CStaticLaserProfile()
{
}

void CStaticLaserProfile::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}



void CStaticLaserProfile::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStaticLaserProfile, CWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CStaticLaserProfile message handlers
void CStaticLaserProfile::Create(CWnd* pParent)
{
	m_pParent = pParent;
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), pParent, 2, NULL);
	ASSERT(m_hWnd != NULL);

	SetTimer(1, 50, NULL);
	EnableWindow(TRUE);
	InvalidateRgn(NULL);
}

void CStaticLaserProfile::OnPaint()
{
	CRect rect;
	CPaintDC dc(this);

	GetClientRect(&rect);

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	ASSERT(memDC.m_hDC);

	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());

	CBitmap* pBitmap = memDC.SelectObject(&bitmap);
	DrawLaserProfile(&memDC);
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
}


void CStaticLaserProfile::DrawLaserProfile(CDC * pDC)
{
	int i, x, y;
	CRect rect;
	GetClientRect(&rect);


	// Draw the background of the laser display
	CBrush black_brush;
	CPen PenPrimaryEdge(PS_SOLID, 2, RGB(10, 10, 250));
	CPen PenSecondaryEdge(PS_SOLID, 1, RGB(250, 250, 10));
	CPen PenTrackingPoint(PS_SOLID, 2, RGB(10, 200, 10));
	black_brush.CreateSolidBrush(RGB(10, 10, 10));
	pDC->FillRect(&rect, &black_brush);


	if (m_laserControl.IsLaserOn())
	{
		// Display Profile
		for (i = 0; i < FRAME_WIDTH; i++)
		{
			x = g_disp_margin_h + int(float(i) * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(float(g_profile.hits[i].pos1) * g_disp_height_factor);
			pDC->SetPixel(x, y, RGB(250, 50, 50));
		}
		if (g_valid_edges)
		{
			pDC->SelectObject(&PenPrimaryEdge);
			x = g_disp_margin_h + int(g_measure.mp[1].x * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(g_measure.mp[1].y * g_disp_height_factor);
			pDC->MoveTo(x, y - 3);
			pDC->LineTo(x, y + 3);

			x = g_disp_margin_h + int(g_measure.mp[2].x * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(g_measure.mp[2].y * g_disp_height_factor);
			pDC->MoveTo(x, y - 3);
			pDC->LineTo(x, y + 3);
		}
		// Display Tracking Point
		if (g_valid_joint_pos)
		{
			x = g_disp_margin_h + int(g_measure.mp[0].x * g_disp_width_factor);
			y = g_disp_margin_v + g_disp_height - int(g_measure.mp[0].y * g_disp_height_factor);
			pDC->SelectObject(&PenTrackingPoint);
			pDC->MoveTo(x - 7, y);
			pDC->LineTo(x + 7, y);
			pDC->MoveTo(x, y - 7);
			pDC->LineTo(x, y + 7);
		}
	}
}

void CStaticLaserProfile::OnTimer(UINT_PTR nIDEvent)
{
	static int count = 0;
	static int count_comm_err = 0;
	CString tstr;

	double h_mm = 0.0, v_mm = 0.0;
	double h2_mm = 0.0, v2_mm = 0.0;


	if (m_laserControl.IsConnected() && m_laserControl.IsLaserOn() && m_laserControl.GetLaserMeasurment(g_measure))
	{
		if (g_measure.mp[0].status == 0)
		{
			ConvPixelToMm((double)g_measure.mp[0].x, (double)g_measure.mp[0].y, &h_mm, &v_mm);
			m_jointPos_str.Format("(%.1f,%.1f)", h_mm, v_mm);
			g_valid_joint_pos = true;
		}
		else
		{
			m_jointPos_str = "-----";
			g_valid_joint_pos = false;
		}
		if ((g_measure.mp[1].status == 0) || (g_measure.mp[2].status == 0))
		{
			ConvPixelToMm((double)g_measure.mp[1].x, (double)g_measure.mp[1].y, &h_mm, &v_mm);
			ConvPixelToMm((double)g_measure.mp[2].x, (double)g_measure.mp[2].y, &h2_mm, &v2_mm);
			m_edgesPos_str.Format("%+5.1f,%+5.1f %+5.1f,%+5.1f", h_mm, v_mm, h2_mm, v2_mm);
			g_valid_edges = true;
		}
		else
		{
			m_edgesPos_str = "-----";
			g_valid_edges = false;
		}
		if (g_measure.mv[0].status == 0)
		{
			m_gapVal_str.Format("%+5.1f", g_measure.mv[0].val);
		}
		else
		{
			m_gapVal_str = "-----";
		}
		if (g_measure.mv[1].status == 0)
		{
			m_mismVal_str.Format("%+5.1f", g_measure.mv[1].val);
		}
		else
		{
			m_mismVal_str = "-----";
		}
		if (m_laserControl.GetProfile(g_profile))
		{
			g_image_count++;
		}
		m_profile_count = g_image_count;

		if (m_profile_count % 10 == 0)
			m_pParent->PostMessageA(m_nMsg);
	}

	InvalidateRgn(NULL);
	CWnd::OnTimer(nIDEvent);
}




