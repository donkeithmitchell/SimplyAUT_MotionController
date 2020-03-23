#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "SimplyAUT_MotionControllerDlg.h"
#include "MotionControl.h"
#include "Gclib2.h"

static double PI = 3.14159265358979323846;
//static const  double COUNTS_PER_TURN = 768000.0;
static const  double COUNTS_PER_TURN = (768000.0 / 0.68);
static const double WHEEL_DIAMETER = 50.0; // MM


CMotionControl::CMotionControl()
{
    m_pParent = NULL;
    m_nMsg = 0;
	m_pGclib = NULL;
}

CMotionControl::~CMotionControl()
{
    if (m_pGclib)
    {
        StopMotors();
        m_pGclib->StopMotors(); //stop all motion and programs
        m_pGclib->WaitForMotorsToStop(); //Block until motion is complete on vector plane S
        m_pGclib->MotorsOff();
    }

	delete m_pGclib;
	m_pGclib = NULL;
}

void CMotionControl::Init(CWnd* pParent, UINT nMsg)
{
    m_pParent = pParent;
    m_nMsg = nMsg;
}

void CMotionControl::SendDebugMessage(const CString& msg)
{
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SEND_DEBUGMSG, (WPARAM)&msg);
    }
}

void CMotionControl::SendErrorMessage(const CString& msg)
{
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_ERROR_MSG, (WPARAM)&msg);
    }
}

void CMotionControl::EnableControls()
{
    if (m_pParent && m_nMsg && IsWindow(m_pParent->m_hWnd) && m_pParent->IsKindOf(RUNTIME_CLASS(CSimplyAUTMotionControllerDlg)))
    {
        m_pParent->SendMessage(m_nMsg, CSimplyAUTMotionControllerDlg::MSG_SETBITMAPS);
    }
}

BOOL CMotionControl::IsConnected()const
{
	return m_pGclib != NULL && m_pGclib->IsConnected();	
}

BOOL CMotionControl::Disconnect()
{
    if (m_pGclib == NULL)
        return FALSE;
    
    if (!m_pGclib->GClose())
        return FALSE;

    delete m_pGclib;
    m_pGclib = NULL;
    EnableControls();
    return TRUE;
}

BOOL CMotionControl::Connect(const BYTE address[4], double dScanSpeed)
{
	if (IsConnected() )
		return TRUE;

    if(m_pGclib == NULL )
         m_pGclib = new(Gclib);
	
    // this will enable the GCLIB to send messages to the owner of the motion control
    m_pGclib->Init(m_pParent, m_nMsg);

    if (!m_pGclib->CheckDllExist())
    {
        AfxMessageBox(m_pGclib->GetLastError());
        return FALSE;
    }

    char strAddress[256];
    sprintf_s(strAddress, sizeof(strAddress), "%d.%d.%d.%d  --direct --subscribe ALL", address[0], address[1], address[2], address[3]);
 //   sprintf_s(strAddress, sizeof(strAddress), "%d.%d.%d.%d  --subscribe ALL", address[0], address[1], address[2], address[3]);
    SendDebugMessage(_T("Opening connection to \"") + CString(strAddress) + _T("\"... "));

    if( !m_pGclib->GOpen(strAddress) )
    {
        SendErrorMessage(m_pGclib->GetLastError());
        return FALSE;
    }

    CString str = m_pGclib->GInfo(); //grab connection string
    SendDebugMessage(str);

    str = m_pGclib->GVersion();
    SendDebugMessage(_T("Version: ") + str);


    SendDebugMessage(_T("Initialization of the Galil..."));
    m_pGclib->StopMotors(); //stop all motion and programs
    m_pGclib->WaitForMotorsToStop(); //Block until motion is complete on vector plane S

    m_pGclib->GCommand("KP*=1.05");     // proportional constant
    m_pGclib->GCommand("KI*=0");        // integrator
    m_pGclib->GCommand("KD*=0");        // derivative constant
    m_pGclib->GCommand("ER*=20000");    // magnitude of the position errors cnts
    m_pGclib->GCommand("OE*=1");        // Off On Error function   
    m_pGclib->GCommand("BR*=0");
    m_pGclib->GCommand("AU*=0.5");
    m_pGclib->GCommand("AG*=0");
    m_pGclib->GCommand("TL*=1.655");
    m_pGclib->DefinePosition(0);        // all to zero
    m_pGclib->SetAcceleration(50000);    // acceleration cts/sec
    m_pGclib->SetDeceleration(50000);    // deceleration cts/sec
    m_pGclib->SetJogSpeed(0);           // always connect at zero speed
    m_pGclib->SetServoHere();           // enable all axes
    m_pGclib->SetSlewSpeed(40000);     // speed all axes cts/sec
 //   m_pGclib->GCommand("PR 100000");     // move distance all axes cts
 //   m_pGclib->GCommand("BG");     // move distance all axes cts

  //  StopMotors();
 
    EnableControls();
    return TRUE; //  SetMotorSpeed(dScanSpeed);
}

