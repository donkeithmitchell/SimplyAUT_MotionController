#include "pch.h"
#include "WeldNavigation.h"
#include "LaserControl.h"
#include "MotionControl.h"
#include "MagController.h"
#include "DialogGirthWeld.h"
#include "Misc.h"

#define MIN_TRAVEL_DIST				20 // minimum trav el dist of 20 mm befiore navigation kicks in
#define NAVIGATION_GROW_BY			2048 // at 1 per mm, this would be 2 metres
#define GAP_FILTER_MIN__WIDTH		10 // mm (filter over last 50 mm, 2nd order, so can handle variations)
#define GAP_FILTER_MAX__WIDTH		50 // 100 // mm (filter over last 50 mm, 2nd order, so can handle variations)
#define VEL_FILTER_MIN_WIDTH		10
#define VEL_FILTER_MAX_WIDTH		200
//#define VEL_FILTER_WIDTH			10 // mm (filter over last 10 mm
#define GAP_VELOCITY_WIDTH			20 // mm 
#define GAP_BUFFER_LEN              200 // samples
#define VELOCITY_MIN_COUNT          5
#define UPDATE_DISTANCE_MM			1 // mm
#define NAVIGATION_TARGET_DIST_MM	100
#define WHEEL_SPACING				(10.0 * 2.54)
#define MIN_STEERING_TIME			100
#define MAX_STEERING_TIME			2000
#define DESIRED_SPEED_VARIATION		5 // percent
#define MAX_SPEED_VARIATION			20 // percent
#define MAX_GAP_CHANGE_PER_MM       1

#define MIN_GAP						0.1
#define MAX_GAP						1.0 // 2.0
#define MIN_TURN_RATE               0.95
#define MAX_TURN_RATE               0.80 // 0.85

static double PI = 4 * atan(1.0);
static double g_X[GAP_BUFFER_LEN], g_Y[GAP_BUFFER_LEN];
enum { SOURCE_RAW_GAP = 0, SOURCE_FILT_GAP, SOURCE_RAW_VEL };

#ifdef _DEBUG_TIMING
static clock_t g_NoteNextLaserPositionTime = 0;
static int g_NoteNextLaserPositionCount = 0;

static clock_t g_SetMotorSpeedTime = 0;
static int g_SetMotorSpeedCount = 0;

static clock_t g_LowPassFilterGapTime = 0;
static int g_LowPassFilterGapCount = 0;
#endif

static UINT ThreadSteerMotors(LPVOID param)
{
	CWeldNavigation* this2 = (CWeldNavigation*)param;
	return this2->ThreadSteerMotors();
}

static UINT ThreadNoteLaser(LPVOID param)
{
	CWeldNavigation* this2 = (CWeldNavigation*)param;
	return this2->ThreadNoteLaser();
}

CWeldNavigation::CWeldNavigation(CMotionControl& motion, CLaserControl& laser)
	: m_motionControl(motion)
	, m_laserControl(laser)
{
	m_bScanning = FALSE;
	m_bSteerMotors = FALSE;
	m_hThreadSteerMotors = NULL;
	m_hThreadNoteLaser = NULL;
	m_fMotorSpeed = FLT_MAX;
	m_fMotorAccel = FLT_MAX;
	m_fWeldOffset = 0;
	m_pParent = NULL;
	m_nMsg = 0;
}

CWeldNavigation::~CWeldNavigation()
{
	StopSteeringMotors();
}

// these are used to send messages back to the owner object
// this object will be running a thread
// it is niot desireable to talk to the MAG controller in other than the start up thread
void CWeldNavigation::Init(CWnd* pWnd, UINT nMsg)
{
	m_pParent = pWnd;
	m_nMsg = nMsg;
}

static double HammingWindow(double dist, double nWidth)
{
	double Wk = 0.54 - 0.46 * cos(PI * (nWidth-dist) / nWidth);
	return Wk;
}

static double ModelY(double x, const double coeff[], int order)
{
	if (order == 2)
		return coeff[2] * x * x + coeff[1] * x + coeff[0];
	else if (order == 1)
		return coeff[1] * x + coeff[0];
	else
		return FLT_MAX;
}

/*
static int RemoveOutliers(double X[], double Y[], int count, int max_sd)
{
	// get the average of the Y values
	double coeff[3];
	polyfit(X, Y, count, 0, coeff);
	double avg = coeff[0];

	// get the sd of the values
	double sum2 = 0;
	for (int i = 0; i < count; ++i)
		sum2 += pow(g_Y[i] - avg, 2.0);
	double sd = sqrt(sum2 / count);

	// remove all that are outside 2 SD
	for (int i = 0; i < count; ++i)
	{
		double diff = fabs(g_Y[i] - avg);
		if (diff > max_sd * sd)
		{
			memcpy(X + i, X + i + 1, (count - i - 1) * sizeof(double));
			memcpy(Y + i, Y + i + 1, (count - i - 1) * sizeof(double));
			count--;
			i--;
		}
	}


	return count;
}

*/

// fit to a polynomial and then check for outliers from thaqt poilynomial
static int RemoveOutliers(double X[], double Y[], int order, int count, int max_sd )
{
	double coeff[3];

	// look for Y changes greater than 1 mm
	if (count > 4)
	{
		// get the average
		polyfit(X, Y, count, 0, coeff);
		double avg = coeff[0];

		for (int i = 1; i < count; ++i)
		{
			// if the difference is > 1 mm
			// remvoed the entry furthest from the average
			double diff = fabs(Y[i - 1] - Y[i]) / fabs(X[i - 1] - X[i]);
			if (diff >= 1.0)
			{
				double diff1 = fabs(Y[i - 1] - avg);
				double diff2 = fabs(Y[i - 0] - avg);
				if (diff1 > diff2)
				{
					memcpy(X + i - 1, X + i, (count - i) * sizeof(double));
					memcpy(Y + i - 1, Y + i, (count - i) * sizeof(double));
				}
				else
				{
					memcpy(X + i, X + i + 1, (count - i - 1) * sizeof(double));
					memcpy(Y + i, Y + i + 1, (count - i - 1) * sizeof(double));
				}
				count--;
				i--;
			}
		}
	}

	// remove the largest varation until the SDS < 0.1
	double sd = FLT_MAX;
	while (sd > 0.1 && count > 2)
	{
		polyfit(X, Y, count, order, coeff);

		// get the RMS variation 
		double sum2 = 0;
		for (int i = 0; i < count; ++i)
		{
			double y = ModelY(X[i], coeff, order);
			double diff = y - Y[i];
			sum2 += diff * diff;
		}
		double sd = sqrt(sum2 / count);
		if (sd < 0.1)
			break;

		// now remove the largest variation
		int indMax = -1;
		double diffMax = 0;

		// remove any variation more than max_sd
		for (int i = 0; i < count; ++i)
		{
			double y = ModelY(X[i], coeff, order);
			double diff = fabs(y - Y[i]);
			if (indMax == -1 || diff > diffMax)
			{
				diffMax = diff;
				indMax = i;
			}
		}
		if (indMax != -1)
		{
			memcpy(X + indMax, X + indMax + 1, (count - indMax - 1) * sizeof(double));
			memcpy(Y + indMax, Y + indMax + 1, (count - indMax - 1) * sizeof(double));
			count--;
		}
	}

	return count;
}


