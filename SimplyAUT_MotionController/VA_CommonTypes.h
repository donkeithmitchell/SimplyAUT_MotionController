// VA_CommonTypes.h : header file
//

#ifndef VA_COMMONTYPES_H
#define VA_COMMONTYPES_H

// Program version structure
typedef struct
{
	int major;		// id-s different types of sensor applications
	int minor;		// updated after each official s/w release is shipped to the customer
	int update;		// updated after any changes in s/w
	int dataBase;	// parameter structure id, can be used for automated updates of the sensor parameters
} MS_Version;

MS_Version VA_GetVersion ();

// "Intrinsic types" - integer etc. types used for the project variables
// as independent as possible from a particular compiler implementation
/*******************************************************************************************
*	MT_pos		- type to store data related to extracted stipe position by fpga
*	MT_strength	- type to store data related to extracted stripe strength (convolution)
*	MT_icalc	- type used in internal integer calculations
*******************************************************************************************/
typedef short MT_pos;	// 16 bit signed integer
typedef short MT_strength;	// 16 bit signed integer
typedef long MT_icalc;	// 32 bit signed integer
typedef unsigned char MT_bool;	// 8 bit boolean type
#define MC_false 0	// boolean false
#define MC_true 1	// boolean true

// Max sizes defines
#define VASTEP_MAX_NUMBER_OF_PARAMETERS	64	// maximum number of step control parameters
#define EXTRACT_HITDATA_MAX_NUMBER	8
#define SEGMENT_CUMBREAK_LIST_SIZE	5	// maximum number of cummulative break indexes per segment
#define SMD_POSITIONS_MAXNUMBER	12		// maximum number of feature points defined in seam model
#define SMD_SEGMENTS_MAXNUMBER	10		// maximum number of feature segments defined in seam model

// Error/Result codes
typedef enum
{
	eStatusUknown				= -1,
	eStatusOK					= 0,
	eError_General				= 10000, /* General error, no specifics */
	eError_WrongParameter		= 10001, /* Invalid parameter has been passed to the function */
	eError_WrongVisionParameter	= 10002, /* Invalid vision control parameter */
	eError_NoMemory				= 10003, /* Memory allocation failed */
	eError_MissingFeature		= 10010, /* Necessary joint feature is missing */
	eError_SegmentInvalidation	= 10011, /* Detected segments were invalidated */
	eError_TooManySegments		= 10012, /* Too many segments have been detected */
	eError_InvalidSegmentFP		= 10013, /* Requested segment FP is invalid */
	// Conditional errors
	eError_FalseSuccess			= 10080, /* Step succeeded, while had to be failed*/
	// Hit data extraction
	eError_Hit_NoData			= 10300, /* No hit data were extracted*/
	// Segmentation errors
	eError_Seg_NoStart			= 10600, /* Can not to find start of the segment */
	eError_Seg_FalseStarts		= 10601, /* Too many starts of the segments without completion */
	eError_Seg_TooShort			= 10602, /* No segments with minimum segment lentgh */
	eError_Seg_InvalidEndSlope	= 10603, /* No segments with valid slope at the end */
	eError_Seg_beyondROI		= 10604, /* Segmentation hit ROI finish border */
	eError_Seg_TooMany			= 10605, /* Too many segments */
	// Matcher errors
	eError_Match_NoMatch		= 12400, /* No matched seam segments */
	eError_Match_NotUnique		= 12401, /* The segmentation produced multiple matched seam segments */
	eError_Match_NoMatchNoGap	= 12402, /* No seam segments with matched gap */
	eError_Match_NoMatchNoHilo	= 12403, /* No seam segments with matched both gap and hilo */
	eError_Match_NoMatchNoAngle	= 12404, /* No seam segments with matched all gap, hilo, and angle */
	eError_Match_NoMatchNoPop	= 12405, /* No seam segments with matched all gap, hilo, angle, and hit population */

	// Seam Expert (SE) errors
	eError_SE_NoSmdStep			= 20000, /* SE - no seam model definition step */
	eError_SE_WrongFPsNum		= 20001, /* SE - wrong total number of defined seam model feature points */
	eError_SE_BadSeamSettings	= 20002, /* SE - bad seam settings */
	eError_SE_MissingSeamMPs	= 20003, /* SE - some seam model points are not set */
	eError_SE_SegNoMinMaxSlope	= 20010, /* SE - failed to find segmentation min/max slope */
	eError_SE_SegNoMinGap		= 20011, /* SE - failed to find individual segmentation gap min */
	eError_SE_SegNoMinStep		= 20012, /* SE - failed to find individual segmentation step min */
	eError_SE_SegNoMinGapStep	= 20013, /* SE - failed to find joint segmentation gap and step min */
	eError_SE_MatchNoSegments	= 20020, /* SE - failed to extract segments during matcher optimization */
	eError_SE_NoMatch			= 20021, /* SE - failed to calculate matcher coefficients */
	eError_SE_NoSoftChoice		= 20022, /* SE - failed to get matcher soft choice */

	eLastError
} ME_Status;

