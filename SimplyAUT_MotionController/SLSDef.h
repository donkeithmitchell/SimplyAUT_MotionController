//**************************************************************************************************
//	Project		: SLS(Smart Laser Sensor), Smart Laser Probe / Pilot
//
//	File		: SLSdef.h
//	Description	: SLS definition
//
//	Copyright (c) Meta Vision Systems. 2011
//--------------------------------------------------------------------------------------------------
//	[Revision History]
//		- 08/2009 : initial version
//**************************************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma pack(push,1)

//==================================================================================================
// INCLUDE
//==================================================================================================

#include "VA_CommonTypes.h"

//==================================================================================================
// DEFINITIONS
//==================================================================================================

//**************************************************************************************************
// FIRMWARE VERSION
//**************************************************************************************************
#define		SLS_HW_VER_H	1
#define		SLS_HW_VER_L	0

#define		SLS_SW_VER_H	1			// V1.0.0 - Initial Version
#define		SLS_SW_VER_M	0			// V1.0.1 - Fixed 3 Video Problems
#define		SLS_SW_VER_L	12			// V1.0.2 - added webpage feature in wiznet.c

// Imager resolution 1024 x 1280 = 1.3MPixel
#define	SENSOR_DEPTH	1
#define	SENSOR_WIDTH	1024
#define	SENSOR_HEIGHT	1280
#define	SENSOR_DHEIGHT	2560


#define	MSG_BUF_LENGTH	1500
#define UDP_MAX_LENGTH	1000


//////////////////////////////////////////////////////////////////////////////////////////////////
//	UINT8	UCHAR
//	UINT16	WORD
//	UINT32	DWORD
//	INT16	short int

// Declare basic data types used in TI for VC
#ifdef _MSC_VER
	typedef unsigned short Uint16;
#endif


