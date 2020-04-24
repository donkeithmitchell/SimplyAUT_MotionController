#include "pch.h"
#include "WeldNavigation.h"
#include "LaserControl.h"
#include "MotionControl.h"
#include "MagController.h"
#include "DialogGirthWeld.h"
#include "Define.h"
#include "Misc.h"

// this is an object to bath maintain a list of laser measurments by positioon
// as well as navigate to the weld cap offset

static double PI = 4 * atan(1.0);
static double g_X[GAP_BUFFER_LEN], g_Y[GAP_BUFFER_LEN];
enum { SOURCE_RAW_GAP = 0, SOURCE_FILT_GAP, SOURCE_RAW_VEL };

#ifdef _DEBUG_TIMING_
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

CWeldNavigation::CWeldNavigation(CMotionControl& motion, CLaserControl& laser, const NAVIGATION_PID& pid)
	: m_motionControl(motion)
	, m_laserControl(laser)
	, m_pid(pid)
{
	m_bScanning = FALSE;
	m_bSteerMotors = FALSE;
	m_hThreadSteerMotors = NULL;
	m_hThreadNoteLaser = NULL;
	m_fMotorSpeed = FLT_MAX;
	m_fMotorAccel = FLT_MAX;
	m_nEndPos = 0;
	m_nStartPos = 0;
	m_fWeldOffset = 0;
	m_pParent = NULL;
	m_nMsg = 0;

	// used fgor the PID navigation
	m_p_error = 0;
	m_d_error = 0;
	m_i_error = 0;
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
/*
static double HammingWindow(double dist, double nWidth)
{
	double Wk = 0.54 - 0.46 * cos(PI * (nWidth-dist) / nWidth);
	return Wk;
}
*/

// using the coefficients and given order calculate 'y'
// y = A x^2 + B x + C
// y = m x + b
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
// the offset from the weld cap seems to have outliers
// important that do not navigate to them
// they must be removed before filtering the weld cap offset
// fit to a polynomial and then check for outliers from that poilynomial
// keep removing outliers until the SD is <= 0.1
static int RemoveOutliers(double X[], double Y[], int order, int count, int max_sd )
{
	double coeff[3];

	// if too few data data samples then not possible to say which are the outliers
	if (count > 4)
	{
		// get the average (order 0)
		polyfit(X, Y, count, 0, coeff);
		double avg = coeff[0];

		// look at the difference betwen index (i) and (i-1)
		for (int i = 1; i < count; ++i)
		{
			// if the difference is > 1 mm per mm
			// remvoed the entry furthest from the average
			double diff = fabs(Y[i - 1] - Y[i]) / fabs(X[i - 1] - X[i]);
			if (diff > MAX_GAP_CHANGE_PER_MM)
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

		// get the RMS variation from the above model
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

// the weld cap offsets are filtered by modelling the previous samples to a 2nd order polynomial and using that mnodel to say where the current value should be
static BOOL GetGapCoefficients(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction, double coeff[3], int order, int source, int nMinWidth, int nMaxWidth)
{
	// may need to minimize the width if not enough data
	int nSize = (int)buff1.GetSize();
	if (nSize < 2)
		return FALSE;

	// start with the last entered value
	// add values to the sum until too far away
	double pos2 = buff1[nSize - 1].measures.measure_pos_mm;
//	double val2 = (source == SOURCE_RAW_GAP) ? buff1[nSize - 1].gap_raw : (source == SOURCE_RAW_VEL ? buff1[nSize - 1].vel_raw  : buff1[nSize - 1].gap_filt);

	// start at the most recent and look back in time for a maximum of nMaxWidth mm
	int count = 0;
	for (int i = nSize-1; i >= 0 && count < GAP_BUFFER_LEN; --i)
	{
		double val1 = (source == SOURCE_RAW_GAP) ? buff1[i].gap_raw : (source == SOURCE_RAW_VEL ? buff1[i].vel_raw : buff1[i].gap_filt);
		if (val1 == FLT_MAX)
			continue;

		// where was thje crawler at this measure
		double pos1 = buff1[i].measures.measure_pos_mm;

		// if first order get no values prior to the last manoeuvre
		// note (direction) as may be travelling backwards
		if (order == 1)
		{
			if (last_manoeuvre_pos != FLT_MAX && direction*(pos1+ nMinWidth) < direction*last_manoeuvre_pos)
				break;
		}
		// if second order restrict to no furthjer previous to the manoeuvre than after it
		else if( order == 2 )
		{
			if ((last_manoeuvre_pos != FLT_MAX) && direction*(last_manoeuvre_pos - pos1) > direction*(pos2 - last_manoeuvre_pos) )
				break;
		}

		// add this value to a vector to be modelled
		g_X[count] = pos1;
		g_Y[count] = val1;
		count++;
	//	val2 = val1;

		// check after adding to insure that pass the check later on for minimum width
		// i.e. allow to have one value past the designated maximum width
		if (fabs(pos2 - pos1) > nMaxWidth)
			break;

	}


	// for 2nd order need at least three samples, for 1st order at least 2
	if (count < order + 1)
		return FALSE;

	// just get the average, dont worry if have minimnum width
	if (order == 0)
	{
		polyfit(g_X, g_Y, count, order, coeff);
		return TRUE;
	}


	// check if have minimum length of data (10 mm) to get a filtered value
	if (fabs(g_X[0] - g_X[count - 1]) < (double)nMinWidth)
		return FALSE;

	// outliers can corrupt the model
	count = RemoveOutliers(g_X, g_Y, order, count, 2/*max_sd*/);
	if (count < 1+order)
		return FALSE;

	// get the coefficients f the data set
	polyfit(g_X, g_Y, count, order, coeff);
	return TRUE;
}

// this is intended to filter only the last added value
// use the position offset for the qwindow, not the index
static FILTER_RESULTS LowPassFilterGap(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction)
{
	clock_t t1 = clock();
	double coeff[4];
	FILTER_RESULTS ret;

	// filter the last 50 mm
	// 2nd order, so can handle changes in the offset distance
	// using 1st order only, causes more delay in the filtering
	if (GetGapCoefficients(buff1, last_manoeuvre_pos, direction, coeff, 2/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MAX__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].measures.measure_pos_mm;
		ret.gap_filt = coeff[2] * pos2*pos2 + coeff[1] * pos2 + coeff[0];
		ret.vel_raw = 1000.0 * 2 * coeff[2] * pos2 + coeff[1]; // differentiated

		// predict the future
		double pos = pos2 + GAP_PREDICT*(double)direction;
		ret.gap_predict = coeff[2] * pos * pos + coeff[1] * pos + coeff[0];
		ret.pos_predict = pos;
	}

	// if can't get coefficitns, then try for an average value over the minimum width
	else if (GetGapCoefficients(buff1, last_manoeuvre_pos, direction, coeff, 1/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MIN__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].measures.measure_pos_mm;
		ret.gap_filt = coeff[1] * pos2 + coeff[0];
		ret.vel_raw = 1000.0 * coeff[1]; // differentiated

		// predict the future
		double pos = pos2 + GAP_PREDICT * (double)direction;
		ret.gap_predict = coeff[1] * pos + coeff[0];
		ret.pos_predict = pos;
	}

	// this just retuns the average, allow to be only 5 mm 
	// if can't get coefficitns, then try for an average value over the minimum width
	else if (GetGapCoefficients(buff1, last_manoeuvre_pos, direction, coeff, 0/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH/2, GAP_FILTER_MIN__WIDTH/2))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].measures.measure_pos_mm;

		ret.gap_filt = coeff[0];
		ret.vel_raw = FLT_MAX;
		ret.gap_predict = coeff[0];
		ret.pos_predict = pos2;
	}
	else
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].measures.measure_pos_mm;

		ret.gap_filt = FLT_MAX;
		ret.gap_predict = FLT_MAX;
		ret.pos_predict = pos2;
		ret.vel_raw = FLT_MAX;
	}


	// now get the slope over the last 5 to 10 mm
	// this is used for the (D) in the PID navigation
	// this is much mopre stable than using the difference in the previous 2 samples (which may not be eactly 1 mm apart)
	if (GetGapCoefficients(buff1, FLT_MAX, direction, coeff, 1/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH, 2 * GAP_FILTER_MIN__WIDTH))
		ret.slope10 = coeff[1]; // mm per mm
	else
		ret.slope10 = FLT_MAX;

#ifdef _DEBUG_TIMING_
	g_LowPassFilterGapTime += clock() - t1;
	g_LowPassFilterGapCount++;
#endif

	return ret;
}

