#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "LaserControl.h"
#include "sls_comms.h"

static int __stdcall callback_fct(int val);
static int g_sensor_initialised = FALSE;
static int g_log_msgs[100];
static int g_log_ptr = 0;

CLaserControl::CLaserControl()
{
	m_nLaserPower = 0;
	m_nCameraShutter = 0;
    m_pParent = NULL;
    m_nMsg = 0;
}

CLaserControl::~CLaserControl()
{
}

void CLaserControl::Init(CWnd* pParent, UINT nMsg)
{
    m_pParent = pParent;
    m_nMsg = nMsg;
}

void CLaserControl::SendDebugMessage(CString msg)
{
    if (m_pParent && ::IsWindow(m_pParent->m_hWnd) && m_nMsg != 0)
    {
        m_pParent->SendMessage(m_nMsg, MSG_SEND_DEBUGMSG, (WPARAM)&msg);
    }
}

void CLaserControl::EnableControls()
{
    if (m_pParent && ::IsWindow(m_pParent->m_hWnd) && m_nMsg != 0)
    {
        m_pParent->SendMessage(m_nMsg, MSG_SETBITMAPS);
    }
}

BOOL CLaserControl::IsConnected()const
{
    return g_sensor_initialised;
}

BOOL CLaserControl::Disconnect()
{
	if (g_sensor_initialised)
	{
		if (IsLaserOn())
			TurnLaserOn(FALSE);

		::StopNetworkThread();
		g_sensor_initialised = FALSE;
		EnableControls();
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CLaserControl::IsLaserOn()
{
	SensorStatus SensorStatus;
	::DLLGetSensorStatus(&SensorStatus);
	::SLSGetSensorStatus();

	return (SensorStatus.LaserStatus == 0) ? FALSE : TRUE;
}

BOOL CLaserControl::GetLaserTemperature(LASER_TEMPERATURE& temperature)
{
	if (IsLaserOn())
	{
		SensorStatus SensorStatus;
		::DLLGetSensorStatus(&SensorStatus);
		::SLSGetSensorStatus();

		double t1 = (((double)SensorStatus.MainBrdTemp) / 100.0) - 100.0;
		double t2 = (((double)SensorStatus.LaserTemp) / 100.0) - 100.0;
		double t3 = (((double)SensorStatus.PsuBrdTemp) / 100.0) - 100.0;

		CString str;
		str.Format("%.1f %.1f %.1f", t1, t2, t3);
		SendDebugMessage(str);
		
		temperature.BoardTemperature = t1;
		temperature.SensorTemperature = t2;
		temperature.LaserTemperature = t3;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CLaserControl::SetAutoLaserCheck(BOOL bAuto)
{
	if (IsLaserOn())
	{
		if (bAuto)
			SLSSetLaserOptions(1, 1, 1, 1);
		else
		{
			SLSSetLaserOptions(0, 0, 0, 0);
			SetLaserIntensity(m_nLaserPower);
		}
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CLaserControl::SetLaserIntensity(int nLaserPower)
{
	if (IsConnected())
	{
		m_nLaserPower = nLaserPower;
		::SetLaserIntensity(m_nLaserPower, m_nLaserPower, m_nLaserPower, m_nLaserPower);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CLaserControl::SetCameraShutter(int nCameraShutter)
{
	if (IsConnected())
	{
		m_nCameraShutter = nCameraShutter;
		::SetCameraShutter(m_nCameraShutter);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CLaserControl::SetCameraRoi()
{
	if (IsConnected())
	{
		CString str;
		int ROI_x1 = 0;
		int ROI_y1 = 0;
		int ROI_x2 = 1023;
		int ROI_y2 = 1279;

		SLSSetCameraRoi(ROI_x1, ROI_y1, ROI_x2, ROI_y2);
		str.Format("%d,%d-%d,%d", (int)ROI_x1, (int)ROI_y1, (int)ROI_x2, (int)ROI_y2);
		SendDebugMessage(str);
		return TRUE;
	}
	else
		return FALSE;
}



BOOL CLaserControl::Connect(const BYTE address[4])
{
	CString str;
	if (IsConnected() )
		return TRUE;

	int Port = htons(8002) << 16;

	// Set a timer to look for being connected
	char ip_address[32];
	sprintf_s(ip_address, sizeof(ip_address), "%d.%d.%d.%d", address[0], address[1], address[2], address[3] );
	::BeginNetworkThread(ip_address, Port, &callback_fct); // from the DLL

	Sleep(200);
	::SLSGetSensorVersion();
	::SLSGetCalibration();

	Sleep(100);
	::SLSGetSensorStatus();

	Sleep(200);

	// Should have received responses by now 
	// 

	if (!g_sensor_initialised)
		return FALSE;

	SendDebugMessage( "Sensor Comm Initialised" );

	SensorStatus SensorStatus;
	::DLLGetSensorStatus(&SensorStatus);
	bool bLaserOn = SensorStatus.LaserStatus == 0 ? false : true;
	int BoardTemperature = (int)(SensorStatus.MainBrdTemp - 10000.0) / 100;
	int SensorTemperature = (int)(SensorStatus.PsuBrdTemp - 10000.0) / 100;
	int LaserTemperature = (int)(SensorStatus.LaserTemp - 10000.0) / 100;


	unsigned short major = 0, minor = 0;
	::SLSGetSensorVersion();
	::DLLGetSensorVersion(&major, &minor);

	str.Format("Laser Version: %d.%d", (int)major, (int)minor);
	SendDebugMessage(str);


	::SLSSetLaserOptions(1, 1, 1, 1);	// Turns Auto Laser ON
	SetCameraRoi();

	TurnLaserOn(TRUE);
    return TRUE;
}

BOOL CLaserControl::GetLaserMeasurment(Measurement& meas)
{
	if (!IsConnected())
		return FALSE;

	else if (!IsLaserOn())
		return FALSE;

	else if (!::DLLGetMeasurement(&meas))
		return FALSE;

	else
		return TRUE;
}

BOOL CLaserControl::TurnLaserOn(BOOL bLaserOn)
{
	if (!IsConnected())
		return FALSE;
	else if ( !bLaserOn )
	{
		::Profile_Sending(FALSE);
		::MsgMeasurementSending(FALSE);
		::LaserOn(FALSE);
	}
	else
	{
		::LaserOn(TRUE);
		::Profile_Sending(TRUE);
		::MsgMeasurementSending(TRUE);
	}
	return TRUE;
}

BOOL CLaserControl::GetProfile(Profile* pProfile)
{
	if (!IsConnected())
		return FALSE;
	else if (!IsLaserOn())
		return FALSE;
	else
		return ::DLLGetProfile(pProfile);
}

BOOL CLaserControl::GetProfilemm(Profilemm* pProfile, int hit_no)
{
	if (!IsConnected())
		return FALSE;
	else if (!IsLaserOn())
		return FALSE;
	else
		return ::DLLGetProfilemm(pProfile, hit_no);
}

void CLaserControl::Frm_Find_Sensors()
{
//	::SLSFrm_Find_Sensors();

}


static int __stdcall callback_fct(int val)
{
	/* This is called from DLL so we can't access any
	 * forms/controls here!. */

	 // Function called when a message is received from the sensor
	 // "val" contains the message type

	g_sensor_initialised = TRUE;

	g_log_msgs[g_log_ptr++] = val;
	if (g_log_ptr == 100)
		g_log_ptr = 0;

	return 0;

}


