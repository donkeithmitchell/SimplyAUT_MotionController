#include "pch.h"
#include "MagController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "define.h"
#include <Ws2tcpip.h>

static BOOL g_sensor_initialised = FALSE;
#ifdef _DEBUG_TIMING_
clock_t g_ThreadReadSocketTime = 0;
int g_ThreadReadSocketCount = 0;
#endif

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

// recv() only returns once it has read the required number of byes
// to prevent the main thread from being locked, use a thread to read the byutes
// can always timeout if not going to be able to read all requested bytes
static UINT ThreadReadSocket(LPVOID param)
{
    CMagControl* this2 = (CMagControl*)param;
    return this2->ThreadReadSocket();
}

// if unable to connect, then will hang up
// thus best to connect with a thread
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

    for (int i = 0; i < 6; ++i)
        m_magStatus[i] = INT_MAX;
    
    m_nConnect = 0;
    m_server  = INVALID_SOCKET;
    m_hTheadReadSocket = NULL;
    m_hTheadConnectSocket = NULL;
    m_fRGBCalibration = FLT_MAX;

    m_rgbData.SetSize(0);
    m_rgbCalibration.Reset();
    m_rgbPos = FLT_MAX;
}

CMagControl::~CMagControl()
{
    Disconnect();
}

// used to pass messages to the owner diaolog
void CMagControl::Init(CWnd* pParent, UINT nMsg)
{
    m_pParent = pParent;
    m_nMsg = nMsg;
}