//==================================================================================================
// Ethernet communications MESSAGES
//==================================================================================================
typedef enum {
													//   0 (0x000), UNKNOWN
	MSG_GET_SENSOR_VERSION			= 1,			//	 1 (0x001)
													//   2 (0x002), MSG_READ_SENSOR_TYPE
	MSG_SET_LASERS_OPTIONS			= 3,			//	 3 (0x003)
													//   4 (0x004), MSG_ECHO
	MSG_SET_LASERS_INTENSITY		= 5,			//	 5 (0x005)
	MSG_SET_CAMERA_SHUTTER,							//	 6 (0x006)
	MSG_SET_LASER_ON,								//	 7 (0x007), MSG_SET_LASER_ON
	MSG_SET_LASER_OFF,								//	 8 (0x008), MSG_SET_LASER_OFF
													//	 9 (0x009), MSG_SET_CAMERA_AE
	MSG_GET_CAMERA_REG				= 10,			//	10 (0x00A)
	MSG_SET_CAMERA_REG,								//	11 (0x00B)
	MSG_SET_CAMERA_ROI,								//	12 (0x00C)
	MSG_GET_BRIGHTNESS,								//  13 (0x00D)
	MSG_SET_BRIGHTNESS,								//  14 (0x00E)

	MSG_GET_SENSOR_STATUS			= 15,			//  15 (0x00F)
													//  16 (0x010), MSG_GET_ACTIVE_STATE
	MSG_SET_SENSOR_MODE             = 20,           //  20 (0x014), MSG_SET_SENSOR_MODE
	
	MSG_EEPROM_R_SYSTEM_DATA		= 30,			//  30 (0x01E)
	MSG_EEPROM_W_SYSTEM_DATA,						//  31 (0x01F)
	MSG_EEPROM_FACTORY_RESET,						//  32 (0x020)
	MSG_FLASH_R_SYSTEM_DATA,						//  33 (0x021)
	MSG_FLASH_W_SYSTEM_DATA,						//  34 (0x022)
	MSG_FLASH_FACTORY_RESET,						//	35 (0x023)
	
	MSG_SET_ANALYSIS_SENSITIVITY	= 39,			//	39 (0x027)

	MSG_SET_BASIC_SEAM_TYPE			= 40,			//  40 (0x028)
	MSG_SET_SEAM_DATA,								//  41 (0x029)
	MSG_LOAD_SEAM_DATA,								//  42 (0x02A)
	MSG_WRITE_SEAM_DATA,							//  43 (0x02B)
	MSG_DEL_SEAM_DATA,								//	44 (0x02C)
    MSG_SET_MEASUREMENT_LIMITS,						//	45 (0x02D)
	MSG_GET_MEASUREMENT_LIMITS,						//	46 (0x02E)
	MSG_SET_MEASUREMENT_OPTIONS,					//	47 (0x02F)
	MSG_GET_SEAM_LIST,								//	48 (0x030)
	MSG_SET_TEACH_POSITION,							//	49 (0x031)
	
	MSG_SET_CAN_OPTIONS				= 50,			//	50 (0x032)
	MSG_SET_CUR_TEACH_POSITION,						//	51 (0x033)
	MSG_START_TRACKING,								//	52 (0x034)
	MSG_STOP_TRACKING,								//	53 (0x035)
	MSG_GET_CUR_CORRECTIONS,						//	54 (0x036)
	MSG_APPLY_OFFSET,								//  55 (0x037)

	MSG_GET_FIRMWARE_VERSION		= 100,			// 100 (0x064)
	MSG_GET_VERSIONS				= 101,			// 101 (0x065)

	MSG_GET_MAIN_BD_TEMP			= 105,			// 105 (0x069)
	MSG_GET_SENSOR_TEMP,							// 106 (0x06A)
	MSG_GET_LASER_TEMP,								// 107 (0x06B)
	MSG_SET_LASER_HEATER_ON			= 108,			// 108 (0x06C)
	MSG_SET_LASER_HEATER_OFF		= 109,			// 109 (0x06D)
													// 110 (0x06E), MSG_CCD_FAST_MODE_ON
													// 111 (0x06F), MSG_CCD_FAST_MODE_OFF

													// 120 (0x078), MSG_SET_SCAN_RESOLUTION
													// 121 (0x079), MSG_GET_SCAN_RESOLUTION

													// 130 (0x082), MSG_SET_GALVO_POS
													// 131 (0x083), MSG_GET_GALVO_POS

	MSG_START_VIDEO_DATA_SENDING	= 144,			// 144 (0x090)
	MSG_STOP_VIDEO_DATA_SENDING,					// 145 (0x091)
	MSG_START_PROFILE_SENDING,						// 146 (0x092)
	MSG_STOP_PROFILE_SENDING,						// 147 (0x093)
	MSG_START_MEASUREMENT_SENDING,					// 148 (0x094)
	MSG_STOP_MEASUREMENT_SENDING,					// 149 (0x095)
    MSG_START_MEASUREMENT_SENDING_IN_MM,			// 150 (0x096)
	MSG_STOP_MEASUREMENT_SENDING_IN_MM,				// 151 (0x097)
	MSG_GIO_START_PROFILE_SENDING,					// 152 (0x098)
	MSG_GIO_STOP_PROFILE_SENDING,					// 153 (0x099)
	MSG_START_NEW_PROFILE_SENDING,					// 154 (0x09A)
	MSG_STOP_NEW_PROFILE_SENDING,					// 155 (0x09B)
	MSG_START_SEGMENTS_SENDING,						// 156 (0x09C)
	MSG_STOP_SEGMENTS_SENDING,						// 157 (0x09D)
	MSG_START_SEGMENTED_PROFILE_SENDING,			// 158 (0x09E)
	MSG_STOP_SEGMENTED_PROFILE_SENDING,				// 159 (0x09F)
													
	MSG_GIO_GET_SYS_SETTINGS		= 180,			// 180 (0x0B4),													 
	MSG_GIO_SET_SYS_SETTINGS,						// 181 (0x0B5),
	MSG_GIO_GET_SEAM_SETTINGS,						// 182 (0x0B6), 
	MSG_GIO_SET_SEAM_SETTINGS,						// 183 (0x0B7),

													// 199 (0x0C7), 
	MSG_GET_EIO_VERSION				= 200,			// 200 (0x0C8), MSG_GET_EIO_VERSION
	MSG_GET_EIO_INPUTS,								// 201 (0x0C9), MSG_GET_EIO_INPUTS
	MSG_SET_EIO_OUTPUTS,							// 202 (0x0CA), MSG_SET_EIO_OUTPUTS
	MSG_GET_EIO_SETUP,								// 203 (0x0CB), MSG_GET_EIO_SETUP
	MSG_SET_EIO_SETUP,								// 204 (0x0CC), MSG_SET_EIO_SETUP
											
	MSG_UPDATE_FIRMWARE				= 250,
	MSG_WRITE_FIRMWARE,											

	MSG_ERROR_REPORT				= 255			// 255 (0x0FF)
} SlsMessages;

//////////////////////////////////////////////////////////////////////////////////////////////////
// DATA STRUCTURE FOR THE MESSAGES

// MSG_GET_SENSOR_VERSION
typedef struct {
	unsigned short			major;
	unsigned short			minor;
} SensorVersion;

// MSG_SET_LASERS_OPTIONS
typedef struct {
	unsigned short			opt[4];
} LaserOption;

// MSG_SET_LASERS_INTENSITY
typedef struct {
	unsigned short			val[4];
} LaserInt;

// MSG_SET_CAMERA_SHUTTER
typedef struct {
	unsigned short			shutter;
} CameraShutter;

// MSG_GET_CAMERA_REG, MSG_SET_CAMERA_REG
typedef struct {
	unsigned short			addr;
	unsigned short			data;
} CameraReg;

// MSG_SET_CAMERA_ROI
typedef struct {
	unsigned short			x1;
	unsigned short			y1;
	unsigned short			x2;
	unsigned short			y2;
	unsigned short			sc;
	unsigned short			sr;
} CameraRoi;
//MSG_GET_BRIGHTNESS 
//MSG_SET_BRIGHTNESS
typedef struct {
	unsigned short			SeamId;
	unsigned char			LaserIntensity[2];
	unsigned char			CameraShutter;
	unsigned char			ControlOptions[2];
	unsigned char			CameraGain;
	unsigned char			Spare[4];
}BrightnessControl;

