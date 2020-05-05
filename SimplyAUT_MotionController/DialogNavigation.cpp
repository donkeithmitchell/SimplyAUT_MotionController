// DialogNavigation.cpp : implementation file
//

#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "DialogNavigation.h"
#include "WeldNavigation.h"
#include "define.h"

static double PI = 4 * atan(1.0);

IMPLEMENT_DYNAMIC(CStaticNavigation, CWnd)

CStaticNavigation::CStaticNavigation(const NAVIGATION_PID& pid, const CArray<double,double>& fft_data)
	: CWnd()
	, m_pid(pid)
	, m_fft_data(fft_data)
{
}

CStaticNavigation::~CStaticNavigation()
{
}

BEGIN_MESSAGE_MAP(CStaticNavigation, CWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
END_MESSAGE_MAP()


void CStaticNavigation::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}



// CStaticLaser message handlers
void CStaticNavigation::Create(CWnd* pParent)
{
	BOOL ret = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), pParent, 1, NULL);
	ASSERT(m_hWnd != NULL);

	EnableWindow(TRUE);
	InvalidateRgn(NULL);
}

void CStaticNavigation::OnPaint()
{
	CRect rect;
	CPaintDC dc(this);
	GetClientRect(&rect);

	// use bitblt so faster, and so that do not see drawing
	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	ASSERT(memDC.m_hDC);

	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());

	CBitmap* pBitmap = memDC.SelectObject(&bitmap);
	DrawNavigationProfile(&memDC); 
	//	m_wndLaserProfile.InvalidateRgn(NULL);
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
}

void CStaticNavigation::OnSize(UINT nFlag, int cx, int cy)
{
	CWnd::OnSize(nFlag, cx, cy);
}

// draw a circle with the current location of the crawler noted on the circle
// also note the SET home location and the line detected by the RGB sensor
// this acts as a progress report for a scan
// 911 the actual position can be set, but not required
void CStaticNavigation::DrawNavigationProfile(CDC* pDC)
{
	CRect rect1;
	GetClientRect(&rect1);

	CBrush brush(::GetSysColor(COLOR_3DFACE));
	pDC->FillRect(&rect1, &brush);

	// draw thew pipe
	CGdiObject* pBrush1 = pDC->SelectStockObject(HOLLOW_BRUSH);
	CPen pen(PS_SOLID, 0, RGB(0, 0, 0));
	CPen* pPen = pDC->SelectObject(&pen);

	int nSize = (int)m_fft_data.GetSize();
	if (nSize == 0)
		return;

	double fMin = FLT_MAX;
	double fMax = -FLT_MAX;
	for (int i = 0; i < nSize; ++i)
	{
		fMin = min(fMin, m_fft_data[i]);
		fMax = max(fMax, m_fft_data[i]);
	}

	if (fMax == -FLT_MAX || fMin == FLT_MAX || fMax == fMin)
		return;

	pDC->MoveTo(0, 0);
	pDC->LineTo(0, rect1.Height());

	pDC->MoveTo(0, rect1.Height() / 2);
	pDC->LineTo(rect1.Width(), rect1.Height() / 2);

	double scaleX = (double)rect1.Width() / (double)nSize;
	double scaleY = (double)rect1.Height() / (fMax - fMin);

	for (int i = 0; i < nSize; ++i)
	{
		int x = (int)(scaleX * (double)i + 0.5);
		int y = (int)((double)rect1.Height()/2 - scaleY * (m_fft_data[i]) + 0.5);
		(i == 0) ? pDC->MoveTo(x, y) : pDC->LineTo(x, y);
	}

	CPen pen2(PS_SOLID, 0, RGB(255, 0, 0));
	CPen* pPen2 = pDC->SelectObject(&pen2);

	// now draw the profile based on Tu
	double len = 1000.0 * nSize / m_pid.Tu_srate; // ms
	double scaleY2 = (double)rect1.Height() / 4.0;
	double phase = (double)m_pid.Phz * PI / 180.0;
	for (int i = 0; i < nSize; ++i)
	{
		int x = (int)(scaleX * (double)i + 0.5);
		double val = sin(phase + 2 * PI * i / nSize * len / m_pid.Tu);
		int y = (int)((double)rect1.Height() / 2 - scaleY2 * (val) + 0.5);
		(i == 0) ? pDC->MoveTo(x, y) : pDC->LineTo(x, y);
	}


}


// CDialogNavigation

IMPLEMENT_DYNAMIC(CDialogNavigation, CDialogEx)

CDialogNavigation::CDialogNavigation(NAVIGATION_PID& pid, CArray<double,double>& fft_data, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_NAVIGATION, pParent)
	, m_staticNavigation(pid, fft_data)
	, m_pid(pid)
{
	m_pParent = pParent;
	m_nMsg = 0;
	m_bCheck = FALSE;
	m_bInit = FALSE;
}

