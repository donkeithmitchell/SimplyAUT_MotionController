#include "pch.h"
#include "WeldNavigation.h"
#include "LaserControl.h"
#include "MotionControl.h"
#include "MagController.h"
#include "DialogGirthWeld.h"
#include "Misc.h"

static double PI = 4 * atan(1.0);
enum { SOURCE_RAW_GAP = 0, SOURCE_FILT_GAP, SOURCE_RAW_VEL };

#define MIN_TRAVEL_DIST				20 // minimum trav el dist of 20 mm befiore navigation kicks in
#define NAVIGATION_GROW_BY			2048 // at 1 per mm, this would be 2 metres
#define GAP_FILTER_MIN__WIDTH		10 // mm (filter over last 50 mm, 2nd order, so can handle variations)
#define GAP_FILTER_MAX__WIDTH		100 // mm (filter over last 50 mm, 2nd order, so can handle variations)
#define VEL_FILTER_MIN_WIDTH		5
#define VEL_FILTER_MAX_WIDTH		10
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

CWeldNavigation::CWeldNavigation()
{
	m_bSteerMotors = FALSE;
	m_hThreadSteerMotors = NULL;
	m_hThreadNoteLaser = NULL;
	m_fMotorSpeed = FLT_MAX;
	m_pParent = NULL;
	m_nMsg = 0;
	m_last_pos.Reset();
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

static int RemoveOutliers(double* X, double* Y, int count)
{
	if (count < 2)
		return count;

	// get the average value
	double sum = 0;
	for (int i = 0; i < count; ++i)
		sum += Y[i];

	double avg = sum / count;
	double sum2 = 0;
	for (int i = 0; i < count; ++i)
		sum2 += (Y[i] - avg) * (Y[i] - avg);

	double sd = sqrt(sum2 / count);

	for (int i = 0; i < count; ++i)
	{
		double diff = fabs(avg - Y[i]);
		if (diff > 5 * sd)
		{
			memcpy(X + i, X + i + 1, (count - i - 1) * sizeof(double));
			memcpy(Y + i, Y + i + 1, (count - i - 1) * sizeof(double));
			count--;
			i--;
		}
	}

	return count;
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

static double g_X[GAP_BUFFER_LEN], g_Y[GAP_BUFFER_LEN];
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

		// never use the first 10 mm of data, as it does not appear to be stable
		if (pos1 < 10)
			break;

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
		if (pos2 - pos1 > nMaxWidth)
			break;

	}

	if (count == 0)
		return FALSE;

	// remove any raw value changes of more than 1mm
	// this can not happen within 2 mm or so of travel
	if (source == SOURCE_RAW_GAP && count > 2)
	{
		// gert the average of the Y values
		polyfit(g_X, g_Y, count, 0/*order*/, coeff);
		double avg = coeff[0];

		// get the sd of the values
		double sum2 = 0;
		for (int i = 0; i < count; ++i)
			sum2 += pow(g_Y[i] - avg, 2.0);
		double sd = sqrt(sum2 / count);
		for (int i = 0; i < count; ++i)
		{
			double diff = fabs(g_Y[i] - avg);
			if (diff > 2 * sd)
			{
				memcpy(g_X + i, g_X + i + 1, (count - i - 1) * sizeof(double));
				memcpy(g_Y + i, g_Y + i + 1, (count - i - 1) * sizeof(double));
				count--;
				i--;
			}
		}
	}


	// just get the average, dont worry if have minimnum width
	if (order == 0)
	{
		polyfit(g_X, g_Y, count, order, coeff);
		return TRUE;
	}

	if( count < 2)
		return FALSE;

	if (g_X[0] - g_X[count - 1] < nMinWidth)
		return FALSE;

	// now get the slope of this line, that is the velocutty
	polyfit(g_X, g_Y, count, order, coeff);

	// don't remove outliers if just getting the average
	if (order == 0)
		return TRUE;

	// get the RMS variation and remove any values which exceed 2 SD
	double sum2 = 0;
	for (int i = 0; i < count; ++i)
	{
		double y = ModelY(g_X[i], coeff, order);
		double diff = y - g_Y[i];
		sum2 += diff * diff;
	}
	double sd = sqrt(sum2 / count);

	// remove any variation more than 2*sd
	int count2 = count;
	if( sd > 0.1 )
	for (int i = 0; i < count2; ++i)
	{
		double y = ModelY(g_X[i], coeff, order);
		double diff = fabs(y - g_Y[i]);
		if (diff > 4 * sd)
		{
			memcpy(g_X + i, g_X + i + 1, (count - i - 1) * sizeof(double));
			memcpy(g_Y + i, g_Y + i + 1, (count - i - 1) * sizeof(double));
			count2--;
			i--;
		}
	}

	if (count2 < 2)
		return FALSE;

	// if have removed values,a then redo the poly fit
	if (count2 < count)
		polyfit(g_X, g_Y, count2, order, coeff);

	return TRUE;
}

