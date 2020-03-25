#pragma once

enum {
    MAG_STAT_IND_SC = 0,        // AA 
    MAG_IND_ENC_DIR,            // BB (1=forward, 0=reverse)
    MAG_IND_RGB_LINE,           // CC (1=line present, 0=line not present)
    MAG_IND_MAG_ON,             // DD (1=magnets on, 0=magnets off)
    MAG_IND_ENC_CNT,            // EE (count)
    MAG_IND_MAG_LOCKOUT         // FF (1=locked out, 0=enabled)
};

#define SOCKET_BUFF_LENGTH 256


class CMagControl
{
public:
    CMagControl();
    ~CMagControl();

    void    Init(CWnd* pParent, UINT nMsg);
    BOOL    IsConnected()const;
    BOOL    Connect(const BYTE address[4], u_short port);
    BOOL    Disconnect();
    int     GetMagStatus(int status[6]);
    BOOL    GetMagVersion(int version[5]);
    int     GetMagRGBCalibration();
    BOOL   SetMagRGBCalibration(int);
    //   int     AreMagnetsEngaged();
 //   int     GetMagSwitchLockedOut();
 //   int     GetEncoderCount();
    int     GetRGBValues(int& red, int& green, int& blue);
    int     GetRGBSum();
    BOOL    ResetEncoderCount();
    BOOL    EnableMagSwitchControl(BOOL);
    int     GetMagRegister(int reg);
    size_t  ReadMagBuffer(char* buff, size_t nSize );
    BOOL    IsMagSwitchEnabled();
    UINT    ThreadReadSocket();
    UINT    ThreadConnectSocket();
    void    SendDebugMessage(const CString& msg);
    void    SendErrorMessage(const CString& msg);
    int     Send(const CString& str);

protected:
    CWnd*   m_pParent;
    UINT    m_nMsg;
    SOCKET  m_server;
    int     m_nConnect;
    sockaddr_in  m_sockaddr_in;
    HANDLE  m_hTheadReadSocket;
    HANDLE  m_hTheadConnectSocket;
    char    m_strBuffer[SOCKET_BUFF_LENGTH];
    CEvent  m_eventSocket;

};