CDialogNavigation::~CDialogNavigation()
{
}


BEGIN_MESSAGE_MAP(CDialogNavigation, CWnd)
	ON_WM_SIZE()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TU, &CDialogNavigation::OnDeltaposSpinTu)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_PHZ, &CDialogNavigation::OnDeltaposSpinPhz)

	ON_BN_CLICKED(IDC_BUTTON_RESET, &CDialogNavigation::OnButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_CALC_PI, &CDialogNavigation::OnButtonCalcPI)
	ON_BN_CLICKED(IDC_BUTTON_CALC_PD, &CDialogNavigation::OnButtonCalcPD)
	ON_BN_CLICKED(IDC_BUTTON_CALC_PID2, &CDialogNavigation::OnButtonCalcPID)
	ON_BN_CLICKED(IDC_BUTTON_CALC_NO_OVERSHOOT, &CDialogNavigation::OnButtonCalcCrit)
	ON_BN_CLICKED(IDC_BUTTON_CALC_ENABLE, &CDialogNavigation::OnButtonCalcEnable)

	ON_BN_CLICKED(IDC_CHECK_SIMULATION, &CDialogNavigation::OnButtonSimulation)
	ON_BN_CLICKED(IDC_RADIO1, &CDialogNavigation::OnClickNavType)
	ON_BN_CLICKED(IDC_RADIO2, &CDialogNavigation::OnClickNavType)
	ON_BN_CLICKED(IDC_BUTTON_SIMULATE, &CDialogNavigation::OnClickSimulate)

//	ON_EN_CHANGE(IDC_EDIT_NAV_P, &CDialogNavigation::OnEditChangePID)
//	ON_EN_CHANGE(IDC_EDIT_NAV_I, &CDialogNavigation::OnEditChangePID)
//	ON_EN_CHANGE(IDC_EDIT_NAV_D, &CDialogNavigation::OnEditChangePID)
	ON_EN_CHANGE(IDC_EDIT_NAV_TU, &CDialogNavigation::OnEditChangePID)
	ON_EN_CHANGE(IDC_EDIT_NAV_PHZ, &CDialogNavigation::OnEditChangePID)
END_MESSAGE_MAP()

void CDialogNavigation::DoDataExchange(CDataExchange* pDX)
{
	CString text;
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO1, m_pid.nav_type);
	DDX_Control(pDX, IDC_SPIN_TU, m_spinTu);
	DDX_Control(pDX, IDC_SPIN_PHZ, m_spinPhz);
	if (!pDX->m_bSaveAndValidate)
	{
		UINT nIDD[] = { IDC_STATIC_PID_RMS, IDC_STATIC_PID_RMS_P, IDC_STATIC_PID_RMS_I, IDC_STATIC_PID_RMS_D };
		for (int i = 0; i < 4; ++i)
		{
			text.Format("%.3f", m_pid.PID_rms[i]);
			GetDlgItem(nIDD[i])->SetWindowTextA(text);
		}
	}

	DDX_Text(pDX, IDC_EDIT_NAV_P, m_pid.Kp);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.Kp, 0.1, 100.0);
	DDX_Text(pDX, IDC_EDIT_NAV_I, m_pid.Ki);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.Ki, 0, 100.0);
	if (pDX->m_bSaveAndValidate)
	{
		GetDlgItem(IDC_EDIT_NAV_I_ACCUM)->GetWindowTextA(text);
		if( atoi(text) == 0 )
			GetDlgItem(IDC_EDIT_NAV_I_ACCUM)->SetWindowTextA("0");
	}
	DDX_Text(pDX, IDC_EDIT_NAV_I_ACCUM, m_pid.I_accumulate_ms);
	if (!pDX->m_bSaveAndValidate && m_pid.I_accumulate_ms == 0)
		GetDlgItem(IDC_EDIT_NAV_I_ACCUM)->SetWindowTextA("All");
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.I_accumulate_ms, 0, 10000);
	DDX_Text(pDX, IDC_EDIT_NAV_D, m_pid.Kd);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.Kd, 0, 2000);
	DDX_Text(pDX, IDC_EDIT_NAV_D_LEN, m_pid.D_length_ms);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.D_length_ms, 100, 2500);
	DDX_Text(pDX, IDC_EDIT_NAV_PIVOT, m_pid.pivot_percent);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.pivot_percent, 10, 95);

	DDX_Text(pDX, IDC_EDIT_NAV_DELAY, m_pid.turn_dist);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.turn_dist, 0, 20.0);

	DDX_Text(pDX, IDC_EDIT_NAV_MAX_TURN, m_pid.max_turn_rate);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.max_turn_rate, MAX_TURN_RATE1, MIN_TURN_RATE1);

	DDX_Text(pDX, IDC_EDIT_NAV_MAX_TURN_LEN, m_pid.max_turn_rate_len);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.max_turn_rate_len, 0, 1000);

	DDX_Text(pDX, IDC_EDIT_NAV_MAX_TURN_100, m_pid.max_turn_rate_pre);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.max_turn_rate_pre, MAX_TURN_RATE1, MIN_TURN_RATE1);

	DDX_Text(pDX, IDC_EDIT_NAV_TU, m_pid.Tu);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.Tu, 0, 10000);

	DDX_Text(pDX, IDC_EDIT_NAV_PHZ, m_pid.Phz);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.Phz, -360, 360);

	DDX_Text(pDX, IDC_EDIT_NAV_START_SPEED, m_pid.start_speed);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.start_speed, MIN_MOTOR_SPEED, MAX_MOTOR_SPEED);

	DDX_Text(pDX, IDC_EDIT_NAV_START_DIST, m_pid.start_dist);
	if (m_bCheck)
		DDV_MinMaxDouble(pDX, m_pid.start_dist, MIN_START_DIST, MAX_START_DIST);

	DDX_Text(pDX, IDC_EDIT_NAV_GAP_PREDICT, m_pid.gap_predict);
	if (m_bCheck)
		DDV_MinMaxInt(pDX, m_pid.gap_predict, -10, 10);

	DDX_Check(pDX, IDC_CHECK_SIMULATION, m_pid.simulation);

	if (!pDX->m_bSaveAndValidate)
		text = _T(m_pid.simulation_file);
	DDX_Text(pDX, IDC_EDIT_SIMULATION, text);
	if (pDX->m_bSaveAndValidate)
		strncpy_s(m_pid.simulation_file, text, sizeof(m_pid.simulation_file));
}