// this is intended to filter only the last added value
// set the hammin window to the length available up to nWidth
// use the position offset for the qwindow, not the index
static CDoublePoint LowPassFilterGap(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos)
{
	double coeff[4];
	// filter the last 100 mm
	// 2nd order, so can handle changes in the gap distance
	if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 2/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MAX__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double gap = coeff[2] * pos2*pos2 + coeff[1] * pos2 + coeff[0];
		double vel = 2 * coeff[2] * pos2 + coeff[1]; // differentiated
		return CDoublePoint(gap,1000*vel);
	}

	// if can't get coefficitns, then try for an average value over the minimum width
	else if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 1/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MIN__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double gap = coeff[1] * pos2 + coeff[0];
		double vel = coeff[2]; // differentiated
		return CDoublePoint(gap, 1000 * vel);
	}

	// this just retuns the average, allow to be only 5 mm 
	// if can't get coefficitns, then try for an average value over the minimum width
	else if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 0/*order*/, SOURCE_RAW_GAP/*source*/, GAP_FILTER_MIN__WIDTH/2, GAP_FILTER_MIN__WIDTH/2))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double gap = coeff[0];
		return CDoublePoint(gap, FLT_MAX);
	}
	else
		return CDoublePoint(FLT_MAX, FLT_MAX);
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
	if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 2/*ORDER*/, SOURCE_FILT_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MAX__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double vel = 2*coeff[2] * pos2 + coeff[1]; //  1000 * coeff[1]; // mm/M
		return 1000.0 * vel;
	}
	else if (GetGapCoefficients(buff1, last_manoeuvre_pos, coeff, 1/*ORDER*/, SOURCE_FILT_GAP/*source*/, GAP_FILTER_MIN__WIDTH, GAP_FILTER_MIN__WIDTH))
	{
		int nSize = (int)buff1.GetSize();
		double pos2 = buff1[nSize - 1].pos;
		double vel = coeff[1]; //  1000 * coeff[1]; // mm/M
		return 1000.0 * vel;
	}
	else
		return FLT_MAX;
}


LASER_POS CWeldNavigation::GetLastNotedPosition()
{
	m_crit1.Lock();
	LASER_POS ret = m_last_pos;
	m_crit1.Unlock();
	return ret;
}

BOOL CWeldNavigation::GetLaserMeasurment(LASER_MEASURES& measure)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		return (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_GET_MEASURE, (LPARAM)&measure);
	else
		return FALSE;
}

BOOL CWeldNavigation::GetMotorSpeed(double speed[4])
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		return (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_GET_MOTOR_SPEED, (LPARAM)speed);
	else
		return  FALSE;
}

BOOL CWeldNavigation::SetMotorSpeed(const double speed[4] )
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		return (BOOL)m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SET_MOTOR_SPEED, (LPARAM)speed);
	else
		return FALSE;
}

double CWeldNavigation::GetAvgMotorPosition()
{
	double ret = FLT_MAX;
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_GET_MOTOR_POS, (LPARAM)&ret);

	return  ret;
}


double CWeldNavigation::GetLastManoeuvrePosition()
{
	double ret = FLT_MAX;
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_LAST_MAN_POSITION, (LPARAM)&ret);

	return  ret;
}

void CWeldNavigation::SendDebugMessage(const CString& str)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SEND_DEBUG_MSG, (LPARAM)&str);
}

