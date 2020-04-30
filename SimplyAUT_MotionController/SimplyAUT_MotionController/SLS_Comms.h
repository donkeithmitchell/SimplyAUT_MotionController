#include "SLSDef.h"

typedef int (__stdcall *AppCallbackFn)(int val);

extern "C"
{	

	__declspec(dllimport) void BeginNetworkThread(char *ip, int available_host_port, AppCallbackFn cbf);
	__declspec(dllimport) void StopNetworkThread();

	__declspec(dllimport) void SLSGetSensorVersion(void);
	__declspec(dllimport) void DLLGetSensorVersion(unsigned short *CSgbSensorVerH, unsigned short *CSgbSensorVerL);

	__declspec(dllimport) void SLSGetCalibration();
	__declspec(dllimport) void DLLGetCalibration(SlsCalibration		*Calibration);

	__declspec(dllimport) void SLSGetSeamList(void);
	__declspec(dllimport) void DLLGetSeamList(SeamList *CSgbSeamList);

	__declspec(dllimport) void SLSGetSensorStatus(void);
	__declspec(dllimport) void DLLGetSensorStatus(SensorStatus *CSgbSensorStatus);

	__declspec(dllimport) void SLSLoadSeamData(unsigned short SeamId);
	__declspec(dllimport) void DLLLoadSeamData(SeamData *CSgbSeamData);

	__declspec(dllimport) void DLLSaveSeam(char *file_name, int fname_length, unsigned short seam_id);
	__declspec(dllimport) void DLLRestoreSeam(char *file_name, int fname_length, unsigned short seam_id);

	__declspec(dllimport) void SLSDelSeamData(unsigned short SeamId);

	__declspec(dllimport) void DLLFullSysBackup(char *file_name);

	__declspec(dllimport) void ConvPixelToMm(double row, double col, double *sw, double *hw);

	__declspec(dllimport) void LaserOn(BOOL bLaser);
	__declspec(dllimport) void MsgMeasurementSending(BOOL bStart);
	__declspec(dllimport) void MsgMeasurementSending_Mm(BOOL bStart);
	__declspec(dllimport) void Profile_Sending(BOOL bStart) ;

	__declspec(dllimport) bool DLLGetMeasurement(Measurement	*CSgbMeasurement);
	__declspec(dllimport) bool DLLGetMeasurementMm(Measurement_Mm	*CSgbMeasurement_Mm);

	__declspec(dllimport) bool DLLGetProfile(Profile *CSProfile);
	__declspec(dllimport) bool DLLGetProfilemm(Profilemm *CSProfilemm, int hit_no);
	__declspec(dllimport) void DLLGetRawVideo(char* video_bmp);
	__declspec(dllimport) void Video_Sending(BOOL bStart) ;

	
	__declspec(dllimport) void DLLStartProfileSaving(char *dir_name, int dname_length, 
													 unsigned int NumOfFilesReq, unsigned short interval);
	__declspec(dllimport) void DLLStopProfileSaving();
	__declspec(dllimport) void DLLGetHistogramProfileVal(double *CSgbHistogramX, double *CSgbHistogramY);

	__declspec(dllimport) void SetCameraShutter(unsigned short val) ;
	__declspec(dllimport) void SetLaserIntensity(unsigned short val1, unsigned short val2, 
												 unsigned short val3, unsigned short val4) ;
	__declspec(dllimport) void SLSSetLaserOptions(unsigned short val1, unsigned short val2, 
												 unsigned short val3, unsigned short val4) ;

	__declspec(dllimport) void SLSSetCameraRoi( unsigned short x1, unsigned short y1, 
												unsigned short x2, unsigned short y2) ;


}
