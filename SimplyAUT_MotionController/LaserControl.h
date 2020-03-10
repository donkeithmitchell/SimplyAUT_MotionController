#pragma once

#include "slsdef.h"

struct LASER_TEMPERATURE
{
	double BoardTemperature;
	double SensorTemperature;
	double LaserTemperature;
};

class CLaserControl
{
public:
	CLaserControl();
	~CLaserControl();

	void Init(CWnd* pParent, UINT nMsg);
	BOOL IsConnected()const;
	BOOL IsLaserOn();
	BOOL Disconnect();
	BOOL Connect(const BYTE address[4]);
	void SendDebugMessage(CString);
	void EnableControls();
	BOOL TurnLaserOn(BOOL);
	BOOL GetLaserTemperature(LASER_TEMPERATURE&);
	BOOL GetLaserMeasurment(Measurement&);
	BOOL SetAutoLaserCheck(BOOL);
	BOOL SetLaserIntensity(int nLaserPower);
	BOOL SetCameraShutter(int nCameraShutter);
	BOOL SetCameraRoi();
	BOOL GetProfile(Profile* pProfile);
	BOOL GetProfilemm(Profilemm* pProfile, int hit_no);
	void Frm_Find_Sensors();

	int		m_nLaserPower;
	int		m_nCameraShutter;
	CWnd* m_pParent;
	UINT  m_nMsg;
};