// get these every mm
BOOL CWeldNavigation::NoteNextLaserPosition()
{
	LASER_MEASURES measure;
	double gap1 = FLT_MAX;
	if (GetLaserMeasurment(measure) )
	{
		// m_measure will be updatre regularily 
		double motor_pos = GetAvgMotorPosition();
		if (motor_pos == FLT_MAX)
			return FALSE;

		// mkust get this before locking the critical section to avoid grid lock
		double last_manoeuvre_pos = GetLastManoeuvrePosition();

		// if the motor has not yet moved a full mm, then return
		// less than this and the velocity will not be stable
		m_crit1.Lock();
		int nSize = (int)m_listLaserPositions.GetSize();

		if (nSize > 0 && motor_pos < m_listLaserPositions[nSize - 1].pos + 1)
		{
			m_crit1.Unlock();
			return FALSE;
		}
		// don't allow a change in the distance from the weld to change by more than 1 mm
		// don't use values prior to the last manoeuvre
		m_listLaserPositions.SetSize(++nSize, NAVIGATION_GROW_BY);
		m_listLaserPositions[nSize - 1].time_noted	= clock(); // niote wshen this position was, used to calculate velocities
		m_listLaserPositions[nSize - 1].pos			= motor_pos;
		m_listLaserPositions[nSize - 1].gap_raw		= measure.weld_cap_mm.x;
		CDoublePoint ret	= ::LowPassFilterGap(m_listLaserPositions, last_manoeuvre_pos);

		// if not value set, then replace with the previous value
		// the distance from the weld will not change all the fast regardless
		if (ret.x == FLT_MAX)
			ret.x = gap1;

		// if a previous value has been set, and this value is more than 1 mm difference, use the previous value
		else if (gap1 != FLT_MAX && fabs(ret.x - gap1) > MAX_GAP_CHANGE_PER_MM)
			ret.x = gap1;


		// there appears to be a delay of 1 measure between raw and filtered
		m_listLaserPositions[nSize - 1].gap_filt = ret.x;
		gap1 = ret.x;

		m_listLaserPositions[nSize - 1].vel_raw = ret.y;
		m_listLaserPositions[nSize - 1].vel_filt = ::CalcGapVelocity(m_listLaserPositions, last_manoeuvre_pos);

		// write these to a file to use in excel
		char my_documents[MAX_PATH];
		HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
		if (result == S_OK)
		{
			FILE* fp = NULL;
			CString szFile;
			szFile.Format("%s\\test2.txt", my_documents);
			if (fopen_s(&fp, szFile, "a") == 0 && fp != NULL)
			{
				fprintf(fp, "%.1f\t%.3f\t%.3f\t%.3f\t%.3f\t%.1f\n",
					m_last_pos.pos,
					(m_listLaserPositions[nSize - 1].gap_raw == FLT_MAX) ? 0 : m_listLaserPositions[nSize - 1].gap_raw,
					(m_listLaserPositions[nSize - 1].gap_filt == FLT_MAX) ? 0 : m_listLaserPositions[nSize - 1].gap_filt,
					(m_listLaserPositions[nSize - 1].vel_raw == FLT_MAX) ? 0 : m_listLaserPositions[nSize - 1].vel_raw,
					(m_listLaserPositions[nSize - 1].vel_filt == FLT_MAX) ? 0 : m_listLaserPositions[nSize - 1].vel_filt,
					(last_manoeuvre_pos == FLT_MAX) ? 0 : last_manoeuvre_pos);
				fclose(fp);
			}
		}

		m_last_pos = m_listLaserPositions[nSize - 1];
		m_crit1.Unlock();

	}
	return TRUE;
}

void CWeldNavigation::StartSteeringMotors(int nSteer, double motor_speed/*=0*/)
{
	StopSteeringMotors();
	if ( nSteer)
	{
		m_fMotorSpeed = motor_speed;
		m_bSteerMotors = TRUE;
		m_crit1.Lock();
		m_crit1.Lock();
		m_listLaserPositions.SetSize(0);
		m_crit1.Unlock();
		m_crit2.Lock();
		m_listManoevers.SetSize(0);
		m_crit2.Unlock();
		m_last_pos.Reset();
		m_crit1.Unlock();
		if( nSteer & 0x1 )
			m_hThreadNoteLaser = AfxBeginThread(::ThreadNoteLaser, this);
		if( nSteer & 0x2 )
			m_hThreadSteerMotors = AfxBeginThread(::ThreadSteerMotors, this);
	}
}