//MSG_SET_SENSOR_MODE 
//MSG_SET_SENSOR_MODE
typedef struct {
	unsigned short			OperationMode;	
	unsigned short			RunMode;	
	unsigned short			Spare[64];
}SensorMode;

//MSG_START_TRACKING 
//MSG_START_TRACKING Generic Interface
typedef struct {
	unsigned short			RunMode;	
	unsigned short			Spare[64];
}GenericMode;

//MSG_SET_TEACH_POSITION 
//MSG_SET_TEACH_POSITION
typedef struct {
	short					teachPos_y;							// Y Teach Position (mm x 100)
	short					teachPos_z;							// Z Teach Position (mm x 100)
}TeachPosition;

//MSG_APPLY_OFFSET 
//MSG_APPLY_OFFSET
typedef struct {
	char					horizontal_offset;					// offset = 0 (none), 1 (left), 2 (right)
	char					vertical_offset;					// offset = 0 (none), 1 (up), 2 (down)
}ApplyOffset;

//MSG_SET_ANALYSIS_SENSITIVITY 
//MSG_SET_ANALYSIS_SENSITIVITY
typedef struct {
	float					SliderValues[64];
	float					Spare1[64];
}AnalysisSensitivity;

// MSG_GET_SENSOR_STATUS
typedef struct {
	unsigned short					Mode;
	unsigned short					ErrorMode;
	unsigned short					spare1[15];
	unsigned short					MainBrdTemp;	
	unsigned short					PsuBrdTemp;	
	unsigned short					LaserTemp;	
	unsigned short					HeaterStatus;	
	unsigned short					spare2[16];
	unsigned short					CameraGain;
	unsigned short					CameraShutter;
	unsigned short					CameraOptions;
	unsigned short					roi_x1;
	unsigned short					roi_y1;
	unsigned short					roi_x2;
	unsigned short					roi_y2;
	unsigned short					spare3[16];
	unsigned short					LaserStatus;
	char							LaserIntensity[4];
	char							LaserOptions[4];
	unsigned short					spare4[16];
	unsigned short					SeamId;
	unsigned short					SeamType;
	char							SeamName[40];
	unsigned short					spare5[64];
	unsigned short					CAN_Status;
	// EIO Status
	// EIO Inputs
	unsigned short					EioIdentificantionIn;
	unsigned short					EioControlIn;
	unsigned short					DigitalIn1;
	unsigned short					DigitalIn2;
	unsigned short					Pen1Bts;
	unsigned short					Pen2Bts;
	unsigned short					AnalogIn1;
	unsigned short					AnalogIn2;
	unsigned int					IncEncoder1;							// Incremental Encoder 1
	unsigned int					IncEncoder2;							// Incremental Encoder 2
	unsigned int					IncEncoder3;							// Incremental Encoder 3
	unsigned int					IncEncoder4;							// Incremental Encoder 4
	unsigned int					SsiEncoder1;							// SSI Encoder 1
	unsigned int					SsiEncoder2;							// SSI Encoder 2
	unsigned int					SsiEncoder3;							// SSI Encoder 3
	unsigned int					SsiEncoder4;							// SSI Encoder 4
	unsigned short					spare6[40];
	// EIO Outputs
	unsigned short					EioIdentificantionOut;
	unsigned short					EioControlOut;				
	unsigned short					DigitalOut;
	unsigned short					RelayOut;
	unsigned short					Pen1Lamps;
	unsigned short					Pen2Lamps;
	unsigned short					AnalogOut1;
	unsigned short					AnalogOut2;
	unsigned short					AnalogOut3;
	unsigned short					AnalogOut4;
	unsigned short					spare7[52];
	int								weld_speed;
}SensorStatus;

// MSG_EEPROM_R_SYSTEM_DATA, MSG_EEPROM_W_SYSTEM_DATA
typedef struct {
	unsigned short			seEepromID;				// EEPROM ID
	unsigned short			seDBVerH;				// System Data DB Version(H)
	unsigned short			seDBVerL;				// System Data DB Version(L)
	unsigned short			seDBCheckSum;			// System Data DB Check Sum
	unsigned short			seDBSize;				// System Data DB Size
	char					seName[40];				// System Name
	char					seSN[8];				// Serial Number
	unsigned char			seMAC[6];				// MAC Address
	unsigned char			seIP[6];				// IP Address
	unsigned char			seLaserNum;				// Number of laser stripes
	unsigned char			sePeakNum;				// Number of peaks detected by sensor head
	unsigned short			seFieldOfView;			// field of view, 0.1mm unit
	unsigned short			seDepthOfField;			// depth of field, 0.1mm unit
	unsigned short			seCamResH;				// Camera horizontal resolution
	unsigned short			seCamResV;				// Camera vertical resolution
	char					seLaserWL[2][4];		// Laser's wavelength
	unsigned char			seProductionW;			// Week of production
	unsigned char			seProductionY;			// Year of production
	unsigned char			seServiceW;				// Week of the last service
	unsigned char			seServiceY;				// Year of the last service
	unsigned short			seOpTempMax;			// Maximum working temperature (oC x 100 + 10,000)
	unsigned short			seOpTempMin;			// Minimum			"		   (oC x 100 + 10,000)
	unsigned short			seHeaterOnTemp;			// Heater turn-on temperature  (oC x 100 + 10,000)
	unsigned short			seHeaterOffTemp;		// Heater turn-off 		"	   (oC x 100 + 10,000)
	unsigned short			seSpare[128];
} SysE2promData;

