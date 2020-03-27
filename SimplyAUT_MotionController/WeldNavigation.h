#pragma once
#include "time.h"
#include "SLSDef.h"
#include "LaserControl.h"

struct LASER_POS
{
	LASER_POS() { Reset(); }
	void Reset(){ memset(this, 0x0, sizeof(LASER_POS)); gap_raw = gap_filt = vel_raw = vel_filt = FLT_MAX; }

	double  gap_raw;	// distance to the weld (mm)
	double  gap_filt;	// distance to the weld (mm)
	double  pos;	// dist travelled (mm)
	double  vel_raw;
	double  vel_filt;
	double  dummy8;
	clock_t tim;	// time that measure taken (ms)
	long    dummy4;
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
	LASER_POS GetLastNotedPosition();
	void      StartSteeringMotors(BOOL, double speed=0);
	UINT      ThreadSteerMotors();
	UINT      ThreadNoteLaser();
	void      GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>&);

private:
	void	StopSteeringMotors();
	CDoublePoint CalculateLeftRightTravel(double dir1, double dir2);
	BOOL	GetLaserMeasurment(LASER_MEASURES& measure);
	void	Wait(int delay);
	void	SendDebugMessage(const CString& msg);
	double  GetAvgMotorPosition();
	BOOL    GetMotorSpeed(double speed[]);
	BOOL    SetMotorSpeed(const double speed[]);


	CArray<LASER_POS, LASER_POS> m_posLaser;
	CCriticalSection m_crit;
	BOOL m_bSteerMotors;
	HANDLE m_hThreadSteerMotors;
	HANDLE m_hThreadNoteLaser;
	LASER_POS m_last_pos;
	double m_fMotorSpeed;
	CWnd* m_pParent;
	UINT m_nMsg;
};

