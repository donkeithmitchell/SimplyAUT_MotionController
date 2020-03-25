#pragma once

#include "slsdef.h"
#include "misc.h"

struct LASER_TEMPERATURE
{
	double BoardTemperature;
	double SensorTemperature;
	double LaserTemperature;
};

struct LASER_MEASURES
{
	LASER_MEASURES() { memset(this, 0x0, sizeof(LASER_MEASURES)); status = -1; }
	double GetDnSideWeldHeight()const { return ds_coeff[1] * weld_right + ds_coeff[0]; }
	double GetDnSideStartHeight()const { return ds_coeff[1] * weld_left/2 + ds_coeff[0]; }
	double GetUpSideWeldHeight()const { return us_coeff[1] * weld_right + us_coeff[0]; }
	double GetUpSideEndHeight()const { return us_coeff[1] * (weld_right+SENSOR_WIDTH)/2 + us_coeff[0]; }

	CDoublePoint weld_cap_pix;
	CDoublePoint weld_cap_mm;
	double us_coeff[2];
	double ds_coeff[2];
	int weld_left;
	int weld_right;
	int status;
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
	void SendDebugMessage(const CString&);
	void SendErrorMessage(const CString&);
	void EnableControls();
	BOOL TurnLaserOn(BOOL);
	BOOL GetLaserTemperature(LASER_TEMPERATURE&);
	BOOL GetLaserStatus(SensorStatus& SensorStatus);
	BOOL GetLaserMeasurment(Measurement&);
	BOOL CalcLaserMeasures_old(const Hits[], LASER_MEASURES&);
	BOOL CalcLaserMeasures(const Hits[], LASER_MEASURES&);
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

	double m_polyX[SENSOR_WIDTH];
	double m_polyY[SENSOR_WIDTH];

	int		m_nLaserPower;
	int		m_nCameraShutter;
	CWnd* m_pParent;
	UINT  m_nMsg;
};

