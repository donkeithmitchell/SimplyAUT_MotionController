
// SimplyAUT_MotionController.h : main header file for the PROJECT_NAME application
//

#pragma once

// enable this line to get a debug dialogs, files, etc.
#ifdef _DEBUG
#define _DEBUG_TIMING_ 1
#endif

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CSimplyAUTMotionControllerApp:
// See SimplyAUT_MotionController.cpp for the implementation of this class
//

enum GALIL_STATE { GALIL_IDLE = 0, GALIL_SCAN, GALIL_FWD, GALIL_BACK, GALIL_GOHOME };
enum SCAN_TYPE { SCANTYPE_CIRC = 0, SCANTYPE_DIST };

class CSimplyAUTMotionControllerApp : public CWinApp
{
public:
	CSimplyAUTMotionControllerApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CSimplyAUTMotionControllerApp theApp;
