#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "LaserControl.h"
#include "sls_comms.h"
#include "Filter.h"

static int __stdcall callback_fct(int val);
static int g_sensor_initialised = FALSE;
static int g_log_msgs[100];
static int g_log_ptr = 0;
#define FILTER_MARGIN 100

CLaserControl::CLaserControl()
	: m_filter(SENSOR_WIDTH+FILTER_MARGIN)
{
	m_nLaserPower = 0;
	m_nCameraShutter = 0;
    m_pParent = NULL;
    m_nMsg = 0;
	memset(m_polyX, 0x0, sizeof(m_polyX));
	memset(m_polyY, 0x0, sizeof(m_polyY));
	memset(m_work_buffer1, 0x0, sizeof(m_work_buffer1));
	memset(m_work_buffer2, 0x0, sizeof(m_work_buffer2));
}

CLaserControl::~CLaserControl()
{
}

void CLaserControl::Init(CWnd* pParent, UINT nMsg)
{
    m_pParent = pParent;
    m_nMsg = nMsg;
}

void CLaserControl::SendDebugMessage(const CString& msg)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
	}
}

void CLaserControl::SendErrorMessage(const CString& msg)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)&msg);
	}
}

void CLaserControl::EnableControls()
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SETBITMAPS);
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
	{
		SendErrorMessage("Laser ERROR: Already Disconnected");
		return FALSE;
	}
}

BOOL CLaserControl::IsLaserOn()
{
//	static clock_t total_time = 0;
//	static int count = 0;
//	clock_t t1 = clock();

	SensorStatus SensorStatus;
	::DLLGetSensorStatus(&SensorStatus);
	::SLSGetSensorStatus();

	// this was used to show that this function takes less than a ms
	// not an issue
//	clock_t dt = clock() - t1;
//	total_time += dt;
//	count++;

//	CString str2;
//	str2.Format("IsLaserOn: %d ms, avg: %d ms", dt, total_time / count);
//	SendDebugMessage(str2);


	return (SensorStatus.LaserStatus == 0) ? FALSE : TRUE;
}

BOOL CLaserControl::GetLaserStatus(SensorStatus& SensorStatus)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
	else
	{
		::DLLGetSensorStatus(&SensorStatus);
		::SLSGetSensorStatus();
		return TRUE;
	}
}

int CLaserControl::GetSerialNumber()
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return 0;
	}
	else
		return 10357;
}


BOOL CLaserControl::GetLaserTemperature(LASER_TEMPERATURE & temperature)
{
	SensorStatus SensorStatus;
	if (IsConnected() && GetLaserStatus(SensorStatus) )
	{
		double t1 = (((double)SensorStatus.MainBrdTemp) / 100.0) - 100.0;
		double t2 = (((double)SensorStatus.LaserTemp) / 100.0) - 100.0;
		double t3 = (((double)SensorStatus.PsuBrdTemp) / 100.0) - 100.0;

// happens too often to put in the status window
//		CString str;
//		str.Format("%.1f %.1f %.1f", t1, t2, t3);
//		SendDebugMessage(str);
		
		temperature.BoardTemperature = t1;
		temperature.SensorTemperature = t2;
		temperature.LaserTemperature = t3;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CLaserControl::SetLaserOptions(int opt)
{
	return SetLaserOptions(opt, opt, opt, opt);
}

BOOL CLaserControl::SetLaserOptions(int opt1, int opt2, int opt3, int opt4)
{
	::SLSSetLaserOptions(opt1, opt2, opt3, opt4);
	return TRUE;
}

BOOL CLaserControl::SetAutoLaserCheck(BOOL bAuto)
{
	if (!IsLaserOn())
	{
		SendErrorMessage(_T("Laser ERROR: Not On"));
		return FALSE;
	}
	else
	{
		if (bAuto)
			SetLaserOptions(1);
		else
		{
			SetLaserOptions(0);
			SetLaserIntensity(m_nLaserPower);
		}
		return TRUE;
	}
}

BOOL CLaserControl::SetLaserIntensity(int nLaserPower)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
	else
	{
		m_nLaserPower = nLaserPower;
		::SetLaserIntensity(m_nLaserPower, m_nLaserPower, m_nLaserPower, m_nLaserPower);
		return TRUE;
	}
}

BOOL CLaserControl::SetCameraShutter(int nCameraShutter)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
	else
	{
		m_nCameraShutter = nCameraShutter;
		::SetCameraShutter(m_nCameraShutter);
		return TRUE;
	}
}

BOOL CLaserControl::SetCameraRoi(const CRect& rect)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
	else
	{
		CString str;
		SLSSetCameraRoi((unsigned short)rect.left, (unsigned short)rect.top, (unsigned short)rect.right, (unsigned short)rect.bottom);
		str.Format("%d,%d-%d,%d", rect.left, rect.top, rect.right, rect.bottom);
		SendDebugMessage(str);
		return TRUE;
	}
}