// MSG_FLASH_R_SYSTEM_DATA, MSG_FLASH_W_SYSTEM_DATA
typedef struct {
	unsigned short			sfFlashID;				// System Data FLASH ID for FLASH FILE SYSTEM
	unsigned short			sfBST;					// Basic Seam Type
	unsigned short			sfCurrSeamID;			// Current Seam ID
	unsigned short			sfLang;					// Language
	unsigned int			sfOptions;				// System Options, Bit operation
	unsigned short			sfUnit;					// Type of unit, mm/inch/pixel
	unsigned short			sfTypeMajor;			//	Major Version Number
	unsigned short			sfTypeMinor;			//	Minor Version Number
	unsigned short			sfLookAhead;			// mm x 100 (35mm = 3,500)
	short					sfHeadAngle;			// angle x 100 (signed)
	unsigned short			sfSpare3[2];			// Spare 3 (Removed sfTackLength and sfTrackWindow)
	
	// System Information (Moved from EEPROM)
	unsigned short			seDBVerH;				// System Data DB Version(H)
	unsigned short			seDBVerL;				// System Data DB Version(L)
	char					seSN[8];				// Serial Number
	unsigned char			seProductionW;			// Week of production
	unsigned char			seProductionY;			// Year of production
	unsigned char			seServiceW;				// Week of the last service
	unsigned char			seServiceY;				// Year of the last service
	unsigned short			seLaserWL1;				// Laser's wavelength
	unsigned short			seLaserWL2;				// Laser's wavelength
	unsigned char			seLaserNum;				// Number of laser stripes
	unsigned char			sePeakNum;				// Number of peaks detected by sensor head
	unsigned short			seFieldOfView;			// field of view, 0.1mm unit
	unsigned short			seDepthOfField;			// depth of field, 0.1mm unit
	unsigned short			seOpTempMax;			// Maximum working temperature (oC x 100 + 10,000)
	unsigned short			seOpTempMin;			// Minimum			"		   (oC x 100 + 10,000)
	unsigned short			seHeaterOnTemp;			// Heater turn-on temperature  (oC x 100 + 10,000)
	unsigned short			seHeaterOffTemp;		// Heater turn-off 		"	   (oC x 100 + 10,000)
	unsigned char			seMAC[6];				// MAC Address
	char					seName[40];				// System Name
	unsigned char			seIP[6];				// IP Address	
	// End of System Information (Moved from EEPROM)
	
	// Offset Y and Z
	short					sfOffsetY;				// Offset Y
	short					sfOffsetZ;				// Offset Z
	unsigned short			sfCanOptions;			// CAN Baud Rate Option		(0 - Inspection, 1 - Tracking)

	// Newly added for SLP systems
	unsigned char			sfPendt_EnableStartStop;// Pendant Enable / Disable Start and Stop Button
	unsigned char			sfPendt_EnableTeach;	// Pendant Enable / Disable Teach Button
	unsigned char			sfPendt_EnableSeamChange;// Pendant Enable / Disable Seam Number option
	unsigned char			sfPendt_EnableOffset;	// Pendant Enable / Disable Offset Option
	unsigned short			sfSlide_SpeedLR;		// Left / Right Slide Speed, mm x 100 / sec
	unsigned short			sfSlide_SpeedUD;		// UP / Down Slide Speed, mm x 100 / sec
	unsigned char			sfSlide_LimitsOpenLR;	// Left / Right Slide Limits Open / Closed 
	unsigned char			sfSlide_LimitsOpenUD;	// Up / Down Slide Limits Open / Closed 
	unsigned char			sfSlide_OnlyLR;			// Left / Right Slide only
	unsigned char			sfSlide_OnlyUD;			// Up / Down Slide only
	unsigned char			sfSlide_ReverseLR;		// Left / Right Slide in reverse mode
	unsigned char			sfSlide_ReverseUD;		// Up / Down Slide in reverse mode
	unsigned char			sfPendt_ReverseLRjog;	// Left / Right Slide in reverse jog mode
	unsigned char			sfSlide_ReverseUDjog;	// Up / Down Slide in reverse jog mode
	unsigned short			sfInterfaceType;		// System Type 1 - Laser Probe, 
													// 2 - MTF, 
													// 3 - Meta Tracking with speed info from robot
													// 4 - Meta Tracking with no speed info from robot
													// 5 - Robot Tracking
	// cooling turn on/off temperature
	unsigned short			sfCoolingOnTemp;		// Cooling turn-on temperature (oC x 100 + 10,000)
	unsigned short			sfCoolingOffTemp;		// Cooling turn-off       "	   (oC x 100 + 10,000)													
	unsigned short			sfPendt_MaxSeamNumber;	// Maximum seam number allowed to navigate through Pendant.
	unsigned short			sfSpare1[6];			// Spare 1

	// Sensor Calibration Data
	unsigned short			sfCalVer;				// Calibration Version
	// Calibration Coefficients
	float					sfCalX0;				// X0
	float					sfCalY0;				// Y0
	float					sfCalM;					// M
	float					sfCalK;					// K
	float					sfCalC;					// C
	float					sfCalE;					// E
	float					sfCalA;					// A
	float					sfCalL;					// L
	float					sfCalD;					// D
	float					sfCalF;					// F
	float					sfCalZX;				// ZX
	unsigned short			sfSpare2[64];			// Spare2	
} SysFlashData;