static BOOL GetGapCoefficients(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, double coeff[3], int order, int source, int nMinWidth, int nMaxWidth)
{
	// may need to minimize the width if not enough data
	int nSize = (int)buff1.GetSize();
	if (nSize < 2)
		return FALSE;

	// start with the last entered value
	// add values to the sum until too far away
	double pos2 = buff1[nSize - 1].pos;
	double val2 = (source == SOURCE_RAW_GAP) ? buff1[nSize - 1].gap_raw : (source == SOURCE_RAW_VEL ? buff1[nSize - 1].vel_raw  : buff1[nSize - 1].gap_filt);

	// if pos1 != 0, then do not use values less than it
	int count = 0;
	for (int i = nSize-1; i >= 0 && count < GAP_BUFFER_LEN; --i)
	{
		double val1 = (source == SOURCE_RAW_GAP) ? buff1[i].gap_raw : (source == SOURCE_RAW_VEL ? buff1[i].vel_raw : buff1[i].gap_filt);
		if (val1 == FLT_MAX)
			continue;

		// check if the gap has changed by more than 1 mm 
		// getting positioons abgout every 1 mm, thus not able to change by this much
//		if (source != SOURCE_RAW_VEL && fabs(val2 - val1) > MAX_GAP_CHANGE_PER_MM)
//			continue;

		double pos1 = buff1[i].pos;

//		// never use the first 10 mm of data, as it does not appear to be stable
//		if (pos1 < 10)
//			break;

		// if first order get no values prior to the lastr manoeuvre
		if (order == 1)
		{
			if (last_manoeuvre_pos != FLT_MAX && pos1+ nMinWidth < last_manoeuvre_pos)
				break;
		}
		else if( order == 2 )
		{
			// if second order restrict to no more prior to manoeuvre, than after it (NO)
			if ((last_manoeuvre_pos != FLT_MAX) && (last_manoeuvre_pos - pos1) > (pos2 - last_manoeuvre_pos) )
				break;
		}

		g_X[count] = pos1;
		g_Y[count] = val1;
		count++;
		val2 = val1;

		// check after adding to insure that pass the check later on for minimum width
		if (fabs(pos2 - pos1) > nMaxWidth)
			break;

	}


	// for 2nd order need at least two samples
	if (count < order + 1)
		return FALSE;
	// remove outliers
//	if (source == SOURCE_RAW_GAP && count > 2)
//		count = RemoveOutliers(g_X, g_Y, count, 2/*sd*/);


	// just get the average, dont worry if have minimnum width
	if (order == 0)
	{
		polyfit(g_X, g_Y, count, order, coeff);
		return TRUE;
	}


	// check if have minimum length of data (10 mm) to get a filtered value
	if (fabs(g_X[0] - g_X[count - 1]) < nMinWidth)
		return FALSE;

	count = RemoveOutliers(g_X, g_Y, order, count, 2/*max_sd*/);
	if (count < 1+order)
		return FALSE;

	// get the coefficients f the data set
	polyfit(g_X, g_Y, count, order, coeff);
	return TRUE;
}

// this is intended to filter only the last added value
// set the hammin window to the length available up to nWidth
// use the position offset for the qwindow, not the index
static CDoublePoint LowPassFilterGap(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos)
{
	clock_t t1 = clock();
	double coeff[4];
	CDoublePoint ret;

	// filter the last 50 mm
	// 2nd order, so can handle changes in the gap distance
	// using 1st order only, causes more delay in the filtering
	if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 2/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MAX__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double gap = coeff[2] * pos2*pos2 + coeff[1] * pos2 + coeff[0];
		double vel = 2 * coeff[2] * pos2 + coeff[1]; // differentiated
		ret = CDoublePoint(gap,1000*vel);
	}

	// if can't get coefficitns, then try for an average value over the minimum width
	else if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 1/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MIN__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double gap = coeff[1] * pos2 + coeff[0];
		double vel = coeff[1]; // differentiated
		ret = CDoublePoint(gap, 1000 * vel);
	}

	// this just retuns the average, allow to be only 5 mm 
	// if can't get coefficitns, then try for an average value over the minimum width
	else if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 0/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH/2, GAP_FILTER_MIN__WIDTH/2))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double gap = coeff[0];
		ret = CDoublePoint(gap, FLT_MAX);
	}
	else
	{
		ret = CDoublePoint(FLT_MAX, FLT_MAX);
	}

#ifdef _DEBUG_TIMING
	g_LowPassFilterGapTime += clock() - t1;
	g_LowPassFilterGapCount++;
#endif

	return ret;
}
// this is intended to filter only the last added value
// set the hammin window to the length available up to nWidth
// use the position offset for the qwindow, not the index
// this function will end using the filtered gap locations

static double CalcGapVelocity(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos)
{
	double coeff[3];
	// filter the last 50 mm
	// 1st order, only want the slope
	if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 2/*ORDER*/, SOURCE_FILT_GAP/*source*/, VEL_FILTER_MIN_WIDTH, VEL_FILTER_MAX_WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double vel = 2*coeff[2] * pos2 + coeff[1]; //  1000 * coeff[1]; // mm/M
		return 1000.0 * vel;
	}
	else 
		if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 1/*ORDER*/, SOURCE_FILT_GAP/*source*/, VEL_FILTER_MIN_WIDTH, VEL_FILTER_MIN_WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double vel = coeff[1]; //  1000 * coeff[1]; // mm/M
		return 1000.0 * vel;
	}
	else
		return FLT_MAX;
}


LASER_POS CWeldNavigation::GetLastNotedPosition(int ago_mm)
{
	LASER_POS ret;
	ret.pos = FLT_MAX;

	m_crit1.Lock();
	int nSize = (int)m_listLaserPositions.GetSize();
	if( nSize > 0 )
	{
		if (ago_mm == 0)
			ret = m_listLaserPositions[nSize - 1];
		else
		{
			double pos2 = m_listLaserPositions[nSize - 1].pos; // want the one that is at least ago_mm prior to this
			for (int i = nSize - 1; i >= 0; --i)
			{
				if (pos2 - m_listLaserPositions[i].pos > ago_mm)
				{
					ret = m_listLaserPositions[i];
					break;
				}
			}
		}
	}
	m_crit1.Unlock();
	return ret;
}

BOOL CWeldNavigation::StopMotors()
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
	{
		BOOL ret = (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_STOP_MOTORS);
		return ret;
	}
	else
		return FALSE;
}

double CWeldNavigation::GetLRPositionDiff()
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
	{
		double ret = (double)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_LR_DIFFEENCE) / 100.0;
		return ret;
	}
	else
		return 0.0;
}

BOOL CWeldNavigation::SetMotorDeceleration(double decel)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
	{
		BOOL ret = (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SET_MOTOR_DECEL, (int)(100*decel+0.5));
		return ret;
	}
	else
		return FALSE;
}


BOOL CWeldNavigation::SetMotorSpeed(const double speed[4] )
{
	clock_t t1 = clock();
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
	{
		BOOL ret = (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SET_MOTOR_SPEED, (LPARAM)speed);
#ifdef _DEBUG_TIMING
		g_SetMotorSpeedTime += clock() - t1;
		g_SetMotorSpeedCount++;
#endif
		return ret;
	}
	else
		return FALSE;
}

void CWeldNavigation::SendDebugMessage(const CString& str)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SEND_DEBUG_MSG, (LPARAM)&str);
}