BOOL CMagControl::Connect(const BYTE address[4], u_short Port)
{
	CString str;
	if (IsConnected())
		return TRUE;


	// Set a timer to look for being connected
	char ip_address[32];
    u_long ip = 0;
    
    // fiormat the IP address
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

    // get a handle for a socket
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

    // unlikelty that TerminateThread() will work, but if unable to connect, then liokely will terminate code anyways
    if (ret != WAIT_OBJECT_0 )
    {
        DWORD exit_code = 0;
        GetExitCodeThread(m_hTheadConnectSocket, &exit_code);
        SendErrorMessage("Connect MAG socket timed out");
        
        m_nConnect = SOCKET_ERROR;
        ::TerminateThread(m_hTheadConnectSocket, 0);
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

    // note that initialized
    g_sensor_initialised = TRUE;
    
    // check that can talk to the MAG board
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

// used so that main thread will not be permanently locked up if fail
UINT CMagControl::ThreadConnectSocket()
{
    m_nConnect = ::connect(m_server, (SOCKADDR*)&m_sockaddr_in, sizeof(m_sockaddr_in));
    m_eventSocket.SetEvent();
    return 0;
}

// 911 note that reading the register takes about 200 ms
int CMagControl::GetRGBCalibrationValue()
{
    int nValue = GetMagRegister(MAG_REG_RGB_CAL);
    return nValue;
}

// not used
double CMagControl::GetUserRGBCalibration()
{
    return m_fRGBCalibration;
}

// actually cannot set the calibration value
// it can only be set as the value being noted when this call is made
BOOL CMagControl::SetMagRGBCalibrationValue(double fCal)
{
    CString str;
    if (!IsConnected())
        return FALSE;

    m_fRGBCalibration = fCal;
    str.Format("RW %d %.0f\n", MAG_REG_RGB_CAL, fCal);
    int nRecv = Send(str);
    Sleep(SOCKET_RECV_DELAY);

    nRecv = GetMagRegister(MAG_REG_RGB_CAL);
    return nRecv == (int)(fCal+0.5);
}

// 911 this also takes about 200 ms (way too slow)
int  CMagControl::GetMagStatus()
{
    clock_t t1 = clock();
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

    m_critMagStatus.Lock();
    memset(m_magStatus, 0x0, 6 * sizeof(int));
    int ret = sscanf_s(buff + 5, "%d,%d,%d,%d,%d,%d",
        m_magStatus + MAG_STAT_IND_SC,
        m_magStatus + MAG_IND_ENC_DIR,
        m_magStatus + MAG_IND_RGB_LINE,
        m_magStatus + MAG_IND_MAG_ON,
        m_magStatus + MAG_IND_ENC_CNT,
        m_magStatus + MAG_IND_MAG_LOCKOUT);

    // insure that unread values are invalidated
    for (int i = ret; i < 6; ++i)
        m_magStatus[i] = INT_MAX;

    m_critMagStatus.Unlock();

// ind _DEBUG note the slow time to read the stgatus
#ifdef _DEBUG_TIMING_
    static clock_t first_time = -1;
    clock_t t2 = clock();
    if (first_time == -1 || t1 - first_time > 1000)
    {
        if (t2 - t1 > 10)
        {
            CString str;
            str.Format("GetMagStatus: %d ms", (int)(t2 - t1));
            SendDebugMessage(str);
        }
        first_time = t1;
    }
#endif
    return ret;
}

// this takes < 1 ms
// it is only the recv() which is slow
int CMagControl::Send(const CString& str)
{
    time_t t1 = clock();
    int nRecv = ::send(m_server, str, str.GetLength(), 0);

    if (clock() - t1 > 10)
    {
        CString str;
        str.Format("Send: %d ms", (int)(clock() - t1));
        SendDebugMessage(str);
    }

    return nRecv;
}

// this enables or disables the swith on the unit
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


// read the socket with a thread so can't permanently lock up the main thread
size_t CMagControl::ReadMagBuffer(char* buff, size_t nSize)
{
    clock_t t1 = clock();
    // spin a thread to read the MAG buffer
    // the thread will read until find the terminating character
    m_eventSocket.ResetEvent();
    m_strBuffer[0] = 0;

    // recv() is in a thread as it does not return until it received the requested number of bytes
    // if it hangs, it is possibloe to terminate the thread
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
        // TerminateThread() probably does not work (911)
        // this should not happend regardless
        DWORD exit_code = 0;
        GetExitCodeThread(m_hTheadReadSocket, &exit_code);
        SendErrorMessage("Reading MAG board timed out");
        ::TerminateThread(m_hTheadReadSocket, 0);

        m_strBuffer[0] = 0;
        len = 0;
    }
    else
    {
        strcpy_s(buff, SOCKET_BUFF_LENGTH, m_strBuffer);
        len = (int)strlen(m_strBuffer);
    }

    // send the timing to the debug status dialog
    // but not more oftern than every second
#ifdef _DEBUG_TIMING_
    static clock_t first_time = -1;
    clock_t t2 = clock();
    if (first_time == -1 || t1 - first_time > 1000)
    {
        if (t2 - t1 > 1000)
        {
            CString str;
            str.Format("ReadMagBuffer: %d ms", (int)(t2 - t1));
            SendDebugMessage(str);
        }
        first_time = t1;
    }
#endif
    return len;
}
// read bytes from the socket until get the terminating character (\r)
// tried to read the initial block as 10 characters, then 1 per after that
// unbless a significant delay prior to asking for characters, would return NULL characters
// the intial delay removed any advantage of asking for a larger block
UINT CMagControl::ThreadReadSocket()
{
    memset(m_strBuffer, 0x0, SOCKET_BUFF_LENGTH);

    char stop_char = '\n';
    clock_t t1 = clock();
    for (int i = 0; i < SOCKET_BUFF_LENGTH - 1; i += 1)
    {
        // there are no responses less than 0 characters
        int nRecv = ::recv(m_server, m_strBuffer + i, 1, 0);

        if (nRecv == 0 || nRecv == SOCKET_ERROR)
        {
            // this should not happen, so clear the buffer
            // don't assume that have valid data
            memset(m_strBuffer, 0x0, SOCKET_BUFF_LENGTH);
            break;
        }
        // toss any NULL characters at the start
        // there may be left over \r or \n characters from a pervious message
        else if (m_strBuffer[i] == 0)
            i--;
        else if (i == 0 && m_strBuffer[i] == '\r')
            i--;
        else if (i == 0 && m_strBuffer[i] == '\n')
            i--;

        // always set a NULL characxter behind each character read
        // thus can use string functionsd on at all times
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

#ifdef _DEBUG_TIMING_
    g_ThreadReadSocketTime += clock() - t1;
    g_ThreadReadSocketCount++;
#endif  

    // check if \r was the first character
    // 911 this should have been dealt with above
    // but was used before that code was written
    size_t len = strlen(m_strBuffer);
    if (len > 0 && m_strBuffer[0] == '\r')
    {
        for (int i = 1; i < (int)len; ++i)
            m_strBuffer[i - 1] = m_strBuffer[i];
        m_strBuffer[len - 1] = 0;
    }

    m_eventSocket.SetEvent();

    return 0;
}

// not used
BOOL CMagControl::IsMagSwitchEnabled()
{
    return TRUE;
}

// get the requested register 
// 911 this takes about 200 ms
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

    // verify that this response is for the register requested
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

// this is the sum of the RED,GREEN,BLUE values
// it is way to slow to ask for all three let alone just 1
int CMagControl::GetRGBSum()
{
    CString str;
    if (!IsConnected())
        return INT_MAX;

    int colour = GetMagRegister(MAG_REG_RGB_ALL);
    return colour;
}

// this is not used at this time as too slow to get all three
// 911 that would be 3*200 ms to get all three
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

// prior to a scan it is required to reset the encoder count
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

// only shown in the mag dialoig in _DEBUG mode
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

// close the socket and clean up
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

// only used in _DEBUG mode
void CMagControl::SendDebugMessage(const CString& msg)
{
#ifdef _DEBUG_TIMING_
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
    }
#endif
}

// show any errors in the error static
void CMagControl::SendErrorMessage(const char* msg)
{
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)msg);
    }
}

