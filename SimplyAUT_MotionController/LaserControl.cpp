#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "LaserControl.h"
#include "sls_comms.h"
#include "Define.h"
#include "Filter.h"

static int __stdcall callback_fct(int val);
static int g_sensor_initialised = FALSE;
static int g_log_msgs[100];
static int g_log_ptr = 0;
static CCriticalSection g_critMeasures;

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
	memset(m_hitBuffer, 0x0, sizeof(m_hitBuffer));
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
#ifdef _DEBUG_TIMING_
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
	}
#endif
}

void CLaserControl::SendErrorMessage(const char* msg)
{
	if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
	{
		m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)msg);
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

LASER_MEASURES CLaserControl::GetLaserMeasures2()
{
	g_critMeasures.Lock();
	LASER_MEASURES  ret = m_measure2;
	g_critMeasures.Unlock();
	return ret;
}

void CLaserControl::GetLaserHits(MT_Hits_Pos hits[], double hitBuffer[], int nSize)
{
	g_critMeasures.Lock();
	nSize = min(nSize, SENSOR_WIDTH);
	for (int i = 0; i < nSize; ++i)
		hits[i] = m_profile.hits[i].pos1;

	memcpy(hitBuffer, m_hitBuffer, nSize * sizeof(double));
	g_critMeasures.Unlock();
}


void CLaserControl::SetLaserMeasures2(const LASER_MEASURES& meas)
{
	g_critMeasures.Lock();
	m_measure2 = meas;
	g_critMeasures.Unlock();
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

//		for (int i = 0; i < 100 && !IsLaserOn(); ++i)
//			Sleep(1);
//		for (int i = 0; i < 100 && !GetProfile(); ++i)
//			Sleep(1);

	}
	return TRUE;
}

// some values will be set to 10000 or ) if invalid
// mcheck that not too many of these
static BOOL ValidateProfile(const Profile& profile)
{
	int cnt = 0;
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (profile.hits[i].pos1 <= 0 || profile.hits[i].pos1 >= SENSOR_HEIGHT)
			cnt++;
		else
			cnt = 0;

		if (cnt > PROFILE_VALIDATE_LENGTH)
			return FALSE;
	}
	return TRUE;
}

BOOL CLaserControl::GetProfile(int tries)
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

	// while onl;y called by the startup thread, 
	// due to the Sleep() if this fails and the Sleep() used
	// it ism possible for an OnTimer() to use m_profile, while it is invalid
	Profile profile;
	for (int i = 0; i < tries; ++i)
	{
		if (::DLLGetProfile(&profile) && ValidateProfile(profile))
		{
			// always access m_profile thouygh a critical section to make thread safe
			g_critMeasures.Lock();
			memcpy(&m_profile, &profile, sizeof(profile));
			g_critMeasures.Unlock();

			return TRUE;
		}
		// this Sleep() makes this re-entrant through an OnTimer() or message function
		// 
		Sleep(10);
	}

	SendErrorMessage(_T("Laser ERROR: Cant get Profile"));
	return FALSE;
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
	for (i1 = ind - 1; i1 >= 0 && (buffer[i1] <= 0 || buffer[i1] >= SENSOR_HEIGHT); --i1);
	for (i2 = ind + 1; i2 < nSize && (buffer[i2] <= 0 || buffer[i2] >= SENSOR_HEIGHT); ++i2);

	if (i1 >= 0 && i2 < nSize)
		return buffer[i1] + (ind - i1) * (buffer[i2] - buffer[i1]) / (i2 - i1);
	else if (i2 < nSize)
		return buffer[i2];
	else if (i1 >= 0)
		return buffer[i1];
	else
		return 0;
}

