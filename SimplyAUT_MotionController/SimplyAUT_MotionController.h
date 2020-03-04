
// SimplyAUT_MotionController.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CSimplyAUTMotionControllerApp:
// See SimplyAUT_MotionController.cpp for the implementation of this class
//

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