// get these every mm
BOOL CWeldNavigation::NoteNextLaserPosition()
{
	clock_t t1 = clock();

	LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();
	if (measure2.weld_cap_mm.x != FLT_MAX )
	{
		// m_measure will be updatre regularily 
		double motor_pos = measure2.measure_pos_mm; //  GetAvgMotorPosition();
		if (motor_pos == FLT_MAX)
			return FALSE;

		// mkust get this before locking the critical section to avoid grid lock
		double last_manoeuvre_pos = m_motionControl.GetLastManoeuvrePosition();

		// if the motor has not yet moved a full mm, then return
		// less than this and the velocity will not be stable
		m_crit1.Lock();
		int nSize = (int)m_listLaserPositions.GetSize();

		// mujst have moved at least 1 mm from the last value
		if (nSize > 0 && fabs(motor_pos - m_listLaserPositions[nSize - 1].pos) < 1 )
		{
			m_crit1.Unlock();
			return FALSE;
		}

		// this RAW value must be within 1 mm per 10 mm of travel
		// else consider it to be bogus
		if (nSize > 5)
		{
			double gap_diff = fabs(measure2.weld_cap_mm.x - m_listLaserPositions[nSize - 1].gap_filt);
			double pos_diff = fabs(motor_pos - m_listLaserPositions[nSize - 1].pos);
			if (gap_diff / pos_diff > 0.1)
			{
				m_crit1.Unlock();
				return FALSE;
			}
		}

		// add to the list, then check if get a valoid filtered value
		// if noit remove it from the list
		m_listLaserPositions.SetSize(++nSize, NAVIGATION_GROW_BY);
		m_listLaserPositions[nSize - 1].pos = motor_pos;
		m_listLaserPositions[nSize - 1].gap_raw = measure2.weld_cap_mm.x;
		CDoublePoint ret = ::LowPassFilterGap(m_listLaserPositions, last_manoeuvre_pos);
		if (nSize > 1 && fabs(ret.x - m_listLaserPositions[nSize - 2].gap_filt) > 1)
		{
			int xx = 1;
		}

		m_listLaserPositions[nSize - 1].gap_filt = ret.x;
		m_listLaserPositions[nSize - 1].vel_raw  = ret.y;

		if (m_listLaserPositions[nSize - 1].gap_filt == FLT_MAX)
			m_listLaserPositions[nSize - 1].gap_filt = m_listLaserPositions[nSize - 1].gap_raw;

		m_listLaserPositions[nSize - 1].time_noted	= clock(); // niote wshen this position was, used to calculate velocities
		m_listLaserPositions[nSize - 1].pos			= motor_pos;
		m_listLaserPositions[nSize - 1].rgb_sum = measure2.rgb_sum;
		m_listLaserPositions[nSize - 1].last_manoeuvre_pos = last_manoeuvre_pos;
		m_listLaserPositions[nSize - 1].measures = measure2;
		//		m_listLaserPositions[nSize - 1].vel_filt = ::CalcGapVelocity(m_listLaserPositions, last_manoeuvre_pos);
		m_crit1.Unlock();
#ifdef _DEBUG_TIMING
		g_NoteNextLaserPositionTime += clock() - t1;
		g_NoteNextLaserPositionCount++;
#endif
	}
	return TRUE;
}

BOOL CWeldNavigation::IsNavigating()const
{
	return m_hThreadSteerMotors != NULL;
}

static void InterpolateVector(const CArray<double,double>& X, const CArray<double,double>& src, CArray<double, double>& dest, int start_pos, int end_pos)
{
	static double X2[100], Y2[100];
	int len = abs(end_pos - start_pos) + 1;
	dest.SetSize(len);

	double coeff[3];
	int nSize = (int)X.GetSize();
	len = 0;
	for (int pos = start_pos; pos <= end_pos; ++pos)
	{
		int i1, i2;
		for (i1 = 0; i1 < nSize && X[i1] < pos-10; ++i1);
		for (i2 = nSize - 1; i2 >= 0 && i2 > pos+10; --i2); 

		if (i2 - i1 < 3 && i1 - 1 >= 0) i1--;
		if (i2 - i1 < 3 && i2 + 1 < nSize) i2++;

		int count = 0;
		for (int i = i1; i < i2 && count < 100; ++i)
		{
			X2[count] = X[i];
			Y2[count] = src[i];
			count++;
		}
		if (count >= 3)
		{
			polyfit(X2, Y2, count, 2, coeff);
			dest[len] = coeff[2] * pos * pos + coeff[1] * pos + coeff[0];
			len++;
		}
		else if (count >= 2)
		{
			polyfit(X2, Y2, count, 1, coeff);
			dest[len] = coeff[1] * pos + coeff[0];
			len++;
		}
		else if (count >= 1)
		{
			polyfit(X2, Y2, count, 0, coeff);
			dest[len] = coeff[0];
			len++;
		}
	}
}

BOOL CWeldNavigation::WriteScanFile()
{
	char my_documents[MAX_PATH];
	HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	if (result != S_OK)
		return FALSE;

	// get a list of the existing files
	CString path;
	path.Format("%s\\SimplyUTFiles\\*.txt", my_documents);

	int nMaxFile = 0;
	CFileFind find;
	if (find.FindFile(path))
	{
		int ret = 1;
		for (int i = 0; ret != 0; ++i)
		{
			ret = find.FindNextFileA();
			CString szFile = find.GetFileName();
			int ind = szFile.Find("_");
			if (ind != -1)
			{
				int nFile = atoi(szFile.Mid(ind + 1));
				nMaxFile = max(nMaxFile, nFile);
			}
		}
	}

	path.Format("%s\\SimplyUTFiles\\File_%d.txt", my_documents, nMaxFile+1);
	FILE* fp = NULL;
	fopen_s(&fp, path, "w");
	if (fp == NULL)
		return FALSE;

	// pos, cap H, cap W, L/R diff, weld offset
	fprintf(fp, "Pos\tCap H\tCap W\tHi/Lo\tOffset\n");

	int nSize = (int)m_listLaserPositions.GetSize();
	CArray<double, double> X, Y1[2], Y2[2], Y3[2], Y4[2];
	X.SetSize(nSize);
	Y1[0].SetSize(nSize);
	Y2[0].SetSize(nSize);
	Y3[0].SetSize(nSize);
	Y4[0].SetSize(nSize);

	for (int i = 0; i < nSize; ++i)
	{
		const LASER_MEASURES& measure2 = m_listLaserPositions[i].measures;
		double avg_side_height = (measure2.weld_left_height_mm + measure2.weld_right_height_mm) / 2;
		double weld_cap_height = measure2.weld_cap_mm.y;

		X[i] = m_listLaserPositions[i].pos;
		Y1[0][i] = avg_side_height - weld_cap_height;
		Y2[0][i] = fabs(measure2.weld_right_mm - measure2.weld_left_mm);
		Y3[0][i] = fabs(measure2.weld_left_height_mm - measure2.weld_right_height_mm);
		Y4[0][i] = m_listLaserPositions[i].gap_filt;
	}


	InterpolateVector(X, Y1[0], Y1[1], m_nStartPos, m_nEndPos);
	InterpolateVector(X, Y2[0], Y2[1], m_nStartPos, m_nEndPos);
	InterpolateVector(X, Y3[0], Y3[1], m_nStartPos, m_nEndPos);
	InterpolateVector(X, Y4[0], Y4[1], m_nStartPos, m_nEndPos);

	for(int i=0; i < Y1[1].GetSize(); ++i)
	{
		fprintf(fp, "%d\t%7.3f\t%7.3f\t%7.3f\t%7.3f\n",
			i, Y1[1][i], Y2[1][i], Y3[1][i], Y4[1][i]);
	}

	fclose(fp);
	return TRUE;
}