// i1: start index (0 if first half)
// i2: end inddex (maxInd if first half)
//m dir1: +1 if cap is +Ve, -1 if cap is a gap
static int CalculateWeldEdge(const double hitBuffer[], int i1, int i2, int dir1)
{
	double sum1 = 0;
	int cnt1 = 0;
	for (int i = i1; i < i2; ++i)
	{
		sum1 += hitBuffer[i];
		cnt1++;
	}
	double avgPos = cnt1 ? sum1 / cnt1 : 0;

	double sum2 = 0;
	int cnt2 = 0;
	for (int i = i1; i < i2; ++i)
	{
		sum2 += pow(hitBuffer[i] - avgPos, 2.0);
		cnt2++;
	}
	double sd = cnt2 ? sqrt(sum2 / cnt2) : 0;

	// now recalculate he average that is above this SD
		// now recalculate the sd and average not including the weld data
	sum1 = 0;
	cnt1 = 0;
	for (int i = i1; i < i2; ++i)
	{
		if (dir1 * hitBuffer[i] < dir1 * (avgPos + dir1 * sd))
		{
			sum1 += hitBuffer[i];
			cnt1++;
		}
	}
	avgPos = cnt1 ? sum1 / cnt1 : 0;

	sum2 = 0;
	cnt2 = 0;
	for (int i = i1; i < i2; ++i)
	{
		if (dir1 * hitBuffer[i] < dir1 * (avgPos + dir1 * sd))
		{
			sum2 += pow(hitBuffer[i] - avgPos, 2.0);
			cnt2++;
		}
	}
	// set the threshold as 1/2 SD above the average value
	sd = cnt2 ? sqrt(sum2 / cnt2) : 0;
	double threshold1 = avgPos + dir1 * sd / 2.0;

	// now note the start and end of this region above the threshold 
	// start at the maximum indesxd, and work out from it
	int ind;
	int dir2 = (i1 == 0) ? -1 : 1;
	int maxInd = (i1 == 0) ? i2 : i1;

	for (ind = maxInd; ind >= 0 && ind < SENSOR_WIDTH; ind += dir2)
	{
		if (dir1 * hitBuffer[ind] < dir1 * threshold1)
			break;
	}

	ind = max(ind, 0);
	ind = min(ind, SENSOR_WIDTH - 1);

	return ind;
}

// dir: 1 the peak is a maximum, -1: the peak is a minimum
static int CalculateMaximumIndex(const double hitBuffer[], int nSamp, int& rDir)
{
	int maxInd = -1;
	int minInd = -1;
	double sum1 = 0;

	// find the maximum and minimum samples
	// calcualte the average
	for (int i = 0; i < nSamp; ++i)
	{
		if (minInd == -1 || hitBuffer[i] < hitBuffer[minInd])
			minInd = i;

		// only conser about the previous maximum
		if (maxInd == -1 || hitBuffer[i] > hitBuffer[maxInd])
			maxInd = i;

		sum1 += hitBuffer[i];
	}
	double avgPos = sum1 / nSamp;

	// which has more energy under it, the minimum (0) or the maximum (1)
	// energy is sum of (value-avg)^2
	double S[2];
	int ind[2] = { minInd, maxInd };

	for (int i = 0; i < 2; ++i)
	{
		double sum2 = 0;
		int cnt2 = 0;
		for (int j = max(ind[i] - 10, 0); j <= min(ind[i] + 10, nSamp - 1); ++j)
		{
			sum2 += pow(hitBuffer[j] - avgPos, 2.0);
			cnt2++;
		}
		S[i] = cnt2 ? sum2 / cnt2 : 0;
	}

	// now note if looking for a minimum or a maximum
	rDir = (S[0] > S[1]) ? -1 : 1;
	maxInd = (S[0] > S[1]) ? ind[0] : ind[1];

	return maxInd;
}

int CLaserControl::CalcLaserMeasures(double pos_avg, const double velocity4[4], int last_cap_pix)
{
#define SCATTER_WIDTH 10


	g_critMeasures.Lock();
	m_measure2.status = -1;
	if (!IsConnected() || !IsLaserOn())
	{
		g_critMeasures.Unlock();
		return FALSE;
	}

	// also get the F/W measures
	Measurement measures1;
	if (GetLaserMeasurment(measures1))
	{
		m_measure2.weld_cap_pix1.x = measures1.mp[0].x;
		m_measure2.weld_cap_pix1.y = measures1.mp[0].y;
	}

	// get rid of scatter when copying to the buffer by 
	// replacing each value with the max over a short range
	// the scatter will be lower values
	memset(m_hitBuffer, 0x0, sizeof(m_hitBuffer));
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		int maxInd = -1;
		for (int j = max(i - SCATTER_WIDTH, 0); j <= min(i + SCATTER_WIDTH, SENSOR_WIDTH - 1); ++j)
		{
			// both '0'; and >= SENSOR_HEIGHT are invaliud values
			if (m_profile.hits[j].pos1 > 0 && m_profile.hits[j].pos1 < SENSOR_HEIGHT)
			{
				if (maxInd == -1 || m_profile.hits[j].pos1 > m_profile.hits[maxInd].pos1)
					maxInd = j;
			}
		}
		m_hitBuffer[i] = (maxInd == -1) ? 0 : m_profile.hits[maxInd].pos1;
	}

	// interpolate invalid value
	// can not just remove, as this will shift the data
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (m_hitBuffer[i] == 0 || m_hitBuffer[i] >= SENSOR_HEIGHT)
			m_hitBuffer[i] = ::InterplateLaserHit(m_hitBuffer, i, SENSOR_WIDTH);
	}

	// pad the end with copies of the last value
	for (int i = SENSOR_WIDTH; i < SENSOR_WIDTH + FILTER_MARGIN; ++i)
		m_hitBuffer[i] = m_hitBuffer[SENSOR_WIDTH - 1];

	// low-pass filter the data
	m_filter.IIR_HighCut(m_hitBuffer, 100, 12.50);

	// get both the maximumk and the minimum and the average
	int dir, maxInd = CalculateMaximumIndex(m_hitBuffer, SENSOR_WIDTH, dir);

	// the threshold is 1/2 way from average to maxVal
	// incase following a gap, not a weld cap, check which is larget
	// average to minimum or average to maximum
	int i1 = CalculateWeldEdge(m_hitBuffer, 0, maxInd, dir);
	int i2 = CalculateWeldEdge(m_hitBuffer, maxInd, SENSOR_WIDTH, dir);

	// set to the same width as each other
