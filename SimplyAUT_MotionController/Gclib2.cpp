#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "Gclib2.h"
#include "include\gclib.h"
#include "include\gclibo.h"

Gclib::Gclib()
{
    m_ConnectionHandle = NULL;
    CheckDllExist();
    m_BufferSize = 500000;
    m_Buffer = new char[m_BufferSize];
    m_pParent = NULL;
    m_nMsg = 0;
}

Gclib::~Gclib()
{
    GClose();
    m_BufferSize = 0;
    delete[] m_Buffer;
    m_Buffer = NULL;
}

void Gclib::Init(CWnd* pParent, UINT nMsg)
{
    m_pParent = pParent;
    m_nMsg = nMsg;
}

void Gclib::SendDebugMessage(CString msg)
{
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
    }
}

int Gclib::GetMotorSpeed(GCStringIn axis, int& rAccel)
{
    GSize bytes_read = 0;
    char Command[256];

    rAccel = INT_MAX;
    if (this == NULL || !IsConnected())
        return INT_MAX;

    sprintf_s(Command, sizeof(Command), "MG _TV%s", axis);
    GReturn rc1 = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    int speed = (rc1 == G_NO_ERROR) ? atoi(Trim(m_Buffer)) : INT_MAX;

    sprintf_s(Command, sizeof(Command), "MG _AC%s", axis);
    GReturn rc2 = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    rAccel = (rc2 == G_NO_ERROR) ? atoi(Trim(m_Buffer)) : INT_MAX;

    return speed;
}

int Gclib::GetMotorPosition(GCStringIn axis)
{
    GSize bytes_read = 0;
    char Command[256];
    
    if (this == NULL || !IsConnected() )
        return INT_MAX;
    
    sprintf_s(Command, sizeof(Command), "MG _TP%s", axis);
    GReturn rc1 = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    int pos = (rc1 == G_NO_ERROR) ? atoi(Trim(m_Buffer)) : INT_MAX;
    return pos;
}


BOOL Gclib::IsConnected()const 
{
    return m_ConnectionHandle != NULL; 
}


BOOL Gclib::CheckDllExist()
{
    CString szGclibDllPath(_T("DLL\\x64\\gclib.dll"));
    CString szGcliboDllPath(_T("DLL\\x64\\gclibo.dll"));
    
    /// <remarks>Checks to ensure gclib dlls are in the correct location.</remarks>
    if (::GetFileAttributes(szGclibDllPath) == INVALID_FILE_ATTRIBUTES)
    {
        AfxMessageBox(_T("Could not find gclib dll at ") + szGclibDllPath);
        throw new CFileException(CFileException::fileNotFound);
        return FALSE;
    }
    if (::GetFileAttributes(szGcliboDllPath) == INVALID_FILE_ATTRIBUTES)
    {
        AfxMessageBox(_T("Could not find gclibo dll at ") + szGcliboDllPath);
        throw new CFileException(CFileException::fileNotFound);
        return FALSE;
    }

    return TRUE;
}

CString Gclib::GOpen(GCStringIn address)
{
    GReturn rc = ::GOpen(address, &m_ConnectionHandle);

    if (m_ConnectionHandle == NULL)
        return _T("ERROR: ") + GError(rc) + _T(" Check if Power On");
    if (rc != G_NO_ERROR)
        return _T("ERROR: " ) + GError(rc);
    return _T("");
}

//! Uses GUtility() and @ref G_UTIL_INFO to provide a useful connection string.
CString Gclib::GInfo()
{
    GSize info_len = 256;
    GCStringOut info = new char[info_len];

    GReturn rc = ::GInfo(m_ConnectionHandle, info, info_len);
    CString str(info);

    delete[] info;
    if (rc != G_NO_ERROR)
        return _T("ERR");

    else
        return str;
}

//! Uses GUtility(), @ref G_UTIL_VERSION and @ref G_UTIL_GCAPS_VERSION to provide the library and @ref gcaps version numbers.
CString Gclib::GVersion()
{
    if (this == NULL || !IsConnected())
        return _T("ERR");

    GSize info_len = 256;
    GCStringOut info = new char[info_len];

    GReturn rc = ::GVersion(info, info_len);
    CString str(info);

    delete[] info;
    if (rc != G_NO_ERROR)
        return _T("ERR");

    else
        return str;

}

//! Blocking call that returns once all axes specified have completed their motion.
GReturn Gclib::GMotionComplete(GCStringIn axes)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;

    GReturn rc = ::GMotionComplete(m_ConnectionHandle, axes);
    return rc;
}

