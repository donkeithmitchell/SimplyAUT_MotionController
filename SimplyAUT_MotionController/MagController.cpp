#include "pch.h"
#include "MagController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include <Ws2tcpip.h>

static BOOL g_sensor_initialised = FALSE;

enum{MAG_REG_HW=1, 
    MAG_REG_FW_MAJ,
    MAG_REG_FW_MIN,
    MAG_REG_FW_REV,
    MAG_REG_FW_DATE,
    MAG_REG_ENC_CNT=100,
    MAG_REG_LOCK_OUT=105,
    MAG_REG_RGB_CAL=108,
    MAG_REG_RGB_RED = 110,
    MAG_REG_RGB_GREEN = 111,
    MAG_REG_RGB_BLUE = 112,
    MAG_REG_RGB_ALL=116
};

#define SOCKET_RECV_DELAY 1
#define SOCKET_RECV_TIMEOUT 2000
#define SOCKET_CONNECT_TIMEOUT 500


static UINT ThreadReadSocket(LPVOID param)
{
    CMagControl* this2 = (CMagControl*)param;
    return this2->ThreadReadSocket();
}

static UINT ThreadConnectSocket(LPVOID param)
{
    CMagControl* this2 = (CMagControl*)param;
    return this2->ThreadConnectSocket();
}

CMagControl::CMagControl()
{
    m_pParent = NULL;
    m_nMsg = 0;
    memset(m_strBuffer, 0x0, sizeof(m_strBuffer));
    memset(&m_sockaddr_in, 0x0, sizeof(m_strBuffer));

    m_nConnect = 0;
    m_server  = INVALID_SOCKET;
    m_hTheadReadSocket = NULL;
    m_hTheadConnectSocket = NULL;
}

CMagControl::~CMagControl()
{
    Disconnect();
}

void CMagControl::Init(CWnd* pParent, UINT nMsg)
{
    m_pParent = pParent;
    m_nMsg = nMsg;
}



// this is only a stub
BOOL CMagControl::Connect(const BYTE address[4], u_short Port)
{
	CString str;
	if (IsConnected())
		return TRUE;


	// Set a timer to look for being connected
	char ip_address[32];
    u_long ip = 0;
    
    sprintf_s(ip_address, sizeof(ip_address), "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);
    int nRecv = InetPton(AF_INET, ip_address, (void*)&ip);
    if(nRecv != 1)
    {
        SendErrorMessage("Error: InetPton Error");
        return FALSE;
    }


	////////////////////////////////////
    WSADATA wsaData;
    nRecv = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(nRecv != NO_ERROR)
    {
        int nRec2 = WSAGetLastError();
        str.Format("Error: WSAStartup Error (%d)", nRec2);
        SendErrorMessage(str);
        return FALSE;
    }
    m_server = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server == INVALID_SOCKET)
    {
        int nRec2 = WSAGetLastError();
        str.Format("Error: Socket Error (%d)", nRec2);
        ::closesocket(m_server);
        m_server = INVALID_SOCKET;
        ::WSACleanup();
        SendErrorMessage(str);
        return FALSE;
    }

    memset(&m_sockaddr_in, 0x0, sizeof(m_sockaddr_in));

    m_sockaddr_in.sin_family = AF_INET;
    m_sockaddr_in.sin_addr.s_addr = ip;
    m_sockaddr_in.sin_port = htons(Port);

    // if the power is not on, then this will hang
    // spin a thread to do, and wait for about 2 seconds
    m_eventSocket.ResetEvent();
    m_nConnect = 0;
    m_hTheadConnectSocket = ::AfxBeginThread(::ThreadConnectSocket, this);
    int ret = ::WaitForSingleObject(m_eventSocket, SOCKET_CONNECT_TIMEOUT);
//    nRecv = ::connect(m_server, (SOCKADDR*)&addr, sizeof(addr));

    if (ret != WAIT_OBJECT_0 )
    {
        m_nConnect = SOCKET_ERROR;
        ::TerminateThread(m_hTheadReadSocket, 0);
    }

 //   nRecv = ::connect(m_server, (SOCKADDR*)&addr, sizeof(addr));
    if(m_nConnect == SOCKET_ERROR)
    {
        int nRec2 = WSAGetLastError();
        str.Format("Error: Connect Error (%d)", nRec2);
        SendErrorMessage(str);
        ::closesocket(m_server);
        m_server = INVALID_SOCKET;
        ::WSACleanup();
        return FALSE;
    }

    g_sensor_initialised = TRUE;
    
    if( !ResetEncoderCount() )
    {
        SendErrorMessage("Error: Disconnected from the Server.");
        ::closesocket(m_server);
        m_server = INVALID_SOCKET;
        ::WSACleanup();
        g_sensor_initialised = FALSE;
        return FALSE;
    }

 
	// thjis is likely set in a call back functiion
	return TRUE;
}