//	if (i2 - maxInd > maxInd - i1)
//		i2 = min(maxInd + (maxInd - i1), SENSOR_WIDTH - 1);
//	else if (maxInd - i1 > i2 - maxInd)
//		i1 = max(maxInd - (i2 - maxInd), 0);

	// how many samples are +/- 5 mm
	double hw, sw1, sw2;
	ConvPixelToMm(i2, 5, sw2, hw);
	ConvPixelToMm(i1, 5, sw1, hw);
	double scale = (double)(i2 - i1) / (sw2 - sw1); // pixels / mm

	// ideally only want to use about +/- 5 mm
	int i11 = max((int)(maxInd - 5.0 * scale + 0.5), 0);
	int i22 = min((int)(maxInd + 5.0 * scale + 0.5), SEWNSOR_WIDTH - 1);


	// now put these values into a double vector and gtet the 2nd order polynomial parameters for
	int j = 0;
	for (int i = i11; i < i22; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = m_hitBuffer[i];
		j++;
	}

	// these coefficients will be for mm not laser units
	double coeff[3];
	::polyfit(m_polyX, m_polyY, j, 2, coeff);

	double centre = (coeff[2] == 0) ? (i2 - i1) / 2 : -coeff[1] / (2 * coeff[2]);

	// if too far from maxInd, then asasume that did not mopdel correctly
	// just replace with maxInd
	if (fabs(centre - maxInd) > (i2 - i1) / 2)
		centre = (double)maxInd; //  (double)(i1 + i2) / 2.0;

	centre = max(centre, 0.0);
	centre = min(centre, (double)SENSOR_WIDTH - 1.0);

	// now differentiate the coeff to dind the location of the maximum
	m_measure2.weld_cap_pix2.x = centre;
	m_measure2.weld_cap_pix2.y = coeff[2] * m_measure2.weld_cap_pix2.x * m_measure2.weld_cap_pix2.x + coeff[1] * m_measure2.weld_cap_pix2.x + coeff[0];
	m_measure2.weld_left_pix = i1;
	m_measure2.weld_right_pix = i2;
//	*/

	// convert laser pixels to mm
	// will draw with pixels, but navigate with mm
	ConvPixelToMm((int)(m_measure2.weld_cap_pix2.x + 0.5), (int)(m_measure2.weld_cap_pix2.y + 0.5), m_measure2.weld_cap_mm.x, m_measure2.weld_cap_mm.y);

	// get a 1st order fit to the left
	// use only the first 1/2 of the data
	j = 0;
	m_measure2.weld_left_start_pix = m_measure2.weld_left_pix / 4;
	for (int i = m_measure2.weld_left_start_pix; i < m_measure2.weld_left_pix; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = m_hitBuffer[i];
		j++;
	}
	// calculate the Y value at i1 to be an estimate of the height on the left
	::polyfit(m_polyX, m_polyY, j, 2, m_measure2.ds_coeff);

	// now get the slope of the up side
	// get a 1st order fit to the left
	j = 0;
	m_measure2.weld_right_end_pix = SENSOR_WIDTH - (SENSOR_WIDTH - m_measure2.weld_right_pix) / 4;
	for (int i = m_measure2.weld_right_pix; i < m_measure2.weld_right_end_pix; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = m_hitBuffer[i];
		j++;
	}
	// calculate the Y value at i1 to be an estimate of the height on the left
	::polyfit(m_polyX, m_polyY, j, 2, m_measure2.us_coeff);

	// the F/W does a better job of findiung the centre of a gap
	// enable this to use the F/W detection of the centre