// this is only used in _DEBUG
// it is an attempt to calculate the weld cap offset trend (velocvity to from weld)
#ifdef _DEBUG_TIMING_
static double CalcGapVelocity(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction)
{
	double coeff[3];
	// filter the last 50 mm
	// 1st order, only want the slope
	if (GetGapCoefficients(buff1, last_manoeuvre_pos, direction, coeff, 2/*ORDER*/, SOURCE_RAW_VEL/*source*/, VEL_FILTER_MIN_WIDTH, VEL_FILTER_MAX_WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].measures.measure_pos_mm;
		double vel = coeff[2] * pos2*pos2 + coeff[1] * pos2 + coeff[0]; // mm/M
		return vel;
	}
/*	else
		if (GetGapCoefficients(buff1, last_manoeuvre_pos, direction, coeff, 2, SOURCE_FILT_GAP, VEL_FILTER_MIN_WIDTH, VEL_FILTER_MAX_WIDTH))
		{
			int nSize = (int)buff1.GetSize();
			double pos2 = buff1[nSize - 1].measures.measure_pos_mm;
			double vel = 2 * coeff[2] * pos2 + coeff[1]; //  1000 * coeff[1]; // mm/M
			return 1000.0 * vel;
		}
		else
			if (GetGapCoefficients(buff1, last_manoeuvre_pos, direction, coeff, 1, SOURCE_FILT_GAP, VEL_FILTER_MIN_WIDTH, VEL_FILTER_MIN_WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].measures.measure_pos_mm;
		double vel = coeff[1]; //  1000 * coeff[1]; // mm/M
		return 1000.0 * vel;
	}
	else*/
		return FLT_MAX;
}
#endif

// get the last noted position
// this must be thread safe
// optionally look back in time by ago_mm
LASER_POS CWeldNavigation::GetLastNotedPosition(int ago_mm)
{
	LASER_POS ret;
	ret.measures.measure_pos_mm = FLT_MAX;

	m_crit1.Lock();
	int nSize = (int)m_listLaserPositions.GetSize();
	if( nSize > 0 )
	{
		if (ago_mm == 0)
			ret = m_listLaserPositions[nSize - 1];
		else
		{
			int direction = (m_nEndPos > m_nStartPos) ? 1 : -1;
			double pos2 = m_listLaserPositions[nSize - 1].measures.measure_pos_mm; // want the one that is at least ago_mm prior to this
			for (int i = nSize - 1; i >= 0; --i)
			{
				if (direction *(pos2 - m_listLaserPositions[i].measures.measure_pos_mm) > direction*ago_mm)
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

// must only use the main thread to stop the motors
BOOL CWeldNavigation::StopMotors(BOOL bWait)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
	{
		BOOL ret = (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_STOP_MOTORS, (LPARAM)bWait);
		return ret;
	}
	else
		return FALSE;
}
// not used 911
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

// just prior to stopping the motors, increase the deceleration rate
// want the motors to travel as short a distance as possible while stopping
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

// this is used to steer the crawler
// set the L/R wheels speeds faster/slower
// again a thread should not talk to the controller
BOOL CWeldNavigation::SetMotorSpeed(const double speed[4] )
{
	clock_t t1 = clock();
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
	{
		BOOL ret = (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SET_MOTOR_SPEED, (LPARAM)speed);
#ifdef _DEBUG_TIMING_
		g_SetMotorSpeedTime += clock() - t1;
		g_SetMotorSpeedCount++;
#endif
		return ret;
	}
	else
		return FALSE;
}

double CWeldNavigation::GetAvgMotorSpeed()
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
	{
		int speed = m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_GET_AVG_SPEED);
		return speed == INT_MAX ? FLT_MAX : speed / 1000.0;
	}
	else
		return FLT_MAX;
}

// used in _DERBUG to note the timing
void CWeldNavigation::SendDebugMessage(const CString& str)
{
#ifdef _DEBUG_TIMING_
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SEND_DEBUG_MSG, (LPARAM)&str);
#endif
}