// resstr the motor positioons
// will use this zero as a reference for the start
// can only do this if the mnotor is stopped
void CMotionControl::ZeroPositions()
{
    m_pGclib->StopMotors(); //stop all motion and programs
    m_pGclib->WaitForMotorsToStop(); //Block until motion is complete on vector plane S
    m_pGclib->DefinePosition(0);        // all to zero
}
void CMotionControl::GoToHomePosition()
{
    m_pGclib->GCommand("PA 0,0,0,0");
    m_pGclib->GCommand("SH");           // enable all axes
    if (!m_pGclib->GCommand("BG*"))   // Begin motion on all Axis
    {
        SendErrorMessage(m_pGclib->GetLastError());
    }
}
    

BOOL CMotionControl::GoToPosition(double pos_mm, double fSpeed, double fAccel)
{
    CString str;

    // set the motor speeds
    m_pGclib->SetAcceleration(DistancePerSecondToEncoderCount(fAccel));
    m_pGclib->SetDeceleration(DistancePerSecondToEncoderCount(fAccel));
    SetSlewSpeed(fSpeed);

    int pos_cnt = DistancePerSecondToEncoderCount(pos_mm);
    int posA = AxisDirection("A") * pos_cnt;
    int posB = AxisDirection("B") * pos_cnt;
    int posC = AxisDirection("C") * pos_cnt;
    int posD = AxisDirection("D") * pos_cnt;
    str.Format("PA %d, %d, %d, %d", posA, posB, posC, posD);

    m_pGclib->GCommand(str);
    m_pGclib->GCommand("SH");           // enable all axes
    if (!m_pGclib->GCommand("BG*"))   // Begin motion on all Axis
    {
        SendErrorMessage(m_pGclib->GetLastError());
        return FALSE;
    }


//    int posA = AxisDirection("A") * DistancePerSecondToEncoderCount(pos_mm);
//    int posB = AxisDirection("B") * DistancePerSecondToEncoderCount(pos_mm);
//    int posC = AxisDirection("C") * DistancePerSecondToEncoderCount(pos_mm);
//    int posD = AxisDirection("D") * DistancePerSecondToEncoderCount(pos_mm);
//    m_pGclib->GoToPosition(posA, posB, posC, posD);
 //   m_pGclib->GoToPosition(pos_cnt);

 //   m_pGclib->SetServoHere();
 //   m_pGclib->BeginMotors();   // Begin motion on all Axis

    // now wait for the mnotors to stop
    // this xshould be called by a thread
    m_pGclib->WaitForMotorsToStop();
    m_pGclib->StopMotors();
    return TRUE;
}

void CMotionControl::StopMotors()
{
    m_pGclib->StopMotors();    // stop all motors
    m_pGclib->WaitForMotorsToStop(); 
}
BOOL CMotionControl::SetMotorJogging(double speed, double accel)
{
    return SetMotorJogging(speed, speed, speed, speed, accel);
}

double CMotionControl::GetMotorSpeed(GCStringIn axis, double& rAccel)
{
    int accel, speed = m_pGclib->GetMotorSpeed(axis, accel); // thjis is in counts, want the speed in mm/sec
    rAccel = EncoderCountToDistancePerSecond(accel);
    double fSpeed = EncoderCountToDistancePerSecond(speed);
    return (fSpeed == FLT_MAX) ? FLT_MAX : AxisDirection(axis) * fSpeed;
}

double CMotionControl::GetMotorPosition(GCStringIn axis)
{
    int pos = m_pGclib->GetMotorPosition(axis); // thjis is in ticks, want thew speed in mm/sec
    double fPos = EncoderCountToDistancePerSecond(pos);
    return AxisDirection(axis) *  fPos;
}

int CMotionControl::AxisDirection( GCStringIn axis)const
{
    switch (axis[0])
    {
    case 'A': return -1;
    case 'B': return 1;
    case 'C': return 1;
    case 'D': return -1;
    default: return 1;
    }

}

BOOL CMotionControl::AreMotorsRunning()
{
    double accel = 0;

    double fSA = GetMotorSpeed("A", accel);
    double fSB = GetMotorSpeed("B", accel);
    double fSC = GetMotorSpeed("C", accel);
    double fSD = GetMotorSpeed("D", accel);

    return fabs(fSA) > 0 || fabs(fSB) > 0 || fabs(fSC) > 0 || fabs(fSD) > 0;
}