//		m_measure2.weld_cap_pix2.x = measures1.mp[0].x;
//		m_measure2.weld_cap_pix2.y = measures1.mp[0].y;


	ConvPixelToMm((int)m_measure2.weld_left_pix, (int)m_measure2.GetDnSideWeldHeight(), m_measure2.weld_left_mm, m_measure2.weld_left_height_mm);
	ConvPixelToMm((int)m_measure2.weld_right_pix, (int)m_measure2.GetUpSideWeldHeight(), m_measure2.weld_right_mm, m_measure2.weld_right_height_mm);
	ConvPixelToMm((int)m_measure2.weld_cap_pix2.x, (int)m_measure2.weld_cap_pix2.y, m_measure2.weld_cap_mm.x, m_measure2.weld_cap_mm.y);


	// now convert the laser units to mm
	m_measure2.measure_pos_mm = pos_avg; 
	if (velocity4)
	{
		memcpy(m_measure2.wheel_velocity4, velocity4, 4 * sizeof(double));
	}

	m_measure2.status = 0;
	int ret = (int)(m_measure2.weld_cap_pix2.x + 0.5);
	g_critMeasures.Unlock();

	return ret;
}


BOOL CLaserControl::CalcLaserMeasures_old(LASER_MEASURES& measures)
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
		m_hitBuffer[i] = (double)m_profile.hits[i].pos1;

	// interpolate invalid value
	// can not just remove, as this will shift the data
	for (int i = 0; i < SENSOR_WIDTH; ++i)
	{
		if (m_hitBuffer[i] < 0 || m_hitBuffer[i] >= SENSOR_HEIGHT)
			m_hitBuffer[i] = ::InterplateLaserHit(m_hitBuffer, i, SENSOR_WIDTH);
	}

	// now low pas filter this m_hitBuffer
	m_filter.IIR_HighCut(m_hitBuffer, 100, 5.0);

	// find all local max or min values ibn the m_hitBuffer
	int dir = 1;
	int peaks[SENSOR_WIDTH];
	int nPeaks = 0;
	for (int i = 1; i < SENSOR_WIDTH - 1; ++i)
	{
		if (dir * m_hitBuffer[i] > dir* m_hitBuffer[i - 1] && dir* m_hitBuffer[i] > dir* m_hitBuffer[i + 1])
			peaks[nPeaks++] = i;
	}

	// get the average value about each peak
	double maxVar = 0;
	int maxInd = -1;
	for (int j = 0; j < nPeaks; ++j)
	{
		int i = peaks[j];
		double val = m_hitBuffer[i];

		int count = 0;
		double sum = 0;
		for (int k = max(0, i - 50); k < min(i + 50, SENSOR_WIDTH); ++k)
		{
			sum += m_hitBuffer[k];
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
	double avg = m_hitBuffer[maxInd] - dir * maxVar;
	double threshold = (avg + m_hitBuffer[maxInd]) / 2;

	// now note the start and end of this region above the threshold 
	int i1, i2;
	for (i1 = maxInd-1; i1 >= 0; --i1)
	{
		if (dir*m_hitBuffer[i1] < dir*threshold)
			break;
	}
	for (i2 = maxInd+1; i2 < SENSOR_WIDTH; ++i2)
	{
		if (dir*m_hitBuffer[i2] < dir*threshold)
			break;
	}

	// now put these values into a double vector and gtet the 2nd order polynomial parameters for
	int j = 0;
	for (int i = i1; i < i2; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = m_hitBuffer[i];
		j++;
	}

	// these coefficients will be for mm not laser units
	double coeff[3];
	::polyfit(m_polyX, m_polyY, j, 2, coeff);

	// now differentiate the coeff to dind the location of the maximum
	measures.weld_cap_pix2.x = (coeff[2] == 0) ? maxInd : -coeff[1] / (2 * coeff[2]);
	measures.weld_cap_pix2.y = coeff[2] * measures.weld_cap_pix2.x * measures.weld_cap_pix2.x + coeff[1] * measures.weld_cap_pix2.x + coeff[0];
	measures.weld_left_pix = i1;
	measures.weld_right_pix = i2;

	// convert laser pixels to mm
	// will draw with pixels, but navigate with mm
	ConvPixelToMm((int)(measures.weld_cap_pix2.x + 0.5), (int)(measures.weld_cap_pix2.y + 0.5), measures.weld_cap_mm.x, measures.weld_cap_mm.y);

	// get a 1st order fit to the left
	// use only the first 1/2 of the data
	j = 0;
	measures.weld_left_start_pix = measures.weld_left_pix / 4;
	for (int i = measures.weld_left_start_pix; i < measures.weld_left_pix; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = m_hitBuffer[i];
		j++;
	}
	// calculate the Y value at i1 to be an estimate of the height on the left
	::polyfit(m_polyX, m_polyY, j, 1, measures.ds_coeff);

	// now get the slope of the up side
	// get a 1st order fit to the left
	j = 0;
	measures.weld_right_end_pix = SENSOR_WIDTH - (SENSOR_WIDTH - measures.weld_right_pix) / 4;
	for (int i = measures.weld_right_pix; i < measures.weld_right_end_pix; ++i)
	{
		m_polyX[j] = i;
		m_polyY[j] = m_hitBuffer[i];
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