// MSG_SET_BASIC_SEAM_TYPE
typedef struct {
	unsigned short			bst;
} BasicSeamType;

// MSG_SET_SEAM_DATA, MSG_WRITE_SEAM_DATA
typedef struct {
	unsigned short			seam_id;
	unsigned short			data_size;
	char					seam_name[40];
	unsigned char			laser_int[2];
	unsigned char			laser_mode[2];
	unsigned short			spare2[2];
	unsigned short			cam_sht;
	unsigned short			spare3[5];
	unsigned int			cam_options;
	unsigned short			roi_x1;
	unsigned short			roi_y1;
	unsigned short			roi_x2;
	unsigned short			roi_y2;
	unsigned short			seam_type;
	unsigned char			filter_option;						// Filter Options (0 - No Filter, 1- Median Filter, 2 - Smoothing Filter for tracking, 3 - Continuity Filter for tracking)
	unsigned char			check_measurement_limits;			// Check the Measurement Limits (0 - Do not Check, 1 - Check)
	float					measurement_limit_min[16];			// Minimum Limit
	float					measurement_warning_min[16];		// Minimum Warning Limit
	float					measurement_warning_max[16];		// Maximum Warning Limit
	float					measurement_limit_max[16];			// Maximum Limit
	unsigned short			Offset_y;							// Offset Y
	unsigned short			Offset_z;							// Offset Z
	
	// Newly added for SLP
	short					teachPos_y;							// Y Teach Position (mm x 100)
	short					teachPos_z;							// Z Teach Position (mm x 100)
	int						weld_speed;							// Welding Speed (cm / sec)
	int						head_angle;							// Sensor Head angle 
	unsigned char			side_only;							// Side only correction
	unsigned char			height_only;						// Height only correction
	unsigned short			tack_length;						// weld Tack Length (mm x 100)
	int						track_accept_window;				// Tracking acceptance window (mm x 100)

	// Analog Input Weld Speed
	int						ana_weld_max_voltage;				// Analog Input: Weld max voltage
	int						ana_weld_min_voltage;				// Analog Input: Weld min voltage
	int						ana_weld_max_speed;					// Analog Input: Weld max speed
	int						ana_weld_min_speed;					// Analog Input: Weld min speed

	// Extended Search
	unsigned char			ext_search_enabled;					// Extended Search Enabled / Disabled
	unsigned char			ext_search_alt_direction;			// Extended search in Alternate direction enabled
	unsigned short			spare4;								// Spare 4
	int						ext_search_no_step;					// Number of stepds for extended search
	int						ext_search_step_size;				// Extended search step size
	int						ext_search_angle;					// Extended search Angle

	// Vertical Search
	unsigned char			vert_search_enabled;				// Vertical Search Enabled / Disabled
	unsigned char			spare5[3];							// Spare5
	int						vert_search_max_dist;				// Vertical search maximum distance

	// Extended Move Variables
	unsigned char			ext_move_enabled;					// Extended Move Enabled / Disabled
	unsigned char			spare6[3];							// Spare6
	signed char				ext_move[18];						// Extended Moves
	unsigned short			ext_move_delay;						// Extended move delay
	unsigned short			ext_move_speed[18];					// Extended move Speed					
	unsigned short			spare7[2];							// Spare7
	unsigned short			ext_move_distance[18];				// Extended move distance

	// Adaptive Welding and Tracking Algorithm
	unsigned char			tracking_algorithm;				// Tracking Algorithm.	
	unsigned char			tracking_smoothing_factor;		// Tracking Smoothing Factor.

	unsigned char			ana_adapWeld_enable;			// Analogue Adaptive Weld Enabled/Disabled.
	unsigned char			dig_adapWeld_enable;			// Digital Adaptive Weld Enabled/Disabled.
	short					ana_adapWeld_min_voltage[2];	// Analog Output: Adaptive Weld min voltage (voltage x 100)
	short					ana_adapWeld_max_voltage[2];	// Analog Output: Adaptive Weld max voltage (voltage x 100)
	float					ana_adapWeld_min_measurement[2];// Analogue Adaptive Weld Minimum Measurements.
	float					ana_adapWeld_max_measurement[2];// Analogue Adaptive Weld Maximum Measurements.
	float					dig_adapWeld_min_measurement[2];// Digital Adaptive Weld Minimum Measurements.
	float					dig_adapWeld_max_measurement[2];// Digital Adaptive Weld Maximum Measurements.

	
	unsigned char			continuity_filter_factor;	//Continuity Filter Factor for Tack
	
	//For setting ROI dynamically
	unsigned char			roi_auto_enable;		// ROI Auto enable
	unsigned char			topPosIndex;			// Top Measurement Position Index
	unsigned char			bottomPosIndex;			// Top Measurement Position Index
	unsigned char			leftPosIndex;			// Top Measurement Position Index
	unsigned char			rightPosIndex;			// Top Measurement Position Index	
	unsigned short			topWinSize;				// Window Size above the Top Measurement Position
	unsigned short			bottomWinSize;			// Window Size below to the Bottom Measurement Position 
	unsigned short			leftWinSize;			// Window Size left to the Left Measurement Position 
	unsigned short			rightWinSize;			// Window Size right to the Measurement Position
	
	unsigned short			ext_move_speed_special[6];		// Special Extended move Speed
	unsigned short			ext_move_distance_special[6];	// Special Extended move distance
	signed char				ext_move_special[6];			// Special Extended Moves
	unsigned short			spare10[256];					// Spare10
} SeamParameter;