// do not want to check control values on every call to UpdateData(TRUE)
// only when about to scan, etc.
BOOL CDialogNavigation::CheckVisibleTab()
{
	m_bCheck = TRUE; // enable DDV calls
	BOOL ret = UpdateData(TRUE);
	m_bCheck = FALSE;
	return ret;
}

void CDialogNavigation::OnDeltaposSpinTu(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	int inc = (pNMUpDown->iDelta < 0) ? -1 : 1;

	UpdateData(TRUE);
	m_pid.Tu += inc * 10;
	UpdateData(FALSE);

	m_staticNavigation.InvalidateRgn(NULL);
	*pResult = 0;
}

void CDialogNavigation::OnDeltaposSpinPhz(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	int inc = (pNMUpDown->iDelta < 0) ? -1 : 1;

	UpdateData(TRUE);
	m_pid.Phz += inc * 10;
	UpdateData(FALSE);

	m_staticNavigation.InvalidateRgn(NULL);
	*pResult = 0;
}



void CDialogNavigation::Create(CWnd* pParent)
{
	CDialogEx::Create(IDD_DIALOG_NAVIGATION, pParent);
	ShowWindow(SW_HIDE);
}

BOOL CDialogNavigation::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_staticNavigation.Create(GetDlgItem(IDC_STATIC_NAV));
	m_staticNavigation.ShowWindow(SW_SHOW);
	m_spinTu.SetRange(0, 1000);
	m_spinPhz.SetRange(-360, 360);

	m_bInit = TRUE;
	EnableControls();
	PostMessage(WM_SIZE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDialogNavigation::EnableControls()
{
	GetDlgItem(IDC_EDIT_NAV_P)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_I)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_D)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_PIVOT)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_EDIT_NAV_DELAY)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_BUTTON_CALC_ENABLE)->EnableWindow(m_pid.nav_type == 0);
	GetDlgItem(IDC_BUTTON_CALC_PI)->EnableWindow(m_pid.nav_type == 0 && m_pid.Tu > 0);
	GetDlgItem(IDC_BUTTON_CALC_PID2)->EnableWindow(m_pid.nav_type == 0 && m_pid.Tu > 0);
	GetDlgItem(IDC_EDIT_SIMULATION)->EnableWindow(m_pid.simulation);
	GetDlgItem(IDC_BUTTON_SIMULATE)->EnableWindow(m_pid.simulation);

	UpdateData(FALSE);
}

void CDialogNavigation::OnEditChangePID()
{
	CString str;
	GetDlgItem(IDC_EDIT_NAV_TU)->GetWindowText(str);
	m_pid.Tu = atoi(str);
	GetDlgItem(IDC_EDIT_NAV_PHZ)->GetWindowText(str);
	m_pid.Phz = atoi(str);

	UpdateData(TRUE);
	EnableControls();
	m_staticNavigation.InvalidateRgn(NULL);
}


// CDialogLaser message handlers
void CDialogNavigation::Init(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
}

