#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "Gclib2.h"
#include "include\gclib.h"

Gclib::Gclib()
{
    m_ConnectionHandle = NULL;
    CheckDllExist();
    m_BufferSize = 500000;
    m_Buffer = new char[m_BufferSize];
}

Gclib::~Gclib()
{
    GClose();
    m_BufferSize = 0;
    delete[] m_Buffer;
    m_Buffer = NULL;
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
        ::AfxMessageBox(_T("[ ERROR ]\nUnable to Open The GC Lib"));
        return FALSE;
    }
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\nUnable to Open The GC Lib"));
        return FALSE;
    }
    return TRUE;
}


/// <summary>
/// Used to close a connection to Galil hardware.
/// </summary>
/// <remarks>Wrapper around gclib GClose(),
/// http://www.galil.com/sw/pub/all/doc/gclib/html/gclib_8h.html#a24a437bcde9637b0db4b94176563a052
/// Be sure to call GClose() whenever a connection is finished.</remarks>
void Gclib::GClose()
{
 //   extern GReturn GCALL GClose(GCon g);
    ::GClose(m_ConnectionHandle);
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
    extern GReturn GCALL GCommand(GCon g, GCStringIn command, GBufOut buffer, GSize buffer_len, GSize * bytes_returned);
    
    GSize bytes_read = 0;
    GReturn rc = ::GCommand(m_ConnectionHandle, Command, m_Buffer, m_BufferSize, &bytes_read);
    if (rc != G_NO_ERROR)
    {
        ::AfxMessageBox(_T("[ ERROR ]\n") + GError(rc) );
        return _T("");
    }

    CString ret(m_Buffer);
    int len = ret.GetLength();

    if (bTrim)
    {

        if (ret[len - 1] == ':')
            ret = ret.Left(len - 1);

        ret.TrimLeft();
        ret.TrimRight();
        return ret;
    }
    else
        return ret;
}

