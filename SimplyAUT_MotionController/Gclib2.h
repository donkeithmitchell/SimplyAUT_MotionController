#pragma once
#include "include\gclib.h"

class Gclib
{
public:
	Gclib();
	~Gclib();

	BOOL	GOpen(GCStringIn address);
	BOOL	GClose();
	void	Init(CWnd*, UINT);

	BOOL IsConnected()const;
	void SendDebugMessage(CString);

	BOOL CheckDllExist();
	CString GCommand(GCStringIn Command, bool Trim = true);
	GReturn GCmd(GCStringIn command);
	CString GCmdT(GCStringIn command);
	CString Trim(CString str);
	int     GetMotorSpeed(GCStringIn axis, int& accel); // encoder counts
	int     GetMotorPosition(GCStringIn axis);

	GReturn GRead(GBufOut buffer, GSize buffer_len, GSize* bytes_read);
	GReturn GWrite(GBufIn buffer, GSize buffer_len);
	GReturn GProgramDownload(GCStringIn program, GCStringIn preprocessor);
	GReturn GProgramUpload(GBufOut buffer, GSize buffer_len);
	GReturn GArrayDownload(const GCStringIn array_name, GOption first, GOption last, GCStringIn buffer);
	GReturn GArrayUpload(const GCStringIn array_name, GOption first, GOption last, GOption delim, GBufOut buffer, GSize buffer_len);
	GReturn GRecord(union GDataRecord* record, GOption method);
	GReturn GMessage(GCStringOut buffer, GSize buffer_len);
	GReturn GInterrupt(GStatus* status_byte);
	GReturn GFirmwareDownload(GCStringIn filepath);
	GReturn GUtility(GOption request, GMemory memory1, GMemory memory2);
	CString GInfo();
	CString GVersion();
	GReturn GMotionComplete(GCStringIn axes);

	void    StopMotors();
	void    MotorsOff();
	void    WaitForMotorsToStop();
	void    BeginMotors();
	void    SetSlewSpeed(int speed);
	void    SetSlewSpeed(int A, int B, int C, int D);
	void    SetJogSpeed(GCStringIn, int speed);
	void    SetJogSpeed(int speed);
	void    SetAcceleration(int accel);
	void    SetDeceleration(int accel);
	void    DefinePosition(int pos);
	void    GoToPosition(int pos);
	void    GoToPosition(int A, int B, int C, int D);
	void    SetServoHere();

private:
	CString GError(GReturn ErrorCode);

	GCon	m_ConnectionHandle;
	GSize	m_BufferSize;
	GBufOut	m_Buffer;

	CWnd*	m_pParent;
	UINT	m_nMsg;

};