typedef struct {
	SeamParameter			seam_param;
	unsigned int			step_num;
	MS_VisionAnalysisStep	vaSteps[70];
} SeamData;

typedef struct
{
	short 					multiSeg_ID;	// id of multi-segment structure to which the segment belong
	short 					spare1;
	float 					side_beg;		// side coordinate of the segment beginning
	float 					height_beg;		// height coordinate of the segment beginning
	float 					side_end;		// side coordinate of the segment end
	float 					height_end;		// height coordinate of the segment end
	Uint16					Spare2[128];	// Spare2
} SegLimits;

typedef struct {
	Uint16				totalSegsNumber;
	Uint16 				spare1;
	SegLimits			MultiSegEdges[32];
	Uint16				Spare2[128];		// Spare1
} AnalysisSegments;

// MSG_LOAD_SEAM_DATA, MSG_DEL_SEAM_DATA
typedef struct {
	unsigned short			id;
} SeamInfo;

// MSG_SET_MEASUREMENT_LIMITS
typedef struct {
	unsigned short			seam_id;							// Seam Number
	unsigned short			seam_type;							// Seam Type
	float					measurement_limit_min[16];			// Minimum Limit
	float					measurement_warning_min[16];		// Minimum Warning Limit
	float					measurement_warning_max[16];		// Maximum Warning Limit
	float					measurement_limit_max[16];			// Maximum Limit
	unsigned short			spare[512];
}SensorMeasurementLimits;

// MSG_GET_FIRMWARE_VERSION
typedef struct {
	unsigned short			firmware_major;
	unsigned short			firmware_minor;
	unsigned short			firmware_patch;
	unsigned short			va_major;		// id-s different types of sensor applications
	unsigned short			va_minor;		// updated after each official s/w release is shipped to the customer
	unsigned short			va_update;		// updated after any changes in s/w
	unsigned short			va_dataBase;	// parameter structure id, can be used for automated updates of the sensor parameters
	unsigned short			spare[32];		// for future use
} FirmwareVersion;

// MSG_SET_MEASUREMENT_OPTIONS
typedef struct {
	unsigned short			seam_id;							// Seam Number
	unsigned short			seam_type;							// Seam Type
	unsigned short			filter_option;						// Filter Options (0 - No Filter, 1- Median Filter)
	unsigned short			check_measurement_limits;			// (0 - No Check, 1 -Apply Measurement Limits Check)
	unsigned short			spare[128];		
} MeasurementOptions;

//MSG_GET_SEAM_LIST
typedef struct {
	unsigned int			seam_list[8];
}SeamList;
// MSG_GET_MAIN_BD_TEMP
typedef struct {
	unsigned short			temp;
} MainBdTemp;

// MSG_GET_SENSOR_TEMP
typedef struct {
	unsigned short			temp;
} SensorTemp;

//	MSG_GET_LASER_TEMP
typedef struct {
	unsigned short			temp;
	unsigned short			HeaterStatus;
} LaserTemperature;

// MSG_SET_LASER_HEATER

typedef struct {
	unsigned short			Mode;
	unsigned short			Temp1;
	unsigned short			Temp2;
	unsigned short			Ontimer1;
	unsigned short			Offtimer1;
	unsigned short			Ontimer2;
	unsigned short			Offtimer2;
} HeaterTimer;

// MSG_START_VIDEO_DATA_SENDING
typedef struct {
	unsigned short			frame_num;
	unsigned short			line_num;
	char					pixel[SENSOR_DHEIGHT];
} VideoData;

