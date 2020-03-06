#pragma once


// Gclib class

class Gclib
{
public:
	Gclib();   // standard constructor
	virtual ~Gclib();

	BOOL	CheckDllExist();
	CString GAddresses();
	void GArrayDownload(CString array_name, double data[], INT first = -1, INT last = -1);
	void GArrayDownloadFile(CString Path);
	double* GArrayUpload(CString array_name, INT first = -1, INT last = -1);
	void GArrayUploadFile(CString Path, CString Names);
	void GAssign(CString ip, CString mac);
	void GClose();
	CString GCommand(CString Command, BOOL Trim = TRUE);
	void GFirmwareDownload(CString filepath);
	CString GInfo();
	byte GInterrupt();
	char* GIpRequests();
	CString GMessage();
	void GMotionComplete(CString axes);
	void GOpen(CString address);
	void GProgramDownload(CString program, CString preprocessor = _T(""));
	void GProgramDownloadFile(CString file_path, CString preprocessor = _T(""));
	CString LGProgramUpload();
	void GProgramUploadFile(CString file_path);
	byte* GRead();
	byte* GRecord(bool async);
	void GRecordRate(double period_ms);
	void GTimeout(INT timeout_ms);
	CString GVersion();
	void GWrite(CString buffer);
	char* GclibGSetupDownloadFile(CString path, INT options);

protected:

private:
	CString GError(INT ErrorCode);
	INT DllGAddresses(CString& addresses);
	INT DllGArrayDownloadFile(CString Path);

	int   m_BufferSize_		/*= 500000*/; //size of "char *" buffer. Big enough to fit entire 4000 program via UL/LS, or 24000 elements of array data.
	char* m_Buffer_;		//used to pass a "char *" to gclib.
	byte* m_ByteArray_; //byte array to hold data record and response to GRead
	UINT_PTR m_ConnectionHandle_; //keep track of the gclib connection handle.


};