void CWeldNavigation::StartSteeringMotors(int nSteer, int start_mm, int end_mm, double motor_speed, double motor_accel, double offset, BOOL bScanning)
{
	StopSteeringMotors();
	if ( nSteer)
	{
		m_fMotorSpeed = motor_speed;
		m_fMotorAccel = motor_accel;
		m_fWeldOffset = offset;
		m_nStartPos = start_mm;
		m_nEndPos = end_mm;

		m_bScanning = bScanning;
		m_bSteerMotors = TRUE;
		m_crit1.Lock();
		m_listLaserPositions.SetSize(0);
		m_crit1.Unlock();
#ifdef _DEBUG_TIMING
		g_NoteNextLaserPositionTime = 0;
		g_NoteNextLaserPositionCount = 0;

		g_SetMotorSpeedTime = 0;
		g_SetMotorSpeedCount = 0;

		g_LowPassFilterGapTime = 0;
		g_LowPassFilterGapCount = 0;
#endif
		if( nSteer & 0x1 )
			m_hThreadNoteLaser = AfxBeginThread(::ThreadNoteLaser, this);
		if (nSteer & 0x2)
			m_hThreadSteerMotors = AfxBeginThread(::ThreadSteerMotors, this);
	}
	else
	{
#ifdef _DEBUG_TIMING
		char my_documents[MAX_PATH];
		HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
		if (result == S_OK)
		{
			CString szFile;
			szFile.Format("%s\\test2.txt", my_documents);
			FILE* fp1;  fopen_s(&fp1, szFile, "w");

			if (fp1)
			{
				for (int i = 0; i < m_listLaserPositions.GetSize(); ++i)
				{
					fprintf(fp1, "%.1f\t%d\t%.3f\t%.3f\t%.3f\t%.3f\t%.1f\n",
						m_listLaserPositions[i].pos,
						m_listLaserPositions[i].rgb_sum,
						(m_listLaserPositions[i].gap_raw == FLT_MAX) ? 0 : m_listLaserPositions[i].gap_raw,
						(m_listLaserPositions[i].gap_filt == FLT_MAX) ? 0 : m_listLaserPositions[i].gap_filt,
						(m_listLaserPositions[i].vel_raw == FLT_MAX) ? 0 : m_listLaserPositions[i].vel_raw,
						(m_listLaserPositions[i].vel_filt == FLT_MAX) ? 0 : m_listLaserPositions[i].vel_filt,
						(m_listLaserPositions[i].last_manoeuvre_pos == FLT_MAX) ? 0 : m_listLaserPositions[i].last_manoeuvre_pos);
				}
				fclose(fp1);
			}
		}

		CString text;
		text.Format("NoteNextLaserPosition: %.1f ms (%d)", g_NoteNextLaserPositionCount ? (double)g_NoteNextLaserPositionTime / g_NoteNextLaserPositionCount : 0, g_NoteNextLaserPositionTime);
		SendDebugMessage(text);
		text.Format("SetMotorSpeed: %.1f ms (%d)", g_SetMotorSpeedCount ? (double)g_SetMotorSpeedTime / g_SetMotorSpeedCount : 0, g_SetMotorSpeedTime);
		SendDebugMessage(text);
		text.Format("LowPassFilterGap: %.1f ms (%d)", g_LowPassFilterGapCount ? (double)g_LowPassFilterGapTime / g_LowPassFilterGapCount : 0, g_LowPassFilterGapTime);
		SendDebugMessage(text);
#endif
	}
}

void CWeldNavigation::StopSteeringMotors()
{
	if (m_hThreadNoteLaser != NULL)
	{
		m_bSteerMotors = FALSE;
		WaitForSingleObject(m_hThreadNoteLaser, INFINITE);
		m_hThreadNoteLaser = NULL;
	}
	if (m_hThreadSteerMotors != NULL)
	{
		m_bSteerMotors = FALSE;
		WaitForSingleObject(m_hThreadSteerMotors, INFINITE);
		m_hThreadSteerMotors = NULL;
	}
}

// try to get a new position every 5 mm
UINT CWeldNavigation::ThreadNoteLaser()
{
	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while (m_bSteerMotors)
	{
		Sleep(1);
		NoteNextLaserPosition();
	}
	return 0;
}

void CWeldNavigation::GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>& list)
{
	m_crit1.Lock();
	list.Copy(m_listLaserPositions);
	m_crit1.Unlock();
}


static CDoublePoint GetTurnRateAndSpeed(double gap)
{
	CDoublePoint ret(1.0, 1.0);
	if (gap < MIN_GAP)
		ret.x = MIN_TURN_RATE;
/*	else if (gap > 4 * MAX_GAP) // > 2.0
	{
		ret.x = MAX_TURN_RATE - 0.3; // 0.5
		ret.y = 0.25; // run at 1/4 speed
	}

	else */ if (gap > 3 * MAX_GAP) // > 1.5
	{
		ret.x = MAX_TURN_RATE - 0.2; // 0.6
		ret.y = 0.25; // run at 1/4 speed
	}

	else if (gap > 2 * MAX_GAP) // > 1.0
	{
		ret.x = MAX_TURN_RATE - 0.1; // 0.7
		ret.y = 0.25; // run at 1/4 speed
	}

	else if (gap > MAX_GAP) // > 0.5
	{
		ret.x = MAX_TURN_RATE; // 0.8
		ret.y = 0.5; // run at 1/2 speed
	}
	else
	{
		double rate = (gap - MIN_GAP) * (MAX_TURN_RATE - MIN_TURN_RATE) / (MAX_GAP - MIN_GAP) + MIN_TURN_RATE;
		rate = max(rate, MAX_TURN_RATE); // 0.85
		rate = min(rate, MIN_TURN_RATE); // 0.9

		ret.x = fabs(rate);
	}

	ret.y = 1;
	return ret;
}

UINT CWeldNavigation::ThreadSteerMotors()
{
	if (m_nEndPos > m_nStartPos)
		ThreadSteerMotorsForward();
	else
		ThreadSteerMotorsBackard();

	return 0;
}