// Vision analysis parameters

// Vision Analysis Steps id-s
// It is used to id-s types of vision analylis steps and label data with steps id-s where they were generated
typedef enum
{
	eStep_Invalid					= 0, // Indicates invalid or not ininitialized data
	eStep_Generic					= 1, // Generic ID - indicates data different form main vision analysis step results
	eStep_SetROI					= 2, // Set ROI - VA_ROI.c
	eStep_ExtractHitData			= 3, // Hit data extraction - VA_ExtractProfile.c & extractHitData()
	eStep_ExtractProfile			= 4, // Profile extraction - VA_ExtractProfile.c
	eStep_FilterProfile				= 5, // Filter profile data - VA_ExtractProfile.c
	eStep_Segmentation				= 6, // Profile segmentation - VA_Segment.c
	eStep_Match2Segments			= 7, // Simple (soon to be obsolete) segments matcher - VA_SegmentMatcher.c
	eStep_SetLineSegment_Segment	= 8, // Calculate line segment corresponding to profile segment - VA_Segment.c
	eStep_SetLineSegment_LSeg		= 9, // Set line segment relative to the existing line segment - VA_Segment.c
	eStep_SetLineSegment_Pts		= 10, // Set line segment from 2 points - VA_Segment.c
	eStep_SetArcSegment_Segment		= 11, // Calculate arc segment corresponding to profile segment - VA_Segment.c
	eStep_AdjustLSeg_LSeg			= 12, // Adjust existing line segment relative to another line segment - VA_Segment.c
	eStep_AdjustLSeg_Segment		= 13, // Adjust existing line segment relative to a profile segment - VA_Segment.c
	eStep_AdjustSegment_Segment		= 14, // Adjust existing profile segment relative to another profile segment - VA_Segment.c
	eStep_AdjustSegment_Fillet		= 15, // Adjust existing profile segments forming fillet joint - VA_Segment.c
	eStep_AdjustSegment_LSeg		= 16, // Adjust existing profile segment definition with passed linear segment - VA_Segment.c
	eStep_Combine2Segments			= 17, // Combine two profile segments into resulting one - VA_Segment.c
	eStep_SeamDefinition			= 18, // Sets seam definition - VA_SeamDefinition.c
	eStep_Match2Segments_SeamDef	= 19, // Match 2 segments based on Seam Definition - VA_SegmentMatcher.c
	eStep_MatchCalibTarget			= 20, // Match calibration target prfoile segmets - VA_SegmentMatcher.c
	eStep_SetSeamSlopeLimits		= 21, // Set seam model slope limits (like min/max segmentation slopes) - VA_SeamDefinition.c
	eStep_CalcSeamSlopeLimits_MPs	= 22, // Calculate seam model slope limits - VA_SeamDefinition.c
	eStep_CalcSeamSlopeLimits_FPs	= 23, // Calculate seam model slope limits - VA_SeamDefinition.c
	eStep_SegmentsMatcher_NoScale	= 24, // Match two segments (mm parameters are not scaled to 100 mm fov) - VA_SegmentMatcher.c
	eStep_SetSensitivity			= 25, // Set positions of sensititivity sliders - ...
	eStep_SetSensitiveSeamLimits	= 26, // Set seam limits accordingly to positions of sensitivity sliders
	eStep_SeamModelDefinition		= 27, // Sets seam definition (new, compatible with Seam Expert) - VA_SeamDefinition.c
	eStep_SegmentsMatcher			= 28, // Match two segments - VA_SegmentMatcher.c
	eStep_History_Set				= 30, // Set history processing parameters - VA_History.c
	eStep_ExtractFP_LSeg			= 100, // Extract feature point from existing line segment - VA_ExtractFP.c
	eStep_ExtractFP_Seg				= 101, // Extract feature point from existing profile segment - VA_ExtractFP.c
	eStep_ExtractFP_LSegInt			= 102, // Extract line segments intersection - VA_ExtractFP.c
	eStep_ExtractFP_SeamModel		= 103, // Extract feature point from the seam model - VA_ExtractFP.c
	eStep_ExtractFPs_SeamModel		= 104, // Extract multiple feature points from the seam model - VA_ExtractFP.c
	eStep_ExtractFPs_LSeg			= 105, // Extract multiple FPs from existing line segments - VA_ExtractFP.c
	eStep_ExtractFPs_Seg			= 106, // Extract multiple FPs from existing profile segments - VA_ExtractFP.c
	eStep_ExtractFPs_Arc			= 107, // Extract feature points from existing arc segment - VA_ExtractFP.c
	eStep_ExtractFP_Pts				= 108, // Extract feature point between two existing feature points - VA_ExtractFP.c
	eStep_ExtractMeas_Gap			= 200, // Extract gap measurement between 2 feature points - VA_Measure.c
	eStep_ExtractMeas_Mismatch		= 201, // Extract mismatch measurement between 2 feature points - VA_Measure.c
	eStep_ExtractMeas_LSegAngle		= 202, // Extract line segment angle - VA_Measure.c
	eStep_ExtractMeas_ArcAngle		= 203, // Extract arc segment angle - VA_Measure.c
	eStep_ExtractMeas_Volume		= 204, // Extract joint volume measurement - VA_Measure.c
	eStep_ExtractMeas_Radius		= 205, // Extract arc radius from segment profile - VA_Measure.c
	eStep_ExtractMeas_RPHeight		= 206, // Extract RP height measurement - VA_Measure.c
	eStep_ExtractMeas_RPWeldStatus	= 207, // Extract RP weld status - VA_Measure.c
	eStep_ExtractMeas_Dist_Pts		= 208, // Extract gap measurement between 2 feature points - VA_Measure.c
	eStep_ExtractMeas_SeamLimit		= 209, // Extract seam limit value from seam limit array - VA_SeamDefinition.c
	// Test steps id-s
	eStep_Test = 1000,
	eStep_TestLineSegment_1,
	eStep_TestLineSegment_2,
	eStep_TestLineSegment_3
} ME_VisionAnalysisStepID;