// The SH commands tells the controller to use the current motor position as the command position and to enable servo control at the current position.
BOOL Gclib::SetServoHere()
{
    return GCommand("SH") != _T("ERR");
}

BOOL Gclib::MotorsOff()
{
    return GCommand("MO") != _T("ERR");
}

BOOL Gclib::StopMotors()
{
    return GCommand("ST") != _T("ERR");; //stop all motion and programs
}

BOOL Gclib::BeginMotors()
{
    return GCommand("BG*") != _T("ERR"); //stop all motion and programs
}

BOOL Gclib::SetSlewSpeed(int speed)
{
    return SetSlewSpeed(speed, speed, speed, speed);
}
BOOL Gclib::SetSlewSpeed(int A, int B, int C, int D)
{
    CString str;
    str.Format("SP %d,%d,%d,%d", A, B, C, D);
    return GCommand(str) != _T("ERR");
}

BOOL Gclib::SetJogSpeed(int speed)
{
    BOOL ret1 = SetJogSpeed("A", speed);
    BOOL ret2 = SetJogSpeed("B", speed);
    BOOL ret3 = SetJogSpeed("C", speed);
    BOOL ret4 = SetJogSpeed("D", speed);

    return ret1 && ret2 && ret3 && ret4;
}
BOOL Gclib::SetJogSpeed(GCStringIn axis, int speed)
{
    CString str;
    str.Format("JG%s=%d", axis, speed);
    return GCommand(str) != _T("ERR");
}

BOOL Gclib::SetAcceleration(int accel)
{
    CString str;
    str.Format("AC*=%d", accel);
    return GCommand(str) != _T("ERR");
}
BOOL Gclib::SetDeceleration(int accel)
{
    CString str;
    str.Format("DC*=%d", accel);
    return GCommand(str) != _T("ERR");
}

// used to define where the zero is
BOOL Gclib::DefinePosition(int pos)
{
    CString str;
    str.Format("DP*=%d", pos);
    return GCommand(str) != _T("ERR");
}

BOOL Gclib::GoToPosition(int pos)
{
    return GoToPosition(pos, pos, pos, pos);
}

BOOL Gclib::GoToPosition(int A, int B, int C, int D)
{
    CString str;
    str.Format("PA=%d,%d,%d,%d", A, B, C, D);
    return GCommand(str) != _T("ERR");
}


BOOL Gclib::WaitForMotorsToStop()
{
    GReturn ret1 = GMotionComplete("A");
    GReturn ret2 = GMotionComplete("B");
    GReturn ret3 = GMotionComplete("C");
    GReturn ret4 = GMotionComplete("D");

    return ret1 == G_NO_ERROR && ret2 == G_NO_ERROR && ret3 == G_NO_ERROR && ret4 == G_NO_ERROR;
}


/// <summary>
/// Used to close a connection to Galil hardware.
/// </summary>
/// <remarks>Wrapper around gclib GClose(),
/// http://www.galil.com/sw/pub/all/doc/gclib/html/gclib_8h.html#a24a437bcde9637b0db4b94176563a052
/// Be sure to call GClose() whenever a connection is finished.</remarks>
BOOL Gclib::GClose()
{
 //   extern GReturn GCALL GClose(GCon g);
    if (m_ConnectionHandle == NULL)
        return FALSE;

    try
    {
        if (::GClose(m_ConnectionHandle) != G_NO_ERROR)
            return FALSE;

        m_ConnectionHandle = NULL;
        return TRUE;
    }
    catch (...)
    {
        return FALSE;
    }
}


/// <summary>
/// Used for command-and-response transactions.
/// </summary>
/// <param name="Command">The command to send to the controller. Do not append a carriage return. Use only ASCII-based commmands.</param>
/// <param name="Trim">If true, the response will be trimmed of the trailing colon and any leading or trailing whitespace.</param>
/// <returns>The command's response.</returns>
/// <remarks>Wrapper around gclib GCommand(),
/// http://www.galil.com/sw/pub/all/doc/gclib/html/gclib_8h.html#a5ac031e76efc965affdd73a1bec084a8
/// </remarks>
/// <exception cref="System.Exception">Will throw an exception if anything other than G_NO_ERROR is received from gclib.</exception>
CString Gclib::GCommand(GCStringIn Command, bool bTrim /*= true*/)
{
    GSize bytes_read = 0;
    if (this == NULL || !IsConnected())
        return _T("ERR");

    SendDebugMessage(_T("Downloading Program --> ") + CString(Command));
    
    GReturn rc = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    if (rc != G_NO_ERROR)
    {
        GReturn rc2 = ::GCommand(m_ConnectionHandle, "TC1", m_Buffer, m_BufferSize, &bytes_read);
        ::AfxMessageBox(_T("[ ERROR ]\n") + CString("GCommand(") + _T(Command) 
            + _T(")\n") + GError(rc) + _T("\n") + Trim(_T(m_Buffer)) );
        return _T("ERR");
    }
    else if (bytes_read > 0)
    {
        SendDebugMessage(Trim(m_Buffer));
    }

    return (bTrim) ? Trim(_T(m_Buffer)) : _T(m_Buffer);
}