UINT CWeldNavigation::ThreadSteerMotorsForward()
{
	const int MAX_TRAVEL_TIME = 1000;
	clock_t tim1 = clock();
	CString str;

	int steer = 0;
	int  time_first = 0;

	CArray<DRIVE_POS, DRIVE_POS> listVel;
	CArray<POS_MANOEVER, POS_MANOEVER> listManoevers;

	// if the gap is to one side or the other by more than about 0.1 mm
	// jog the crawler to the centre, and then check again\
	// a jog is a turn first towartds the centrte, then back to the original direction
	while (m_bSteerMotors)
	{
		// avoid risk of a tight loop
		Sleep(1);

		// check if near the end, if so end the navigation
		// this is a pre-distance to get close before actual recording starts
		LASER_POS pos0 = GetLastNotedPosition(0);										// where is the crawler now
		if (pos0.pos == FLT_MAX)
			continue;

		// have programmed the motors to travel further than desired
		// thus issue a stop in this case
		// that way nall the mnotors will stop at the sazme exact time
		if (pos0.pos >= m_nEndPos)
		{
			SetMotorDeceleration(2 * m_fMotorAccel);
			StopMotors();
			break; // stop navigating now
		}

		// what is the gap right now
		else if (pos0.gap_filt == FLT_MAX)													// not known so do nothing
			continue;

		// do nothing if close to the weld already
		else if (fabs(pos0.gap_filt) < 0.1)													// trying to correct very small errors may be counter productive
			continue;

		// if to the left, turn to the right
		int dir = (pos0.gap_filt > 0) ? -1 : 1;											// which way to navigate (right or left)

		// the closer to the weld, the slower the required turn rate
		CDoublePoint turn_rate1 = GetTurnRateAndSpeed(pos0.gap_filt);
		double fSpeed1 = m_fMotorSpeed * turn_rate1.y;
		double speedLeft = m_fMotorSpeed - dir * (1 - turn_rate1.x) * fSpeed1 / 2;
		double speedRight = m_fMotorSpeed + dir * (1 - turn_rate1.x) * fSpeed1 / 2;
		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };
		SetMotorSpeed(speed1);

		// keep turning until the gap is halved
		// it will take time to strighten out
		clock_t t1 = clock();

		BOOL bCheckWrongWay = TRUE;
		while (m_bSteerMotors) // && clock() - t1 < MAX_TRAVEL_TIME)
		{
			// avoid a tight loop
			Sleep(10);
			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now

			// stop if change side of weld that on 
			// incase below is missed
			if ((pos0.gap_filt < 0) != (last_pos.gap_filt < 0))
				break;

			// stop mwhen have halfed the gap
			if (fabs(last_pos.gap_filt) < fabs(pos0.gap_filt) / 2)
				break;

			// check if the gap is getting worse
			// only make one correction
	/*		BOOL bWrongWay = fabs(last_pos.pos) > fabs(pos0.gap_filt);
			if (bCheckWrongWay && bWrongWay && clock() - t1 > 500 )
			{
				double tr2 = 1 - 3 * (1 - turn_rate1.x) / 2;
				double speedLeft = m_fMotorSpeed - dir * (1 - tr2) * fSpeed1 / 2;
				double speedRight = m_fMotorSpeed + dir * (1 - tr2) * fSpeed1 / 2;
				double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };
				SetMotorSpeed(speed1);
				bCheckWrongWay = FALSE;
			}
*/
// note the position and gap distance at this time
			int nSize2 = (int)listVel.GetSize();
			if (nSize2 == 0 || fabs(listVel[nSize2 - 1].x - last_pos.pos) >= 1)
				listVel.Add(DRIVE_POS(last_pos.pos, last_pos.gap_filt, 0));
		}

		// how long did it take to perform this correction
		// used in manoeuvre file
		clock_t turn_time1 = clock() - t1;
		LASER_POS pos1 = GetLastNotedPosition(0);				// where is the crawler now

		// now turn to travel straight
		// note, this does not mean stright along the weld, but means maintain the current direction (only halved the distance to weld)
		// note, the crawler does not appear to maintain the current direction, but changes slightly 
		CDoublePoint turn_rate2 = GetTurnRateAndSpeed(pos1.gap_filt);
		double fSpeed2 = m_fMotorSpeed * turn_rate2.y;

		double speed2[] = { fSpeed2, fSpeed2, fSpeed2, fSpeed2 };
		SetMotorSpeed(speed2);

		// travel the turn time to see if now straight
		// run for about a second to see if actually travelling straight
		// quit the turn when was 1/2 way there
		// drive straight until changes direction from/to weld
		t1 = clock();
		while (m_bSteerMotors && clock() - t1 < MAX_TRAVEL_TIME)
		{
			// avoid a tight loop
			Sleep(10);
			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now

			// no longer on the same side of the weld
			if ((pos0.gap_filt < 0) != (last_pos.gap_filt < 0))
				break;

			// stop if the gap starts increasing
			if (fabs(last_pos.gap_filt) > fabs(pos0.gap_filt))
				break;

			int nSize2 = (int)listVel.GetSize();
			if (nSize2 == 0 || fabs(listVel[nSize2 - 1].x - last_pos.pos) >= 1)
				listVel.Add(DRIVE_POS(last_pos.pos, last_pos.gap_filt, 1));
		}

		clock_t turn_time3 = clock() - t1;
		LASER_POS pos3 = GetLastNotedPosition(0);				// where is the crawler now

		// keep a record of manoeuvres
		// will use this to insure that don't calculate gap or velocity across a manoeuvre
		// need to note desired change in direction, how acomplished and for how long
		int nSize2 = (int)listManoevers.GetSize();
		listManoevers.SetSize(++nSize2);
		listManoevers[nSize2 - 1].time_start = clock();			// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos0 = pos0.pos;				// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos1 = pos1.pos;				// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos2 = 0;						// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos3 = pos3.pos;				// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].gap0 = pos0.gap_filt;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].gap1 = pos1.gap_filt;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].gap2 = 0;						// what offset were we trying to corect
		listManoevers[nSize2 - 1].gap3 = pos3.gap_filt;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_time1 = turn_time1;		// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_time2 = 0;				// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_time3 = turn_time3;		// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_direction = dir;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_rate = turn_rate1.x;			// what offset were we trying to corect
	}
#ifdef _DEBUG_TIMING
	WriteManoeuvreFile(listManoevers, listVel);
#endif
	return 0;
}

void CWeldNavigation::WriteManoeuvreFile(const CArray<POS_MANOEVER, POS_MANOEVER>& listManoevers, const CArray<DRIVE_POS, DRIVE_POS>& listVel)
{
	char my_documents[MAX_PATH];
	HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	if (result != S_OK)
		return;

	CString szFile;
	szFile.Format("%s\\Manoeuvres.txt", my_documents);
	FILE* fp2;  fopen_s(&fp2, szFile, "w");

	// write a list of the manoeuvres to a file
	if (fp2)
	{
		int dir = (m_nEndPos > m_nStartPos) ? 1 : -1;

		fprintf(fp2, "Pos0\tgap0\tRate\ttime1\tPos1\tgap1\ttime2\tPos2\tgap2\ttime3\tgap3\tPos3\n");

		int nSize2 = (int)listManoevers.GetSize();
		for (int i = 0; i < nSize2; ++i)
		{
			fprintf(fp2, "%7.1f\t%7.1f\t%5.2f\t%5.0f\t%7.1f\t%7.1f\t%5.0f\t%7.1f\t%7.1f\t%5.0f\t%7.1f\t%7.1f\n",
				listManoevers[i].pos0,
				listManoevers[i].gap0,
				listManoevers[i].turn_rate * listManoevers[i].turn_direction,
				listManoevers[i].turn_time1,
				listManoevers[i].pos1,
				listManoevers[i].gap1,
				listManoevers[i].turn_time2,
				listManoevers[i].pos2,
				listManoevers[i].gap2,
				listManoevers[i].turn_time3,
				listManoevers[i].pos3,
				listManoevers[i].gap3);

			// now output all the noted velocities from this time until the next time
			for (int j = 0; j < listVel.GetSize(); ++j)
			{
				if (dir*listVel[j].x >= dir*listManoevers[i].pos0 && 
					(i == nSize2 - 1 || dir*listVel[j].x < dir*listManoevers[i + 1].pos0))
				{
					fprintf(fp2, "%7.1f\t%7.1f\t%d\n",
						listVel[j].x,
						listVel[j].y,
						listVel[j].segment);
				}
			}
		}
		fclose(fp2);
	}
}