BOOL CLaserControl::GetCameraRoi(CRect& rect)
{
	SensorStatus SensorStatus;
	if (GetLaserStatus(SensorStatus))
	{
		rect.left = min(max(SensorStatus.roi_x1, 0), SENSOR_WIDTH - 1);
		rect.right = min(max(SensorStatus.roi_x2, 0), SENSOR_WIDTH - 1);
		rect.top = min(max(SensorStatus.roi_y1, 0), SENSOR_HEIGHT - 1);
		rect.bottom = min(max(SensorStatus.roi_y2, 0), SENSOR_HEIGHT - 1);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
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
	{
		SendErrorMessage(_T("Laser ERROR: Not Initialized, Check if Power On"));
		return FALSE;
	}

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
	SetCameraRoi( CRect(0,0, 1023, 1279) );

// only turn on ikf motor run n ing
//	TurnLaserOn(TRUE);
    return TRUE;
}

BOOL CLaserControl::ConvPixelToMm(int row, int col, double& sw, double& hw)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
	else
	{
		::ConvPixelToMm(row, col, &sw, &hw);
		return TRUE;
	}
}

BOOL CLaserControl::GetLaserVersion(unsigned short& major, unsigned short& minor)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}

	major = minor = 0;
	::SLSGetSensorVersion();
	::DLLGetSensorVersion(&major, &minor);
	return TRUE;
}
BOOL CLaserControl::GetLaserMeasurment(Measurement& meas)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}

	else if (!IsLaserOn())
	{
		SendErrorMessage(_T("Laser ERROR: Not On"));
		return FALSE;
	}

	else if (!::DLLGetMeasurement(&meas))
		return FALSE;

	else
		return TRUE;
}

BOOL CLaserControl::TurnLaserOn(BOOL bLaserOn)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
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

BOOL CLaserControl::GetProfile(Profile& profile)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
	else if (!IsLaserOn())
	{
		SendErrorMessage(_T("Laser ERROR: Not On"));
		return FALSE;
	}
	else if (!::DLLGetProfile(&profile))
	{
		SendErrorMessage(_T("Laser ERROR: Cant get Profile"));
		return FALSE;
	}
	else
		return TRUE;
}

BOOL CLaserControl::GetProfilemm(Profilemm* pProfile, int hit_no)
{
	if (!IsConnected())
	{
		SendErrorMessage(_T("Laser ERROR: Not Connected"));
		return FALSE;
	}
	else if (!IsLaserOn())
	{
		SendErrorMessage(_T("Laser ERROR: Not On"));
		return FALSE;
	}
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

static double InterplateLaserHit(const double* buffer, int ind, int nSize)
{
	// find a previous value
	int i1, i2;
	for (i1 = ind - 1; i1 >= 0 && (buffer[i1] < 0 || buffer[i1] >= SENSOR_HEIGHT); --i1);
	for (i2 = ind + 1; i2 < nSize && (buffer[i2] < 0 || buffer[i2] >= SENSOR_HEIGHT); ++i2);

	if (i1 >= 0 && i2 < nSize)
		return buffer[i1] + (ind - i1) * (buffer[i2] - buffer[i1]) / (i2 - i1);
	else if (i2 < nSize)
		return buffer[i2];
	else if (i1 >= 0)
		return buffer[i1];
	else
		return 0;
}
BOOL CLaserControl::CalcLaserMeasures(const Hits hits[], LASER_MEASURES& measures2, double pos, double buffer[])
{
	measures2.status = -1;
	if (!IsConnected() || !IsLaserOn())
		return FALSE;

	// also get the F/W measures
	Measurement measures1;
	if (GetLaserMeasurment(measures1))
	{
		measures2.weld_cap_pix1.x = measures1.mp[0].x;
		measures2.weld_cap_pix1.y = measures1.mp[0].y;
	}

	// calculate the average, min and max values
// gety rid of not set values
	for (int i = 0; i < SENSOR_WIDTH; ++i)
		buffer[i] = (double)hits[i].pos1;

	// interpolate invalid value
	// can not just remove, as this will shift the data
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (buffer[i] < 0 || buffer[i] >= SENSOR_HEIGHT)
			buffer[i] = ::InterplateLaserHit(buffer, i, SENSOR_WIDTH);
	}

	// pad the end with copies of the last value
	for (int i = SENSOR_WIDTH; i < SENSOR_WIDTH + FILTER_MARGIN; ++i)
		buffer[i] = buffer[SENSOR_WIDTH-1];

	m_filter.IIR_HighCut(buffer, 100, 2.5);

	// this filter addedded delay, remove it
	int shift = 15;
	for (int i = SENSOR_WIDTH - 1; i >= shift; --i)
		buffer[i] = buffer[i - shift];

	int maxInd = -1;
	int minInd = -1;

	double sum1 = 0;
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (minInd == -1 || buffer[i] < buffer[minInd])
			minInd = i;
		if (maxInd == -1 || buffer[i] > buffer[maxInd])
			maxInd = i;

		// also note the average
		sum1 +=buffer[i];
	}
	double avgPos = sum1 / SENSOR_WIDTH;

	// get the SD (this includes the weld data)
	// set the edge threshold as 1 SD above the average
	double sum2 = 0;
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{

		// also note the average
		double val = buffer[i] - avgPos;
		sum2 += val * val;
	}
	double sd = sqrt(sum2 / SENSOR_WIDTH);

	// now recalculate the sd and average not including the weld data
	sum1 = 0;
	int cnt = 0;
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (buffer[i] < avgPos + sd)
		{
			sum1 += buffer[i];
			cnt++;
		}
	}
	avgPos = sum1 / cnt;

	sum2 = 0;
	cnt = 0;
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (buffer[i] < avgPos + sd)
		{
			double val = buffer[i] - avgPos;
			sum2 += val * val;
			cnt++;
		}
	}
	sd = sqrt(sum2 / cnt);


	// the threshold is 1/2 way from average to maxVal
	// incase following a gap, not a weld cap, check which is larget
	// average to minimum or average to maximum
	int dir = 1; //  (maxPos - avgPos) > (avgPos - minPos) ? 1 : -1;