// get laser measures VS motor positions as often as possibler
// ideally every mm
BOOL CWeldNavigation::NoteNextLaserPosition()
{
	clock_t t1 = clock();

	// get the last measured value
	// this is thread safe, different thread writing it as this one
	LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();

	// checkm if there is a weld cap height, if not, then the measures are not valid
	if (measure2.weld_cap_mm.x == FLT_MAX  )
		return FALSE;
	
	double motor_pos = measure2.measure_pos_mm; //  GetAvgMotorPosition();
	if (motor_pos == FLT_MAX)
		return FALSE;

	// mkust get this before locking the critical section to avoid grid lock
	double last_manoeuvre_pos = m_motionControl.GetLastManoeuvrePosition();

	// if the motor has not yet moved a full mm, then return
	// less than this and the velocity will not be stable
	// can only access m_listLaserPositions inside a critical section
	m_crit1.Lock();
	int nSize = (int)m_listLaserPositions.GetSize();

	// mujst have moved at least 1 mm from the last value
	if (nSize > 0 && fabs(motor_pos - m_listLaserPositions[nSize - 1].measures.measure_pos_mm) < 1 )
	{
		m_crit1.Unlock();
		return FALSE;
	}

	// this RAW value must be within 1 mm per 1 mm of travel
	// else consider it to be bogus
	if (nSize > 5)
	{
		// do not allow to fault more than twice in a row, 
		// this could potentially cause this to be hit every tiime if no limit
		static int err_count = 0;
		double gap_diff = fabs(measure2.weld_cap_mm.x - m_listLaserPositions[nSize - 1].gap_filt);
		double pos_diff = fabs(motor_pos - m_listLaserPositions[nSize - 1].measures.measure_pos_mm);
		if (gap_diff / pos_diff > MAX_GAP_CHANGE_PER_MM)
		{
			err_count++;
			if (err_count < 2)
			{
				m_crit1.Unlock();
				return FALSE;
			}
		}
		else
			err_count = 0;
	}

// counld be driving forwards or back
	int direction = (m_nEndPos > m_nStartPos) ? 1 : -1;

	// the grow by should be longer than any ;potential run
	// thus minimize reallocates
	m_listLaserPositions.SetSize(++nSize, NAVIGATION_GROW_BY);
	m_listLaserPositions[nSize - 1].measures.measure_pos_mm = motor_pos;
	m_listLaserPositions[nSize - 1].gap_raw = measure2.weld_cap_mm.x;

	// get a filtered value for the weld cap offset
	// alsdo get a predicted offset at 5 mm in the future
	// 911 will want to see if sterring latency make s this better
	// (i.e. drive to where you think you are going, not where you think you are)
	FILTER_RESULTS ret = ::LowPassFilterGap(m_listLaserPositions, last_manoeuvre_pos, direction);
	m_listLaserPositions[nSize - 1].gap_filt	= ret.gap_filt;
	m_listLaserPositions[nSize - 1].gap_predict = ret.gap_predict;
	m_listLaserPositions[nSize - 1].pos_predict = ret.pos_predict;
	m_listLaserPositions[nSize - 1].vel_raw		= ret.vel_raw;

	// if there is no filtered value for the offset, jusrt use the original raw value
	if (m_listLaserPositions[nSize - 1].gap_filt == FLT_MAX)
		m_listLaserPositions[nSize - 1].gap_filt = m_listLaserPositions[nSize - 1].gap_raw;

	m_listLaserPositions[nSize - 1].slope10		= ret.slope10; // the slope (mm/mm) of ther last 10 mm, used for (D) in PID navigation
	m_listLaserPositions[nSize - 1].time_noted	= clock(); // niote wshen this position was, used to calculate velocities
	m_listLaserPositions[nSize - 1].measures.measure_pos_mm	= motor_pos;
	m_listLaserPositions[nSize - 1].last_manoeuvre_pos		= last_manoeuvre_pos;
	m_listLaserPositions[nSize - 1].measures				= measure2;

	// calculate the desired steering at this point from the PID values
	m_listLaserPositions[nSize - 1].pid_steering			= GetPIDSteering();
	m_listLaserPositions[nSize - 1].manoeuvre1.x			= CalculateTurnRate(m_listLaserPositions[nSize - 1].pid_steering);

	// 911 these are included to go into the test_xx.txt file
	// this is useful to deterfmine optimal PID constants
	m_listLaserPositions[nSize - 1].pid_error[0]		= m_p_error;
	m_listLaserPositions[nSize - 1].pid_error[1]		= m_i_error;
	m_listLaserPositions[nSize - 1].pid_error[2]		= m_d_error;

#ifdef _DEBUG_TIMING_
	m_listLaserPositions[nSize - 1].vel_filt = ::CalcGapVelocity(m_listLaserPositions, last_manoeuvre_pos, direction);
#endif
	m_crit1.Unlock();


#ifdef _DEBUG_TIMING_
	g_NoteNextLaserPositionTime += clock() - t1;
	g_NoteNextLaserPositionCount++;
#endif
	return TRUE;
}

// the handle will be NULL before and after navigation
BOOL CWeldNavigation::IsNavigating()const
{
	return m_hThreadSteerMotors != NULL;
}

// used when writing File_XX.txt file to set the interval to 1 mm
// the measured position interval though intended to be 1 mm, will vary as not a real time operating system
static void InterpolateVector(const CArray<double,double>& X, const CArray<double,double>& src, CArray<double, double>& dest, int start_pos, int end_pos)
{
	static double X2[100], Y2[100];
	int len = abs(end_pos - start_pos) + 1;
	dest.SetSize(len);

	double coeff[3];
	int nSize = (int)X.GetSize();
	len = 0;

	// get amn interpolated value every mm from start to end
	for (int pos = start_pos; pos <= end_pos; ++pos)
	{
		// get the 1st and last value within 10 mm of (pos), these may be before start or afgter end
		int i1, i2;
		for (i1 = 0; i1 < nSize && X[i1] < pos-10L; ++i1);
		for (i2 = nSize - 1; i2 >= 0 && i2 > pos+10L; --i2); 

		// want at least 3 values so can use 2nd order model
		// first add earlier values, and if that is not enough add later ones
		if (i2 - i1 < 3 && i1 - 1 >= 0) i1--;
		if (i2 - i1 < 3 && i2 + 1 < nSize) i2++;

		// add the values to double vectors to use in the model
		int count = 0;
		for (int i = i1; i < i2 && count < 100; ++i)
		{
			X2[count] = X[i];
			Y2[count] = src[i];
			count++;
		}

		// use 2nd order if have at least 3 samples
		if (count >= 3)
		{
			polyfit(X2, Y2, count, 2, coeff);
			dest[len] = coeff[2] * pos * pos + coeff[1] * pos + coeff[0];
			len++;
		}

		// use first order if at least 2 samples
		else if (count >= 2)
		{
			polyfit(X2, Y2, count, 1, coeff);
			dest[len] = coeff[1] * pos + coeff[0];
			len++;
		}

		// use (0) order if only 1
		else if (count >= 1)
		{
			polyfit(X2, Y2, count, 0, coeff);
			dest[len] = coeff[0];
			len++;
		}
		else
		{
			dest[len] = 0;
			len++;
		}
	}
}

