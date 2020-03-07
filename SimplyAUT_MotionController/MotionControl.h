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
	BOOL SetScanSpeed(double speed);
	BOOL SetScanSpeed(double speedA, double speedB, double speedC, double speedD);
	void SendDebugMessage(CString);
	void EnableControls();
	void StopScanning();
	void RunTest1();
	void RunTest2();

	Gclib* m_pGclib;
	CWnd* m_pParent;
	UINT  m_nMsg;
};

