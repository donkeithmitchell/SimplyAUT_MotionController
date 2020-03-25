#include "pch.h"
#include "WeldNavigation.h"
#include "LaserControl.h"
#include "MotionControl.h"
#include "MagController.h"
#include "DialogGirthWeld.h"
#include "Misc.h"

#define NAVIGATION_GROW_BY			2048 // at 1 per mm, this would be 2 metres
#define FILTER_WIDTH				10
#define UPDATE_DISTANCE_MM			10
#define NAVIGATION_TARGET_DIST_MM	100
#define WHEEL_SPACING				(10.0 * 2.54)
#define MIN_STEERING_TIME			100
#define MAX_STEERING_TIME			1000
#define DESIRED_SPEED_VARIATION		1 // percent
#define MAX_SPEED_VARIATION			10 // percent
static const double PI = acos(-1.0);

static UINT ThreadSteerMotors(LPVOID param)
{
	CWeldNavigation* this2 = (CWeldNavigation*)param;
	return this2->ThreadSteerMotors();
}

CWeldNavigation::CWeldNavigation()
{
	m_bSteerMotors = FALSE;
	m_hThreadSteerMotors = NULL;
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

static double HammingWindow(int ind, int nWidth)
{
	double Wk = 0.54 - 0.46 * cos(PI * ind / nWidth);
	return Wk;
}

// only need to L/P filter the last value
// for now ghet the average of the last 5 values
static double LowPassFilterGap(const LASER_POS* buff1, int nSize)
{
	// assumne get a value per mm
	// use a hamming window (1/2) to filter

	double sum = 0;
	double count = 0;
	for (int i = 0; i < nSize; ++i)
	{
		if (buff1[i].gap_raw != FLT_MAX)
		{
			double Wk = HammingWindow(i, nSize);
			sum += Wk * buff1[i].gap_raw;
			count += Wk;
		}
	}

	return (count > 0) ? sum / count : FLT_MAX;
}

// navigation required knowing the velocity to from the weld
// not just the distance fromn thew weld
static double CalcGapVelocity(const LASER_POS* buff1, int nSize)
{
	// note the average velocity of the last FILTER_WIDTH entries
	// use the filtered positions

	double sum = 0;
	double count = 0;
	for (int i = 1; i < nSize; ++i)
	{
		// -Ve gap is to the left
		// -Ve velocity os driving further to the left
		if (buff1[i].gap_filt != FLT_MAX && buff1[i - 1].gap_filt != FLT_MAX)
		{
			double gap_diff = buff1[i].gap_filt - buff1[i - 1].gap_filt; // change in gap 
			double tim_diff = buff1[i].tim - buff1[i - 1].tim;
			double pos_diff = buff1[i].pos - buff1[i - 1].pos;

			// divide by 1000, so the velocity is mm/M
			double vel = (pos_diff != 0) ? gap_diff / (pos_diff / 1000.0) : 0; // mm/M
			double Wk = HammingWindow(i, nSize);
			sum += Wk * vel;
				count += Wk;
		}
	}

	return (count > 0) ? sum / count : FLT_MAX;
}

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
		return m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_GET_MEASURE, (LPARAM)&measure);
	else
		return FALSE;
}

BOOL CWeldNavigation::GetMotorSpeed(double speed[4])
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		return m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_GET_MOTOR_SPEED, (LPARAM)speed);
	else
		return  FALSE;
}

