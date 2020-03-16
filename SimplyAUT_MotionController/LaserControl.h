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
	BOOL GetLaserStatus(SensorStatus& SensorStatus);
	BOOL GetLaserMeasurment(Measurement&);
	BOOL SetLaserMeasurment(const Measurement&);
	BOOL SetAutoLaserCheck(BOOL);
	BOOL SetLaserIntensity(int nLaserPower);
	BOOL SetCameraShutter(int nCameraShutter);
	BOOL GetProfile(Profile& profile);
	BOOL GetProfilemm(Profilemm* pProfile, int hit_no);
	void Frm_Find_Sensors();
	int  GetSerialNumber();
	BOOL GetCameraRoi(CRect& rect);
	BOOL SetCameraRoi(const CRect& rect);
	BOOL SetLaserOptions(int opt);
	BOOL SetLaserOptions(int opt1, int opt2, int opt3, int opt4);
	BOOL GetLaserVersion(unsigned short& major, unsigned short& minor);
	BOOL ConvPixelToMm(int row, int col, double& sw, double& hw);

	int		m_nLaserPower;
	int		m_nCameraShutter;
	CWnd* m_pParent;
	UINT  m_nMsg;
};