void CWeldNavigation::StopSteeringMotors()
{
	if (m_hThreadNoteLaser != NULL)
	{
		m_bSteerMotors = FALSE;
		WaitForSingleObject(m_hThreadNoteLaser, INFINITE);
	}
	if (m_hThreadSteerMotors != NULL)
	{
		m_bSteerMotors = FALSE;
		WaitForSingleObject(m_hThreadSteerMotors, INFINITE);
	}
}

// try to get a new position every 5 mm
UINT CWeldNavigation::ThreadNoteLaser()
{
	// aim for a new measure every mm
	int delay = max((int)(UPDATE_DISTANCE_MM * 1000.0 / m_fMotorSpeed + 0.5), 1);

	while (m_bSteerMotors)
	{
		clock_t tim1 = clock();
		NoteNextLaserPosition();
		clock_t tim2 = clock();

		if (tim2 - tim1 < delay)
			Sleep(delay - (tim2 - tim1));
	}
	return 0;
}

void CWeldNavigation::GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>& list)
{
	m_crit1.Lock();
	list.Copy(m_listLaserPositions);
	m_crit1.Unlock();
}





UINT CWeldNavigation::ThreadSteerMotors()
{
	clock_t tim1 = clock();
	CString str;

	int steer = 0;
	int  time_first = 0;

	char my_documents[MAX_PATH];
	HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

	CArray<CDoublePoint, CDoublePoint> listVel;
	while (m_bSteerMotors)
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
		double gap_dist = m_last_pos.gap_filt;	// current distance from the weld
		double gap_vel1 = m_last_pos.vel_filt;	// current mm/M to the weld
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
		// do nothing if already in the correct direction
		if( fabs(gap_vel2 - gap_vel1) < 1 )
		{
			Sleep(1); // avoid a tight loop
			continue;
		}

		// keep a record of manoeuvres
		// will use this to insure that don't calculate gap or velocity across a manoeuvre
		// need to note desired change in direction, how acomplished and for how long
		m_crit2.Lock();
		int nSize2 = (int)m_listManoevers.GetSize();
		m_listManoevers.SetSize(++nSize2);
		m_listManoevers[nSize2 - 1].manoeuvre_pos = pos; // at what ms did this manoeuvre begin
		m_listManoevers[nSize2 - 1].time_start = clock(); // at what ms did this manoeuvre begin
		m_listManoevers[nSize2 - 1].gap_start = gap_dist; // what offset were we trying to corect
		m_listManoevers[nSize2 - 1].vel_start = gap_vel1; // how long is this manoeuvre going to last
		m_listManoevers[nSize2 - 1].vel_target = gap_vel2; // how long is this manoeuvre going to last
		m_crit2.Unlock();

		// calculkate how fast to sterr the crawler
		// the further to go, the faster to turn, so doesn';t take too long
		// ideally can get to where going within a sdecond or two
		double turn_rate = CalculateTurnRate(gap_dist, gap_vel1, gap_vel2);



		// enact this steering manoeuvre
		// change the wheels by 5%, until measure the desired direction
		// if travel.x < 0 then want to travel to the right (left wheels faster)
		double change = (gap_vel2 < gap_vel1) ? -(turn_rate * m_fMotorSpeed / 100.0) : (turn_rate * m_fMotorSpeed / 100.0);
		double speedLeft = m_fMotorSpeed - change / 2;
		double speedRight = m_fMotorSpeed + change / 2;
		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };

		SetMotorSpeed(speed1);

		// now watch unmtil gewt to 1/2 way to the desired velocity or gap width
		// whichever comes first (avoid overshoot)
		// note the current time, so that can calculate how fast the direction is changing
		clock_t t1 = clock();
		double max_accel = 0;

		// stop when 1/2 way to the desired velociuty to avoid over-shoot
		// also limit to a maximum time
		clock_t t11 = clock();
		double stop_vel = gap_vel1 + 1*(gap_vel2 - gap_vel1) / 2;
		while (m_bSteerMotors && clock()-t11 < MAX_STEERING_TIME)
		{
			// wait a ms, then check the direction
			Sleep(1);

			LASER_POS last_pos = GetLastNotedPosition(); // make thread safe
			if (last_pos.gap_filt == FLT_MAX || last_pos.vel_filt == FLT_MAX)
				continue;

			// check if have achieved the desired gap velocity
			// when have will straighten back out
			// note how fast this change is taking place, so that can change back before getting there
			double change = last_pos.vel_filt - gap_vel1;
			double accel = 1000*change / (clock() - t1); // (mm/M per ms) will not be zero due to sleep(1) above
			max_accel = max(max_accel, fabs(accel));

			// at the 1/2 way point of the gap
			// if go all the way, there will be over shoot
			if (fabs(last_pos.gap_filt) < fabs(gap_dist/2))
				break;

			// check if are at or have exceed 1/2 way to the deisred velocty
			if (gap_vel2 > gap_vel1 && last_pos.vel_filt >= stop_vel)
				break;
			if (gap_vel2 < gap_vel1 && last_pos.vel_filt <= stop_vel)
				break;

			listVel.Add(CDoublePoint(last_pos.pos, last_pos.vel_filt));
		}
		clock_t manoeuvre_time = clock()-t1;

		// set back to driving straight (but in new direction)
		// no need to set back to straight, then next calculation will determine the best directiojn
		// NO, LEAVE IN THE MANOEVER DIRECTION, NEXT MANOEVER WILL DESIRED NEW DIRECTION
