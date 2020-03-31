#pragma once

#include "slsdef.h"
#include "misc.h"
#include "Filter.h"

struct LASER_TEMPERATURE
{
	double BoardTemperature;
	double SensorTemperature;
	double LaserTemperature;
};

struct LASER_MEASURES
{
	LASER_MEASURES() { memset(this, 0x0, sizeof(LASER_MEASURES)); status = -1; }
	double GetDnSideWeldHeight()const { return ds_coeff[1] * weld_left + ds_coeff[0]; }
	double GetDnSideStartHeight()const { return ds_coeff[1] * weld_left_start + ds_coeff[0]; }
	double GetUpSideWeldHeight()const { return us_coeff[1] * weld_right + us_coeff[0]; }
	double GetUpSideEndHeight()const { return us_coeff[1] * weld_right_end + us_coeff[0]; }

	CDoublePoint weld_cap_pix1; // from F/W
	CDoublePoint weld_cap_pix2; // from S/W
	CDoublePoint weld_cap_mm;
	double us_coeff[2];
	double ds_coeff[2];
	double measure_pos; // position that was at when took measure
	int weld_left;
	int weld_right;
	int weld_left_start;
	int weld_right_end;
	int status;
	int rgb_sum;
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
	BOOL CalcLaserMeasures_old(const Hits[], LASER_MEASURES&, double*);
	BOOL CalcLaserMeasures(const Hits[], LASER_MEASURES&, double pos, double*);
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

	CIIR_Filter m_filter;

	double  m_work_buffer1[SENSOR_WIDTH];
	double  m_work_buffer2[SENSOR_WIDTH];
	int		m_nLaserPower;
	int		m_nCameraShutter;
	CWnd* m_pParent;
	UINT  m_nMsg;
};