// Vision Analysis status to returned by a main vision analysis routine
typedef struct
{
	ME_Status	status;		// status code
	short		stepIdx;	// status step number - step responsible for the analysis status if the analysis fails, number of analysis steps otherwise
	ME_VisionAnalysisStepID	stepType;	// status step type
	char	spare[8];
} MS_VA_Status;

// This structure contains the set of calibration coeffitients
typedef struct
{
	float Xo, Yo;			// Position of centre in pixels
	float m;				// magnification
	float k,C,E;			// horizontal (along stripe) scale factors
	float A,l,D,F;			// vertical (height) scale factors
	float z_to_x_factor;	// look ahead vs height change
} MS_StripeCalibration;

// This structure defines region of interest - min and max limits along each sensor axis
typedef struct
{
	MT_pos S1;
	MT_pos S2;
	MT_pos H1;
	MT_pos H2;
} MS_SensorROI;

// This structure defines relative scale of existing or planned lines of SLS.
// The scale 1.0 corresponds to SLS 100 (FOV - 100 mm)
typedef struct
{
	float fov_mm;
	float scale_stripe;
	float scale_height;
} MS_SensorFOVScale;

// Exracted hit data peak type order
typedef enum
{
	eHT_Maximum,	// Peak with maximum strength comes first
	eHT_Top,		// The highest peak comes first
	eHT_Bottom		// The lowest peak comes first
} ME_ExtractHitData_HitType;