CString Gclib::GError(GReturn ErrorCode)
{
    switch (ErrorCode)
    {
    case G_NO_ERROR :  //!< Return value if function succeeded. 
        return _T("");
    case G_GCLIB_ERROR: //!< General library error. Indicates internal API caught an unexpected error. Contact Galil support if this error is returned, softwaresupport@galil.com.
        return _T(G_GCLIB_ERROR_S); //  "gclib unexpected error");

    case G_GCLIB_UTILITY_ERROR: //!< An invalid request value was specified to GUtility.
        return _T(G_GCLIB_UTILITY_ERROR_S); // "invalid request value or bad arguments were specified to GUtility()"

    case G_GCLIB_UTILITY_IP_TAKEN: //!< The IP cannot be assigned because ping returned a reply. 
        return _T(G_GCLIB_UTILITY_IP_TAKEN_S); //  "ip address is already taken by a device on the network"

    case G_GCLIB_NON_BLOCKING_READ_EMPTY: //!< GMessage, GInterrupt, and GRecord can be called with a zero timeout. If there wasn't data waiting in memory, this error is returned.
        return _T(G_GCLIB_NON_BLOCKING_READ_EMPTY_S); //  "data was not waiting for a zero-timeout read"

    case G_GCLIB_POLLING_FAILED: //!< GWaitForBool out of polling trials.
        return _T(G_GCLIB_POLLING_FAILED_S); //  "exit condition not met in specified polling period"

    case G_TIMEOUT: //!< Operation timed out. Timeout is set by the --timeout option in GOpen() and can be overriden by GSetting().
        return _T(G_TIMEOUT_S); //   "device timed out"

    case G_OPEN_ERROR: //!< Device could not be opened. E.G. Serial port or PCI device already open.
        return _T(G_OPEN_ERROR_S); //   "device failed to open"

    case G_READ_ERROR: //!< Device read failed. E.G. Socket was closed by remote host. See @ref G_UTIL_GCAPS_KEEPALIVE.
        return _T(G_READ_ERROR_S); //   "device read error"

    case G_WRITE_ERROR: //!< Device write failed. E.G. Socket was closed by remote host. See @ref G_UTIL_GCAPS_KEEPALIVE.
        return _T(G_WRITE_ERROR_S); //   "device write error"

    case G_INVALID_PREPROCESSOR_OPTIONS: //!< GProgramDownload was called with a bad preprocessor directive.
        return _T(G_INVALID_PREPROCESSOR_OPTIONS_S); //   "preprocessor did not recognize options"

    case G_COMMAND_CALLED_WITH_ILLEGAL_COMMAND: //!< GCommand() was called with an illegal command, e.g. ED, DL or QD.
        return _T(G_COMMAND_CALLED_WITH_ILLEGAL_COMMAND_S); //   "illegal command passed to command call"

    case G_DATA_RECORD_ERROR: //!< Data record error, e.g. DR attempted on serial connection.
        return _T(G_DATA_RECORD_ERROR_S); //   "data record error"

    case G_UNSUPPORTED_FUNCTION: //!< Function cannot be called on this bus. E.G. GInterrupt() on serial.
        return _T(G_UNSUPPORTED_FUNCTION_S); //   "function not supported on this communication bus"

    case G_FIRMWARE_LOAD_NOT_SUPPORTED: //  !< Firmware is not supported on this bus, e.g. Ethernet for the DMC-21x3 series.
        return _T(G_FIRMWARE_LOAD_NOT_SUPPORTED_S); //  "firmware cannot be loaded on this communication bus to this hardware"

    case G_ARRAY_NOT_DIMENSIONED: //!< Array operation was called on an array that was not in the controller's array table, see LA command.
        return _T(G_ARRAY_NOT_DIMENSIONED_S); //   "array not dimensioned on controller or wrong size"

    case G_CONNECTION_NOT_ESTABLISHED: //!< Function was called with no connection.
        return _T(G_CONNECTION_NOT_ESTABLISHED_S); //   "connection to hardware not established"

    case G_ILLEGAL_DATA_IN_PROGRAM: //!< Data to download not valid, e.g. \ in data.
        return _T(G_ILLEGAL_DATA_IN_PROGRAM_S); //   "illegal ASCII character in program"

    case G_UNABLE_TO_COMPRESS_PROGRAM_TO_FIT: //!< Program preprocessor could not compress the program within the user's constraints.
        return _T(G_UNABLE_TO_COMPRESS_PROGRAM_TO_FIT_S); //   "program cannot be compressed to fit on the controller"

    case G_BAD_RESPONSE_QUESTION_MARK: //!< Operation received a ?, indicating controller has a TC error.
        return _T(G_BAD_RESPONSE_QUESTION_MARK_S); //   "question mark returned by controller"

    case G_BAD_VALUE_RANGE: //!< Bad value or range, e.g. GCon *g* variable passed to function was bad.
        return _T(G_BAD_VALUE_RANGE_S); //   "value passed to function was bad or out of range"

    case G_BAD_FULL_MEMORY: //!< Not enough memory for an operation, e.g. all connections allowed for a process already taken.
        return _T(G_BAD_FULL_MEMORY_S); //   "operation could not complete because of a memory error"

    case G_BAD_LOST_DATA: //!< Lost data, e.g. GCommand() response buffer was too small for the controller's response.
        return _T(G_BAD_LOST_DATA_S); //   "data was lost due to buffer or fifo limitations"

    case G_BAD_FILE: //!< Bad file path, bad file contents, or bad write.
        return _T(G_BAD_FILE_S); //   "file was not found, contents are invalid, or write failed"

    case G_BAD_ADDRESS: //!< Bad address
        return _T(G_BAD_ADDRESS_S); // "a bad address was specified in open"

    case G_BAD_FIRMWARE_LOAD: //!< Bad firmware upgrade
        return _T(G_BAD_FIRMWARE_LOAD_S); //   "Firmware upgrade failed"

    case G_GCAPS_OPEN_ERROR: //!< gcaps connection couldn't open. Server is not running or is not reachable.
        return _T(G_GCAPS_OPEN_ERROR_S); //   "gcaps connection could not be opened"

    case G_GCAPS_SUBSCRIPTION_ERROR: //!< GMessage(), GRecord(), GInterrupt() called on a connection without --subscribe switch.
        return _T(G_GCAPS_SUBSCRIPTION_ERROR_S); //   "function requires subscription not specified in GOpen()"

    default:
        return _T("");
    }
}
