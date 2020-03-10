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
	BOOL SetMotorSpeed(double speed, double accel);
	BOOL AreMotorsRunning();
	double GetMotorSpeed(GCStringIn axis, double& rAccel);
	double GetMotorPosition(GCStringIn axis);
	BOOL SetMotorSpeed(double speedA, double speedB, double speedC, double speedD, double accel);
	void SendDebugMessage(CString);
	void EnableControls();
	void StopMotors();
	void RunTest1();
	void RunTest2();
	double EncoderCountToDistancePerSecond(double encoderCount)const;
	double DistancePerSecondToEncoderCount(double DistancePerSecond)const;
	int    AxisDirection(GCStringIn axis)const;
	void   ZeroPositions();
	void   GoToHomePosition();
	BOOL   SteerMotors(BOOL bRight, BOOL bDown);

	Gclib* m_pGclib;
	CWnd* m_pParent;
	UINT  m_nMsg;
};

