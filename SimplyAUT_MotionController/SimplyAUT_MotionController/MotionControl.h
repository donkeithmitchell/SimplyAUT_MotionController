#pragma once
#include "Gclib2.h"

class CMotionControl
{
public:
	CMotionControl();
	~CMotionControl();

	void Init(CWnd* pParent, UINT nMsg);
	BOOL IsConnected()const;
	BOOL Disconnect();
	BOOL Connect(const BYTE address[4], double dScanSpeed);
	BOOL SetMotorJogging(double speed, double accel);
	BOOL AreMotorsRunning();
	double GetMotorSpeed(GCStringIn axis, double& rAccel);
	void   NoteIfMotorsRunning();
	double GetMotorPosition(GCStringIn axis);
	BOOL SetMotorJogging(double speedA, double speedB, double speedC, double speedD, double accel);
	void SendDebugMessage(const CString&);
	void SendErrorMessage(const char*);
	void EnableControls();
	void StopMotors(BOOL);
	BOOL WaitForMotorsToStop();
	double EncoderCountToDistancePerSecond(int encoderCount)const;
	int DistancePerSecondToEncoderCount(double DistancePerSecond)const;
	int    AxisDirection(GCStringIn axis)const;
	void   DefinePositions(double);
	BOOL   GoToPosition(double pos, BOOL bWait);
	BOOL   GoToPosition2(double left, double right);
	BOOL   SteerMotors(double fSpeed, double rate);
	BOOL   SetSlewSpeed(double A, double B, double C, double D);
	BOOL	SetSlewDeceleration(double decel_mm_sec_sec);
	BOOL   SetSlewSpeed(double speed, double accel);
	void   GoToHomePosition();
	double GetAvgMotorPosition();
	double GetAvgMotorSpeed();
	double GetLastManoeuvrePosition();
	void   ResetLastManoeuvrePosition();
	void   SetLastManoeuvrePosition();
	int    GetDestinationPosition()const {return m_nGotoPosition;	}
	BOOL   AreTheMotorsRunning();

private:
	int		m_nGotoPosition;
	BOOL	m_bMotorsRunning;
	CCriticalSection m_critLastManoeuvre;
	CCriticalSection m_critMotorsRunning;
	double m_manoeuvre_pos;
	Gclib* m_pGclib;
	CWnd* m_pParent;
	UINT  m_nMsg;
};

