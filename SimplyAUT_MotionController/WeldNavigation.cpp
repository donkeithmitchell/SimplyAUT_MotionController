#include "pch.h"
#include "WeldNavigation.h"
#include "LaserControl.h"
#include "MotionControl.h"
#include "MagController.h"
#include "DialogGirthWeld.h"
#include "Misc.h"

static double PI = 4 * atan(1.0);


#define NAVIGATION_GROW_BY			2048 // at 1 per mm, this would be 2 metres
#define GAP_FILTER_WIDTH			10 // mm (filter over last 10 mm
#define VEL_FILTER_WIDTH			10 // mm (filter over last 10 mm
#define VELOCITY_WIDTH				50 // mm (use positioons 5 mm apart
#define VELOCITY_MIN_COUNT          5
#define UPDATE_DISTANCE_MM			2 // mm
#define NAVIGATION_TARGET_DIST_MM	100
#define WHEEL_SPACING				(10.0 * 2.54)
#define MIN_STEERING_TIME			100
#define MAX_STEERING_TIME			1000
#define DESIRED_SPEED_VARIATION		1 // percent
#define MAX_SPEED_VARIATION			10 // percent

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

// this is intended to filter only the last added value
// set the hammin window to the length available up to nWidth
// use the position offset for the qwindow, not the index
static double LowPassFilterGap(const CArray<LASER_POS, LASER_POS >& buff1)
{
	static double X[2*GAP_FILTER_WIDTH], Y[2*GAP_FILTER_WIDTH];

	// may need to minimize the width if not enough data
	int nSize = (int)buff1.GetSize();
	if (nSize == 0)
		return FLT_MAX;

	// start with the last entered value
	// add values to the sum until too far away
	double pos2 = buff1[nSize - 1].pos;

//	double cnt = 0;
//	double sum = 0;
	int count = 0;
	for (int i = nSize - 1; i >= 0 && count < 2*GAP_FILTER_WIDTH; --i)
	{
		if (buff1[i].gap_raw == FLT_MAX)
			continue;

		double pos1 = buff1[i].pos;
		if (pos2 - pos1 > GAP_FILTER_WIDTH)
			break;

		X[count] = pos1;
		Y[count] = buff1[i].gap_raw;
		if( count == 0 || fabs(Y[count] - Y[count-1]) <= 1 )
			count++;
//		double Wk = HammingWindow((pos2 - pos1), nWidth_mm);
//		sum += Wk * buff1[i].gap_raw;
//		cnt += Wk;
	}

	if (count < 2)
		return FLT_MAX;

	// now get the slope of this line, that is the velocutty
	double coeff[2];
	polyfit(X, Y, count, 1, coeff);

	// calcualte the estimated location at pos2
	// Y = mX + b
	double gap = coeff[1] * pos2 + coeff[0];
	return gap;
	//	return (cnt != 0) ? sum / cnt : FLT_MAX;
}

// this is intended to filter only the last added value
// set the hammin window to the length available up to nWidth
// use the position offset for the qwindow, not the index
static double LowPassFilterVel(const CArray<LASER_POS, LASER_POS >& buff1, double nWidth_mm)
{
	// may need to minimize the width if not enough data
	int nSize = (int)buff1.GetSize();
	if (nSize == 0)
		return FLT_MAX;

	// start with the last entered value
	// add values to the sum until too far away
	double pos2 = buff1[nSize - 1].pos;

	int count = 0;
	double cnt = 0;
	double sum = 0;
	for (int i = nSize - 1; i >= 0; --i)
	{
		if (buff1[i].vel_raw == FLT_MAX)
			continue;

		double pos1 = buff1[i].pos;
		if (pos2 - pos1 > nWidth_mm)
			break;

		double Wk = HammingWindow((pos2 - pos1), nWidth_mm);
		sum += Wk * buff1[i].vel_raw;
		cnt += Wk;
		count++;
	}

	return (cnt != 0 && count >= VELOCITY_MIN_COUNT) ? sum / cnt : FLT_MAX;
}

