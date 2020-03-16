#include "pch.h"
#include "MagController.h"
#include <Ws2tcpip.h>

static BOOL g_sensor_initialised = FALSE;

enum{MAG_REG_HW=1, 
    MAG_REG_FW_MAJ,
    MAG_REG_FW_MIN,
    MAG_REG_FW_REV,
    MAG_REG_FW_DATE,
    MAG_REG_ENC_CNT=100,
    MAG_REG_LOCK_OUT=105,
    MAG_REG_RGB_CNT=108
};

enum {
    MAG_STAT_IND_SC = 0,        // AA 
    MAG_IND_ENC_DIR,            // BB (1=forward, 0=reverse)
    MAG_IND_RGB_STAT,           // CC (1=line present, 0=line not present)
    MAG_IND_MAG_ON,             // DD (1=magnets on, 0=magnets off)
    MAG_IND_ENC_CNT,            // EE (count)
    MAG_IND_MAG_LOCKOUT         // FF (1=locked out, 0=enabled)
};


static UINT ThreadReadSocket(LPVOID param)
{
    CMagControl* this2 = (CMagControl*)param;
    return this2->ThreadReadSocket();
}

CMagControl::CMagControl()
{
    m_server  = INVALID_SOCKET;
    m_hTheadReadSocket = NULL;
}

CMagControl::~CMagControl()
{
    Disconnect();
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
        AfxMessageBox("InetPton error occured");
        return FALSE;
    }


	////////////////////////////////////
    WSADATA wsaData;
    nRecv = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(nRecv != NO_ERROR)
    {
        int nRec2 = WSAGetLastError();
        str.Format("WSAStartup error (%d) occured", nRec2);
        :: AfxMessageBox(str);
        return FALSE;
    }
    m_server = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server == INVALID_SOCKET)
    {
        int nRec2 = WSAGetLastError();
        str.Format("socket error (%d) occured", nRec2);
        ::closesocket(m_server);
        m_server = INVALID_SOCKET;
        ::WSACleanup();
        ::AfxMessageBox(str);
        return FALSE;
    }

    sockaddr_in addr;
    memset(&addr, 0x0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(Port);

    nRecv = ::connect(m_server, (SOCKADDR*)&addr, sizeof(addr));
    if(nRecv == SOCKET_ERROR)
    {
        int nRec2 = WSAGetLastError();
        str.Format("connect error (%d) occured", nRec2);
        ::AfxMessageBox(str);
        ::closesocket(m_server);
        m_server = INVALID_SOCKET;
        ::WSACleanup();
        return FALSE;
    }

    g_sensor_initialised = TRUE;

    // now spin a thread to receive the bytes
    // recv() does not respond if no bytes to receive
  //  m_hTheadReadSocket = ::AfxBeginThread(::ThreadReadSocket, this);

    
    if( !ResetEncoderCount() )
    {
        ::AfxMessageBox("Disconnected from the server.");
        ::closesocket(m_server);
        m_server = INVALID_SOCKET;
        ::WSACleanup();
        g_sensor_initialised = FALSE;
        return FALSE;
    }

 
	// thjis is likely set in a call back functiion
	return TRUE;
}

int  CMagControl::GetMagStatus(int status[6])
{
    if (!IsConnected())
        return 0;

    ResetMagBuffer();
    int nRecv = ::send(m_server, "ST\n", 4, 0);
    Sleep(200);

    // read one byte at a time until get a \r
    // unlike a request for a register value
    // this requests is only azppended by \r, not \r\n
    char buff[2048];
  //  nRecv = ReadMagBuffer(buff, sizeof(buff), '\n');
    nRecv = ReadMagBuffer(buff, sizeof(buff), '\r');

    if (nRecv == 0)
        return 0;

    if (strncmp(buff, "$STA,",5) != 0)
        return 0;

    memset(status, 0x0, 6 * sizeof(int));
    int ret = sscanf_s(buff + 5, "%d,%d,%d,%d,%d,%d\n",
        status + MAG_STAT_IND_SC,
        status + MAG_IND_ENC_DIR,
        status + MAG_IND_RGB_STAT,
        status + MAG_IND_MAG_ON,
        status + MAG_IND_ENC_CNT,
        status + MAG_IND_MAG_LOCKOUT);
    return ret;
}