CString Gclib::Trim(CString str)
{
    int len = str.GetLength();
    if (str[len - 1] == ':')
        str = str.Left(len - 1);

    str.TrimLeft();
    str.TrimRight();
    str.Replace("\r", "");
    str.Replace("\n", "");
    return str;
}


CString Gclib::GCmdT(GCStringIn command)
{
    if (this == NULL || !IsConnected())
        return _T("ERR");
    
    GSize response_len = 256;
    GCStringOut trimmed_response = new char[response_len];
    GCStringOut front;

    GReturn rc = ::GCmdT(m_ConnectionHandle, command, trimmed_response, response_len, &front);
    if (rc != G_NO_ERROR)
    {
        GSize bytes_read = 0;
        GReturn rc2 = ::GCommand(m_ConnectionHandle, "TC1", m_Buffer, m_BufferSize, &bytes_read);
        ::AfxMessageBox(_T("[ ERROR ]\n") + CString("GCmdT(") + _T(command)
            + _T(")\n") + GError(rc) + _T("\n") + _T(m_Buffer));
        return _T("ERR");
    }
    
    CString str(trimmed_response);
    delete[] trimmed_response;

    return (rc == G_NO_ERROR) ? str : _T("ERR");
}

GReturn Gclib::GCmd(GCStringIn command)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GCmd(m_ConnectionHandle, command);
    if (rc != G_NO_ERROR)
    {
        GSize bytes_read = 0;
        GReturn rc2 = ::GCommand(m_ConnectionHandle, "TC1", m_Buffer, m_BufferSize, &bytes_read);
        ::AfxMessageBox(_T("[ ERROR ]\n") + CString("GCmd(") + _T(command)
            + _T(")\n") + GError(rc) + _T("\n") + _T(m_Buffer));
        return rc;
    }

    return rc;
}

GReturn Gclib::GRead(GBufOut buffer, GSize buffer_len, GSize* bytes_read)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GRead(m_ConnectionHandle, buffer, buffer_len, bytes_read);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GWrite(GBufIn buffer, GSize buffer_len)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GWrite(m_ConnectionHandle, buffer, buffer_len);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GProgramDownload(GCStringIn program, GCStringIn preprocessor)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GProgramDownload(m_ConnectionHandle, program, preprocessor);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}


GReturn Gclib::GProgramUpload(GBufOut buffer, GSize buffer_len)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GProgramUpload(m_ConnectionHandle, buffer, buffer_len);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GArrayDownload(const GCStringIn array_name, GOption first, GOption last, GCStringIn buffer)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;

    GReturn rc = ::GArrayDownload(m_ConnectionHandle, array_name, first, last, buffer);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GArrayUpload(const GCStringIn array_name, GOption first, GOption last, GOption delim, GBufOut buffer, GSize buffer_len)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GArrayUpload(m_ConnectionHandle, array_name, first, last, delim, buffer, buffer_len);

    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GRecord(union GDataRecord* record, GOption method)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GRecord(m_ConnectionHandle, record, method);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GMessage(GCStringOut buffer, GSize buffer_len)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GMessage(m_ConnectionHandle, buffer, buffer_len);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GInterrupt(GStatus* status_byte)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GInterrupt(m_ConnectionHandle, status_byte);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GFirmwareDownload(GCStringIn filepath)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GFirmwareDownload(m_ConnectionHandle, filepath);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}

GReturn Gclib::GUtility(GOption request, GMemory memory1, GMemory memory2)
{
    if (this == NULL || !IsConnected())
        return G_GCLIB_ERROR;
    
    GReturn rc = ::GUtility(m_ConnectionHandle, request, memory1, memory2);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc));
        return rc;
    }
    return rc;
}



CString Gclib::GError(GReturn ErrorCode)
{
    if (this == NULL)
        return _T("ERR");

    if (this == NULL || !IsConnected())
        return _T("Not Connected");

    GSize error_len = 256;
    GCStringOut error = new char[error_len];
    ::GError(ErrorCode, error, error_len);

    CString str;
    str.Format("(%d) %s", ErrorCode, error);
    SendDebugMessage(str);

    delete[] error;
    return str;
}
\