// open the next File_XX.txt or Test_XX.txt file to write to
FILE* CWeldNavigation::OpenNextFile(const char* szFile1)
{
	CString path;
	char my_documents[MAX_PATH];

	// get the decoment folder of the current user
	HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	if (result != S_OK)
		return NULL;

	// insure that the sub follder exists
	path.Format("%s\\SimplyAUTFiles", my_documents);
	DWORD attrib = GetFileAttributes(path);
	if (attrib == INVALID_FILE_ATTRIBUTES)
	{
		if (!::CreateDirectory(path, NULL))
			return NULL;
	}

	// get a list of the existing files
	// need to check which value for XX
	path.Format("%s\\SimplyAUTFiles\\*.txt", my_documents);

	int nMaxFile = 0;
	CFileFind find;
	if (find.FindFile(path))
	{
		int ret = 1;
		for (int i = 0; ret != 0; ++i)
		{
			ret = find.FindNextFileA();
			CString szFile2 = find.GetFileName();
			int ind = szFile2.Find(szFile1);
			if (ind != -1)
			{
				int nFile = atoi(szFile2.Mid(ind + strlen(szFile1)));
				nMaxFile = max(nMaxFile, nFile);
			}
		}
	}

	// format and open the file
	path.Format("%s\\SimplyAUTFiles\\%s%d.txt", my_documents, szFile1, nMaxFile + 1);
	FILE* fp1 = NULL;
	fopen_s(&fp1, path, "w");
	return fp1;
}

#ifdef _DEBUG_TIMING_
// this file is only writen in _DEBUG
// it has data to help with navigation development
BOOL CWeldNavigation::WriteTestFile()
{
	int nSize1 = (int)m_listLaserPositions.GetSize();
	if (nSize1 < 10 )
		return FALSE;

	FILE* fp1 = OpenNextFile("Test_");
	if (fp1 == NULL)
		return FALSE;

#define NUM_VECTORS 9
	fprintf(fp1, "Pos0\tTime\tLF\tRF\tRR\tLR\tGap 1\tGap 2\tPos+5\tGap+5\tVel 1\tVel 2\tMan1\tMan2\tPID\tP\tI\tD\n");

	clock_t time1 = m_listLaserPositions[0].time_noted;

	for (int i = 0; i < nSize1; ++i)
	{
		double pos = m_listLaserPositions[i].measures.measure_pos_mm;

		// locate the predicted gap values, so they line up in the plots
		double pos_predict;
		double gap_predict;
		// backup to the first prediction for this position
		int k;
		for (k = i; k >= 0; --k)
		{
			if (m_listLaserPositions[k].pos_predict != 0 && m_listLaserPositions[k].pos_predict < pos)
				break;
		}
		// (k) is now for the position greater than the target position
		if (k >= 0 && k + 1 < nSize1)
		{
			double pos1 = m_listLaserPositions[k + 1].pos_predict; // < pos
			double pos2 = m_listLaserPositions[k - 0].pos_predict; // >= pos
			double gap1 = m_listLaserPositions[k + 1].gap_predict;
			double gap2 = m_listLaserPositions[k - 0].gap_predict;

			pos_predict = m_listLaserPositions[k].pos_predict;
			gap_predict = gap1 + (gap2 - gap1) * (pos - pos1) / (pos2 - pos1);
		}
		else
		{
			pos_predict = 0;
			gap_predict = 0;
		}


		fprintf(fp1, "%.1f\t%d\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.0f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.6f\n",
			pos,	 // pos
			m_listLaserPositions[i].time_noted - time1, // time
			m_listLaserPositions[i].measures.wheel_velocity4[0], // LF
			m_listLaserPositions[i].measures.wheel_velocity4[1], // RF
			m_listLaserPositions[i].measures.wheel_velocity4[2], // RR
			m_listLaserPositions[i].measures.wheel_velocity4[3], // LR
			m_listLaserPositions[i].gap_raw,	// gap 1
			m_listLaserPositions[i].gap_filt,	// gap 2
			pos_predict, // gap + 1
			gap_predict, // gap + 1
			(m_listLaserPositions[i].vel_raw == FLT_MAX) ? 0 : m_listLaserPositions[i].vel_raw, // vel
			(m_listLaserPositions[i].vel_filt == FLT_MAX) ? 0 : m_listLaserPositions[i].vel_filt, // vel2
			m_listLaserPositions[i].manoeuvre1.x,
			m_listLaserPositions[i].manoeuvre2.x,
			m_listLaserPositions[i].pid_steering,
			m_listLaserPositions[i].pid_error[0],
			m_listLaserPositions[i].pid_error[1],
			m_listLaserPositions[i].pid_error[2]
			);

//			m_listLaserPositions[pos].measures.rgb_sum,
//			(m_listLaserPositions[pos].vel_raw == FLT_MAX) ? 0 : m_listLaserPositions[pos].vel_raw,
//			(m_listLaserPositions[pos].vel_filt == FLT_MAX) ? 0 : m_listLaserPositions[pos].vel_filt,
//			(m_listLaserPositions[pos].last_manoeuvre_pos == FLT_MAX) ? 0 : m_listLaserPositions[pos].last_manoeuvre_pos);
	}
	fclose(fp1);

	return TRUE;
}
#endif

