#pragma once
#include "time.h"
#include "SLSDef.h"
#include "LaserControl.h"
#include "Define.h"

struct NAVIGATION_PID
{
	NAVIGATION_PID() { Reset(); }
	void Reset() {
		P = NAVIGATION_P; I = NAVIGATION_I; D = NAVIGATION_D; D_LEN = NAVIGATION_D_LEN;  pivot = NAVIGATION_PIVOT; turn_dist = NAVIGATION_TURN_DIST; nav_type = 0; max_turn = MAX_TURN_RATE; Tu = 0;}

	double P;
	double I;
	double D;
	double pivot;
	double Tu;
	double max_turn;
	double turn_dist;
	double dummy8[1];
	int nav_type;
	int D_LEN;
};

struct FILTER_RESULTS
{
	FILTER_RESULTS() { memset(this, 0x0, sizeof(FILTER_RESULTS)); }

	double gap_filt;
	double vel_raw;
	double slope10;
	double gap_predict;
	double pos_predict;
	double dummy8[8 - 5];
};

struct LASER_POS
{
	LASER_POS() { Reset(); }
	void Reset(){ memset(this, 0x0, sizeof(LASER_POS)); gap_raw = gap_filt = vel_raw = vel_filt = FLT_MAX; }

	LASER_MEASURES measures;
	CDoublePoint manoeuvre1;
	CDoublePoint manoeuvre2;
	double  pid_error[3];
	double  pid_steering;
	double  gap_raw;	// distance to the weld (mm)
	double  gap_filt;	// distance to the weld (mm)
	double  vel_raw;
	double  vel_filt;
	double  last_manoeuvre_pos;
	double  gap_predict;
	double  pos_predict;
	double  slope10;
	double dummy81[16 - 12];
	clock_t time_noted;	// time that measure taken (ms)
	clock_t dummy82[2 - 1];
};

struct DRIVE_POS
{
	DRIVE_POS() { memset(this, 0x0, sizeof(DRIVE_POS)); }
	DRIVE_POS(double _x, double _y, int seg) { memset(dummy4, 0x0, sizeof(dummy4)); x = _x; y = _y; segment = seg; }
	double x;
	double y;
	int segment;
	int dummy4[2 - 1];
};

class CMotionControl;
class CLaserControl;
class CMagControl;
class CDoublePoint;

class CWeldNavigation
{
public:
	CWeldNavigation(CMotionControl&, CLaserControl&, NAVIGATION_PID& pid);
	~CWeldNavigation();
	void	Init(CWnd*, UINT);
	BOOL	NoteNextLaserPosition();
	LASER_POS GetLastNotedPosition(int ago_mm);
	void    StartNavigation(int nSteer, int start_pos, int end_pos, double speed, double accel, double offset, BOOL bScanning);
	UINT    ThreadSteerMotors();
	UINT    ThreadNoteLaser();
	void    GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>&);
	BOOL	IsNavigating()const;

private:
	UINT    ThreadSteerMotors_try3();
	UINT    ThreadSteerMotors_PID();	
	UINT    ThreadSteerMotors_Test();
	BOOL	DriveTest(double rate, double pivot, int delay);
	void	StopSteeringMotors();
	void	SendDebugMessage(const CString& msg);
	BOOL    SetMotorSpeed(const double speed[]);
	double  GetAvgMotorSpeed();
	double  GetLRPositionDiff();
	BOOL	StopMotors(BOOL);
	BOOL    SetMotorDeceleration(double);
	void    SetScanning(BOOL bScan) { m_bScanning = bScan; }
	double	GetPIDSteering();
	BOOL	WriteScanFile();
	BOOL	WriteTestFile();
	FILE*	OpenNextFile(const char* szFile);
	double	CalculateTurnRate(double steering)const;
	FILTER_RESULTS LowPassFilterGap(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction);
	void   CalculatePID_Navigation(const CArray<double, double>& Y, CArray<double, double>& out);

	CMotionControl& m_motionControl;
	CLaserControl&	m_laserControl;
	NAVIGATION_PID&	m_pid;

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
	CWnd* m_pParent;
	UINT m_nMsg;

	double m_i_error;
	double m_p_error;
	double m_d_error;
	int    m_i_time;
};

