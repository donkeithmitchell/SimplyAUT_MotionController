#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "MotionControl.h"
#include "Gclib2.h"

static double PI = 3.14159265358979323846;
static const  double COUNTS_PER_TURN = 768000.0;
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
        SetMotorSpeed(0,0);
        m_pGclib->GCommand("ST"); //stop all motion and programs
        m_pGclib->GMotionComplete("S"); //Block until motion is complete on vector plane S
        m_pGclib->GCommand("MO");
    }

	delete m_pGclib;
	m_pGclib = NULL;
}

void CMotionControl::Init(CWnd* pParent, UINT nMsg)
{
    m_pParent = pParent;
    m_nMsg = nMsg;
}

void CMotionControl::SendDebugMessage(CString msg)
{
    if (m_pParent && ::IsWindow(m_pParent->m_hWnd) && m_nMsg != 0)
    {
        m_pParent->SendMessage(m_nMsg, MSG_SEND_DEBUGMSG, (WPARAM)&msg);
    }
}

void CMotionControl::EnableControls()
{
    if (m_pParent && ::IsWindow(m_pParent->m_hWnd) && m_nMsg != 0)
    {
        m_pParent->SendMessage(m_nMsg, MSG_SETBITMAPS);
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
		return FALSE;

    char strAddress[256];
    sprintf_s(strAddress, sizeof(strAddress), "%d.%d.%d.%d  --direct --subscribe ALL", address[0], address[1], address[2], address[3]);
 //   sprintf_s(strAddress, sizeof(strAddress), "%d.%d.%d.%d  --subscribe ALL", address[0], address[1], address[2], address[3]);
    SendDebugMessage(_T("Opening connection to \"") + CString(strAddress) + _T("\"... "));

    if (!m_pGclib->GOpen(strAddress))
        return FALSE;

    CString str = m_pGclib->GInfo(); //grab connection string
    SendDebugMessage(str);

    str = m_pGclib->GVersion();
    SendDebugMessage(_T("Version: ") + str);


    SendDebugMessage(_T("Initialization of the Galil..."));
    m_pGclib->GCommand("ST"); //stop all motion and programs
    m_pGclib->GMotionComplete("S"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("A"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("B"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("C"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("D"); //Block until motion is complete on vector plane S

    m_pGclib->GCommand("KP*=1.05");     // proportional constant
    m_pGclib->GCommand("KI*=0");        // integrator
    m_pGclib->GCommand("KD*=0");        // derivative constant
    m_pGclib->GCommand("ER*=20000");    // magnitude of the position errors cnts
    m_pGclib->GCommand("OE*=1");        // Off On Error function   
    m_pGclib->GCommand("BR*=0");
    m_pGclib->GCommand("AU*=0.5");
    m_pGclib->GCommand("AG*=0");
    m_pGclib->GCommand("TL*=1.655");
    m_pGclib->GCommand("DP*=0");        // all to zero
    m_pGclib->GCommand("AC*=50000");    // acceleration cts/sec
    m_pGclib->GCommand("DC*=50000");    // deceleration cts/sec
    m_pGclib->GCommand("JG*=0");        // always connect at zero speed
    m_pGclib->GCommand("SH");           // enable all axes
 //   m_pGclib->GCommand("SP 40000");     // speed all axes cts/sec
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
    m_pGclib->GCommand("ST"); //stop all motion and programs
    m_pGclib->GMotionComplete("S"); //Block until motion is complete on vector plane S
    m_pGclib->GCommand("DP*=0");        // all to zero
}

void CMotionControl::GoToHomePosition()
{
    m_pGclib->GCommand("PA 0,0,0,0");
    m_pGclib->GCommand("SH");           // enable all axes
    m_pGclib->GCommand("BG*");   // Begin motion on all Axis
}


void CMotionControl::RunTest1()
{
    m_pGclib->GCmd("ST"); //stop all motion and programs

    //-------------------------------------------------------------------------
    // Independent motion
    m_pGclib->GCmd("DP 0"); //define A position zero
    CString str = m_pGclib->GCmdT("RP");

    SendDebugMessage("\nPosition: " + str);
    m_pGclib->GCmd("SP 4000"); //set up speed
    m_pGclib->GCmd("AC 1280000"); //acceleration
    m_pGclib->GCmd("DC 1280000"); //deceleration
    m_pGclib->GCmd("PR 8000"); //Postion Relative.
    m_pGclib->GCmd("SH A"); //Servo Here
    SendDebugMessage("Beginning independent motion... ");
    m_pGclib->GCmd("BG A"); //Begin motion
    m_pGclib->GMotionComplete("A"); //Block until motion is complete on axis A
    SendDebugMessage("Motion Complete on A");
    str = m_pGclib->GCmdT("RP");
    SendDebugMessage("Position: " + str);

    //-------------------------------------------------------------------------
    // Vector motion
    m_pGclib->GCmd("DP 0"); //define position zero on A
    m_pGclib->GCmdT("RP");
    SendDebugMessage("\nPosition: " + str);
    m_pGclib->GCmd("VS 2000");  //set up vector speed, S plane
    m_pGclib->GCmd("VA 100000"); //vector Acceleration
    m_pGclib->GCmd("VD 100000"); //vector deceleration
    m_pGclib->GCmd("VM AN"); //invoke vector mode, use virtual axis for 1-axis controllers
    m_pGclib->GCmd("VP 3000, 3000"); //buffer Vector Position
    m_pGclib->GCmd("VP 6000,0"); //buffer Vector Position
    m_pGclib->GCmd("VE"); //indicate Vector End
    SendDebugMessage("Beginning vector motion... ");
    m_pGclib->GCmd("BG S"); //begin S plane
    m_pGclib->GMotionComplete("S"); //Block until motion is complete on vector plane S
    SendDebugMessage("Motion Complete on vector plane S\n");
    str = m_pGclib->GCmdT("RP");
    SendDebugMessage("Position: " + str);

}

void CMotionControl::RunTest2()
{

//-------------------------------------------------------------------------
// Independent motion
    m_pGclib->GCommand("DP 0"); //define A position zero
    CString trimmed = m_pGclib->GCmdT("RP");
    SendDebugMessage(_T("nPosition: ") + trimmed);

    m_pGclib->GCommand("SP 4000"); //set up speed
    m_pGclib->GCommand("AC 1280000"); //acceleration
    m_pGclib->GCommand("DC 1280000"); //deceleration
    m_pGclib->GCommand("PR 8000"); //Postion Relative.
    m_pGclib->GCommand("SH A"); //Servo Here

    SendDebugMessage("Beginning independent motion... ");
    m_pGclib->GCommand("BG A"); //Begin motion
    m_pGclib->GMotionComplete("A"); //Block until motion is complete on axis A

    SendDebugMessage("Motion Complete on A");
    trimmed = m_pGclib->GCmdT("RP");
    SendDebugMessage("Position: " + trimmed);

    //-------------------------------------------------------------------------
    // Vector motion
    m_pGclib->GCommand("DP 0"); //define position zero on A
    trimmed = m_pGclib->GCmdT("RP");
    SendDebugMessage("Position: " + trimmed);

    m_pGclib->GCommand("VS 2000");  //set up vector speed, S plane
    m_pGclib->GCommand("VA 100000"); //vector Acceleration
    m_pGclib->GCommand("VD 100000"); //vector deceleration
    m_pGclib->GCommand("VM AN"); //invoke vector mode, use virtual axis for 1-axis controllers
    m_pGclib->GCommand("VP 3000, 3000"); //buffer Vector Position
    m_pGclib->GCommand("VP 6000,0"); //buffer Vector Position
    m_pGclib->GCommand("VE"); //indicate Vector End

    SendDebugMessage("Beginning vector motion... ");
    m_pGclib->GCommand("BG S"); //begin S plane
    m_pGclib->GMotionComplete("S"); //Block until motion is complete on vector plane S

    SendDebugMessage("Motion Complete on vector plane S");
    trimmed = m_pGclib->GCmdT("RP");
    SendDebugMessage("Position: " + trimmed);

}
void CMotionControl::StopMotors()
{
    m_pGclib->GCmd("ST");    // stop all motors
 //   m_pGclib->GMotionComplete("*"); //Wait for motion to complete
 //   m_pGclib->GMotionComplete("S"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("A"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("B"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("C"); //Block until motion is complete on vector plane S
    m_pGclib->GMotionComplete("D"); //Block until motion is complete on vector plane S
}
BOOL CMotionControl::SetMotorSpeed(double speed, double accel)
{
    return SetMotorSpeed(speed, speed, speed, speed, accel);
}

double CMotionControl::GetMotorSpeed(GCStringIn axis, double& rAccel)
{
    double speed = m_pGclib->GetMotorSpeed(axis, rAccel); // thjis is in ticks, want thew speed in mm/sec
    rAccel = EncoderCountToDistancePerSecond(rAccel);
    speed = EncoderCountToDistancePerSecond(speed);
    return (speed == FLT_MAX) ? FLT_MAX : AxisDirection(axis) * speed;
}

double CMotionControl::GetMotorPosition(GCStringIn axis)
{
    double pos = m_pGclib->GetMotorPosition(axis); // thjis is in ticks, want thew speed in mm/sec
    pos = EncoderCountToDistancePerSecond(pos);
    return AxisDirection(axis) *  pos;
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


BOOL CMotionControl::SetMotorSpeed(double speedA, double speedB, double speedC, double speedD, double accel)
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
 // m_pGclib->GCommand("DP*=0");        // define position all to zero  (not while motor is run ning)
//  m_pGclib->GCommand("AC*=50000");    // acceleration cts/sec (leave as were)
//  m_pGclib->GCommand("DC*=50000");    // deceleration cts/sec

    // the left side motors are inverted
    char str[256];
    accel = DistancePerSecondToEncoderCount(accel);
    sprintf_s(str, sizeof(str), "AC*=%g", accel);
    m_pGclib->GCommand(str);

    sprintf_s(str, sizeof(str), "DC*=%g", accel);
    m_pGclib->GCommand(str);

  //  speedA = DistancePerSecondToEncoderCount(speedA);
  //  sprintf_s(str, sizeof(str), "JG*=%d", (int)(AxisDirection("A") * speedA + 0.5));
  //  m_pGclib->GCommand(str);

    speedA = AxisDirection("A") * DistancePerSecondToEncoderCount(speedA);
    sprintf_s(str, sizeof(str), "JGA=%d", (int)(speedA+0.5));
    m_pGclib->GCommand(str);

    speedB = AxisDirection("B") * DistancePerSecondToEncoderCount(speedB);
    sprintf_s(str, sizeof(str), "JGB=%d", (int)(speedB + 0.5));
    m_pGclib->GCommand(str);

    speedC = AxisDirection("C") * DistancePerSecondToEncoderCount(speedC);
    sprintf_s(str, sizeof(str), "JGC=%d", (int)(speedC + 0.5));
    m_pGclib->GCommand(str);

    speedD = AxisDirection("D") * DistancePerSecondToEncoderCount(speedD);
    sprintf_s(str, sizeof(str), "JGD=%d", (int)(speedD + 0.5));
    m_pGclib->GCommand(str);

    m_pGclib->GCommand("SH");           // enable all axes
    m_pGclib->GCommand("BG*");   // Begin motion on all Axis
    return TRUE;
}

double CMotionControl::EncoderCountToDistancePerSecond(double encoderCount)const
{
    double distancePerSecond;
    double perimeter = PI * WHEEL_DIAMETER;

    distancePerSecond = (encoderCount == FLT_MAX) ? FLT_MAX : ((double)encoderCount / COUNTS_PER_TURN) * perimeter;
    return distancePerSecond;
}

double CMotionControl::DistancePerSecondToEncoderCount(double DistancePerSecond)const
{
    double encoderCount;
    double perimeter = PI * WHEEL_DIAMETER;

    encoderCount = (DistancePerSecond == FLT_MAX) ? FLT_MAX : (DistancePerSecond * COUNTS_PER_TURN) / perimeter;
    return encoderCount;
}

// steer to the right (TRUE0 or left (FALSE)
// if (bMouseDown) slow the wheels to one side by about 10%
// else return both sides to the same speed
BOOL CMotionControl::SteerMotors(BOOL bRight, BOOL bMouseDown)
{
    double accelA, spA = GetMotorSpeed("A", accelA);
    double accelB, spB = GetMotorSpeed("B", accelB);
    double accelC, spC = GetMotorSpeed("C", accelC);
    double accelD, spD = GetMotorSpeed("D", accelD);

    // the otors must be running in order to steer
    if (spA == 0 && spB == 0 && spC == 0 && spD == 0)
        return FALSE;

    if (bMouseDown)
    {
        // slow down the right hand motors ("B", "C")
        if (bRight)
            spB = spC = 0.5 * spA;
        // slow down the left hand motors
        else
            spA = spD = 0.5 * spC;
    }
    else
    {
        if (bRight)
            spB = spC = spA;
        else
            spA = spD = spC;
    }

    return SetMotorSpeed(spA, spB, spC, spD, accelA);
}