BOOL CMagControl::LockoutMagSwitchControl(BOOL bLockout)
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    str.Format("RW %d,%d\n", MAG_REG_LOCK_OUT, bLockout);
    ResetMagBuffer();
    int nRecv = ::send(m_server, str, str.GetLength(), 0);
    Sleep(200);

    nRecv = GetMagRegister(MAG_REG_LOCK_OUT);
    return TRUE;
}

UINT CMagControl::ThreadReadSocket()
{
    char buffer[1];
    while (m_server != INVALID_SOCKET && g_sensor_initialised )
    {
        int nRecv = ::recv(m_server, buffer, 1, 0);
        if (nRecv == SOCKET_ERROR )
            break;

        // get a critical section to append this to a buffer
        if (nRecv > 0)
        {
            m_critBuffer.Lock();
            m_buffer.Add(buffer[0]);
            m_critBuffer.Unlock();
        }
        Sleep(1);
    }

    return 0;
}

char CMagControl::GetNextBufferValue()
{
    char  ret = 0;
    m_critBuffer.Lock();
    if (m_buffer.GetSize() > 0)
    {
        ret = m_buffer[0];
        m_buffer.RemoveAt(0, 1);
    }
    m_critBuffer.Unlock();
    return ret;
}

size_t CMagControl::ReadMagBuffer(char* buff, size_t nSize, char stop_char)
{
    for (size_t i = 0; i < nSize; ++i)
    {
        int nRecv = ::recv(m_server, buff + i, 1, 0);
        if ((nRecv == SOCKET_ERROR) || (nRecv == 0))
            return i;
        if (buff[i] == stop_char)
             return i;
    }
    return nSize;

}


void CMagControl::ResetMagBuffer()
{
    m_critBuffer.Unlock();
    m_buffer.RemoveAll();
    m_critBuffer.Lock();
}
/*
size_t CMagControl::ReadMagBuffer(char* buff, size_t nSize, char stop_char)
{
    memset(buff, 0x0, nSize);

    for (size_t i = 0; i < nSize; ++i)
    {
        char temp = GetNextBufferValue();
        if (temp == 0)
            return i;

        buff[i] = temp;
        if (buff[i] == stop_char)
            return i;
    }
    return nSize;

}
*/
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
    ResetMagBuffer();
    int nRecv = ::send(m_server, str, str.GetLength(), 0);
    Sleep(200);

    char buff[2048];
    memset(buff, 0x0, sizeof(buff));
    nRecv = ReadMagBuffer(buff, sizeof(buff), '\n');
    if (nRecv == 0)
        return INT_MAX;

    if (strstr(buff, "$ERR") != NULL)
        AfxMessageBox(buff + 5);

    // look for $REG
    const char* ptr = strstr(buff, "$REG,");
    if (ptr == NULL)
        return 0;

    int version[2];
    int ret = sscanf_s(ptr + 5, "%d,%d\r\n", version + 0, version + 1);
    return (version[0] == reg) ? version[1] : INT_MAX;
}

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

int CMagControl::GetRGBValues()
{
    CString str;
    if (!IsConnected())
        return -1;

    str.Format("RR %d\n", MAG_REG_RGB_CNT);
    ResetMagBuffer();
    int nRecv = ::send(m_server, str, str.GetLength(), 0);
    Sleep(200);

    nRecv = GetMagRegister(MAG_REG_RGB_CNT);
    return (nRecv == INT_MAX) ? -1 : nRecv;
}


BOOL CMagControl::ResetEncoderCount()
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    str.Format("RW %d 0\n", MAG_REG_ENC_CNT);
    ResetMagBuffer();
    int nRecv = ::send(m_server, str, str.GetLength(), 0);
    Sleep(200);

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