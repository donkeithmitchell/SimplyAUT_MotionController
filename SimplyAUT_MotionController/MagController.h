#pragma once
class CMagControl
{
public:
    CMagControl();
    ~CMagControl();

    BOOL    IsConnected()const;
    BOOL    Connect(const BYTE address[4], u_short port);
    BOOL    Disconnect();
    int     GetMagStatus(int status[6]);
    BOOL    GetMagVersion(int version[5]);
    int     AreMagnetsEngaged();
    int     GetMagSwitchLockedOut();
    int     GetEncoderCount();
    BOOL    GetRGBValues(int& red, int& green, int& blue);
    BOOL    ResetEncoderCount();
    BOOL    EnableMagSwitchControl(BOOL);
    int     GetMagRegister(int reg);
    size_t  ReadMagBuffer(char* buff, size_t nSize );
    BOOL    IsMagSwitchEnabled();
    UINT    ThreadReadSocket();
    int     ReadSocket(char* buffer, int nSize);

protected:
    SOCKET  m_server;
    HANDLE  m_hTheadReadSocket;
    char    m_byteSocket;
    CEvent  m_eventSocket;

};