// Sensor World, CCD and image axe id-s
typedef enum
{
	eAXIS_STRIPE	= 0,
	eAXIS_HEIGHT	= 1,
	eAXIS_TRAVEL	= 2,
	eAXIS_IMAGE_Columns	= eAXIS_HEIGHT,
	eAXIS_IMAGE_Rows	= eAXIS_STRIPE,
	eAXIS_WORLD_Y		= eAXIS_STRIPE,
	eAXIS_WORLD_X		= eAXIS_TRAVEL,
	eAXIS_WORLD_Z		= eAXIS_HEIGHT
} ME_Axis;

// Vision analysis step definition structure
typedef struct
{
	char stepName[64];				// step proper name
	ME_VisionAnalysisStepID stepID;	// step algorithm ID
	int resultIdx;					// result index in corresponded step result array
	int sourceIdx;					// source data index - refers to a previous step result
	int statusPriority;				// reserved for future use
	int spare2;						// reserved for future use
	int spare3;						// reserved for future use
	int spare4;						// reserved for future use
	int spare5;						// reserved for future use
	int spare6;						// reserved for future use
	int spare7;						// reserved for future use
	int spare8;						// reserved for future use
	int spare9;						// reserved for future use
	int spare10;					// reserved for future use
	float parameters[VASTEP_MAX_NUMBER_OF_PARAMETERS];	// step parameters array
} MS_VisionAnalysisStep;

typedef struct
{
	MS_VisionAnalysisStep *pVASteps;
	int stepsNumber;
	int dataBaseId;
} MS_VisionAnalisysParameters;

// Parameters id-s for hit data extraction step (eStep_ExtractHitData)
// They are placed here, because they are used externally when calling
// for primary stripe profile extraction
typedef enum
{
	eHD_ConvFilter,
	eHD_RowMin,
	eHD_ColMin,
	eHD_RowMax,
	eHD_ColMax,
	eHD_StrengthThreshold,
	eHD_HitType,
	eHD_BaseAxis,
	eHD_NumberOfPeaks,
	eHD_ParametersSize
} ME_ParametersIDs_ExtractHitData;

// FPGA Convolution filters ID-s
typedef enum
{
	eConvFilter_G2_3_1x17,				// 0 - 1D (1x17, s=3), Gaussian 2nd derivative (primary stipe detection)
	eConvFilter_G2_4_1x31,				// 1 - 1D (1x31, s=4), Gaussian 2nd derivative (primary stipe detection)
	eConvFilter_G2_6_1x31,				// 2 - 1D (1x31, s=6), Gaussian 2nd derivative (primary stipe detection)
	eConvFilter_G2_3_31x1,				// 3 - 1D (31x1, s=3), Gaussian 2nd derivative (orthogonal to the above)
	eConvFilter_G2_Gx2_105_1d4_5_31x31,	// 4 - 2D (31x31, s1=1.4, s2=5), Gaussian x2,-75 degrees derivative (1st wall)
	eConvFilter_G2_Gx2_75_1d4_5_31x31,	// 5 - 2D (31x31, s1=1.4, s2=5), Gaussian x2,+75 degrees derivative (2nd wall)
	eConvFilter_G2_Gx2_105_1d4_5_15x17,	// 6 - 2D (15x17, s1=1.4, s2=5), Gaussian x2,-75 degrees derivative (1st wall)
	eConvFilter_G2_Gx2_75_1d4_5_15x17,	// 7 - 2D (15x17, s1=1.4, s2=5), Gaussian x2,+75 degrees derivative (2nd wall)
	eConvFilter_G2_Gx2_95_1d4_5_13x17,	// 8 - 2D (15x17, s1=1.4, s2=5), Gaussian x2,-85 degrees derivative (2nd wall)
	eConvFilter_G2_Gx2_85_1d4_5_13x17	// 9 - 2D (15x17, s1=1.4, s2=5), Gaussian x2,+85 degrees derivative (2nd wall)
} ME_ConvolutionFilterID;

