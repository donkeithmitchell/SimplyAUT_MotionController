#pragma once
#include "time.h"
#include "SLSDef.h"
#include "LaserControl.h"

struct LASER_POS
{
	LASER_POS() { Reset(); }
	void Reset(){ memset(this, 0x0, sizeof(LASER_POS)); gap_raw = gap_filt = vel_raw = vel_filt = FLT_MAX; }

	double  pos;	// dist travelled (mm)
	double  gap_raw;	// distance to the weld (mm)
	double  gap_filt;	// distance to the weld (mm)
	double  vel_raw;
	double  vel_filt;
	double  last_manoeuvre_pos;
	clock_t time_noted;	// time that measure taken (ms)
	long    rgb_sum;;
};

struct POS_MANOEVER
{
	POS_MANOEVER() {memset(this, 0x0, sizeof(POS_MANOEVER)); }
	double gap0;
	double gap1;
	double gap2;
	double gap3;
	double turn_rate;
	double pos0;
	double pos1;
	double pos2;
	double pos3;
	double gap_vel;
	double turn_time1;
	double turn_time2;
	double turn_time3;
	int  manoeuvre_time; // ms
	int turn_direction;
	clock_t time_start;
	clock_t time_end;
};

class CMotionControl;
class CLaserControl;
class CMagControl;
class CDoublePoint;

class CWeldNavigation
{
public:
	CWeldNavigation(CMotionControl&, CLaserControl&);
	~CWeldNavigation();
	void	Init(CWnd*, UINT);
	BOOL	NoteNextLaserPosition();
	LASER_POS GetLastNotedPosition(int ago_mm);
	void      StartSteeringMotors(int nSteer, int start_pos, int end_pos, double speed, double accel, double offset, BOOL bScanning);
	UINT      ThreadSteerMotors();
	UINT      ThreadSteerMotors_try3();
	UINT      ThreadSteerMotors_try2();
	UINT      ThreadNoteLaser();
	void      GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>&);
	CDoublePoint GetLastRGBValue();
	BOOL		IsNavigating()const;
	BOOL		OpenScanFile();

private:
	void	StopSteeringMotors();
//	CDoublePoint CalculateLeftRightTravel(double gap, double vel1, double vel2);
	double	CalculateTurnRate(double gap, double vel1, double vel2);
	void	Wait(int delay);
	void	SendDebugMessage(const CString& msg);
	BOOL    SetMotorSpeed(const double speed[]);
	BOOL	StopMotors();

	CMotionControl& m_motionControl;
	CLaserControl& m_laserControl;

	CArray<LASER_POS, LASER_POS> m_listLaserPositions;
	CCriticalSection m_crit1;
	BOOL m_bSteerMotors;
	HANDLE m_hThreadSteerMotors;
	HANDLE m_hThreadNoteLaser;
	double m_fMotorSpeed;
	double m_fMotorAccel;
	double m_fWeldOffset;
	BOOL	m_bScanning;
	int m_nStartPos;
	int m_nEndPos;
	FILE* m_fpScanFile;
	CWnd* m_pParent;
	UINT m_nMsg;
};