// this is a file with the scan output
// ideally it would be shared with UT recording
// but for now is jhust written tto a afile
BOOL CWeldNavigation::WriteScanFile()
{
	int nSize = (int)m_listLaserPositions.GetSize();
	if (nSize < 10)
		return FALSE;

	if (!m_bScanning)
		return FALSE;

	FILE* fp = OpenNextFile("File_");
	if (fp == NULL)
		return FALSE;

	// pos, cap H, cap W, L/R diff, weld offset
	fprintf(fp, "Pos\t  Cap H\t  Cap W\t  Hi/Lo\t Offset");
	for (int i = 0; i < 21; ++i)
		fprintf(fp, "\t   %d", i - 10);
	fprintf(fp, "\n");

	CArray<double, double> X, Y1[2], Y2[2], Y3[2], Y4[2], Y5[21][2];
	X.SetSize(nSize);
	Y1[0].SetSize(nSize);
	Y2[0].SetSize(nSize);
	Y3[0].SetSize(nSize);
	Y4[0].SetSize(nSize);
	for (int j = 0; j < 21; ++j)
		Y5[j][0].SetSize(nSize);

	for (int i = 0; i < nSize; ++i)
	{
		const LASER_MEASURES& measure2 = m_listLaserPositions[i].measures;
		double avg_side_height = (measure2.weld_left_height_mm + measure2.weld_right_height_mm) / 2;
		double weld_cap_height = measure2.weld_cap_mm.y;

		// write all the values at whatrever interval now exzist into vectors
		X[i] = m_listLaserPositions[i].measures.measure_pos_mm;
		Y1[0][i] = avg_side_height - weld_cap_height;
		Y2[0][i] = fabs(measure2.weld_right_mm - measure2.weld_left_mm);
		Y3[0][i] = fabs(measure2.weld_left_height_mm - measure2.weld_right_height_mm);
		Y4[0][i] = m_listLaserPositions[i].gap_filt;

		for (int j = 0; j < 21; ++j)
			Y5[j][0][i] = measure2.cap_profile_mm[j];
	}

	// interpolate all to 1 mm intervals
	InterpolateVector(X, Y1[0], Y1[1], m_nStartPos, m_nEndPos);
	InterpolateVector(X, Y2[0], Y2[1], m_nStartPos, m_nEndPos);
	InterpolateVector(X, Y3[0], Y3[1], m_nStartPos, m_nEndPos);
	InterpolateVector(X, Y4[0], Y4[1], m_nStartPos, m_nEndPos);
	for (int j = 0; j < 21; ++j)
		InterpolateVector(X, Y5[j][0], Y5[j][1], m_nStartPos, m_nEndPos);

	// write to the file
	for(int i=0; i < Y1[1].GetSize(); ++i)
	{
		fprintf(fp, "%d\t%7.3f\t%7.3f\t%7.3f\t%7.3f",
			i, Y1[1][i], Y2[1][i], Y3[1][i], Y4[1][i]);
		for (int j = 0; j < 21; ++j)
			fprintf(fp, "\t%7.3f", Y5[j][1][i]);
		fprintf(fp, "\n");
	}

	fclose(fp);
	return TRUE;
}

// start the navigation
// nSteer = 0 (stop navigation)
// 0x1 start a thread to note the weld cap offset history
// 0x2 start the navigation and vary L/R wheel speeds
// start_mm, end_mm: note where to navigate from and to, stop mthe motors once reach end_mm
// motor_speed: the default speed for allm motors, weill vary tyop steer
// motor_accel: default acceleration in the control, will double prior to stopping
// offset: may want to navigate to a samall offset from tyhe centre of the weld cap
// bScanning: used to dertmine if to write a scan file at the end of navigation
void CWeldNavigation::StartNavigation(int nSteer, int start_mm, int end_mm, double motor_speed, double motor_accel, double offset, BOOL bScanning)
{
	// if any threads active, stop them now
	StopSteeringMotors();

	if ( nSteer)
	{
		m_fMotorSpeed = motor_speed;
		m_fMotorAccel = motor_accel;
		m_fWeldOffset = offset;
		m_nStartPos = start_mm;
		m_nEndPos = end_mm;
		m_p_error = 0;
		m_d_error = 0;
		m_i_error = 0;

		m_bScanning = bScanning;
		m_bSteerMotors = TRUE;
		m_crit1.Lock();

		// also reset the remembered last positioon
		// it may be left over from the previous run
		m_listLaserPositions.SetSize(0);
		LASER_MEASURES measure2 = m_laserControl.GetLaserMeasures2();
		measure2.measure_pos_mm = FLT_MAX;
		m_laserControl.SetLaserMeasures2(measure2);

		m_crit1.Unlock();
#ifdef _DEBUG_TIMING_
		g_NoteNextLaserPositionTime = 0;
		g_NoteNextLaserPositionCount = 0;

		g_SetMotorSpeedTime = 0;
		g_SetMotorSpeedCount = 0;

		g_LowPassFilterGapTime = 0;
		g_LowPassFilterGapCount = 0;
#endif

		// strart threads to manage these function
		if( nSteer & 0x1 )
			m_hThreadNoteLaser = AfxBeginThread(::ThreadNoteLaser, this);
		if (nSteer & 0x2)
			m_hThreadSteerMotors = AfxBeginThread(::ThreadSteerMotors, this);
	}
	else
	{
		m_crit1.Lock();
#ifdef _DEBUG_TIMING_
		WriteTestFile();
#endif
		WriteScanFile();

		m_listLaserPositions.SetSize(0);
		m_crit1.Unlock();
#ifdef _DEBUG_TIMING_
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

// request any existing theasds to stop and then wait for them to stop
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

// try to get a new position every 1 mm
UINT CWeldNavigation::ThreadNoteLaser()
{
	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL); // 911
	while (m_bSteerMotors)
	{
		Sleep(1); // avoid tight loop
		// will keep lookinjmg until travel at least 1 mm
		NoteNextLaserPosition();
	}
	return 0;
}

// m_listLaserPositions is not thread safe, so may want a copy
// not used at this time
void CWeldNavigation::GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>& list)
{
	m_crit1.Lock();
	list.Copy(m_listLaserPositions);
	m_crit1.Unlock();
}