// ME_SeamTypeID - Meta Seam Types
	// Seam Type			Seam Type ID	Description
typedef enum
{
	// Heavy Metal based Seam Types
	eJ_Seam					= 0,			// CRC J Seam type
	eK_Seam					= 1,			// CRC K Seam type
	eRP_Seam				= 2,			// CRC Root Inspection
	eN_Pass					= 3,			// Multi Pass Seam Type
	eJ_Bevel				= 4,			// CRC J Bevel Inspection
	eK_Bevel				= 5,			// CRC K Bevel Inspection
	eTJ_Seam				= 6,			// CRC Transition J Seam type
	eTK_Seam				= 7,			// CRC Transition K Seam type
	eTJ_Bevel				= 8,			// CRC Transition J Bevel Inspection
	eTK_Bevel				= 9,			// CRC Transition K Bevel Inspection
	eCruet_Seam				= 10,			// CRC Cruet Seam type
	eCruet_Bevel			= 11,			// CRC Cruet Bevel Inspection
	eU_Seam					= 12,			// U Seam
	eJ_Root					= 13,			// J Root

	// Seam Type Number 13 to 49 are reserved for CRC

	// Thin Sheet Seam Types
	eLapLeft				= 50,			// Thin Sheet Left Lap
	eLapRight				= 51,			// Thin Sheet Right Lap
	eButt					= 52,			// Thin Sheet Butt
	eFillet					= 53,			// Thin Sheet Fillet
	eFilletExternal			= 54,			// Thin Sheet External Fillet
	eV_Type					= 55,			// Thin Sheet V Type
	eEdgeLeft				= 56,			// Thin Sheet Left Edge
	eEdgeRight				= 57,			// Thin Sheet Right Edge
	eHeight					= 58,			// Thin Sheet Height
	eM						= 59,			// Thin Sheet (?)M - thick External Fillet

	// Special Seam Types
	eShortSeam_CurvedBase_Left		= 100,	// Short seam (usually on top of the profile) with curved base on the left
	eShortSeam_CurvedBase_Right		= 101,	// Short seam (usually on top of the profile) with curved base on the right

	// Standard (Generic) Seam Type
	eStandard_Seam			= 255
} ME_SeamTypeID;