//		double speed2[] = { m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed };
//		SetMotorSpeed(speed2);

		LASER_POS last_pos = GetLastNotedPosition(); // make thread safe

		// get the raw gap position for this so don't need the critical section
		m_crit2.Lock();
		m_listManoevers[nSize2 - 1].turn_rate = turn_rate;				// time to get to 1/2 way point
		m_listManoevers[nSize2 - 1].manoeuvre_time = manoeuvre_time;				// time to get to 1/2 way point
		m_listManoevers[nSize2 - 1].manoeuvre_accel = max_accel;					// maximum acceleration seen during this time
		m_listManoevers[nSize2 - 1].vel_after = (last_pos.vel_filt == FLT_MAX) ? 9999 : last_pos.vel_filt;		// the velocity seen after the 1/2 way point
		m_listManoevers[nSize2 - 1].gap_end_man = (last_pos.gap_filt==FLT_MAX) ? 9999 : last_pos.gap_filt;		// the gap seen at the 1/2 way point
		m_listManoevers[nSize2 - 1].time_end = clock();															// at what ms did this manoeuvre end

		// will take this many metres to make the manoeuvre
//		double drive_dist = (last_pos.gap_vel == FLT_MAX ) ? 9999 : fabs(1000*last_pos.gap_filt / last_pos.gap_vel); // mm
//		int drive_time1 = (int)(1000 * drive_dist / m_fMotorSpeed + 0.5); // mks
//		m_listManoevers[nSize2 - 1].drive_time1 = 0; //  drive_time1; // at what ms did this manoeuvre begin
		m_crit2.Unlock();

		// drive until the gap is zero
		// the above drive time is only an estimate
		// only drtive for about 100 ms, then re-calcualte a new manoeuvre
		// stop though if get to the weld