// with the given weldf cap offset (gap) what variation is desired between L and R
// this is used by the non PID navigation
static CDoublePoint GetTurnRateAndPivotPoint(double gap, BOOL bInTolerance, int direction)
{
	CDoublePoint ret(1.0, 0.5);
	gap = fabs(gap);

	// less than 0.1 mm just use the minimum variation
	if (gap < MIN_GAP_TOLERANCE)
	{
		ret.x = MIN_TURN_RATE;
		ret.y = 0.5;
	}

	// initially move the pivot point back to increase the turn rate
	// 
	else if (direction == 1 && !bInTolerance && gap > 2* MAX_GAP_TOLERANCE) // > 1.0
	{
		double rate = (gap - 2 * MAX_GAP_TOLERANCE) * (MAX_TURN_RATE - MIN_TURN_RATE) / (10 * MAX_GAP_TOLERANCE - 2 * MAX_GAP_TOLERANCE) + MIN_TURN_RATE;
		ret.x = rate; //  0.95; //  MIN_TURN_RATE;
		ret.y = 2.0 / 5.0;
	}

	// if greater than 1.0 mm, set to the maximum turn rate
	else if (gap > MAX_GAP_TOLERANCE)
	{
		ret.x = (direction == 1) ? MAX_TURN_RATE : 0.95;
		ret.y = 0.5;
	}
	// assume that alwsys within 0.1 -> 1.0 mm
	else
	{
		double rate = (gap - MIN_GAP_TOLERANCE) * (MAX_TURN_RATE - MIN_TURN_RATE) / (2* MAX_GAP_TOLERANCE - MIN_GAP_TOLERANCE) + MIN_TURN_RATE;
		rate = max(rate, MAX_TURN_RATE); // 0.85
		rate = min(rate, MIN_TURN_RATE); // 0.9

		ret.x = fabs(rate);
		ret.y = 0.5;
	}

	return ret;
}

// the PID navigation seems to work forward but not well in reverse at this time
UINT CWeldNavigation::ThreadSteerMotors()
{
	int direction = (m_nEndPos > m_nStartPos) ? 1 : -1;
	if(  direction == -1 )
		ThreadSteerMotors_try3();
	else
		ThreadSteerMotors_PID();

	return 0;
}

// this is used by the PID navigtation
// steering: limit to +/- 1
// +1 mhard turn left, -1 hard turn right
double CWeldNavigation::CalculateTurnRate(double steering)const
{
	steering = max(steering, -1);	// 1=turn hard (L/R MAX_TURN_RATE)
	steering = min(steering, 1);	// 0 = no turn (L/R 100%)
	int dir = (steering > 0) ? 1 : -1;

	// limit the turn rate to 0.7
	// too much as the slippage required will be extreme
	// too little and will not rturn fast enough
	double rate = MAX_TURN_RATE + (1 - fabs(steering)) * (1 - MAX_TURN_RATE);
	return dir*rate;
}

// P: note the current weld cap offset
// I: sum of the weld cap offset values
//    (could also be a sum of the difference of (now - 25 mm ago) )
// D: weld cap tendancy to/from weld cap centre in mm per mm
double CWeldNavigation::GetPIDSteering()
{
	// at 2 mm offset, want hardest steering of 0.8
	const double Kp = m_pid.P;			//  (0.2) 


	// this is the accummulated error, keep turning harding if no correction seen
	// settiong this large, causes the correction to run longer
	const double Ki = m_pid.I;       // (0.00003)		

	// slope mm/mm over the last 10 mm, the larger this is, the sooner start ending the correction
	// i.e. is response lags, this needs to be large
	const double Kd = m_pid.D;		//  1.6;			

	const int I_ERR_LEN = 25;

	// check if near the end, if so end the navigation
	LASER_POS pos0 = GetLastNotedPosition(0);
	LASER_POS pos25 = GetLastNotedPosition(I_ERR_LEN);
	
	// still need to determine if better using a predicted value about 5 mm in the furture
	// this would potentially correct some of the latency in the steering
	// i.e. takeds time to start a turn, must look further doewn the road
	// 911
	double this_pos = pos0.measures.measure_pos_mm;
	double this_gap = pos0.gap_filt;
//	double this_gap = pos0.gap_predict;
	if (this_pos == FLT_MAX || this_gap == FLT_MAX)
		return 0;

	// update the error sums
	m_d_error = (pos0.slope10 == FLT_MAX) ? 0 : pos0.slope10; //  (pos0.vel_raw == FLT_MAX) ? this_gap - m_p_error : pos0.vel_raw / 1000.0;
	m_p_error = this_gap;

	//if (Ki != 0 && pos25.gap_filt != FLT_MAX)
	//	m_i_error += (this_gap - pos25.gap_filt);
	//else
		m_i_error += this_gap;

	// calculate the steering value
	// the constants are fixed if not _DEBUG
	double steer = -(Kp * m_p_error + Kd * m_d_error + Ki * m_i_error);
	return steer;
}

