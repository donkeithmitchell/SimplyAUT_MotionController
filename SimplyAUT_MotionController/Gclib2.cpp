#include "pch.h"
#include "SimplyAUT_MotionController.h"
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
    if (m_pParent && ::IsWindow(m_pParent->m_hWnd) && m_nMsg != 0)
    {
        m_pParent->SendMessage(m_nMsg, MSG_SEND_DEBUGMSG, (WPARAM)&msg);
    }
}

double Gclib::GetMotorSpeed(GCStringIn axis, double& rAccel)
{
    GSize bytes_read = 0;
    char Command[256];

    rAccel = FLT_MAX;
    if (this == NULL)
        return FLT_MAX;

    sprintf_s(Command, sizeof(Command), "MG _TV%s", axis);
    GReturn rc1 = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    double speed = (rc1 == G_NO_ERROR) ? atof(Trim(m_Buffer)) : FLT_MAX;

    sprintf_s(Command, sizeof(Command), "MG _AC%s", axis);
    GReturn rc2 = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    rAccel = (rc2 == G_NO_ERROR) ? atof(Trim(m_Buffer)) : FLT_MAX;

    return speed;
}

double Gclib::GetMotorPosition(GCStringIn axis)
{
    GSize bytes_read = 0;
    char Command[256];
    
    if (this == NULL)
        return FLT_MAX;
    
    sprintf_s(Command, sizeof(Command), "MG _TP%s", axis);
    GReturn rc1 = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    double pos = (rc1 == G_NO_ERROR) ? atof(Trim(m_Buffer)) : FLT_MAX;
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

BOOL Gclib::GOpen(GCStringIn address)
{
    GReturn rc = ::GOpen(address, &m_ConnectionHandle);

    if (m_ConnectionHandle == NULL)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc) );
        return FALSE;
    }
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc) );
        return FALSE;
    }
    return TRUE;
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
        return _T("");

    else
        return str;
}

//! Uses GUtility(), @ref G_UTIL_VERSION and @ref G_UTIL_GCAPS_VERSION to provide the library and @ref gcaps version numbers.
CString Gclib::GVersion()
{
    GSize info_len = 256;
    GCStringOut info = new char[info_len];

    GReturn rc = ::GVersion(info, info_len);
    CString str(info);

    delete[] info;
    if (rc != G_NO_ERROR)
        return _T("");

    else
        return str;

}

//! Blocking call that returns once all axes specified have completed their motion.
GReturn Gclib::GMotionComplete(GCStringIn axes)
{
    GReturn rc = ::GMotionComplete(m_ConnectionHandle, axes);
    return rc;
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
    SendDebugMessage(_T("Downloading Program --> ") + CString(Command));
    
    GReturn rc = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    if (rc != G_NO_ERROR)
    {
        GReturn rc2 = ::GCommand(m_ConnectionHandle, "TC1", m_Buffer, m_BufferSize, &bytes_read);
        ::AfxMessageBox(_T("[ ERROR ]\n") + CString("GCommand(") + _T(Command) 
            + _T(")\n") + GError(rc) + _T("\n") + Trim(_T(m_Buffer)) );
        return _T("");
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
        return _T("");
    }
    
    CString str(trimmed_response);
    delete[] trimmed_response;

    return (rc == G_NO_ERROR) ? str : _T("");
}

GReturn Gclib::GCmd(GCStringIn command)
{
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