// in this case the laser trails not leads the crawler
// unlikely to be able to navigate as close as forward
// however, want to remain over the weld, and not drift too far
UINT CWeldNavigation::ThreadSteerMotorsBackard()
{
	const int MAX_TRAVEL_TIME = 1000;
	clock_t tim1 = clock();
	CString str;

	int steer = 0;
	int  time_first = 0;

	CArray<DRIVE_POS, DRIVE_POS> listVel;
	CArray<POS_MANOEVER, POS_MANOEVER> listManoevers;

	// make lots of very small corrections
	// assume that started close to the weld
	// thus if laser is on one side, need to steer to that side
	while (m_bSteerMotors)
	{
		// avoid risk of a tight loop
		Sleep(1);

		// check if near the end, if so end the navigation
		// this is a pre-distance to get close before actual recording starts
		LASER_POS pos0 = GetLastNotedPosition(0);										// where is the crawler now
		if (pos0.pos == FLT_MAX)
			continue;

		// have programmed the motors to travel further than desired
		// thus issue a stop in this case
		// that way nall the mnotors will stop at the sazme exact time
		if (pos0.pos < m_nEndPos)
		{
			SetMotorDeceleration(2 * m_fMotorAccel);
			StopMotors();
			break; // stop navigating now
		}

		// what is the gap right now
		else if (pos0.gap_filt == FLT_MAX)													// not known so do nothing
			continue;

		// do nothing if close to the weld already
		// trying to correct very small errors may be counter productive
		else if (fabs(pos0.gap_filt) < 0.1)
			continue;

		// if to the left, turn to the right
		int dir = (pos0.gap_filt > 0) ? -1 : 1;											// which way to navigate (right or left)

		// before the manoeuvre check the motor positions
		// to strighten out the crawler after the manoeuvre, set the same L/R difference
		double lr_diff1 = GetLRPositionDiff();

		// only make slow corrections
		double turn_rate1 = 0.95; //  GetTurnRateAndSpeed(pos0.gap_filt);
		double speedLeft = m_fMotorSpeed - dir * (1 - turn_rate1) * m_fMotorSpeed / 2;
		double speedRight = m_fMotorSpeed + dir * (1 - turn_rate1) * m_fMotorSpeed / 2;
		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };
		SetMotorSpeed(speed1);

		// blindly drive for 500 ms
		clock_t t1 = clock();
		while (m_bSteerMotors && clock() - t1 < 500)
		{
			Sleep(10);

			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now
			int nSize2 = (int)listVel.GetSize();
			if (nSize2 == 0 || fabs(listVel[nSize2 - 1].x - last_pos.pos) >= 1)
				listVel.Add(DRIVE_POS(last_pos.pos, last_pos.gap_filt, 0));
		}

		// now turn in the reverse of above
		// drive until the motor posiiotn LR differemnce is same as aboe
		double lr_diff2 = GetLRPositionDiff();
		double lr_target = (lr_diff2 - lr_diff1) / 2;
		
		LASER_POS pos1 = GetLastNotedPosition(0);
		double speed2[] = { speedRight, speedLeft, speedLeft, speedRight };
		SetMotorSpeed(speed2);

		// turn until the above turn has been corrected
		// limit to 1000 ms though (only drove for 500 ms above)
		t1 = clock();
		while (m_bSteerMotors && clock() - t1 < 1000)
		{
			// avoid a tight loop
			Sleep(1);
			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now

			double lr_diff3 = GetLRPositionDiff();
			if ( lr_diff3 - lr_target <= 0)
				break;

			int nSize2 = (int)listVel.GetSize();
			if (nSize2 == 0 || fabs(listVel[nSize2 - 1].x - last_pos.pos) >= 1)
				listVel.Add(DRIVE_POS(last_pos.pos, last_pos.gap_filt, 1));
		}

		// now strighten out 
		LASER_POS pos2 = GetLastNotedPosition(0);
		double speed3[] = { m_fMotorSpeed, m_fMotorSpeed, m_fMotorSpeed, m_fMotorSpeed };
		SetMotorSpeed(speed3);

		// drive blindly for 500 ms
		t1 = clock();
		while (m_bSteerMotors && clock() - t1 < 500)
		{
			Sleep(10);

			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now
			int nSize2 = (int)listVel.GetSize();
			if (nSize2 == 0 || fabs(listVel[nSize2 - 1].x - last_pos.pos) >= 1)
				listVel.Add(DRIVE_POS(last_pos.pos, last_pos.gap_filt, 2));
		}

		LASER_POS pos3 = GetLastNotedPosition(0);
		
		// keep a record of manoeuvres
		// will use this to insure that don't calculate gap or velocity across a manoeuvre
		// need to note desired change in direction, how acomplished and for how long
		int nSize2 = (int)listManoevers.GetSize();
		listManoevers.SetSize(++nSize2);
		listManoevers[nSize2 - 1].time_start = clock();			// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos0 = pos0.pos;				// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos1 = pos1.pos;				// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos2 = 0;						// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].pos3 = pos3.pos;				// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].gap0 = pos0.gap_filt;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].gap1 = pos1.gap_filt;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].gap2 = 0;						// what offset were we trying to corect
		listManoevers[nSize2 - 1].gap3 = pos3.gap_filt;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_time2 = 0;				// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_direction = dir;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_rate = turn_rate1;			// what offset were we trying to corect
	}
#ifdef _DEBUG_TIMING
	WriteManoeuvreFile(listManoevers, listVel);
#endif
	return 0;
}

