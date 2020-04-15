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

static UINT ThreadWaitCalibration(LPVOID param)
{
	CDialogMag* this2 = (CDialogMag*)param;
	return this2->ThreadWaitCalibration();
}


// CDialogMag dialog

IMPLEMENT_DYNAMIC(CDialogMag, CDialogEx)

CDialogMag::CDialogMag(CMotionControl& motion, CLaserControl& laser, CMagControl& mag, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LASER, pParent)
	, m_motionControl(motion)
	, m_laserControl(laser)
	, m_magControl(mag)
	, m_wndMag(mag, m_bScanReverse, m_bCalibrateWithLaser, m_fCalibrationLength)

	, m_szRGBEncCount(_T(""))
	, m_szRGBValue(_T(""))
	, m_szRGBCalValue(_T(""))
	, m_szRGBCalValueSet(_T(""))
	, m_szRGBLinePresent(_T(""))
{
	m_pParent = NULL;
	m_nMsg = 0;
	m_bInit = FALSE;
	m_bCheck = FALSE;
	m_nCalibrating = 0;
	m_hThreadWaitCalibration = FALSE;
	ResetParameters();

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
	DDX_Text(pDX, IDC_STATIC_MAG_ENCODER, m_szRGBEncCount);
	DDX_Text(pDX, IDC_STATIC_MAG_RGB, m_szRGBValue);
	DDX_Text(pDX, IDC_STATIC_MAG_RGB_CAL, m_szRGBCalValue);
	DDX_Text(pDX, IDC_EDIT_RGB_CAL, m_szRGBCalValueSet);
	DDX_Text(pDX, IDC_STATIC_MAG_RGB_LINE, m_szRGBLinePresent);
	DDX_Control(pDX, IDC_STATIC_RGB, m_staticRGB);
	DDX_Text(pDX, IDC_EDIT_RGB_CALIBRATION, m_fCalibrationLength);
	DDX_Text(pDX, IDC_EDIT_RGB_SPEED, m_szCalibrationSpeed);
	DDX_Check(pDX, IDC_CHECK_USE_LASER, m_bCalibrateWithLaser);
	DDX_Check(pDX, IDC_CHECK_RETURN_TO_START, m_bCalibrateReturnToStart);
}


BEGIN_MESSAGE_MAP(CDialogMag, CDialogEx)
	//	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_CHECK_MAG_ENABLE, &CDialogMag::OnClickedCheckMagEnable)
	ON_BN_CLICKED(IDC_BUTTON_MAG_CLEAR, &CDialogMag::OnClickedButtonEncoderClear)
	ON_BN_CLICKED(IDC_BUTTON_SET_CALIBRATION, &CDialogMag::OnClickedButtonSetCalValue)
	ON_BN_CLICKED(IDC_BUTTON_RGB_CALIBRATION, &CDialogMag::OnClickedButtonRgbCalibration)
	ON_BN_CLICKED(IDC_CHECK_USE_LASER, &CDialogMag::OnClickedCheckUseLaser)
	ON_BN_CLICKED(IDC_CHECK_RETURN_TO_START, &CDialogMag::OnClickedCheckReturnToStart)
	ON_MESSAGE(WM_WAIT_CALIBRATION_FINISHED, &CDialogMag::OnUserWaitCalibrationFinished)
	ON_MESSAGE(WM_USER_UPDATE_DIALOG, &CDialogMag::OnUserUpdateDialog)
	ON_MESSAGE(WM_ARE_MOTORS_STOPPED, &CDialogMag::OnUserAreMotorsStopped)
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
	m_wndMag.Create( GetDlgItem(IDC_STATIC_RGB) );

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

