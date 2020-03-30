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
	double  dummy8;
	clock_t time_noted;	// time that measure taken (ms)
	long    dummy4;
};

struct POS_MANOEVER
{
	POS_MANOEVER() {memset(this, 0x0, sizeof(POS_MANOEVER)); }
	double gap_prev;
	double gap_start;
	double gap_end_man;
	double turn_rate;
	double manoeuvre_pos;
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
	CWeldNavigation();
	~CWeldNavigation();
	void	Init(CWnd*, UINT);
	BOOL	NoteNextLaserPosition();
	LASER_POS GetLastNotedPosition(int ago_mm);
	void      StartSteeringMotors(int nSteer, double speed=0, double offset=0);
	UINT      ThreadSteerMotors();
	UINT      ThreadNoteLaser();
	void      GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>&);

private:
	void	StopSteeringMotors();
//	CDoublePoint CalculateLeftRightTravel(double gap, double vel1, double vel2);
	double	CalculateTurnRate(double gap, double vel1, double vel2);
	BOOL	GetLaserMeasurment(LASER_MEASURES& measure);
	void	Wait(int delay);
	void	SendDebugMessage(const CString& msg);
	double  GetAvgMotorPosition();
	double  GetLastManoeuvrePosition();
	BOOL    GetMotorSpeed(double speed[]);
	BOOL    SetMotorSpeed(const double speed[]);


	CArray<LASER_POS, LASER_POS> m_listLaserPositions;
	CArray<POS_MANOEVER, POS_MANOEVER> m_listManoevers;
	CCriticalSection m_crit1;
	CCriticalSection m_crit2;
	BOOL m_bSteerMotors;
	HANDLE m_hThreadSteerMotors;
	HANDLE m_hThreadNoteLaser;
	LASER_POS m_last_pos;
	double m_fMotorSpeed;
	double m_fWeldOffset;
	CWnd* m_pParent;
	UINT m_nMsg;
};