//	double threshold = (dir == 1) ? (buffer[maxInd] + avgPos) / 2 : (buffer[minInd] + avgPos) / 2;
//	double threshold = (dir == 1) ? (buffer[maxInd] + medPos) / 2 : (buffer[minInd] + medPos) / 2;
	double threshold = (dir == 1) ? (buffer[maxInd] + avgPos + sd) / 2 : (buffer[minInd] - avgPos - sd) / 2;

	// now note the start and end of this region above the threshold 
	int i1, i2;
	for (i1 = 0; i1 < SENSOR_WIDTH; ++i1)
	{
		if (dir == 1 && buffer[i1] >= threshold)
			break;
		if (dir == -1 && buffer[i1] <= threshold)
			break;
	}
	for (i2 = SENSOR_WIDTH - 1; i2 >= 0; --i2)
	{
		if (dir == 1 && buffer[i2] >= threshold)
			break;
		if (dir == -1 && buffer[i2] <= threshold)
			break;
	}

	// now put these values into a double vector and gtet the 2nd order polynomial parameters for
	int j = 0;
	for (int i = i1; i < i2; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = buffer[i];
		j++;
	}

	// these coefficients will be for mm not laser units
	double coeff[3];
	::polyfit(m_polyX, m_polyY, j, 2, coeff);

	// now differentiate the coeff to dind the location of the maximum
	measures2.weld_cap_pix2.x = (coeff[2] == 0) ? (i2 - i1) / 2 : -coeff[1] / (2 * coeff[2]);
	measures2.weld_cap_pix2.y = coeff[2] * measures2.weld_cap_pix2.x * measures2.weld_cap_pix2.x + coeff[1] * measures2.weld_cap_pix2.x + coeff[0];
	measures2.weld_left = i1;
	measures2.weld_right = i2;

	// convert laser pixels to mm
	// will draw with pixels, but navigate with mm
	ConvPixelToMm((int)(measures2.weld_cap_pix2.x + 0.5), (int)(measures2.weld_cap_pix2.y + 0.5), measures2.weld_cap_mm.x, measures2.weld_cap_mm.y);

	// get a 1st order fit to the left
	// use only the first 1/2 of the data
	j = 0;
	measures2.weld_left_start = measures2.weld_left / 4;
	for (int i = measures2.weld_left_start; i < measures2.weld_left; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = buffer[i];
		j++;
	}
	// calculate the Y value at i1 to be an estimate of the height on the left
	::polyfit(m_polyX, m_polyY, j, 1, measures2.ds_coeff);

	// now get the slope of the up side
	// get a 1st order fit to the left
	j = 0;
	measures2.weld_right_end = SENSOR_WIDTH - (SENSOR_WIDTH - measures2.weld_right) / 4;
	for (int i = measures2.weld_right; i < measures2.weld_right_end; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = buffer[i];
		j++;
	}
	// calculate the Y value at i1 to be an estimate of the height on the left
	::polyfit(m_polyX, m_polyY, j, 1, measures2.us_coeff);

	// now convert the laser units to mm
	measures2.measure_pos = pos; 
	measures2.status = 0;
	return TRUE;
}