// the original non PID steering technique
// 1. note how fast need to steer towards the centre
// 2. when half eay there if forward, stop turning and continue towards the centre
//    if in reverse stop turning sooner as the laser in the the back of the unit
// 3. drive straight towards the line without any further turning for maximum of 50 mm, or until cross the centre
// 4. repeat
UINT CWeldNavigation::ThreadSteerMotors_try3()
{
	const int MAX_TRAVEL_TIME = 1000;
	clock_t tim1 = clock();
	CString str;

	int steer = 0;
	int  time_first = 0;

	// note if in tolerance
	// note if driving backwards
	BOOL bInTolerance = FALSE;
	int direction = (m_nEndPos > m_nStartPos) ? 1 : -1;

	// if the gap is to one side or the other by more than about 0.1 mm
	// jog the crawler to the centre, and then check again\
	// a jog is a turn first towartds the centrte, then back to the original direction
	CDoublePoint last_rate(0, 0);
	while (m_bSteerMotors)
	{
		// avoid risk of a tight loop
		Sleep(1);

		// check if near the end, if so end the navigation
		// this is a pre-distance to get close before actual recording starts
		LASER_POS pos0 = GetLastNotedPosition(0);										// where is the crawler now
		if (pos0.measures.measure_pos_mm == FLT_MAX)
			continue;

		// have programmed the motors to travel further than desired
		// thus issue a stop in this case
		// that way nall the mnotors will stop at the sazme exact time
		if (direction*pos0.measures.measure_pos_mm >= direction*m_nEndPos)
		{
			SetMotorDeceleration(2 * m_fMotorAccel);
			StopMotors(TRUE);
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

		// note if in tolererance
		// once in tolerance do not allow the pivot point to move
		if (fabs(pos0.gap_filt) <= MAX_GAP_TOLERANCE )
			bInTolerance = TRUE;

		// if well out of tolerance, mopve the pivot point back
		// this will cause the front to turn faster
		// 911, may want to check if works better with predicted gap at +5 mm
		CDoublePoint turn_rate1 = GetTurnRateAndPivotPoint(pos0.gap_filt, bInTolerance, direction);
//		CDoublePoint turn_rate1 = GetTurnRateAndPivotPoint(pos0.gap_predict[0], bInTolerance, direction);

		double r = CRAWLER_WIDTH / 2.0; // front wheel to laser
		double l1 = turn_rate1.y * CRAWLER_LENGTH; // length from rear
		double l2 = (1- turn_rate1.y) * CRAWLER_LENGTH; // length from front
		double h1 = sqrt(pow(l1, 2.0) + pow(r, 2.0)); // rear wheel to laser
		double h2 = sqrt(pow(l2, 2.0) + pow(r, 2.0)); // rear wheel to laser

		double Vrear = m_fMotorSpeed * turn_rate1.x * h1 / h2; //  turn_rate1.x; // rear wheels at 10% of the fwd velocity
		double Vfwd = m_fMotorSpeed * turn_rate1.x * h2 / h1;  // veocity of fwd wheels for fwd to travel faster

		// the closer to the weld, the slower the required turn rate
		double speedLeftFwd1 = m_fMotorSpeed - dir * (1 - turn_rate1.x) * Vfwd; // if was to the left (-1), make left wheels faster
		double speedRightFwd1 = m_fMotorSpeed + dir * (1 - turn_rate1.x) * Vfwd;
		double speedRightBack1 = m_fMotorSpeed + dir * (1 - turn_rate1.x) * Vrear;
		double speedLeftBack1 = m_fMotorSpeed - dir * (1 - turn_rate1.x) * Vrear;
		double speed1[] = { speedLeftFwd1, speedRightFwd1, speedRightBack1, speedLeftBack1 }; // LF, RF, RR, LR

		if( turn_rate1 != last_rate )
			SetMotorSpeed(speed1);

		last_rate = turn_rate1;

		// note this manoeuvre in the list file
		// do this before setting th4e wheel speeds, so can release creitical section as soon as possibkle
		m_crit1.Lock();
		int nSize1 = (int)m_listLaserPositions.GetSize();
		if (nSize1)
			m_listLaserPositions[nSize1 - 1].manoeuvre2.x = dir*turn_rate1.x;
		m_crit1.Unlock();

		// keep turning until the gap is halved
		// it will take time to strighten out
		clock_t t1 = clock();

		// move to tyhe 1/2 way point, and then straghten out
		// the crawler will rebound and not just straigh
		// in reverse, assume that the forward wheels are closer than the laser
		double gap_tol1 = 2 * abs(pos0.gap_filt) / 4;
		double gap_tol2 = 3 * abs(pos0.gap_filt) / 4;
		double gap_tol = (direction == 1) ? gap_tol1 : gap_tol2;

		BOOL bContinue = FALSE;
		while (m_bSteerMotors)
		{
			// avoid a tight loop
			Sleep(10);
			LASER_POS last_pos = GetLastNotedPosition(0);// whjere is the crawler now

			// stop if change side of weld that on 
			// incase below is missed
			if ((pos0.gap_filt < 0) != (last_pos.gap_filt < 0))
				break;

			// stop mwhen have halfed the gap
			if (fabs(last_pos.gap_filt) < gap_tol)
				break;

			// stop if the gap is increasing
			if (fabs(last_pos.gap_filt) > fabs(pos0.gap_filt)+0.1)
			{
				bContinue = TRUE;
				break;
			}
		}
		if (bContinue)
			continue;

		// how long did it take to perform this correction
		// used in manoeuvre file
		clock_t turn_time1 = clock() - t1;
		LASER_POS pos1 = GetLastNotedPosition(0);				// where is the crawler now

		// note that strigtening out
		m_crit1.Lock();
		nSize1 = (int)m_listLaserPositions.GetSize();
		if (nSize1)
			m_listLaserPositions[nSize1 - 1].manoeuvre2.x = 1;
		m_crit1.Unlock();

		// now turn to travel straight
		// note, this does not mean stright along the weld, but means maintain the current direction (only halved the distance to weld)
		// note, the crawler does not appear to maintain the current direction, but changes slightly 
		double speed2[] = { m_fMotorSpeed, m_fMotorSpeed, m_fMotorSpeed, m_fMotorSpeed };
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

			if (fabs(pos0.measures.measure_pos_mm - last_pos.measures.measure_pos_mm) > DIVE_STRAIGHT_DIST)
				break;

			// no longer on the same side of the weld
			if ((pos0.gap_filt < 0) != (last_pos.gap_filt < 0))
				break;

			// stop if the gap starts increasing
			if (fabs(last_pos.gap_filt) > fabs(pos0.gap_filt))
				break;
		}

		clock_t turn_time3 = clock() - t1;
		LASER_POS pos3 = GetLastNotedPosition(0);				// where is the crawler now
	}
	return 0;
}

