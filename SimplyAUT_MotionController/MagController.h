#pragma once
class CMagController
{
public:
    CMagController();
    ~CMagController();

    BOOL    AreWheelsEngaged()const;
    void    SetWheelsEngaged(BOOL set);
    BOOL    IsConnected()const;
    BOOL    Connect(const BYTE address[4], u_short port);
    BOOL    Disconnect();
    BOOL    GetMagStatus(int status[5]);

protected:
    BOOL m_bEngaged;
    SOCKET m_server;

};