BOOL CMotionControl::SetMotorJogging(double speedA, double speedB, double speedC, double speedD, double accel)
    {
    if (!IsConnected())
        return FALSE;

    m_pGclib->GCommand("KP*=1.05");     // proportional constant
    m_pGclib->GCommand("KI*=0");        // integrator
    m_pGclib->GCommand("KD*=0");        // derivative constant
    m_pGclib->GCommand("ER*=20000");    // magnitude of the position errors cnts
    m_pGclib->GCommand("OE*=1");        // Off On Error function   
    m_pGclib->GCommand("BR*=0");
    m_pGclib->GCommand("AU*=0.5");
 // m_pGclib->GCommand("AG*=0");        // amplifier gain (not while motor is run ning)
    m_pGclib->GCommand("TL*=1.655");
 // m_pGclib->DefinePosition(0);        // define position all to zero  (not while motor is run ning)
//  m_pGclib->GCommand("AC*=50000");    // acceleration cts/sec (leave as were)
//  m_pGclib->GCommand("DC*=50000");    // deceleration cts/sec

    // the left side motors are inverted
    m_pGclib->SetAcceleration(DistancePerSecondToEncoderCount(accel));
    m_pGclib->SetDeceleration(DistancePerSecondToEncoderCount(accel));

    m_pGclib->SetJogSpeed("A", AxisDirection("A") * DistancePerSecondToEncoderCount(speedA));
    m_pGclib->SetJogSpeed("B", AxisDirection("B") * DistancePerSecondToEncoderCount(speedB));
    m_pGclib->SetJogSpeed("C", AxisDirection("C") * DistancePerSecondToEncoderCount(speedC));
    m_pGclib->SetJogSpeed("D", AxisDirection("D") * DistancePerSecondToEncoderCount(speedD));

    m_pGclib->GCommand("SH");       // enable all axes
    if (m_pGclib->BeginMotors())   // Begin motion on all Axis
        return TRUE;

    SendErrorMessage(m_pGclib->GetLastError());
    return FALSE;
}

double CMotionControl::EncoderCountToDistancePerSecond(int encoderCount)const
{
    double distancePerSecond;
    double perimeter = PI * WHEEL_DIAMETER;

    distancePerSecond = (encoderCount == INT_MAX) ? FLT_MAX : ((double)encoderCount / COUNTS_PER_TURN) * perimeter;
    return distancePerSecond;
}

int CMotionControl::DistancePerSecondToEncoderCount(double DistancePerSecond)const
{
    double perimeter = PI * WHEEL_DIAMETER;

    int encoderCount = (DistancePerSecond == FLT_MAX) ? INT_MAX : (int)((DistancePerSecond * COUNTS_PER_TURN) / perimeter + 0.5);
    return encoderCount;
}

void CMotionControl::SetSlewSpeed(double A_mm_sec, double B_mm_sec, double C_mm_sec, double D_mm_sec)
{
    int A = AxisDirection("A") * DistancePerSecondToEncoderCount(A_mm_sec);
    int B = AxisDirection("B") * DistancePerSecondToEncoderCount(B_mm_sec);
    int C = AxisDirection("C") * DistancePerSecondToEncoderCount(C_mm_sec);
    int D = AxisDirection("D") * DistancePerSecondToEncoderCount(D_mm_sec);
    m_pGclib->SetSlewSpeed(A, B, C, D);
}

void CMotionControl::SetSlewSpeed(double speed_mm_sec)
{
    SetSlewSpeed(speed_mm_sec, speed_mm_sec, speed_mm_sec, speed_mm_sec);
}

// steer to the right (TRUE0 or left (FALSE)
// if (bMouseDown) slow the wheels to one side by about 10%
// else return both sides to the same speed
BOOL CMotionControl::SteerMotors(double fSpeed, BOOL bRight, double rate)
{
    double accelA, spA = GetMotorSpeed("A", accelA);
    double accelB, spB = GetMotorSpeed("B", accelB);
    double accelC, spC = GetMotorSpeed("C", accelC);
    double accelD, spD = GetMotorSpeed("D", accelD);

    // the otors must be running in order to steer
//    if (spA == 0 && spB == 0 && spC == 0 && spD == 0)
//        return FALSE;
    spA = fSpeed;
    spB = fSpeed;
    spC = fSpeed;
    spD = fSpeed;

 
    // slow down the right hand motors ("B", "C")
    // if ending the turn, then rate=1
    if (bRight)
        spB = spC = rate * spA;
    // slow down the left hand motors
    else
        spA = spD = rate * spC;

    return SetMotorJogging(spA, spB, spC, spD, accelA);
}