void CDialogMag::OnTimer(UINT nIDEvent)
{
	switch (nIDEvent)
	{
		case TIMER_NOTE_VALUES:
		{
			// get the status on a t6ime, then only reference this status
			if (m_laserControl.IsConnected() && IsWindowVisible())
			{
				BOOL bConnected = m_magControl.IsConnected();
				GetDlgItem(IDC_CHECK_MAG_ENABLE)->EnableWindow(bConnected);
				GetDlgItem(IDC_BUTTON_MAG_CLEAR)->EnableWindow(bConnected);

				int nCal = m_magControl.GetRGBCalibrationValue();
				if (nCal != INT_MAX)
					m_szRGBCalValue.Format("%d", nCal);
				else
					m_szRGBCalValue.Format("***");
				GetDlgItem(IDC_STATIC_MAG_RGB_CAL)->SetWindowText(m_szRGBCalValue);

				int nEncoderCount = m_magControl.GetMagStatus(MAG_IND_ENC_CNT);
				if (nEncoderCount != INT_MAX)
					m_szRGBEncCount.Format("%d", nEncoderCount);
				else
					m_szRGBEncCount.Format("***");
				GetDlgItem(IDC_STATIC_MAG_ENCODER)->SetWindowText(m_szRGBEncCount);

				//		int red, green, blue;
				//		int colour = m_magControl.GetRGBValues(red, green, blue);
				int colour = m_magControl.GetRGBSum();
				if (colour != INT_MAX)
				{
					//			m_szRGBValue.Format("(%d,%d,%d) %d", red, green, blue, colour);
					m_szRGBValue.Format("%d", colour);
				}
				else
					m_szRGBValue.Format(_T(""));

				// chevck if the line is presdent
				int bPresent = m_magControl.GetMagStatus(MAG_IND_RGB_LINE) == 1;
				m_szRGBLinePresent.Format(bPresent ? _T("Present") : _T(""));
				GetDlgItem(IDC_STATIC_MAG_RGB_LINE)->SetWindowText(m_szRGBLinePresent);

				int bLocked = m_magControl.GetMagStatus(MAG_IND_MAG_LOCKOUT) == 1;
				m_butMagEnable.SetCheck(bLocked == 0);

				GetDlgItem(IDC_STATIC_MAG_RGB)->SetWindowText(m_szRGBValue);


				// if this changes from the last call, then pass on to the scan tab
				HBITMAP hBitmap1 = (HBITMAP)m_bitmapConnect.GetSafeHandle();
				HBITMAP hBitmap2 = (HBITMAP)m_bitmapDisconnect.GetSafeHandle();

				int bMagOn = m_magControl.GetMagStatus(MAG_IND_MAG_ON) == 1;
				m_butMagOn.SetBitmap((bMagOn == 1) ? hBitmap1 : hBitmap2);
				GetDlgItem(IDC_BUTTON_RGB_CALIBRATION)->EnableWindow(bMagOn);

				// on a change this will cause the p[arenrt to update all tabs
				static int sLastMagOn = INT_MAX;
				if (bMagOn != sLastMagOn && m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
				{
					sLastMagOn = bMagOn;
					m_pParent->PostMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SETBITMAPS);
				}
			}
			break;
		}
		case TIMER_NOTE_CALIBRATION:
		{
			double pos = m_motionControl.GetAvgMotorPosition();
			if (m_nCalibrating == 1)
			{
				double val = GetCalibrationValue();
				m_magControl.NoteRGBCalibration(pos, val, (int)(m_fCalibrationLength + 0.5) );
			}
			else if (m_nCalibrating > 1)
			{
				if (pos >= 0)
					m_magControl.SetRGBCalibrationPos(pos);
			}
			m_wndMag.InvalidateRgn(NULL);
			break;
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void CDialogMag::Serialize(CArchive& ar)
{
	const int MASK = 0xCDCDCDCD;
	int mask = MASK;

	if (ar.IsStoring())
	{
		UpdateData(TRUE);
		ar << m_fCalibrationLength;
		ar << m_szCalibrationSpeed;
		ar << m_bCalibrateWithLaser;
		ar << m_bCalibrateReturnToStart;
		ar << m_bScanReverse;
		ar << mask;
	}
	else
	{
		try
		{
			ar >> m_fCalibrationLength;
			ar >> m_szCalibrationSpeed;
			ar >> m_bCalibrateWithLaser;
			ar >> m_bCalibrateReturnToStart;
			ar >> m_bScanReverse;
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

void CDialogMag::ResetParameters()
{
	m_fCalibrationLength = 250;
	m_szCalibrationSpeed = _T("20.0");
	m_bCalibrateWithLaser = FALSE;
	m_bCalibrateReturnToStart = FALSE;
	m_bScanReverse = FALSE;
}

double CDialogMag::GetCalibrationValue()
{
	if (m_bCalibrateWithLaser)
	{
		if (m_laserControl.GetProfile(10))
		{
			m_laserControl.CalcLaserMeasures(0.0/*pos*/, NULL, -1);
			double avg_side_height = (m_laserControl.m_measure2.weld_left_height_mm + m_laserControl.m_measure2.weld_right_height_mm) / 2;
			double weld_cap_height = m_laserControl.m_measure2.weld_cap_mm.y;
			
			return -weld_cap_height;
	//		return avg_side_height - weld_cap_height; // the sides will not be well calculated
		}
		else
			return FLT_MAX;
	}
	else
		return m_magControl.GetRGBSum();
}




// CDialogMotors message handlers
// this dialog is sized to a tab, and not the size that designed into
// thus, must locate the controls on Size
void CDialogMag::OnSize(UINT nFlag, int cx, int cy)
{
	CDialogEx::OnSize(nFlag, cx, cy);
	if (!m_bInit)
		return;

	CRect rect;
	GetDlgItem(IDC_STATIC_RGB)->GetClientRect(&rect);
	m_wndMag.MoveWindow(&rect);

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

	int bLocked = m_magControl.GetMagStatus(MAG_IND_MAG_LOCKOUT) == 1;
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


void CDialogMag::OnClickedButtonSetCalValue()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	int nCalValue = atoi(m_szRGBCalValueSet);
	m_magControl.SetMagRGBCalibrationValue((double)nCalValue);
	UpdateData(FALSE);
}

// this returns the speed that requested, as compared to the actual speed of the motors
// thus must get from the motor dialog
double CDialogMag::GetRequestedMotorSpeed(double& rAccel)
{
	double speed = 0;
	rAccel = 0;
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_GETSCANSPEED, (LPARAM)(&speed));
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_GETACCEL, (LPARAM)(&rAccel));
	}

	return speed;
}

// drive the crawlewr forward by the specified number of samples, noting the mninimum recorded RGB value
// drive back to that value, set the RGB calibration
// drive at about 20 mm/sec regardless of how set in motor dialog
void CDialogMag::OnClickedButtonRgbCalibration()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);

	double speed = atof(m_szCalibrationSpeed);
	double accel; GetRequestedMotorSpeed(accel);

	int delay2 = (int)(1000.0 / speed + 0.5); // every mm
	m_nCalibrating = (m_nCalibrating > 0) ? 0 : 1;

	if (m_nCalibrating == 0 )
	{
		m_nCalibrating = 3;
		m_motionControl.StopMotors(TRUE);
		m_hThreadWaitCalibration = AfxBeginThread(::ThreadWaitCalibration, (LPVOID)this)->m_hThread;
	}
	else
	{
		KillTimer(TIMER_NOTE_VALUES);
		SetTimer(TIMER_NOTE_CALIBRATION, 1, NULL);

		if (m_bCalibrateWithLaser)
			m_laserControl.TurnLaserOn(TRUE); 

		m_magControl.EnableMagSwitchControl(FALSE);
		m_magControl.ResetEncoderCount();
		StartReadMagStatus(FALSE); // takes too mjuch time

		m_magControl.ResetRGBCalibration();

		GetDlgItem(IDC_BUTTON_RGB_CALIBRATION)->SetWindowText("Halt");
		GetDlgItem(IDC_STATIC_RGB_STATUS)->SetWindowText("Measuring Calibration Data");

		// where is now, drive until desired length past
		// this will drive the crawler pastr the line
		// will wait until the line has been passed, then calculate where it is
		m_motionControl.DefinePositions(0);
		m_motionControl.SetSlewSpeed(speed, accel);
		m_motionControl.GoToPosition(m_fCalibrationLength, FALSE/*don't wait for stop*/);

		// now that the motors are running and recording the data, 
		// wait for them to stop, so can calculate where the line is
		m_hThreadWaitCalibration = AfxBeginThread(::ThreadWaitCalibration, (LPVOID)this)->m_hThread;
	}

	UpdateData(FALSE);
}

LRESULT CDialogMag::OnUserAreMotorsStopped(WPARAM, LPARAM)
{
	return (LRESULT)!m_motionControl.AreMotorsRunning();
}

UINT CDialogMag::ThreadWaitCalibration()
{
	// send a messagew to the startup thread to see if the mnotors have stopped
	int cnt = 0;
	Sleep(100); // wait for the motors to start
	while (cnt < 4)
	{
		Sleep(1);
		if( SendMessage(WM_ARE_MOTORS_STOPPED) )
			cnt++;
		else
			cnt = 0;
	}

	PostMessage(WM_WAIT_CALIBRATION_FINISHED);
	return 0;
}

// at this point the calibration ruyn is finished, now need to calculate whewre the line is,
// navigate back to it, then note the RGB valeue at that point
LRESULT CDialogMag::OnUserWaitCalibrationFinished(WPARAM, LPARAM)
{
	CString str;
	int ret = ::WaitForSingleObject(m_hThreadWaitCalibration, 1000);
	if (ret != WAIT_OBJECT_0 && m_hThreadWaitCalibration != NULL)
		::TerminateThread(m_hThreadWaitCalibration, 0);

	// if gettring calibration data
	double speed = atof(m_szCalibrationSpeed);
	double accel; GetRequestedMotorSpeed(accel);

	// have finished the run
	if (m_nCalibrating == 1)
	{
		m_hThreadWaitCalibration = NULL;
		m_magControl.CalculateRGBCalibration(m_bCalibrateWithLaser);
		// now drive to the calibration location
		m_nCalibrating++;
		GetDlgItem(IDC_STATIC_RGB_STATUS)->SetWindowText("Drive To Calibration Line");

		m_motionControl.SetSlewSpeed(speed, accel);
		m_motionControl.GoToPosition(m_magControl.GetRGBCalibration().pos, FALSE/*don't wait for stop*/);

		// now that the motors are running and recording the data, 
		// wait for them to stop, so can calculate where the line is
		m_hThreadWaitCalibration = AfxBeginThread(::ThreadWaitCalibration, (LPVOID)this)->m_hThread;
	}

	// if not returning to the start, then just end now
	else if (m_nCalibrating == 3 )
	{
		if (m_bCalibrateReturnToStart)
		{
			StartReadMagStatus(TRUE); // restart that now finished
			SetTimer(TIMER_NOTE_VALUES, 500, NULL);
			KillTimer(TIMER_NOTE_CALIBRATION);
			m_laserControl.TurnLaserOn(FALSE);

			m_magControl.EnableMagSwitchControl(TRUE);
			m_magControl.ResetEncoderCount();

			m_nCalibrating = 0;
			GetDlgItem(IDC_STATIC_RGB_STATUS)->SetWindowText("");
			GetDlgItem(IDC_BUTTON_RGB_CALIBRATION)->SetWindowText("RGB Calibration");
		}
	}
	// have arrived at the calibration line
	else if (m_nCalibrating == 2)
	{
		double pos2 = m_magControl.GetRGBCalibration().pos; // line found here
		str.Format("Set  (%.1f mm) as Home Position", pos2);
		int ret = AfxMessageBox(str, MB_YESNOCANCEL);
		double pos1 = 0;
		if (ret == IDYES)
		{
			pos1 = m_motionControl.GetAvgMotorPosition(); // where am now
			m_motionControl.DefinePositions(0);
		}

		if (ret != IDCANCEL)
		{
			// now get the value under the sensor
			double colour = GetCalibrationValue();
			m_magControl.SetMagRGBCalibrationValue(m_magControl.GetRGBCalibration().rgb);
			OnTimer(TIMER_NOTE_VALUES);

			// now return to zero
			GetDlgItem(IDC_STATIC_RGB_STATUS)->SetWindowText("Return to Start");
			m_motionControl.SetSlewSpeed(speed, accel);
			m_motionControl.GoToPosition(-pos1, FALSE/*don't wait for stop*/);
		}

		// now that the motors are running and recording the data, 
		// wait for them to stop, so can calculate where the line is
		m_nCalibrating++;
		m_hThreadWaitCalibration = AfxBeginThread(::ThreadWaitCalibration, (LPVOID)this)->m_hThread;

	}

	return 0L;
}

void CDialogMag::StartReadMagStatus(BOOL bSet)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessageA(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_MAG_STATUS_ON, bSet);
}


void CDialogMag::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: Add your message handler code here
	if (bShow)
		SetTimer(TIMER_NOTE_VALUES, 500, NULL);
	else
		KillTimer(TIMER_NOTE_VALUES);
}


void CDialogMag::OnClickedCheckUseLaser()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
}


void CDialogMag::OnClickedCheckReturnToStart()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
}