UINT CMagControl::ThreadConnectSocket()
{
    m_nConnect = ::connect(m_server, (SOCKADDR*)&m_sockaddr_in, sizeof(m_sockaddr_in));
    m_eventSocket.SetEvent();
    return 0;
}

int CMagControl::GetMagRGBCalibration()
{
    int nValue = GetMagRegister(MAG_REG_RGB_CAL);
    return nValue;

}

BOOL CMagControl::SetMagRGBCalibration(int nCal)
{
    CString str;
    if (!IsConnected())
        return FALSE;

    str.Format("RW %d %d\n", MAG_REG_RGB_CAL, nCal);
    int nRecv = Send(str);
    Sleep(SOCKET_RECV_DELAY);

    nRecv = GetMagRegister(MAG_REG_RGB_CAL);
    return nRecv == nCal;
}

int  CMagControl::GetMagStatus(int status[6])
{
    if (!IsConnected())
        return 0;

    int nRecv = Send(_T("ST\n"));
    Sleep(SOCKET_RECV_DELAY);

    // read one byte at a time until get a \r
    // unlike a request for a register value
    // this requests is only azppended by \r, not \r\n
    char buff[SOCKET_BUFF_LENGTH];
    nRecv = ReadMagBuffer(buff, sizeof(buff));

    if (nRecv == 0)
        return 0;

    if (strncmp(buff, "$STA,",5) != 0)
        return 0;

    memset(status, 0x0, 6 * sizeof(int));
    int ret = sscanf_s(buff + 5, "%d,%d,%d,%d,%d,%d",
        status + MAG_STAT_IND_SC,
        status + MAG_IND_ENC_DIR,
        status + MAG_IND_RGB_LINE,
        status + MAG_IND_MAG_ON,
        status + MAG_IND_ENC_CNT,
        status + MAG_IND_MAG_LOCKOUT);

    // insure that unread values are invalidated
    for (int i = ret; i < 6; ++i)
        status[i] = INT_MAX;

    return ret;
}

// this takes < 1 ms
// it is only the recv() which is slow
int CMagControl::Send(const CString& str)
{
    time_t tim1 = clock();
    int nRecv = ::send(m_server, str, str.GetLength(), 0);
    time_t send_tim = clock() - tim1;

//   CString temp;
//  temp.Format("Send: %d ms", send_tim);
//    SendDebugMessage(temp);
    return nRecv;
}


BOOL CMagControl::EnableMagSwitchControl(BOOL bEnabled)
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    str.Format("RW %d %d\n", MAG_REG_LOCK_OUT, !bEnabled);
    int nRecv = Send(str);
    Sleep(SOCKET_RECV_DELAY);

    nRecv = GetMagRegister(MAG_REG_LOCK_OUT);
    return nRecv == !bEnabled;
}

// spin a thread to read the socket to avoid hanging up
/*
int CMagControl::ReadSocket(char* buffer, int nSize)
{
    int nRecv = ::recv(m_server, buffer, nSize, MSG_PARTIAL);
    return nRecv;
}
*/