BOOL CLaserControl::CalcLaserMeasures_old(const Hits hits[], LASER_MEASURES& measures, double buffer[])
{
	static clock_t total_time = 0;
	static int count = 0;
	clock_t t1 = clock();

	measures.status = -1;
	if (!IsConnected() || !IsLaserOn())
		return FALSE;

	// calculate the average, min and max values
	// gety rid of not set values
	for (int i = 0; i < SENSOR_WIDTH; ++i)
		buffer[i] = (double)hits[i].pos1;

	// interpolate invalid value
	// can not just remove, as this will shift the data
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (buffer[i] < 0 || buffer[i] >= SENSOR_HEIGHT)
			buffer[i] = ::InterplateLaserHit(buffer, i, SENSOR_WIDTH);
	}

	// now low pas filter this buffer
	m_filter.IIR_HighCut(buffer, 100, 5.0);

	// find all local max or min values ibn the buffer
	int dir = 1;
	int peaks[SENSOR_WIDTH];
	int nPeaks = 0;
	for (int i = 1; i < SENSOR_WIDTH - 1; ++i)
	{
		if (dir * buffer[i] > dir* buffer[i - 1] && dir* buffer[i] > dir* buffer[i + 1])
			peaks[nPeaks++] = i;
	}

	// get the average value about each peak
	double maxVar = 0;
	int maxInd = -1;
	for (int j = 0; j < nPeaks; ++j)
	{
		int i = peaks[j];
		double val = buffer[i];

		int count = 0;
		double sum = 0;
		for (int k = max(0, i - 50); k < min(i + 50, SENSOR_WIDTH); ++k)
		{
			sum += buffer[k];
			count++;
		}
		double avg = sum / count;
		double var = fabs(val - avg);
		if (var > maxVar || maxInd == -1)
		{
			maxVar = var;
			maxInd = i;
		}
	}

	// now find the edges of the weld
	// the threshold is now the half way point between the maximum variation peak and its average
	double avg = buffer[maxInd] - dir * maxVar;
	double threshold = (avg + buffer[maxInd]) / 2;

	// now note the start and end of this region above the threshold 
	int i1, i2;
	for (i1 = maxInd-1; i1 >= 0; --i1)
	{
		if (dir*buffer[i1] < dir*threshold)
			break;
	}
	for (i2 = maxInd+1; i2 < SENSOR_WIDTH; ++i2)
	{
		if (dir*buffer[i2] < dir*threshold)
			break;
	}

	// now put these values into a double vector and gtet the 2nd order polynomial parameters for
	int j = 0;
	for (int i = i1; i < i2; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = buffer[i];
		j++;
	}

	// these coefficients will be for mm not laser units
	double coeff[3];
	::polyfit(m_polyX, m_polyY, j, 2, coeff);

	// now differentiate the coeff to dind the location of the maximum
	measures.weld_cap_pix2.x = (coeff[2] == 0) ? maxInd : -coeff[1] / (2 * coeff[2]);
	measures.weld_cap_pix2.y = coeff[2] * measures.weld_cap_pix2.x * measures.weld_cap_pix2.x + coeff[1] * measures.weld_cap_pix2.x + coeff[0];
	measures.weld_left = i1;
	measures.weld_right = i2;

	// convert laser pixels to mm
	// will draw with pixels, but navigate with mm
	ConvPixelToMm((int)(measures.weld_cap_pix2.x + 0.5), (int)(measures.weld_cap_pix2.y + 0.5), measures.weld_cap_mm.x, measures.weld_cap_mm.y);

	// get a 1st order fit to the left
	// use only the first 1/2 of the data
	j = 0;
	measures.weld_left_start = measures.weld_left / 4;
	for (int i = measures.weld_left_start; i < measures.weld_left; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = buffer[i];
		j++;
	}
	// calculate the Y value at i1 to be an estimate of the height on the left
	::polyfit(m_polyX, m_polyY, j, 1, measures.ds_coeff);

	// now get the slope of the up side
	// get a 1st order fit to the left
	j = 0;
	measures.weld_right_end = SENSOR_WIDTH - (SENSOR_WIDTH - measures.weld_right) / 4;
	for (int i = measures.weld_right; i < measures.weld_right_end; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = buffer[i];
		j++;
	}
	// calculate the Y value at i1 to be an estimate of the height on the left
	::polyfit(m_polyX, m_polyY, j, 1, measures.us_coeff);

	// now convert the laser units to mm
	measures.status = 0;

	clock_t dt = clock() - t1;
	total_time += dt;
	count++;

// the above takes less than 1 ms
//	CString str2;
//	str2.Format("CalcLaserMeasures: %d ms, avg: %d ms", dt, total_time / count);
//	SendDebugMessage(str2);

	return TRUE;

}