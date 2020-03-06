#pragma once
#include "include\gclib.h""

class Gclib
{
public:
	Gclib();
	~Gclib();

	BOOL GOpen(GCStringIn address);
	void GClose();

	BOOL CheckDllExist();
	CString GCommand(GCStringIn Command, bool Trim = true);

private:
	CString GError(GReturn ErrorCode);

	GCon	m_ConnectionHandle;
	GSize	m_BufferSize;
	GBufOut	m_Buffer;

};