// MSG_START_SEGMENTED_PROFILE_SENDING
typedef struct
{
	short 			offset;
	short 			size;
	short 			Buffer[SENSOR_WIDTH];
	unsigned short	roi_x1;
	unsigned short	roi_y1;
	unsigned short	roi_x2;
	unsigned short	roi_y2;
	unsigned short	spare1[124];		// Spare
} SegmentedProfile;

typedef struct {
	double x[SENSOR_WIDTH];
	double y[SENSOR_WIDTH];
	} Profilemm;

// MSG_START_SEGMENTED_PROFILE_SENDING
typedef struct
{
	short 			offset;
	short 			size;
	Profilemm       pos;
	unsigned short	roi_x1;
	unsigned short	roi_y1;
	unsigned short	roi_x2;
	unsigned short	roi_y2;
	unsigned short	spare1[124];		// Spare
} SegmentedProfilemm;

// MSG_START_MEASUREMENT_SENDING, MSG_STOP_MEASUREMENT_SENDING
typedef struct {
	float					x;
	float					y;
	int						status;
} MP;

typedef struct {
	float					val;
	int						status;
} MV;

typedef struct {
	MP						mp[16];
	MV						mv[16];
} Measurement;

// Vision Analysis status
typedef struct
{
	ME_Status				status;		// status code
	short					stepIdx;	// status step number - step responsible for the analysis status if the analysis fails, number of analysis steps otherwise
	short					spare1;
	ME_VisionAnalysisStepID	stepType;	// status step type
	char					spare[8];
} VA_Status;

//Measurements in mm
typedef struct {
	unsigned int			ts_fn;			// time stamp or frame number
	MP						mp[16];
	MV						mv[16];
	VA_Status				AnalysisStatus;
	short					head_angle;			// Sensor Head angle
	unsigned short			seam_param_flags;	// Seam Parameter flags
	unsigned short			spare[52];			// Spare
} Measurement_Mm;

//Tracking
typedef struct {
	float                   speed_y;
	float                   speed_z;
	float                   distance_y;
	float                   distance_z;
	unsigned int			ts_fn;			// time stamp or frame number
	MP						mp[8];
	MV						mv[8];
	unsigned short			spare[128];		// Spare
} TrackingCorrection;

//Set Teach Position Data
typedef struct { 
	float				teachPos_y;
	float				teachPos_z;
	unsigned short		error_code;
	unsigned short		spare[65];  // Spare
} GenericTeachResult;

// Measurement Status
typedef enum {
	ValidMeasurement		= 0,			// Valid Measurement
	NoMeasurement			= 1,			// Measurement not available for this seam type
	InvalidMeasurement		= 2,			// Invalid Measurement
	GoodMeasuremnet			= 3,			// Good Measurement
	CloseToLimits 			= 4,			// Measurement in Warning Range
	OutOfSpec				= 5				// Measurement is Out of Spec
}Measurement_Status;

// MSG_START_PROFILE_SENDING, MSG_STOP_PROFILE_SENDING
typedef short MT_Hits_Pos;
typedef short MT_Hits_Str;

typedef struct {
	MT_Hits_Pos					pos1;					// 1st hit's position data
	MT_Hits_Str					str1;					// 1st hit's strength data
	MT_Hits_Pos					pos2;					// 2nd			"
	MT_Hits_Str					str2;					// 2nd			"
	MT_Hits_Pos					pos3;					// 3rd			"
	MT_Hits_Str					str3;					// 3rd			"
	MT_Hits_Pos					pos4;					// 4th			"
	MT_Hits_Str					str4;					// 4th			"
	MT_Hits_Pos					pos5;					// 5th			"
	MT_Hits_Str					str5;					// 5th			"
	MT_Hits_Pos					pos6;					// 6th			"
	MT_Hits_Str					str6;					// 6th			"
	MT_Hits_Pos					pos7;					// 7th			"
	MT_Hits_Str					str7;					// 7th			"
	MT_Hits_Pos					pos8;					// 8th			"
	MT_Hits_Str					str8;					// 8th			"
} Hits;

typedef struct {
	unsigned int			ts_fn;					// time stamp or frame number
	unsigned short			roi_x1;
	unsigned short			roi_y1;
	unsigned short			roi_x2;
	unsigned short			roi_y2;
	Hits					hits[SENSOR_WIDTH];				// HIT DATA
	//Measurement				measurement;
} Profile;

// MSG_START_NEW_PROFILE_SENDING, MSG_STOP_NEW_PROFILE_SENDING
typedef struct {
	short					str1;					// 1st hit's strength data
	short					pos1;					// 1st hit's position data
	short					str2;					// 2nd			"
	short					pos2;					// 2nd			"
	short					str3;					// 3rd			"
	short					pos3;					// 3rd			"
	short					str4;					// 4th			"
	short					pos4;					// 4th			"
} NewHits;

//Profile Data
typedef struct {
	unsigned int    ts_fn;			// time stamp or frame number
	unsigned short	roi_x1;
	unsigned short	roi_y1;
	unsigned short	roi_x2;
	unsigned short	roi_y2;
	unsigned short	spare[64];
	NewHits			new_hits[SENSOR_WIDTH];		// HIT DATA
	//Measurement				measurement;
} NewProfile;