// use the filtered gap_pos if have it, else the raw value
// get values that are at least 5 mm apart
static double CalcGapVelocity(const CArray<LASER_POS, LASER_POS>& buff1)
{
	static double X[2*VELOCITY_WIDTH], Y[2*VELOCITY_WIDTH];
	int nSize = (int)buff1.GetSize();
	if (nSize == 0)
		return FLT_MAX;

	double pos2 = buff1[nSize - 1].pos;
	double gap2 = (buff1[nSize - 1].gap_filt == FLT_MAX) ? buff1[nSize - 1].gap_raw : buff1[nSize - 1].gap_filt;

	int count = 0;
	for (int i = nSize - 1; i >= 0 && count < 2*VELOCITY_WIDTH; --i)
	{
		double pos1 = buff1[i].pos;
		if (pos2 - pos1 >= VELOCITY_WIDTH)
			break;

		X[count] = pos2-pos1;
		Y[count] = (buff1[i].gap_filt == FLT_MAX) ? buff1[i].gap_raw : buff1[i].gap_filt;
		count++;
	}
	// need 2 points to get a velocity
	if (count < 2)
		return FLT_MAX;

	// now get the slope of this line, that is the velocutty
	double coeff[2];
	polyfit(X, Y, count, 1, coeff);

	return coeff[1];
}
/*
static double CalcGapAcceleration(const LASER_POS* buff1, int nSize)
{
	// note the average accel of the last 5 entries
	// use the filtered velocities

	double sum = 0;
	double count = 0;
	for (int i = 1; i < nSize; ++i)
	{
		if (buff1[i].gap_vel != FLT_MAX && buff1[i-1].gap_vel != FLT_MAX)
		{
			double vel_diff = buff1[i].gap_vel - buff1[i - 1].gap_vel; // change in gap 
			double pos_diff = buff1[i].pos - buff1[i - 1].pos;

			double vel = (pos_diff != 0) ? vel_diff / (pos_diff / 1000.0) : 0; // mm/M
			double Wk = HammingWindow(i, nSize);
			sum += Wk * vel;
			count += Wk;
		}
	}

	return (count > 0) ? sum / count : FLT_MAX;
}
*/
LASER_POS CWeldNavigation::GetLastNotedPosition()
{
	m_crit.Lock();
	LASER_POS ret = m_last_pos;
	m_crit.Unlock();
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

void CWeldNavigation::SendDebugMessage(const CString& str)
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SEND_DEBUG_MSG, (LPARAM)&str);
}

// 
BOOL CWeldNavigation::NoteNextLaserPosition()
{
	LASER_MEASURES measure;
	if (GetLaserMeasurment(measure) )
	{
		// m_measure will be updatre regularily 
		double motor_pos = GetAvgMotorPosition();
		if (motor_pos == FLT_MAX)
			return FALSE;

		// if the motor has not yet moved a full mm, then return
		// less than this and the velocity will not be stable
		m_crit.Lock();
		int nSize = (int)m_posLaser.GetSize();

		if (nSize > 0 && motor_pos < m_posLaser[nSize - 1].pos + 1)
		{
			m_crit.Unlock();
			return FALSE;
		}

		// initially use a local variable
		// this keeps the locking of the vector to a minimum
		m_last_pos.tim = clock(); // niote wshen this position was, used to calculate velocities
		m_last_pos.gap_raw	= measure.weld_cap_mm.x;
		m_last_pos.pos		= motor_pos;
		m_last_pos.gap_filt	= ::LowPassFilterGap(m_posLaser);
		m_last_pos.vel_raw	= ::CalcGapVelocity(m_posLaser);
		m_last_pos.vel_filt	= ::LowPassFilterVel(m_posLaser, VEL_FILTER_WIDTH);

		// write thesde to a file to use in xcel
		char my_documents[MAX_PATH];
		HRESULT result = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
		if (result == S_OK)
		{
			static int ind = 0;
			FILE* fp = NULL;
			CString szFile;
			szFile.Format("%s\\test.txt", my_documents);
			if (fopen_s(&fp, szFile, "a") == 0 && fp != NULL)
			{
				fprintf(fp, "%d, %.1f, %.1f, %.1f, %.1f, %.1f\n", ind, m_last_pos.pos, m_last_pos.gap_raw, m_last_pos.gap_filt, m_last_pos.vel_raw, m_last_pos.vel_filt);
				ind++;
				fclose(fp);
			}
		}

		m_posLaser.SetSize(++nSize, NAVIGATION_GROW_BY);
		m_posLaser[nSize - 1] = m_last_pos;
		m_crit.Unlock();

	}
	return TRUE;
}

