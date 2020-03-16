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
    int     GetRGBValues();
    BOOL    ResetEncoderCount();
    BOOL    LockoutMagSwitchControl(BOOL);
    int     GetMagRegister(int reg);
    size_t  ReadMagBuffer(char* buff, size_t nSize, char );
    void    ResetMagBuffer();
    BOOL    IsMagSwitchEnabled();
    UINT    ThreadReadSocket();
    char    GetNextBufferValue();

protected:
    SOCKET m_server;
    HANDLE m_hTheadReadSocket;
    CCriticalSection m_critBuffer;
    CArray<char, char> m_buffer;

};