// MSG_ERROR_REPORT
typedef struct {
	int						err_code;
	int						nop;
} ErrorParamHeader;

typedef struct {
	ErrorParamHeader		header;
	float					param[256];
} ErrorParameters;

typedef struct {
	unsigned int			firmware_size;			// data length
	//unsigned int			firmware1[128000];		// firmware
	
} Firmware;

//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
	unsigned short			msg;				// SlsMessages
	unsigned short			data_size;			// data length
} SlsMsgHeader;

typedef union {
	SensorVersion			sensor_version;
	SensorMode              sensor_mode;
	GenericMode             generic_mode;	
	TeachPosition           teach_position;
	ApplyOffset             apply_offset;
	AnalysisSensitivity     analysis_sensitivity;
	LaserOption				laser_option;
	LaserInt				laser_int;
	CameraShutter			camera_shutter;
	CameraReg				camera_reg;
	CameraRoi				camera_roi;
	BrightnessControl		brightness_control;
	SensorStatus			sensor_status;
	SysE2promData			sys_e2prom_data;
	SysFlashData			sys_flash_data;
	BasicSeamType			basic_seam_type;
	SeamData				seam_data;
	SeamInfo				seam_info;
	SensorMeasurementLimits	sensor_measurement_limits;
	MeasurementOptions		measurement_options;
	SeamList				seam_list;
	FirmwareVersion			firmware_version;
	MainBdTemp				main_bd_temp;
	SensorTemp				sensor_temp;
	LaserTemperature		laser_temp;
	HeaterTimer				heater_timer;
	VideoData				video_data;
	Profile					profile;
	NewProfile				new_profile;
	SegmentedProfile		segmented_profile;
	Measurement				measurement;
	Measurement_Mm			measurement_mm;
	TrackingCorrection      tracking_correction;
	GenericTeachResult		generic_teach_result;
	Firmware				firmware;
	ErrorParameters			error_parameters;
	AnalysisSegments		analysis_segments;
	unsigned char			dummy[32780];
} SlsMsgData;

typedef struct {
	SlsMsgHeader			header;
	SlsMsgData				data;
} SlsMsgPacket;


typedef struct
{

	unsigned short			sfCalVer;				// Calibration Version
	float					sfCalX0;				// Calibration Coefficients
	float					sfCalY0;				// 				"
	float					sfCalM;					// 				"
	float					sfCalK;					// 				"
	float					sfCalC;					// 				"
	float					sfCalE;					// 				"
	float					sfCalA;					// 				"
	float					sfCalL;					// 				"
	float					sfCalD;					// 				"
	float					sfCalF;					// 				"
	float					sfCalZX;				// 				"
}SlsCalibration;

	//Full Backup
	typedef struct
	{
	char				systemtype[40];
	unsigned short		BackupFileVerH;
	unsigned short		BackupFileVerM;
	unsigned short		BackupFileVerL;
	unsigned short		spare[64];
	}BackupHeader;

	typedef struct
	{
	unsigned short		NumSeams;
	unsigned short		SeamDataVerH;
	unsigned short		SeamDataVerM;
	unsigned short		SeamDataVerL;
	unsigned short		spare[64];
	}SeamDataInfo;
	
	typedef struct
	{
	BackupHeader		header;
	SysE2promData		E2promData;
	SysFlashData		SysFlashData;
	SeamDataInfo		SeamInfo;
	}BackupFile;



	typedef struct
	{
	unsigned short		seam_id;
	unsigned short		parameter_status;
	unsigned short		seam_size;
	unsigned short		spare[64];
	}SeamDataHeader;
	
	typedef struct
	{
		float			min;
		float			max;
		float			min_warning;
		float			max_warning;
		unsigned short	spare[60];	
	
	}Limits;
	typedef struct
	{
		unsigned short	seamtype;
		Limits			ML[16];
		unsigned short  Tolerance;
		unsigned short	spare[64];
	}MeasurementLimits;


	typedef struct 
	{
		unsigned short					FilterType;				// Measurement Filtering Type (Median, ...)
		char							DefaultPath[260];		// Default Path to store the log files
		unsigned short					playspeed;				// Play speed for History Dialog
		unsigned short					lastsettingsno;			// Last Setting no used
		unsigned short					units;					// Units (mm, inches, pixel)
		char							LeftLabel[20];
		char							RightLabel[20];
		unsigned short					spare[492];
	}PcPreferences;

	typedef struct
	{
		int				Count;
		int				FileNo;
		unsigned short  TiltPos;
		unsigned short	ErrorType;
		unsigned short	spare[16];
	}ResultsLog;

//==================================================================================================
// Global data
//==================================================================================================

// ROI limits
extern Uint16				gbRoiX1;
extern Uint16				gbRoiY1;
extern Uint16				gbRoiX2;
extern Uint16				gbRoiY2;
extern Uint16				gbRoiWidth;
extern Uint16				gbRoiHeight;

// Original hit data
extern Hits * gbPeakData;

// Current Analysis Segments
extern AnalysisSegments		gbAnalysisSegments;

#pragma pack(pop)