BOOL CWeldNavigation::SetMotorSpeed(const double speed[4] )
{
	if (m_pParent && IsWindow(m_pParent->m_hWnd))
		return m_pParent->SendMessage(m_nMsg, CDialogGirthWeld::NAVIGATE_SET_MOTOR_SPEED, (LPARAM)speed);
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
LASER_POS CWeldNavigation::NoteNextLaserPosition()
{
	LASER_MEASURES measure;
	LASER_POS buffer[FILTER_WIDTH];

	LASER_POS ret = m_last_pos;
	if (GetLaserMeasurment(measure) )
	{
		// m_measure will be updatre regularily 
		double motor_pos = GetAvgMotorPosition();

		// as this is called by a thread
		// lock the array while modifying
		ret.gap_raw = measure.weld_cap_mm.x;
		ret.tim = clock(); // niote wshen this position was, used to calculate velocities
		ret.pos = motor_pos;

		// calculate the filtered position of this last entry as the average of the last 5
		// get a copy of the last 10 entries
		m_crit.Lock();
		int nSize = (int)m_posLaser.GetSize();
		m_posLaser.SetSize(++nSize, NAVIGATION_GROW_BY);

		// if too short can only set the location not the velocity
		if (m_posLaser.GetSize() <= FILTER_WIDTH)
		{
			m_posLaser[nSize - 1] = ret;
			m_crit.Unlock();
			return ret;
		}

		// once in a copy will be thread safe
		for (int i = FILTER_WIDTH - 1, j = m_posLaser.GetSize()-2; i >= 0; --i, --j)
			buffer[i] = m_posLaser[j];

		m_crit.Unlock();

		ret.gap_filt = ::LowPassFilterGap(buffer, FILTER_WIDTH);
		ret.gap_vel = ::CalcGapVelocity(buffer, FILTER_WIDTH);
		if (ret.gap_vel != FLT_MAX)
		{
			ret.gap_accel = ::CalcGapAcceleration(buffer, FILTER_WIDTH);

			// get the actual (VS requested) speed of all the motors
			double speed[4];
			GetMotorSpeed(speed);

			double velLeft = (speed[0] + speed[3]) / 2;
			double velRight = (speed[1] + speed[2]) / 2;
			double ratio = (velRight != 0) ? 100 * velLeft / velRight : 0;
			ret.motor_lr = ratio;
		}

		// hiold the lock() as short a time as posible
		m_crit.Lock();
		m_last_pos = m_posLaser[nSize - 1] = ret;
		m_crit.Unlock();

	}
	return ret;
}

void CWeldNavigation::StartSteeringMotors(BOOL bStart, double motor_speed/*=0*/)
{
	StopSteeringMotors();
	if (bStart)
	{
		m_fMotorSpeed = motor_speed;
		m_bSteerMotors = TRUE;
		m_posLaser.SetSize(0);
		m_last_pos.Reset();
		m_hThreadSteerMotors = AfxBeginThread(::ThreadSteerMotors, this);
	}
}

void CWeldNavigation::StopSteeringMotors()
{
	if (m_hThreadSteerMotors != NULL)
	{
		m_bSteerMotors = FALSE;
		WaitForSingleObject(m_hThreadSteerMotors, INFINITE);
	}
}

UINT CWeldNavigation::ThreadSteerMotors()
{
	clock_t tim1 = -1;
	CString str;
	int delay1 = max((int)(UPDATE_DISTANCE_MM * 1000.0 / m_fMotorSpeed + 0.5), 1);
	while(m_bSteerMotors)
	{
		// this calculates not only how far the crawler is away fromn the weld, 
		// but the angle it is crawling from the weld in (mm/M)
		LASER_POS last_pos = NoteNextLaserPosition();

		// will return zero if not enough locations noted yet
		double gap_vel1 = m_last_pos.gap_vel; // current mm/M to the weld
		double gap_dist = m_last_pos.gap_filt; // current distance from the weld
		if (gap_vel1 == FLT_MAX)
		{
			Wait(delay1);
			continue;
		}

		// now calculate the desired angle (mm/M) to the weld to be at the weld in about 100 mm (NAVIGATION_TARGET_DIST_MM)
		// this is simply the current distancve frtom the weld over 0.1 M
		// a -Ve gap is to the left, a +Ve to the right
		// if to the left, then need to steer to the right (thus -1000)
		double gap_vel2 = -1000 * last_pos.gap_filt / NAVIGATION_TARGET_DIST_MM; // mm/M

		// the change in direction that is required
		double gap_vel = gap_vel2 - gap_vel1;

		// now calculate the extrta travel required by a given crawler side to achieve this change in direction
		// x = 	how long to change for
		// y = how to change by

		CDoublePoint travel = CalculateLeftRightTravel(gap_vel);

		// modify the mnotor speeds for the required time
		if (travel.x != 0)
		{
			double speedLeft = m_fMotorSpeed + travel.x / 2;
			double speedRight = m_fMotorSpeed - travel.x / 2;

			if(tim1 == -1 )
				tim1 = clock();

			str.Format("%d: GAP: %.1f mm, Vel1: %.1f mm/M, Vel2: %.1f mm/M, Steer %.1f mm/M for %.0f ms", 
				clock() - tim1, gap_dist, gap_vel1, gap_vel2, travel.x, travel.y);
			SendDebugMessage(str);

			str.Format("Left: %.1f, Right: %.1f mm/sec", speedLeft, speedRight);
			SendDebugMessage(str);

			double speed1[] = { speedLeft, speedRight, speedRight, speedLeft };
			SetMotorSpeed(speed1);
			Wait((int)(travel.y + 0.5));

			// likely now have completed the turn, set back to driving straight (but in new direction)
			double speed2[] = { m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed ,m_fMotorSpeed };
			SetMotorSpeed(speed2);
		}

		// now wait for 1/2 the required time to get to line, before reccalulating
		// at gap_vel2 it will take how long to travel 100 mm (100 / gap_vel2 sec)
		int delay3 = (int)(NAVIGATION_TARGET_DIST_MM / fabs(gap_vel2) + 0.5);
		str.Format("%d: Drive for: %d ms",
			clock() - tim1, delay3);
		SendDebugMessage(str);
		Wait(delay3/2);
	}
	return 0;
}

// be sure to update the motor position every mm while waiting
void CWeldNavigation::Wait(int delay)
{
	clock_t tim2 = clock();
	int delay1 = max((int)(UPDATE_DISTANCE_MM * 1000.0 / m_fMotorSpeed + 0.5), 1);

	while (m_bSteerMotors && clock() - tim2 < delay)
	{
		NoteNextLaserPosition();
		Sleep(delay1);
	}
}

// calculate the extra distance that one side of the crawler must travel to achieve the desired change in direction
CDoublePoint CWeldNavigation::CalculateLeftRightTravel(double dir_change)
{
	// if want to travel (dir_change) in 1000 mm
	// then calculate how far will travel in 10*2.54 mm
	// the change in crawler angle in  radians
	double dist = dir_change * WHEEL_SPACING / 1000;

	// if speed up the motors by 1%, how long will it take to achieve this travel
	// do not want to have a significant difference left right
	// small changes will make the navigation more predictable
	// this is the relative speed in mm/sec
	// i.e in 1 sec would travel this far
	double fChange = DESIRED_SPEED_VARIATION * m_fMotorSpeed / 100.0f; // mm/Sec

	// this is the time that the speed change is required;
	double change_time = 1000 * dist / fChange;

	// want this time to be between 1/10 sec and 2 sec
	if (change_time < MIN_STEERING_TIME)
	{
		fChange = 1000 * dist / MIN_STEERING_TIME;
		change_time = MIN_STEERING_TIME;
	}
	else if (change_time > MAX_STEERING_TIME)
	{
		fChange = 1000 * dist / MAX_STEERING_TIME;
		change_time = MAX_STEERING_TIME;
	}

	// check that this change is still within the premitted percentage of speeddf
	if (100 * fChange / m_fMotorSpeed > MAX_SPEED_VARIATION)
	{
		fChange = MAX_SPEED_VARIATION * m_fMotorSpeed / 100;
		change_time = dist / fChange;
	}

	return CDoublePoint(fChange, change_time);
}