#include "pch.h"
#include "MagController.h"
#include <Ws2tcpip.h>

static BOOL g_sensor_initialised = FALSE;

CMagController::CMagController()
{
	m_bEngaged = FALSE;
    m_server  = INVALID_SOCKET;
}

CMagController::~CMagController()
{

}

// this is only a stub
BOOL CMagController::Connect(const BYTE address[4], u_short Port)
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
        ::WSACleanup();
        return FALSE;
    }

    g_sensor_initialised = TRUE;
    
    int status[5];
    if( !GetMagStatus(status) )
    {
        ::AfxMessageBox("Disconnected from the server.");
        ::WSACleanup();
        g_sensor_initialised = FALSE;
        return FALSE;
    }

 
	// thjis is likely set in a call back functiion
	return TRUE;
}

BOOL  CMagController::GetMagStatus(int status[5])
{
    if (!IsConnected())
        return FALSE;

    int nRecv = ::send(m_server, "ST\r\n", 4, 0);

    char buff[2048];
    nRecv = ::recv(m_server, buff, sizeof(buff), 0);
    if ((nRecv == SOCKET_ERROR) || (nRecv == 0))
        return FALSE;

    if (strncmp(buff, "$STA,",5) != 0)
        return 0;

    int ret = sscanf_s(buff+5, "%d,%d,%d,%d\r", status + 0, status + 1, status + 2, status + 3);
    return TRUE;
}

BOOL CMagController::Disconnect()
{
	if (g_sensor_initialised)
	{
        ::WSACleanup();
		g_sensor_initialised = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}



BOOL CMagController::AreWheelsEngaged()const
{
	return IsConnected() && m_bEngaged;
}

void CMagController::SetWheelsEngaged(BOOL set)
{
	m_bEngaged = set;
}

BOOL CMagController::IsConnected()const
{
	return g_sensor_initialised;
}