void	CDialogNavigation::Serialize(CArchive& ar) 
{
	const int MASK = 0xCDCDCDCD;
	int mask = MASK;
	if (ar.IsStoring())
	{
		UpdateData(TRUE);
		ar << m_pid.nav_type;
		ar << m_pid.Kp;
		ar << m_pid.Ki;
		ar << m_pid.Kd;
		ar << m_pid.max_turn_rate;
		ar << m_pid.pivot_percent;
		ar << m_pid.D_length_ms;
		ar << m_pid.I_accumulate_ms;
		ar << m_pid.Tu;
		ar << mask;
	}
	else
	{
#ifdef _DEBUG_TIMING_
		try
		{
			ar >> m_pid.nav_type;
			ar >> m_pid.Kp;
			ar >> m_pid.Ki;
			ar >> m_pid.Kd;
			ar >> m_pid.max_turn_rate;
			ar >> m_pid.pivot_percent;
			ar >> m_pid.D_length_ms;
			ar >> m_pid.I_accumulate_ms;
			ar >> m_pid.Tu;
			ar >> mask;
		}
		catch (CArchiveException * e1)
		{
			m_pid.Reset();
			e1->Delete();

		}
		if (mask != MASK)
			m_pid.Reset();
#else
		m_pid.Reset();
#endif
		UpdateData(FALSE);
	}

}

void CDialogNavigation::OnButtonSimulation()
{
	UpdateData(TRUE);
	EnableControls();
}

void CDialogNavigation::OnButtonCalcPID(int mask)
{
	UpdateData(TRUE);

	if (m_pid.Tu > 0)
	{
		double Ku = NAVIGATION_P_OSCILLATE;
		double Tu = m_pid.Tu / 1000.0;
		switch (mask)
		{
		case PID_CRIT: // crtitical damping
			m_pid.Kp = Ku / 5.0;
			m_pid.Ki = 2.0 / 5.0 * Ku / Tu;
			m_pid.Kd = Ku * Tu / 15.0;
			break;
		case PID_P: // P
			m_pid.Kp = Ku * 0.5;
			m_pid.Ki = 0;
			m_pid.Kd = 0.0;
			break;
		case PID_PI: // PI
			m_pid.Kp = Ku * 0.45;
			m_pid.Ki = 0.54 * Ku / Tu;
			m_pid.Kd = 0.0;
			break;
		case PID_PD: // PD
			m_pid.Kp = Ku * 0.80;
			m_pid.Ki = 0.0;
			m_pid.Kd = 1.0 * Ku * Tu / 10.0;
			break;
		case PID_PID: // PID
			m_pid.Kp = Ku * 0.60;
			m_pid.Ki = 1.20 * Ku / Tu;
			m_pid.Kd = 3.0 * Ku * Tu / 40.0;
			break;
		case PID_CALIB:
			m_pid.Kp = Ku;
			m_pid.Ki = 0.0;
			m_pid.Kd = 0.0;
			m_pid.max_turn_rate = MAX_TURN_RATE1;
			m_pid.max_turn_rate_pre = MAX_TURN_RATE1;
			m_pid.max_turn_rate_len = 0;
			break;
		}
	
		m_pid.I_accumulate_ms = (int)(m_pid.Tu / 2.0 + 0.5);
		m_pid.D_length_ms = (int)(m_pid.Tu / 4.0 + 0.5);
	}
	UpdateData(FALSE);
}


void CDialogNavigation::OnButtonReset()
{
	// TODO: Add your command handler code here
	UpdateData(TRUE);
	m_pid.Reset();
	UpdateData(FALSE);
	EnableControls();
}

void CDialogNavigation::OnClickSimulate()
{
	UpdateData(TRUE);

//	char my_documents[MAX_PATH];
//	HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
//	if (result != S_OK)
//		return;
//
//	// insure that the sub follder exists
//	CString szFolder;
//	szFolder.Format("%s\\SimplyAUTFiles\\%s", my_documents, m_szProject);

	CFileDialog dlg(TRUE/*open*/, ".txt", m_pid.simulation_file);
	if (dlg.DoModal() != IDOK)
		return;

	CString szFile = dlg.GetFileName();
	::_splitpath_s(szFile, NULL, 0, NULL, 0, m_pid.simulation_file, sizeof(m_pid.simulation_file), NULL, 0);
	UpdateData(FALSE);
}

void CDialogNavigation::OnClickNavType()
{
	UpdateData(TRUE);
	EnableControls();
}

void CDialogNavigation::OnSize(UINT nFlag, int cx, int cy)
{
	CRect rect;
	if (!m_bInit)
		return;

	CWnd::OnSize(nFlag, cx, cy);
	GetDlgItem(IDC_STATIC_NAV)->GetClientRect(&rect);
	m_staticNavigation.MoveWindow(&rect);
	m_staticNavigation.PostMessage(WM_SIZE);
}