/*		t1 = clock();
		while (m_bSteerMotors && clock() - t1 < 100 )
		{
			// wait a ms, then check the direction
			Sleep(1);

			m_crit1.Lock();
			LASER_POS last_pos = m_last_pos; // make thread safe
			m_crit1.Unlock();

				// check if are at or have exceed the desired velocity
			if (gap_vel2 > gap_vel1 && last_pos.gap_filt >= 0)
				break;
			if (gap_vel2 < gap_vel1 && last_pos.gap_filt <= 0)
				break;
		}
		clock_t drive_time2 = clock() - t1;
//		Wait(drive_time); //  / 2);

		m_crit1.Lock();
		last_pos = m_last_pos; // make thread safe
		m_crit1.Unlock();
		
		// now note the gap at the end of the drive
		m_crit2.Lock();
		m_listManoevers[nSize2 - 1].drive_time2 = drive_time2;
		m_listManoevers[nSize2 - 1].gap_end_drive = (last_pos.gap_filt == FLT_MAX) ? 9999 : last_pos.gap_filt;
		m_crit2.Unlock();
*/
	}
	// write a list of the manoeuvres to a file
	FILE* fp = NULL;
	CString szFile;
	szFile.Format("%s\\manoeuvres.txt", my_documents);
	if (fopen_s(&fp, szFile, "a") == 0 && fp != NULL)
	{
		fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
			"Pos", "gap1", "Rate", "vel1", "vel2", "vel3", "gap2");

		m_crit2.Lock();
		int nSize2 = (int)m_listManoevers.GetSize();
		for (int i = 0; i < nSize2; ++i)
		{
			fprintf(fp, "%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\n",
				m_listManoevers[i].manoeuvre_pos,
				m_listManoevers[i].gap_start,
				m_listManoevers[i].turn_rate,
				m_listManoevers[i].vel_start,
				m_listManoevers[i].vel_target,
				m_listManoevers[i].vel_after,
				m_listManoevers[i].gap_end_man);

			// now output all the noted velocities from this time until the next time
			for (int j = 0; j < listVel.GetSize(); ++j)
			{
				if (listVel[j].x >= m_listManoevers[i].manoeuvre_pos && (i == nSize2 - 1 || listVel[j].x < m_listManoevers[i + 1].manoeuvre_pos))
				{
					fprintf(fp, "%.1f\t\t\t\t%.1f\t\t\n",
						listVel[j].x,
						listVel[j].y);
				}
			}
		}
		m_crit2.Unlock();
		fclose(fp);
	}

	return 0;
}
/*
UINT CWeldNavigation::ThreadSteerMotors()
{
	clock_t tim1 = clock();
	CString str;

	int steer = 0;
	int  time_first = 0;

	char my_documents[MAX_PATH];
	HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

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
		int nSize2 = (int)m_listManoevers.GetSize();
		m_listManoevers.SetSize(++nSize2);
		m_listManoevers[nSize2-1].time_start = clock(); // at what ms did this manoeuvre begin
		m_listManoevers[nSize2-1].gap_start = gap_dist; // what offset were we trying to corect
		m_listManoevers[nSize2-1].vel_start = gap_vel1; // how long is this manoeuvre going to last
		m_listManoevers[nSize2-1].vel_target = gap_vel2; // how long is this manoeuvre going to last
		m_listManoevers[nSize2-1].manoeuvre_time = steer_time; // how long is this manoeuvre going to last
		m_listManoevers[nSize2-1].manoeuvre_speed = travel.x; // 
		m_listManoevers[nSize2-1].drive_time = drive_time; // at what ms did this manoeuvre begin
		m_crit2.Unlock();

		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };
		SetMotorSpeed(speed1);
		Sleep( steer_time );

		// set back to driving straight (but in new direction)
		double speed2[] = { m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed };
		SetMotorSpeed(speed2);

		m_crit2.Lock();
		LASER_MEASURES measure;
		if ( GetLaserMeasurment(measure))
		{
			// get the raw gap position for this so don't need the critical section
			m_listManoevers[nSize2-1].gap_end_man = measure.weld_cap_mm.x;
			m_listManoevers[nSize2-1].time_end = clock(); // at what ms did this manoeuvre begin
		}
		m_crit2.Unlock();

		// drive the remainder at this direction
		// assume that now on location
		Wait(drive_time); //  / 2);
				// now note the gap at the end of the drive
		m_crit2.Lock();
		if (GetLaserMeasurment(measure))
			m_listManoevers[nSize2 - 1].gap_end_drive = measure.weld_cap_mm.x;
		m_crit2.Unlock();

	}
	// write a list of the manoeuvres to a file
	FILE* fp = NULL;
	CString szFile;
	szFile.Format("%s\\manoeuvres.txt", my_documents);
	if (fopen_s(&fp, szFile, "a") == 0 && fp != NULL)
	{
		fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
			"Start", "gap1", "vel1", "vel2", "time", "actual", "speed", "gap2", "drive", "gap3");

		m_crit2.Lock();
		int nSize2 = (int)m_listManoevers.GetSize();
		for (int i = 0; i < nSize2; ++i)
		{
			fprintf(fp, "%d\t%.1f\t%.1f\t%.1f\t%d\t%d\t%.1f\t%.1f\t%d\t%.1f\n",
				m_listManoevers[i].time_start - time_first,
				m_listManoevers[i].gap_start,
				m_listManoevers[i].vel_start,
				m_listManoevers[i].vel_target,
				m_listManoevers[i].manoeuvre_time,
				m_listManoevers[i].time_end - m_listManoevers[i].time_start,
				m_listManoevers[i].manoeuvre_speed,
				m_listManoevers[i].gap_end_man,
				m_listManoevers[i].drive_time,
				m_listManoevers[i].gap_end_man	);
		}
		m_crit2.Unlock();
		fclose(fp);
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