// Seam types ID-s
// Depricated. Used in the old, unupdated code only
typedef enum
{
	eSeamType_Invalid		= 0,	// 0 - Invalid seam type - seam type has not been set
	eSeamType_Generic		= 1,	// 1 - Generic seam type
	eSeamType_Butt			= 2,	// 2 - Butt joint, spare
	eSeamType_Butt2			= 3,	// 3 - Butt joint - 2nd variation, spare
	eSeamType_Butt3			= 4,	// 4 - Butt joint - 3rd variation, spare
	eSeamType_Lap			= 5,	// 5 - Lap joint, spare
	eSeamType_Lap2			= 6,	// 6 - Lap joint - 2nd variation, spare
	eSeamType_Lap3			= 7,	// 7 - Lap joint - 3rd variation, spare
	eSeamType_FullSheet		= 8,	// 8 - Full Sheet, spare
	eSeamType_HalfSheet		= 9,	// 9 - Half Sheet, spare
	eSeamType_V				= 10,	// 10 - V joint, spare
	eSeamType_V2			= 11,	// 11 - V joint - 2nd variation, spare
	eSeamType_V3			= 12,	// 12 - V joint - 3rd variation, spare
	eSeamType_Fillet		= 13,	// 13 - Fillet joint, spare
	eSeamType_Fillet2		= 14,	// 14 - Fillet joint - 2nd variation, spare
	eSeamType_Fillet3		= 15,	// 15 - Fillet joint - 3rd variation, spare
	eSeamType_K				= 16,	// 16 - K seam
	eSeamType_K2			= 17,	// 17 - K seam - 2nd variation, spare
	eSeamType_K3			= 18,	// 18 - K seam - 3rd variation, spare
	eSeamType_J				= 19,	// 19 - J seam
	eSeamType_J2			= 20,	// 20 - J seam - 2nd variation, spare
	eSeamType_J3			= 21,	// 21 - J seam - 3rd variation, spare
	eSeamType_PartFilled	= 22,	// 22 - Partially filled joint - multi-pass welding
	eSeamType_PartFilled2	= 23,	// 23 - Partially filled joint - 2nd variation, spare
	eSeamType_PartFilled3	= 24,	// 24 - Partially filled joint- 3rd variation, spare
	eSeamType_KBevel		= 25,	// 25 - K bevel seam
	eSeamType_TKBevel		= 26,	// 26 - K bevel seam - 2nd variation, TK bevel
	eSeamType_KBevel3		= 27,	// 27 - K bevel seam - 3rd variation, spare
	eSeamType_JBevel		= 28,	// 28 - J bevel seam
	eSeamType_TJBevel		= 29,	// 29 - J bevel seam - 2nd variation, TJ bevel
	eSeamType_JBevel3		= 30	// 30 - J bevel seam - 3rd variation, spare
} ME_SeamTypeID_depricated;

// Point placement types
typedef enum
{
	eFPPlace_Percentage		= 0,	// 0 - percentage
	eFPPlace_RefFP_side		= 1,	// 1 - Projection of reference FP in side direction
	eFPPlace_RefFP_height	= 2		// 2 - Projection of reference FP in height direction
} ME_FP_PlacementType_IDs;

// ROI structure - four 2D points (eight coordinates) defining region of interest
// Abbreviation "s" and "h" stands for coordinates along the Stripe and Height axis
// (h1,s1)-(h2,s2) and (h3,s3)-(h4,s4) define "stripe" sides of the ROI
// (h1,s1)-(h4,s4) and (h2,s2)-(h3,s3) define "height" sides of the ROI
typedef struct
{
	ME_VisionAnalysisStepID vaStepID;
	float s1;
	float h1;
	float s2;
	float h2;
	float s3;
	float h3;
	float s4;
	float h4;
} MS_ROI;

// Structure for single pixel position data buffer
typedef struct
{
	MT_pos offset;
	MT_pos size;
	MT_pos * pBuffer;
} MS_PosBuffer;

// Histogram structure - mainly used for position population histograms
typedef struct
{
	MT_pos totalPopulation;
	MS_PosBuffer histogram;
} MS_Histogram;

typedef struct
{
	ME_VisionAnalysisStepID vaStepID;
	ME_Axis baseAxis;	// Segment base (index) axis
	MT_pos size;
	MT_pos offset;
	int numBuffers;
	MT_pos *ppPosition[EXTRACT_HITDATA_MAX_NUMBER];
	MT_strength *ppStrength[EXTRACT_HITDATA_MAX_NUMBER];
	MS_Histogram posHistogram;
} MS_HitData;


// Segment related data structures:

// Segment limits structure
typedef struct
{
	short multiSeg_ID;	// id of multi-segment structure to which the segment belong
	float side_beg;	// side coordinate of the segment beginning
	float height_beg;	// height coordinate of the segment beginning
	float side_end;	// side coordinate of the segment end
	float height_end;	// height coordinate of the segment end
} MS_SegLimits;

// Line model - slope based 2D line definition
typedef struct
{
	float slope;	// slope of the line position along the "index" axis
	float index;	// "index" coordinate of the line reference point
	float pos;		// "position" coordinate of the line reference point
} MS_LineModel;

// Circle model
typedef struct
{
	float x0;
	float y0;
	float r;
} MS_CircleModel;