// navigate using PID
// 1. note the PID steering calculated in the list of weld cap offsets
// 2. use manoeuvre.x as that is the L/R speed ratio calculated from the PID steering value
// 3. drive for 50 ms (at 50 mm/sec this would be 2.5 mm)
// 4. repeat
UINT CWeldNavigation::ThreadSteerMotors_PID()
{
	// note if driving backwards
	int direction = (m_nEndPos > m_nStartPos) ? 1 : -1;
	double init_pos = GetLastNotedPosition(0).measures.measure_pos_mm;	
	double accel_time = m_fMotorSpeed / m_fMotorAccel;
	double accel_dist = accel_time * m_fMotorSpeed / 2.0;

	// if the gap is to one side or the other by more than about 0.1 mm
	// jog the crawler to the centre, and then check again\
	// a jog is a turn first towartds the centrte, then back to the original direction
	double last_pos = FLT_MAX;
	BOOL bStop = FALSE;
	BOOL bStart = init_pos != FLT_MAX;

	while (m_bSteerMotors)
	{
		// avoid risk of a tight loop
		Sleep(1);

		// check if near the end, if so end the navigation
		// this is a pre-distance to get close before actual recording starts
		LASER_POS pos0 = GetLastNotedPosition(0);										// where is the crawler now
		double this_pos = pos0.measures.measure_pos_mm;
		if( this_pos == FLT_MAX )
			continue;

		if (bStart && fabs(this_pos - init_pos) > accel_dist)
			bStart = FALSE;

		// have programmed the motors to travel further than desired
		// thus issue a stop in this case
		// that way nall the mnotors will stop at the sazme exact time
		if (direction * this_pos >= ((double)direction * (double)m_nEndPos))
		{
			// once the mootors are asked to stop, use the actual average mfotor speed as the referene
			// not m_fMotorSpeed
			SetMotorDeceleration(2 * m_fMotorAccel);
			StopMotors(TRUE);
			bStop = TRUE;
			break; // stop navigating now
		}

		// has it travelled at least a mm from the last check
		if (last_pos != FLT_MAX && fabs(this_pos - last_pos) < 1)
			continue;

		// until deceleration, must use the default speed, else it will drift
		// during decelerqation, use the average as the defrault
//		double fMotorSpeed = (bStop || bStart) ? GetAvgMotorSpeed() : m_fMotorSpeed;
		double fMotorSpeed = (bStop ) ? GetAvgMotorSpeed() : m_fMotorSpeed;
		last_pos = this_pos;
		double turn_rate = pos0.manoeuvre1.x;
		int dir = (turn_rate > 0) ? 1 : -1;

		double r = CRAWLER_WIDTH / 2.0; // front wheel to laser
		double l1 = m_pid.pivot * CRAWLER_LENGTH; // length from rear
		double l2 = (1 - m_pid.pivot) * CRAWLER_LENGTH; // length from front
		double h1 = sqrt(pow(l1, 2.0) + pow(r, 2.0)); // rear wheel to laser
		double h2 = sqrt(pow(l2, 2.0) + pow(r, 2.0)); // rear wheel to laser

		double Vrear = fMotorSpeed * h1 / h2; //  turn_rate1.x; // rear wheels at 10% of the fwd velocity
		double Vfwd = fMotorSpeed * h2 / h1;  // veocity of fwd wheels for fwd to travel faster

		// the closer to the weld, the slower the required turn rate
		double diff1 = Vfwd * (1 - fabs(turn_rate)) / 2.0;
		double speedLeftFwd1 = fMotorSpeed - dir * diff1; // if was to the left (-1), make left wheels faster
		double speedRightFwd1 = fMotorSpeed + dir * diff1;

		double diff2 = Vrear * (1 - fabs(turn_rate)) / 2.0;
		double speedRightBack1 = fMotorSpeed + dir * diff2;
		double speedLeftBack1 = fMotorSpeed - dir * diff2;
		double speed1[] = { speedLeftFwd1, speedRightFwd1, speedRightBack1, speedLeftBack1 }; // LF, RF, RR, LR


		double diff = fMotorSpeed * (1 - fabs(turn_rate)) / 2.0;

		// the closer to the weld, the slower the required turn rate
		double speedLeft = fMotorSpeed - dir*diff; // if was to the left (-1), make left wheels faster
		double speedRight = fMotorSpeed + dir*diff;
//		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft }; // LF, RF, RR, LR

		SetMotorSpeed(speed1);
		Sleep(m_pid.turn_time);
	}
	return 0;
}
#ifdef _DEBUG_TIMING_
UINT CWeldNavigation::ThreadSteerMotors_Test()
{
	// note if driving backwards
	int direction = (m_nEndPos > m_nStartPos) ? 1 : -1;

	// if the gap is to one side or the other by more than about 0.1 mm
	// jog the crawler to the centre, and then check again\
	// a jog is a turn first towartds the centrte, then back to the original direction
	double last_pos = FLT_MAX;
	while (m_bSteerMotors)
	{
		Sleep(1);

		LASER_POS pos0 = GetLastNotedPosition(0);										// where is the crawler now
		double this_pos = pos0.measures.measure_pos_mm;
		if (this_pos == FLT_MAX || this_pos < 10)
			continue;

		DriveTest(1.0, 0.5, 4000);
		DriveTest(0.95, 0.5, 4000);
		DriveTest(1.0, 0.5, 4000);
		DriveTest(-0.95, 0.5, 4000);
		DriveTest(1.0, 0.5, 4000);
		break;
	}

	// end
	SetMotorDeceleration(2 * m_fMotorAccel);
	StopMotors(TRUE);
	return 0;
}

BOOL CWeldNavigation::DriveTest(double rate, double pivot, int delay)
{
	if (!m_bSteerMotors)
		return FALSE;

	int dir = (rate > 0) ? 1 : -1;
	rate = fabs(rate);

	double r = CRAWLER_WIDTH / 2.0; // front wheel to laser
	double l1 = pivot * CRAWLER_LENGTH; // length from rear
	double l2 = (1 - pivot) * CRAWLER_LENGTH; // length from front
	double h1 = sqrt(pow(l1, 2.0) + pow(r, 2.0)); // rear wheel to laser
	double h2 = sqrt(pow(l2, 2.0) + pow(r, 2.0)); // rear wheel to laser

	double Vrear = m_fMotorSpeed * rate * h1 / h2; //  turn_rate1.x; // rear wheels at 10% of the fwd velocity
	double Vfwd = m_fMotorSpeed * rate * h2 / h1;  // veocity of fwd wheels for fwd to travel faster

	// the closer to the weld, the slower the required turn rate
	double speedLeftFwd1 = m_fMotorSpeed - dir * (1 - rate) * Vfwd; // if was to the left (-1), make left wheels faster
	double speedRightFwd1 = m_fMotorSpeed + dir * (1 - rate) * Vfwd;
	double speedRightBack1 = m_fMotorSpeed + dir * (1 - rate) * Vrear;
	double speedLeftBack1 = m_fMotorSpeed - dir * (1 - rate) * Vrear;
	double speed1[] = { speedLeftFwd1, speedRightFwd1, speedRightBack1, speedLeftBack1 }; // LF, RF, RR, LR

	m_crit1.Lock();
	int nSize = (int)m_listLaserPositions.GetSize();
	if (nSize)
		m_listLaserPositions[nSize - 1].manoeuvre2.x = rate;
	m_crit1.Unlock();
	SetMotorSpeed(speed1);

	for (int i = 0; i < 200 && m_bSteerMotors; ++i)
		Sleep(10);
	return m_bSteerMotors;
}
#endif