UINT CWeldNavigation::ThreadSteerMotors_try2()
{
#define MAX_TRAVEL_TIME 2000 // 500 // 1000 (2000 found to give best results)
	clock_t tim1 = clock();
	CString str;

	int steer = 0;
	int  time_first = 0;

	// always wait 25 mm from last manoeuvre before checking anything, to let things settle
	CArray<DRIVE_POS, DRIVE_POS> listVel;
	CArray<POS_MANOEVER, POS_MANOEVER> listManoevers;

	int wait_manoeuvre_mm = 25;
	int wait_manoeuvre_time_ms = (int)(1000.0 * wait_manoeuvre_mm / m_fMotorSpeed + 0.5);
	int compare_gap_mm = 10;
	clock_t last_manoeuvre_time = 0;

	double decel_time = m_fMotorSpeed  / m_fMotorAccel; // sec
	int decel_dist = (int)(decel_time * m_fMotorSpeed / 2.0 + 0.5);

	while (m_bSteerMotors)
	{
		// avoid rtisk of a tight loop
		Sleep(10);


		// make sure that have 
		// insure that have travelled for at least 1/2 second since start or the last manoeuvre
		if (clock() - last_manoeuvre_time < wait_manoeuvre_mm)
			continue;
		// note where are now, and where was 25 mm ago
		// assume that after travelling (compare_gap_mm) will know if driving toward or away from the weld
		LASER_POS this_pos = GetLastNotedPosition(0);				// where is the crawler now
		LASER_POS prev_pos = GetLastNotedPosition(compare_gap_mm);	// where weas it compare_gap_mm ago

		int destination = m_motionControl.GetDestinationPosition();
		if (destination > 0 && this_pos.pos > (double)destination - (double)decel_dist)
			break;

		// note how far from the weld now, and at what position
		// also note where was 1/2 second ago

		// where is the crawler, and is it crawling toward or away from the weld
		// +Ve is to the left, -Ve to the right
		double gap_dist1 = prev_pos.gap_filt;
		double gap_dist2 = this_pos.gap_filt;
		if (gap_dist1 == FLT_MAX || gap_dist2 == FLT_MAX)
			continue;

		// the crawler is already travelling the the desired direction
		// how long will it take to get there
		double gap_vel = (this_pos.pos > prev_pos.pos+5) ? (gap_dist2 - gap_dist1) / (this_pos.pos - prev_pos.pos) : 0; // mm/mm
		double travel_time = (fabs(gap_dist2 - gap_dist1) > 0.1 && gap_vel != 0) ? fabs(1000 * gap_dist2 / m_fMotorSpeed / gap_vel) : 0; // ms to get to the weld
			
																																		 #// note which direction to turn, or if to stay the course
		// +1 = drive to the left
		// -1 = drive to the right
		int dir = 0;
//		if (fabs(gap_dist2) < 0.20)
//			dir = 0;
//		else 
		if (gap_dist1 > 0 /*to the left*/ && gap_dist2 > gap_dist1 /*driving to the left*/)
			dir = -1; // drive to the right
		else if (gap_dist1 > 0 /*to the left*/ && gap_dist2 < gap_dist1 /*driving to the right*/)
			dir = (travel_time < MAX_TRAVEL_TIME) ? 0 : -1; // stay the course
		else if (gap_dist1 < 0 /*to the right*/ && gap_dist2 < gap_dist1 /*driving to the right*/)
			dir = 1; // drive to the left
		else if (gap_dist1 < 0 /*to the right*/ && gap_dist2 > gap_dist1 /*driving to the left*/)
			dir = (travel_time < MAX_TRAVEL_TIME) ? 0 : 1; // stay the course

		// keep a record of manoeuvres
		// will use this to insure that don't calculate gap or velocity across a manoeuvre
		// need to note desired change in direction, how acomplished and for how long
		int nSize2 = (int)listManoevers.GetSize();
		listManoevers.SetSize(++nSize2);
		listManoevers[nSize2 - 1].pos0 = this_pos.pos;	// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].time_start = clock();			// at what ms did this manoeuvre begin
		listManoevers[nSize2 - 1].gap1 = gap_dist1;			// what was the gap 50 mm ago
		listManoevers[nSize2 - 1].gap2 = gap_dist2;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].gap_vel = gap_vel;			// what offset were we trying to corect
		listManoevers[nSize2 - 1].turn_direction = dir;		// trying to turn to tyhe right or left

		// if the direction = 0, then do nothing
		if (dir == 0)
			continue;

		// the closer to the weld, the slower the required turn rate
		// however, if close to weld, but travelling asawyay from the weld require a faster turn
		double turn_rate = 0.9; //  GetTurnRate(gap_dist2);
		double speedLeft = m_fMotorSpeed - dir * (1 - turn_rate) * m_fMotorSpeed / 2;
		double speedRight = m_fMotorSpeed + dir * (1 - turn_rate) * m_fMotorSpeed / 2;
		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };

		// note the position, and do not look at velocity for at least 500m ms
		// the velocity is unstable at start of turn
		last_manoeuvre_time = this_pos.time_noted;
		SetMotorSpeed(speed1);

		// leave at this turn rate for 2000 ms, or until the gap is 1/2 of the previous
		clock_t t1 = clock();
		double pos1 = this_pos.pos; // where was at the start of the manoeuvre
		double start_gap = gap_dist2;
		double start_rate = turn_rate;
		while (m_bSteerMotors && clock()-t1 < MAX_STEERING_TIME)
		{
			// avoid a tight loop
			Sleep(10);
			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now
			double gap_dist3 = last_pos.gap_filt;

			// have travelled far enouygh to check
	//		if ( last_pos.pos - pos1 >= wait_manoeuvre_mm )
			{
				// must have valid values to work with
				if (gap_dist3 == FLT_MAX)
					continue;

				// is it at least 1/2 way to the weld
				if (fabs(gap_dist3) < 0.10 || fabs(gap_dist3) < fabs(gap_dist2) / 2)
					break;

				// as it decreases decrease the turn 
				// half the turn rate
	//			if (gap_dist3 < start_gap && start_gap - gap_dist3 > 0.1)
	//			{
	//				start_rate = 1.0 - (1.0 - start_rate) / 2;
	//				double speedLeft = m_fMotorSpeed - dir * (1 - start_rate) * m_fMotorSpeed / 2;
	//				double speedRight = m_fMotorSpeed + dir * (1 - start_rate) * m_fMotorSpeed / 2;
	//				double speed2[] = { speedLeft, speedRight, speedRight, speedLeft };
	//				SetMotorSpeed(speed2);
	//				start_gap = gap_dist3;
	//			}
			}

			// note the position and gap distance at this time
			listVel.Add(DRIVE_POS(last_pos.pos, last_pos.gap_filt, 0));
		}

		// set back to driving straight (but in new direction)
		// it has been noted that the direction to/from the weld will be only about 60% of that during the turn
		clock_t manoeuvre_time = clock() - last_manoeuvre_time;

		last_manoeuvre_time = clock();
		double speed2[] = { m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed };
		SetMotorSpeed(speed2);

		// note the location for 25 mm
		while (m_bSteerMotors && clock() - t1 < MAX_STEERING_TIME)
		{
			// avoid a tight loop
			Sleep(10);

			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now
			double gap_dist3 = last_pos.gap_filt;

			if (last_pos.pos - pos1 >= wait_manoeuvre_mm)
				break;

			// note the position and gap distance at this time
			listVel.Add(DRIVE_POS(last_pos.pos, last_pos.gap_filt, 1));
		}

		// note where are now that have asked to straighten out
		LASER_POS last_pos = GetLastNotedPosition(0);

		// get the raw gap position for this so don't need the critical section
		listManoevers[nSize2 - 1].turn_rate = turn_rate;														// rate of the turn
		listManoevers[nSize2 - 1].manoeuvre_time = manoeuvre_time;											// time to get to 1/2 way point
		listManoevers[nSize2 - 1].gap3 = (last_pos.gap_filt == FLT_MAX) ? 9999 : last_pos.gap_filt;		// the gap seen at the 1/2 way point
		listManoevers[nSize2 - 1].time_end = last_manoeuvre_time;												// at what ms did this manoeuvre end
	}
#ifdef _DEBUG_TIMING
	WriteManoeuvreFile(listManoevers, listVel);
#endif

	return 0;
}

/*
UINT CWeldNavigation::ThreadSteerMotors()
{
	clock_t tim1 = clock();
	CString str;

	int steer = 0;
	int  time_first = 0;

	while(m_bSteerMotors)
	{
		// a seperate thread is setting this value
		m_crit1.Lock();
		if (m_listLaserPositions.GetSize() == 0 || m_last_pos.pos - m_listLaserPositions[0].pos < MIN_TRAVEL_DIST)
		{
			m_crit1.Unlock();
			Sleep(1); // avoid a tight loop
			continue;
		}

		double pos = m_last_pos.pos;
		double gap_dist = m_last_pos.gap_filt; // current distance from the weld
		double gap_vel1 = m_last_pos.gap_vel; // current mm/M to the weld
		time_first = m_listLaserPositions[0].time_noted; // clock() value of the first measure
		if (gap_dist == FLT_MAX || gap_vel1 == FLT_MAX)
		{
			m_crit1.Unlock();
			Sleep(1); // avoid a tight loop
			continue;
		}

		// now calculate the desired angle (mm/M) to the weld to be at the weld in about 100 mm (NAVIGATION_TARGET_DIST_MM)
		// this is simply the current distancve frtom the weld over 0.1 M
		// a -Ve gap is to the left, a +Ve to the right
		// if to the left, then need to steer to the right (thus -1000)
		double gap_vel2 = -1000 * m_last_pos.gap_filt / NAVIGATION_TARGET_DIST_MM; // mm/M
		m_crit1.Unlock();

		// the change in direction that is required
		double gap_vel = gap_vel2 - gap_vel1;

		// now calculate the extrta travel required by a given crawler side to achieve this change in direction
		// x = 	how long to change for
		// y = how to change by
		CDoublePoint travel = CalculateLeftRightTravel(gap_dist, gap_vel1, gap_vel2);

		// modify the mnotor speeds for the required time
		if (travel.x == 0)
		{
			Sleep(1);
			continue;
		}

		// if travel.x < 0 then want to travel to the right
		double speedLeft = m_fMotorSpeed - travel.x / 2;
		double speedRight = m_fMotorSpeed + travel.x / 2;

		str.Format("%d: GAP: %.1f mm, Vel1: %.1f mm/M, Vel2: %.1f mm/M, Steer %.1f mm/M for %.0f ms", 
			clock() - tim1, gap_dist, gap_vel1, gap_vel2, travel.x, travel.y);
		SendDebugMessage(str);

		str.Format("Left: %.1f, Right: %.1f mm/sec", speedLeft, speedRight);
		SendDebugMessage(str);

		int steer_time = (int)(travel.y + 0.5);
		int drive_time = (int)max(fabs(1000 * gap_dist / m_fMotorSpeed * gap_vel2 - steer_time), 2);

		// keep a record of manoeuvres
		// will use this to insure that don't calculate gap or velocity across a manoeuvre
		// need to note desired change in direction, how acomplished and for how long
		m_crit2.Lock();
		int nSize2 = (int)listManoevers.GetSize();
		listManoevers.SetSize(++nSize2);
		listManoevers[nSize2-1].time_start = clock(); // at what ms did this manoeuvre begin
		listManoevers[nSize2-1].gap_start = gap_dist; // what offset were we trying to corect
		listManoevers[nSize2-1].vel_start = gap_vel1; // how long is this manoeuvre going to last
		listManoevers[nSize2-1].vel_target = gap_vel2; // how long is this manoeuvre going to last
		listManoevers[nSize2-1].manoeuvre_time = steer_time; // how long is this manoeuvre going to last
		listManoevers[nSize2-1].manoeuvre_speed = travel.x; // 
		listManoevers[nSize2-1].drive_time = drive_time; // at what ms did this manoeuvre begin
		m_crit2.Unlock();

		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };
		SetMotorSpeed(speed1);
		Sleep( steer_time );

		// set back to driving straight (but in new direction)
		double speed2[] = { m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed };
		SetMotorSpeed(speed2);

		LASER_MEASURES measure;
		if ( GetLaserMeasurment(measure))
		{
			// get the raw gap position for this so don't need the critical section
			listManoevers[nSize2-1].gap_end_man = measure.weld_cap_mm.x;
			listManoevers[nSize2-1].time_end = clock(); // at what ms did this manoeuvre begin
		}

		// drive the remainder at this direction
		// assume that now on location
		Wait(drive_time); //  / 2);
				// now note the gap at the end of the drive
		if (GetLaserMeasurment(measure))
			listManoevers[nSize2 - 1].gap_end_drive = measure.weld_cap_mm.x;

	}
	// write a list of the manoeuvres to a file
	if (g_fp2)
	{
		fprintf(g_fp2, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
			"Start", "gap1", "vel1", "vel2", "time", "actual", "speed", "gap2", "drive", "gap3");

		int nSize2 = (int)listManoevers.GetSize();
		for (int i = 0; i < nSize2; ++i)
		{
			fprintf(g_fp2, "%d\t%.1f\t%.1f\t%.1f\t%d\t%d\t%.1f\t%.1f\t%d\t%.1f\n",
				listManoevers[i].time_start - time_first,
				listManoevers[i].gap_start,
				listManoevers[i].vel_start,
				listManoevers[i].vel_target,
				listManoevers[i].manoeuvre_time,
				listManoevers[i].time_end - listManoevers[i].time_start,
				listManoevers[i].manoeuvre_speed,
				listManoevers[i].gap_end_man,
				listManoevers[i].drive_time,
				listManoevers[i].gap_end_man	);
		}
	}

	return 0;
}
*/
// be sure to update the motor position every mm while waiting
void CWeldNavigation::Wait(int delay)
{
	clock_t tim2 = clock();
	int delay1 = (int)(UPDATE_DISTANCE_MM * 1000.0 / m_fMotorSpeed + 0.5);
	delay1 = max(delay1, 1);
	delay1 = min(delay1, 100);

	while (m_bSteerMotors && clock() - tim2 < delay)
	{
		NoteNextLaserPosition();
		Sleep(delay1);
	}
}