// Line segment definition structure
typedef struct
{
	ME_VisionAnalysisStepID vaStepID;	// Vision analysis step id where segment was calculated
	ME_Axis baseAxis;		// Segment base (index) axis
	MS_LineModel lineModel;	// Line model of the segment
	float begIndex;		// beginning index limit of the segment
	float endIndex;		// end index limit of the segment
} MS_LineSegment;

// Arc segment definition structure
typedef struct
{
	ME_VisionAnalysisStepID vaStepID;	// Vision analysis step id where segment was calculated
	MS_CircleModel circleModel;	// Circle model of the segment
	float begAngle;		// beginning angle of the segment
	float endAngle;		// end angle of the segment
} MS_ArcSegment;

//Segment cummulative break list structure
typedef struct
{
	int lastCumBreak;
	MT_pos cumBreakIndexes[SEGMENT_CUMBREAK_LIST_SIZE];
} MS_SegmentCumBreakList;


// Segment definition structure

// ID-s for reasons to stop segmentation
typedef enum
{
	eSegStop_uknown,
	eSegStop_gap,
	eSegStop_numInvalid,
	eSegStop_slope,
	eSegStop_cumDev
} ME_SegmentStopper_IDs;

typedef struct
{
	int ID;
	ME_VisionAnalysisStepID vaStepID;	// Vision analysis step id where segment was calculated
	MT_pos fitWinSize; // sliding window size used to fit the line during the segment building
	ME_SegmentStopper_IDs segStopper;	// Reason of stopping the segmentation - gap, number of invalid points or slope
	ME_Axis baseAxis;		// Segment base (index) axis
	MS_LineModel beg;		// line model at the beginning of the segment
	MS_LineModel end;		// line model at the end of the segment
	MS_LineModel maxSlope;	// line model at the maximum slope of the segment
	MS_LineModel minSlope;	// line model at the minimum slope of the segment
	MT_pos minPos_index;	// index of the segment profile with min value
	MT_pos maxPos_index;	// index of the segment profile with max value
	MT_strength mean_strength;	// mean strenght of segment vaild pixels
	MS_SegmentCumBreakList posCumBrkList;	// list of last segment positive cummulative breaks
	MS_SegmentCumBreakList negCumBrkList;	// list of last segment negative cummulative breaks
	MS_PosBuffer segProfile;	// valid stripe profile pixel positions of the segment
	float *pProfile_stripe_mm;	// pointer to array of (float) world coordinates of segment profile pixels along stripe axis
	float *pProfile_height_mm;	// pointer to array of (float) world coordinates of segment profile pixels along height axis
} MS_Segment;

typedef struct
{
	int ID;
	ME_VisionAnalysisStepID vaStepID;	// Vision analysis step id where multi segments were calculated
	short segIdx;						// index of the first segment in the multi segments storage
	short numSegments;					// number of segments extracted for this segmentation
} MS_MultiSegment;

// Calibration point structure
typedef struct
{
	ME_VisionAnalysisStepID vaStepID;
	float stripe_pix;	// Camera coordinate in pixels along the stripe axis
	float height_pix;	// Camera coordinate in pixels along the height axis
	float stripe_mm;	// World coordinate in mm along the stripe axis
	float height_mm;	// World coordinate in mm along the height axis
} MS_CalibPoint;

// Measurement value structure
typedef struct
{
	ME_VisionAnalysisStepID vaStepID;
	float measValue;	// measured value
} MS_MeasurementValue;

// Structure for multiple pixel position data buffers
typedef struct
{
	MT_pos size;
	int numBuffers;
	MT_pos ** ppBuffer;
} MS_PosBuffers;

// Assertion, logging macroses
#ifdef __cplusplus
	#define EXTERNC extern "C"
#else
	#define EXTERNC
#endif

#ifdef _MSC_VER
	// Disable warning C4127: conditional expression is constant
	#pragma warning (disable : 4127)
#endif

#endif //#ifndef VA_COMMONTYPES_H