size_t CMagControl::ReadMagBuffer(char* buff, size_t nSize)
{
    // spin a thread to read the MAG buffer
    // the thread will read until find the terminating character
    m_eventSocket.ResetEvent();
    m_strBuffer[0] = 0;

    memset(buff, 0x0, nSize);
 //   ThreadReadSocket();
    m_hTheadReadSocket = ::AfxBeginThread(::ThreadReadSocket, this);

    // timeout if can't read the entire message
    // by using a thread and a finite timeout
    // avoid locking up code if don't find the tertminating character
    int ret = ::WaitForSingleObject(m_eventSocket, SOCKET_RECV_TIMEOUT);

    int len = 0;
    if (ret != WAIT_OBJECT_0)
    {
        DWORD exit_code = 0;
        GetExitCodeThread(m_hTheadReadSocket, &exit_code);
 //       if (exit_code == STILL_ACTIVE)
        {
            ::TerminateThread(m_hTheadReadSocket, 0);
//            CloseHandle(m_hTheadReadSocket);
        }
        m_strBuffer[0] = 0;
        len = 0;
    }
    else
    {
        strcpy_s(buff, SOCKET_BUFF_LENGTH, m_strBuffer);
        len = (int)strlen(m_strBuffer);
    }

    return len;
}

// read bytes from the socket until get the terminating character (\r)
// tried to read the initial block as 10 characters, then 1 per after that
// unbless a significant delay prior to asking for characters, would return NULL characters
// the intial delay removed any advantage of asking for a larger block
UINT CMagControl::ThreadReadSocket()
{
    static clock_t total_time = 0;
    static clock_t max_time = 0;
    int total_count = 0;

    clock_t recv_time = 0;
    memset(m_strBuffer, 0x0, SOCKET_BUFF_LENGTH);

    char stop_char = '\n';
    int byte_count = 0;
    for (int i = 0; i < SOCKET_BUFF_LENGTH - 1; i += 1)
    {
        // there are no responses less than 0 characters
        clock_t t2 = clock();
        int nRecv = ::recv(m_server, m_strBuffer + i, 1, 0);
        recv_time = clock() - t2;
        byte_count += nRecv;

        if (nRecv == 0 || nRecv == SOCKET_ERROR)
        {
            // this should not happen, so clear the buffer
            // don't assume that have valid data
            memset(m_strBuffer, 0x0, SOCKET_BUFF_LENGTH);
            break;
        }
        // toss any NULL characters at the start
        else if (m_strBuffer[i] == 0)
            i--;
        else if (i == 0 && m_strBuffer[i] == '\r')
            i--;
        else if (i == 0 && m_strBuffer[i] == '\n')
            i--;
        else
        {
            m_strBuffer[i + 1] = 0; // terminate so can use string functions on
            //if (stop_char == '\n' && strstr(m_strBuffer, "$STA") != NULL)
            //    stop_char = '\r';

            if (i > 0 && m_strBuffer[i] == stop_char)
            {
                break;
            }
        }
    }

    size_t len = strlen(m_strBuffer);
    if (len > 0 && m_strBuffer[0] == '\r')
    {
        for (int i = 1; i < (int)len; ++i)
            m_strBuffer[i - 1] = m_strBuffer[i];
        m_strBuffer[len - 1] = 0;
    }

    m_eventSocket.SetEvent();

    total_time += recv_time;
    total_count++;
    max_time = max(max_time, recv_time);

    if (recv_time > 10)
    {
        CString temp = _T(m_strBuffer);
        int len3 = temp.Find("\r");
        if (len3 != -1)
            temp = temp.Left(len3);

        CString str2;
        str2.Format("ThreadReadSocket: len: %d,  max: %d, this: %d ms, avg: %d: %s", byte_count, max_time, recv_time, total_time / total_count, temp);
        SendDebugMessage(str2);
    }
    return 0;
}