// calculate the extra distance that one side of the crawler must travel to achieve the desired change in direction
double CWeldNavigation::CalculateTurnRate(double gap_dist, double gap_vel1, double gap_vel2)
{
#define TIME_CAL_FACTOR 10
	// if want to travel (dir_change) in 1000 mm
	// then calculate how far will travel in 10*2.54 mm
	// the change in crawler angle in  radians
	double gap_vel = gap_vel2 - gap_vel1;
	double dist = gap_vel * WHEEL_SPACING / 1000;

	// if speed up the motors by 5%, how long will it take to achieve this travel
	// do not want to have a significant difference left right
	// small changes will make the navigation more predictable
	// this is the relative speed in mm/sec
	// i.e in 1 sec would travel this far
	double fChange = DESIRED_SPEED_VARIATION * m_fMotorSpeed / 100.0f; // mm/Sec

	// this is the time that the speed change is required;
	// experimentally this was producing a time about 10x too short
	double change_time = fabs(TIME_CAL_FACTOR * 1000.0 * dist / fChange);

	if (change_time > MAX_STEERING_TIME)
	{
		fChange = TIME_CAL_FACTOR * 1000.0 * dist / MAX_STEERING_TIME;
		change_time = MAX_STEERING_TIME;
	}

	// check that this change is still within the pemitted percentage of speed (20%)
	if (100 * fabs(fChange) / m_fMotorSpeed > MAX_SPEED_VARIATION)
	{
		change_time = fabs(TIME_CAL_FACTOR * 1000.0 * dist / (m_fMotorSpeed * MAX_SPEED_VARIATION / 100));
		fChange = TIME_CAL_FACTOR * 1000.0 * dist / change_time;
	}

	return 100.0 * fabs(fChange) / m_fMotorSpeed;
}

CDoublePoint CWeldNavigation::GetLastRGBValue()
{
	CDoublePoint ret;

	m_crit1.Lock();
	int nSize = m_listLaserPositions.GetSize();
	if (nSize > 0 )
	{
		ret.x = m_listLaserPositions[nSize - 1].pos;
		ret.y = m_listLaserPositions[nSize - 1].rgb_sum;
	}
	m_crit1.Unlock();

	return ret;
}

/*
// calculate the extra distance that one side of the crawler must travel to achieve the desired change in direction
CDoublePoint CWeldNavigation::CalculateLeftRightTravel(double gap_dist, double gap_vel1, double gap_vel2)
{
#define TIME_CAL_FACTOR 5
	// if want to travel (dir_change) in 1000 mm
	// then calculate how far will travel in 10*2.54 mm
	// the change in crawler angle in  radians
	double gap_vel = gap_vel2 - gap_vel1;
	double dist = gap_vel * WHEEL_SPACING / 1000;

	// if could turn instantaniously, would need to drive this far to travel (gap) towards the centre line
	// would like to be turned within 1/2 this time
	int drive_time = (int)fabs(1000 * gap_dist / m_fMotorSpeed * gap_vel2);
	//	int max_steering_time = max(drive_time / 2, 10);
	int max_steering_time = max(drive_time, 10);

	// if speed up the motors by 5%, how long will it take to achieve this travel
	// do not want to have a significant difference left right
	// small changes will make the navigation more predictable
	// this is the relative speed in mm/sec
	// i.e in 1 sec would travel this far
	double fChange = DESIRED_SPEED_VARIATION * m_fMotorSpeed / 100.0f; // mm/Sec

	// this is the time that the speed change is required;
	// experimentally this was producing a time about 10x too short
	double change_time = TIME_CAL_FACTOR * 1000 * dist / fabs(fChange);

	// want this time to be between 1/10 sec and 2 sec
	if (change_time < MIN_STEERING_TIME)
	{
		fChange = TIME_CAL_FACTOR * 1000 * dist / MIN_STEERING_TIME;
		change_time = MIN_STEERING_TIME;
	}
	else if (fabs(change_time) > max_steering_time) // MAX_STEERING_TIME)
	{
		fChange = TIME_CAL_FACTOR * 1000 * dist / max_steering_time; //  MAX_STEERING_TIME;
		change_time = max_steering_time; //  MAX_STEERING_TIME;
	}

	// check that this change is still within the premitted percentage of speeddf
	if (100 * fabs(fChange) / m_fMotorSpeed > MAX_SPEED_VARIATION)
	{
		change_time = TIME_CAL_FACTOR * 1000 * fabs(dist) / (m_fMotorSpeed * MAX_SPEED_VARIATION / 100);
		fChange = TIME_CAL_FACTOR * 1000 * dist / change_time;
	}

	if (change_time < 0)
	{
		int xx = 1;
	}

	return CDoublePoint(fChange, change_time);
}
*/