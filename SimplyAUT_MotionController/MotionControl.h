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
	double GetMotorPosition(GCStringIn axis);
	BOOL SetMotorJogging(double speedA, double speedB, double speedC, double speedD, double accel);
	void SendDebugMessage(CString);
	void EnableControls();
	void StopMotors();
	double EncoderCountToDistancePerSecond(int encoderCount)const;
	int DistancePerSecondToEncoderCount(double DistancePerSecond)const;
	int    AxisDirection(GCStringIn axis)const;
	void   ZeroPositions();
	void   GoToPosition(double pos);
	BOOL   SteerMotors(BOOL bRight, BOOL bDown);
	void   SetSlewSpeed(double speed);
	void   GoToHomePosition();

	Gclib* m_pGclib;
	CWnd* m_pParent;
	UINT  m_nMsg;
};