void CWeldNavigation::StartSteeringMotors(BOOL bStart, double motor_speed/*=0*/)
{
	StopSteeringMotors();
	if ( bStart)
	{
		m_fMotorSpeed = motor_speed;
		m_bSteerMotors = TRUE;
		m_crit.Lock();
		m_posLaser.SetSize(0);
		m_last_pos.Reset();
		m_crit.Unlock();
		m_hThreadNoteLaser = AfxBeginThread(::ThreadNoteLaser, this);
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
	m_crit.Lock();
	list.Copy(m_posLaser);
	m_crit.Unlock();
}







UINT CWeldNavigation::ThreadSteerMotors()
{
	clock_t tim1 = clock();
	CString str;

	while(m_bSteerMotors)
	{
		double gap_vel1 = m_last_pos.vel_filt; // current mm/M to the weld
		double gap_dist = m_last_pos.gap_filt; // current distance from the weld
		if (gap_vel1 == FLT_MAX)
		{
			Sleep(1); // avoid a tight loop
			continue;
		}

		// now calculate the desired angle (mm/M) to the weld to be at the weld in about 100 mm (NAVIGATION_TARGET_DIST_MM)
		// this is simply the current distancve frtom the weld over 0.1 M
		// a -Ve gap is to the left, a +Ve to the right
		// if to the left, then need to steer to the right (thus -1000)
		double gap_vel2 = -1000 * m_last_pos.gap_filt / NAVIGATION_TARGET_DIST_MM; // mm/M

		// the change in direction that is required
		double gap_vel = gap_vel2 - gap_vel1;

		// now calculate the extrta travel required by a given crawler side to achieve this change in direction
		// x = 	how long to change for
		// y = how to change by
		CDoublePoint travel = CalculateLeftRightTravel(gap_vel1, gap_vel2);

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

		double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };
//		SetMotorSpeed(speed1);
		int steer_time = (int)(travel.y + 0.5);
		Sleep( steer_time );

		// likely now have completed the turn, set back to driving straight (but in new direction)
		double speed2[] = { m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed };
	//	SetMotorSpeed(speed2);

		// now wait for 1/2 the required time to get to line, before reccalulating
		// at gap_vel2 it will take how long to travel 100 mm (100 / gap_vel2 sec)
		int drive_time = max((int)(NAVIGATION_TARGET_DIST_MM / fabs(gap_vel2) - steer_time + 0.5), 2);
		str.Format("%d: Drive for: %d ms",	clock() - tim1, drive_time/2);
		SendDebugMessage(str);
		Wait(drive_time /2);
	}
	return 0;
}

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
CDoublePoint CWeldNavigation::CalculateLeftRightTravel(double gap_vel1, double gap_vel2)
{
	// if want to travel (dir_change) in 1000 mm
	// then calculate how far will travel in 10*2.54 mm
	// the change in crawler angle in  radians
	double gap_vel = gap_vel2 - gap_vel1;
	double dist = gap_vel * WHEEL_SPACING / 1000;

	// if could turn instantaniously, would need to drive this far
	// would like to be turned within 1/2 this time
	int drive_time = (int)(NAVIGATION_TARGET_DIST_MM / fabs(gap_vel2) + 0.5);
	int max_steering_time = max(drive_time / 2, 10);

	// if speed up the motors by 1%, how long will it take to achieve this travel
	// do not want to have a significant difference left right
	// small changes will make the navigation more predictable
	// this is the relative speed in mm/sec
	// i.e in 1 sec would travel this far
	double fChange = DESIRED_SPEED_VARIATION * m_fMotorSpeed / 100.0f; // mm/Sec

	// this is the time that the speed change is required;
	double change_time = 1000 * dist / fChange;

	// want this time to be between 1/10 sec and 2 sec
	if (fabs(change_time) < MIN_STEERING_TIME)
	{
		fChange = 1000 * dist / MIN_STEERING_TIME;
		change_time = MIN_STEERING_TIME;
	}
	else if (fabs(change_time) > max_steering_time) // MAX_STEERING_TIME)
	{
		fChange = 1000 * dist / max_steering_time; //  MAX_STEERING_TIME;
		change_time = max_steering_time; //  MAX_STEERING_TIME;
	}

	// check that this change is still within the premitted percentage of speeddf
	if (100 * fChange / m_fMotorSpeed > MAX_SPEED_VARIATION)
	{
		fChange = MAX_SPEED_VARIATION * m_fMotorSpeed / 100;
		change_time = dist / fChange;
	}

	return CDoublePoint(fChange, change_time);
}