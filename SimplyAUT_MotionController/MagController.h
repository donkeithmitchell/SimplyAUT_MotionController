#pragma once
#include "Misc.h"

enum {
    MAG_STAT_IND_SC = 0,        // AA 
    MAG_IND_ENC_DIR,            // BB (1=forward, 0=reverse)
    MAG_IND_RGB_LINE,           // CC (1=line present, 0=line not present)
    MAG_IND_MAG_ON,             // DD (1=magnets on, 0=magnets off)
    MAG_IND_ENC_CNT,            // EE (count)
    MAG_IND_MAG_LOCKOUT         // FF (1=locked out, 0=enabled)
};

struct RGB_CALIBRATION
{
    RGB_CALIBRATION() { Reset(); }
    void Reset(){ pos = rgb = median = FLT_MAX; }

    double pos;
    double rgb;
    double median;
    double dummy8;
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
    int     GetMagStatus();
    BOOL    GetMagVersion(int version[5]);
    int     GetRGBCalibrationValue();
    double  GetUserRGBCalibration();
    BOOL   SetMagRGBCalibrationValue(double);
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
    int     GetMagStatus(int ind);
    BOOL    CalculateRGBCalibration(BOOL, BOOL);
    void    ResetRGBCalibration();
    void    NoteRGBCalibration(double pos, double val, int len);
    const RGB_CALIBRATION& GetRGBCalibration()const { return m_rgbCalibration; }
    void    SetRGBCalibrationPos(double pos) { m_rgbPos = pos; }
    double  GetRGBCalibrationPos()const { return m_rgbPos; }
    CDoublePoint GetRGBCalibrationData(int ind)const;
    int GetRGBCalibrationDataSize()const {return (int)m_rgbData.GetSize(); }
    double  GetEncoderDistance();

protected:
    CWnd*   m_pParent;
    UINT    m_nMsg;
    SOCKET  m_server;
    int     m_nConnect;
    double  m_fRGBCalibration;
    sockaddr_in  m_sockaddr_in;
    HANDLE  m_hTheadReadSocket;
    HANDLE  m_hTheadConnectSocket;
    char    m_strBuffer[SOCKET_BUFF_LENGTH];
    CEvent  m_eventSocket;
    int     m_magStatus[6];
    CCriticalSection                    m_critMagStatus;
    CArray<CDoublePoint, CDoublePoint>  m_rgbData;
    RGB_CALIBRATION                     m_rgbCalibration;
    double			                    m_rgbPos;
};