BOOL CMagControl::IsMagSwitchEnabled()
{
    return TRUE;

}

int CMagControl::GetMagRegister(int reg)
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    str.Format("RR %d\n", reg);
    int nRecv = Send(str);
    Sleep(SOCKET_RECV_DELAY);

    char buff[2048];
    memset(buff, 0x0, sizeof(buff));
    nRecv = ReadMagBuffer(buff, sizeof(buff));
    if (nRecv == 0)
        return INT_MAX;

    if (strstr(buff, "$ERR") != NULL)
        SendErrorMessage(CString("ERROR: ") + CString(buff + 5));

    // look for $REG
    const char* ptr = strstr(buff, "$REG,");
    if (ptr == NULL)
        return 0;

    int version[2];
    int ret = sscanf_s(ptr + 5, "%d,%d", version + 0, version + 1);

    return (version[0] == reg) ? version[1] : INT_MAX;
}
/*
int CMagControl::AreMagnetsEngaged()
{
    int status[6];
    if (GetMagStatus(status) <= MAG_IND_MAG_ON)
        return -1;
    else
        return status[MAG_IND_MAG_ON] != 0;
}


int CMagControl::GetMagSwitchLockedOut()
{
    int status[6];
    if (GetMagStatus(status) <= MAG_IND_MAG_LOCKOUT)
        return -1;
    else
        return status[MAG_IND_MAG_LOCKOUT] == 1;
}

int CMagControl::GetEncoderCount()
{
    int status[6];
    if (GetMagStatus(status) <= MAG_IND_ENC_CNT)
        return -1;
    else
        return status[MAG_IND_ENC_CNT];
}
*/
int CMagControl::GetRGBSum()
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    int colour = GetMagRegister(MAG_REG_RGB_ALL);
    return colour;
}

int CMagControl::GetRGBValues(int& red, int& green, int& blue)
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    red = GetMagRegister(MAG_REG_RGB_RED);
    green = GetMagRegister(MAG_REG_RGB_GREEN);
    blue = GetMagRegister(MAG_REG_RGB_BLUE);
    int colour = GetMagRegister(MAG_REG_RGB_ALL);

    return (red == INT_MAX || green == INT_MAX || blue == INT_MAX || colour == INT_MAX) ? INT_MAX : colour;
}


BOOL CMagControl::ResetEncoderCount()
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    str.Format("RW %d 0\n", MAG_REG_ENC_CNT);
    int nRecv = Send(str);
    Sleep(SOCKET_RECV_DELAY);

    nRecv = GetMagRegister(MAG_REG_ENC_CNT);

    return TRUE;
}


BOOL CMagControl::GetMagVersion(int version[5])
{
    if (!IsConnected())
        return FALSE;

    version[0] = GetMagRegister(MAG_REG_HW);
    version[1] = GetMagRegister(MAG_REG_FW_MAJ);
    version[2] = GetMagRegister(MAG_REG_FW_MIN);
    version[3] = GetMagRegister(MAG_REG_FW_REV);
    version[4] = GetMagRegister(MAG_REG_FW_DATE);

    return (version[0] == INT_MAX || 
        version[1] == INT_MAX || 
        version[2] == INT_MAX || 
        version[3] == INT_MAX || 
        version[4] == INT_MAX ) ? FALSE : TRUE;
}

BOOL CMagControl::Disconnect()
{
	if (g_sensor_initialised)
	{
        ::closesocket(m_server);
        m_server = INVALID_SOCKET;
        ::WSACleanup();

        // the thread will only run while the sensor initialized
		g_sensor_initialised = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}


BOOL CMagControl::IsConnected()const
{
	return g_sensor_initialised;
}

void CMagControl::SendDebugMessage(const CString& msg)
{
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
    }
}

void CMagControl::SendErrorMessage(const CString& msg)
{
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)&msg);
    }
}