// 911
// ENCODER_TICKS_PER_MM is at best a guess
// lose ticks if scan too fast
// needs validation
static double CalculateEncoderDistance(int nCount)
{
    if (nCount == INT_MAX)
        return FLT_MAX;

    else
        return -nCount  / ENCODER_TICKS_PER_MM; 
}


double CMagControl::GetEncoderDistance()
{
    int nEncoderCount = GetMagStatus(MAG_IND_ENC_CNT);
    double fEncoderDistance = CalculateEncoderDistance(nEncoderCount);
    return fEncoderDistance;
}

// as slow to get status and get many values
// want to remember those values in a member variable
// mzake asccess thread safe
int CMagControl::GetMagStatus(int ind)
{
    m_critMagStatus.Lock();
    int ret = m_magStatus[ind];
    m_critMagStatus.Unlock();
    return ret;
}

// given a list of RGB (or laser) values in m_rgbData
// calculate there the line (white or black) or weld bump is
BOOL CMagControl::CalculateRGBCalibration(BOOL bWithLaser, BOOL bSeekBlackLine)
{
    // get the minimum point and aassume that is the line
    // get the median value of all, and use 1/2 way from minimum to median as threshold
    int nSize = (int)m_rgbData.GetSize();
    if (nSize == 0)
        return FALSE;

    CArray<double, double> X1, Y1, X2, Y2;
    X1.SetSize(nSize);
    Y1.SetSize(nSize);

    for (int i = 0; i < nSize; ++i)
    {
        X1[i] = m_rgbData[i].x;
        Y1[i] = m_rgbData[i].y;
    }

    // the laser weld cap heigfht is very spiky
    // can get single sample spikes
    // thus must filter it first
    CIIR_Filter filter(nSize);
    if (bWithLaser)
    {
        filter.MedianFilter(Y1.GetData(), 1); // this removes spikes
        filter.AveragingFilter(Y1.GetData(), 2); // woulkd filter those spikes if not for above
    }

    // get the median value
    Y2.Copy(Y1);
    qsort(Y2.GetData(), nSize, sizeof(double), ::MinMaxR8);
    double median = Y2[nSize / 2];
    
    int minInd = 0;
    int maxInd = 0;
    for (int i = 0; i < nSize; ++i)
    {
        // ignore the first 10 mm
        // assume that values higfher than twice mnedian are bogus if a laser
        // the weld cap bump will not be higher than twice the weld cap height
        if (bWithLaser && Y1[i] > 2 * median)
            continue;

        // do not asasume that the first 10 mm is valid data
        if (fabs(X1[i]-X1[0]) > 10)
        {
            if (minInd == -1 || Y1[i] < Y1[minInd])
                minInd = i;
            if (maxInd == -1 || Y1[i] > Y1[maxInd])
                maxInd = i;
        }
    }

    // THIS WILL BE DISPLAYED IN THE CALIBRATION GRAPH
    m_rgbCalibration.median = median;

    int i1, i2;
    double threshold;

    // laser and white line, looking for a value larger than the median
    if (bWithLaser || !bSeekBlackLine )
    {
        threshold = Y1[maxInd] - 2 * (Y1[maxInd] - median) / 3;

        // about the minimum, look for the threshold and model as a 2nd ordert
        for (i1 = maxInd; i1 >= 0 && Y1[i1] > threshold; --i1);
        for (i2 = maxInd; i2 < nSize && Y1[i2] > threshold; ++i2);
    }

    // a black line will be less than the median
    else
    {
        threshold = Y1[minInd] + (median - Y1[minInd]) / 4;

        // about the minimum, look for the threshold and model as a 2nd ordert
        for (i1 = minInd; i1 >= 0 && Y1[i1] < threshold; --i1);
        for (i2 = minInd; i2 < nSize && Y1[i2] < threshold; ++i2);
    }
    i1 = max(i1, 0);
    i2 = min(i2, nSize - 1);

    // noiw fit the data between the thresh9old values to a 2nd ordfer polynom,ial
    X2.SetSize(nSize);
    int cnt2 = 0;
    for (int i = i1; i <= i2; ++i)
    {
        X2[cnt2] = X1[i];
        Y2[cnt2] = Y1[i];
        cnt2++;
    }

    // need at least 3 samples for a 2nd order fit
    if (cnt2 >= 3)
    {
        double coeff[3];
        polyfit(X2.GetData(), Y2.GetData(), cnt2, 2, coeff);

        m_rgbCalibration.pos = -coeff[1] / (2 * coeff[2]);
        m_rgbCalibration.rgb = threshold;
    }
    else
    {
        int ind = bWithLaser ? (i2 + i1) / 2 : minInd;
        m_rgbCalibration.pos = X2[ind];
        m_rgbCalibration.rgb = threshold;
    }

    return TRUE;
}

// before calibration must remove previously noted values
void CMagControl::ResetRGBCalibration()
{
    m_rgbData.SetSize(0);
    m_rgbCalibration.Reset();
    m_rgbPos = FLT_MAX;
}

// get the value for a given idex
CDoublePoint CMagControl::GetRGBCalibrationData(int ind)const
{
    CDoublePoint ret;
    if (ind >= 0 && ind < m_rgbData.GetSize() )
        ret = m_rgbData[ind];

    return ret;
}

// add the noted value by position to a list
// nLength is the grow by value set to the length being scanned
void CMagControl::NoteRGBCalibration(double pos, double val, int nLength)
{

    // get the current position and RGB values
    int len = (int)m_rgbData.GetSize();
    if (pos != FLT_MAX && (len == 0 || fabs(pos - m_rgbData[len - 1].x) > 1))
    {
        if (val != FLT_MAX)
        {
            // set grow by to the maximum length
            m_rgbData.SetSize(++len, nLength);
            m_rgbData[len - 1].x = pos;
            m_rgbData[len - 1].y = val;
        }
    }
}


