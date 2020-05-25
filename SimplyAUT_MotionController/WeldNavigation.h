#pragma once
#include "time.h"
#include "SLSDef.h"
#include "LaserControl.h"
#include "Define.h"

struct NAVIGATION_PID
{
	NAVIGATION_PID() { Reset(); }
	void Reset() {
		memset(this, 0x0, sizeof(NAVIGATION_PID));
		Kp = NAVIGATION_P; Ki = NAVIGATION_I; Kd = NAVIGATION_D; D_length_ms = NAVIGATION_D_LEN;  pivot_percent = NAVIGATION_PIVOT;
		turn_dist = NAVIGATION_TURN_DIST; max_turn_rate = DFLT_TURN_RATE1; max_turn_rate_pre = MAX_TURN_RATE1; gap_predict = 0;
		I_accumulate_ms = NAVIGATION_I_ACCUMULATE; max_turn_rate_len = DFLT_MAX_TURN_RATE_LEN; start_speed = DFLT_START_SPEED; start_dist = DFLT_START_DIST;
		Tu = NAVIGATION_DFLT_Tu; P_length_mm = GAP_FILTER_DFLT_WIDTH;
	}

	char simulation_file[_MAX_FNAME];
	double PID_rms[4];
	double Kp;
	double Ki;
	double Kd;
	double turn_dist;
	double Tu_srate;
	double start_speed;
	double start_dist;
	double dummy8[16 - 11];
	int Tu;
	int Phz;
	int D_length_ms; 
	int P_length_mm;
	int I_accumulate_ms;
	int pivot_percent;
	int max_turn_rate;
	int max_turn_rate_len;
	int max_turn_rate_pre;
	BOOL simulation;
	int gap_predict;
	int dummy4[16 - 8];
};

struct FILTER_RESULTS
{
	FILTER_RESULTS() { memset(this, 0x0, sizeof(FILTER_RESULTS)); }

	double gap_filt1;
	double vel_raw;
	double gap_predict1;
	double pos_predict;
	double dummy8[8 - 4];
};

struct LASER_POS
{
	LASER_POS() { Reset(); }
	void Reset(){ memset(this, 0x0, sizeof(LASER_POS)); gap_raw = gap_filt1 = vel_raw = vel_filt = FLT_MAX; }

	LASER_MEASURES measures;
	CDoublePoint manoeuvre1;
	CDoublePoint manoeuvre2;
	double  pid_error[3];
	double  pid_steering;
	double  gap_raw;	// distance to the weld (mm)
	double  gap_filt1;	// distance to the weld (mm)
	double  vel_raw;
	double  vel_filt;
	double  last_manoeuvre_pos;
	double  gap_predict1;
	double  pos_predict;
	double  diff_slope;
	double dummy8[16 - 12];
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
	CWeldNavigation(CMotionControl&, CLaserControl&, NAVIGATION_PID& pid, CArray<double, double>& data, const CString& szProject);
	~CWeldNavigation();
	void	Init(CWnd*, UINT);
	BOOL	NoteNextLaserPosition();
	LASER_POS GetLastNotedPosition(int ago_mm);
	void    StartNavigation(int nSteer, BOOL bAborted, int start_pos, int end_pos, double speed, double accel, double offset, BOOL bScanning);
	UINT    ThreadSteerMotors();
	UINT    ThreadNoteLaser();
	void    GetCopyOfOffsetList(CArray<LASER_POS, LASER_POS>&);
	BOOL	IsNavigating()const;
	void	StopSteeringMotors();

private:
	UINT    ThreadSteerMotors_try3();
	UINT    ThreadSteerMotors_PID();	
	UINT    ThreadSteerMotors_Test();
	BOOL	DriveTest(double rate, double pivot, int delay);
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
	double	CalculateTurnRate(double steering, double pos)const;
	FILTER_RESULTS LowPassFilterGap(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction);
	double LowPassFilterDiff(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction);
	void	CalculatePID_Navigation(const CArray<double, double>& Y, CArray<double, double>& out);
	double	AccumulateError(int distance);
	int		RemoveOutliers(double X[], double Y[], int order, int count, int max_sd);
	BOOL	GetGapCoefficients(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction, double coeff[3], int order, int source, int nMinWidth, int nMaxWidth);
	double  CalcGapVelocity(const CArray<LASER_POS, LASER_POS >& buff1, double last_manoeuvre_pos, int direction);
	LASER_MEASURES GetLaserSimulation();

	CMotionControl& m_motionControl;
	CLaserControl&	m_laserControl;
	NAVIGATION_PID&	m_pid;

	CArray<LASER_POS, LASER_POS> m_listLaserPositions;
	CMutex m_mutex1;
	BOOL m_bSteerMotors;
	HANDLE m_hThreadSteerMotors;
	HANDLE m_hThreadNoteLaser;
	double m_fMotorSpeed;
	double m_fMotorAccel;
	double m_fWeldOffset;
	BOOL	m_bScanning;
	int		m_nStartPos;
	double	m_nInitPos;
	int m_nEndPos;
	CWnd* m_pParent;
	UINT m_nMsg;
	const CString& m_szProject;
	CArray<double, double>& m_fft_data;
	FILE* m_fpSimulation;


	double m_i_error;
	double m_p_error;
	double m_d_error;
	int    m_i_time;
};

