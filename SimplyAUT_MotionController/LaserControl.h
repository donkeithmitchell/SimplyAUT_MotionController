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
	double GetDnSideWeldHeight()const { return ds_coeff[2] * weld_left_pix * weld_left_pix + ds_coeff[1] * weld_left_pix + ds_coeff[0]; }
	double GetDnSideStartHeight()const { return ds_coeff[2] * weld_left_start_pix * weld_left_start_pix + ds_coeff[1] * weld_left_start_pix + ds_coeff[0]; }
	double GetUpSideWeldHeight()const { return us_coeff[2] * weld_right_pix * weld_right_pix + us_coeff[1] * weld_right_pix + us_coeff[0]; }
	double GetUpSideEndHeight()const { return us_coeff[2] * weld_right_end_pix * weld_right_end_pix + us_coeff[1] * weld_right_end_pix + us_coeff[0]; }

	CDoublePoint weld_cap_pix1; // from F/W
	CDoublePoint weld_cap_pix2; // from S/W
	CDoublePoint weld_cap_mm;
	CDoublePoint dummy16[4 - 3];

	double wheel_velocity4[4];
	double cap_profile_mm[21];
	double us_coeff[3];
	double ds_coeff[3];
	double measure_pos_mm; // position that was at when took measure
	double weld_left_mm;
	double weld_right_mm;
	double weld_left_height_mm;
	double weld_right_height_mm;
	double weld_left_start_mm;
	double weld_right_end_mm;
	double dummy8[42 - 38];

	int weld_left_pix;
	int weld_right_pix;
	int weld_left_start_pix;
	int weld_right_end_pix;
	int status;
	int rgb_sum;
	clock_t measure_tim_ms;
	int dummy4[8 - 7];
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
	void SendErrorMessage(const char*, int=0);
	void EnableControls();
	BOOL TurnLaserOn(BOOL);
	BOOL GetLaserTemperature(LASER_TEMPERATURE&);
	BOOL GetLaserStatus(SensorStatus& SensorStatus);
	BOOL GetLaserMeasurment(Measurement&);
	BOOL CalcLaserMeasures_old(LASER_MEASURES&);
	int  CalcLaserMeasures(double pos, const double velocity[4], int last_cap_pix);
	BOOL SetLaserMeasurment(const Measurement&);
	BOOL SetAutoLaserCheck(BOOL);
	BOOL SetLaserIntensity(int nLaserPower);
	BOOL SetCameraShutter(int nCameraShutter);
	BOOL GetProfile();
	BOOL GetProfilemm(Profilemm* pProfile, int hit_no);
	void Frm_Find_Sensors();
	int  GetSerialNumber();
	BOOL GetCameraRoi(CRect& rect);
	BOOL SetCameraRoi(const CRect& rect);
	BOOL SetLaserOptions(int opt);
	BOOL SetLaserOptions(int opt1, int opt2, int opt3, int opt4);
	BOOL GetLaserVersion(unsigned short& major, unsigned short& minor);
	BOOL ConvPixelToMm(int row, int col, double& sw, double& hw);
	LASER_MEASURES GetLaserMeasures2();
	void GetLaserHits(MT_Hits_Pos hits[], double hitBuffer[], int nSize);
	void SetLaserMeasures2(const LASER_MEASURES& meas);

	CArray<double, double> m_polyX;
	CArray<double, double> m_polyY;
	CArray<double, double> m_work_buffer1;
	CArray<double, double> m_work_buffer2;
	CArray<double, double> m_hitBuffer;

	CIIR_Filter		m_filter;

	Measurement		m_measure1;
	LASER_MEASURES	m_measure2;
	Profile			m_profile;
	int				m_nLaserPower;
	int				m_nCameraShutter;
	CWnd*			m_pParent;
	UINT			m_nMsg;
	BOOL			m_bFW_Gap;
	BOOL			m_bFW_Weld;
};

