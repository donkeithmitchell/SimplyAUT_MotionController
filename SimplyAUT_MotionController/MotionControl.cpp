#include "pch.h"
#include "MotionControl.h"
#include "Gclib2.h"

CMotionControl::CMotionControl()
{
    m_pParent = NULL;
    m_nMsg = 0;
	m_pGclib = NULL;
}

CMotionControl::~CMotionControl()
{
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
        m_pParent->SendMessage(m_nMsg, 0, (WPARAM)&msg);
    }
}

void CMotionControl::EnableControls()
{
    if (m_pParent && ::IsWindow(m_pParent->m_hWnd) && m_nMsg != 0)
    {
        m_pParent->SendMessage(m_nMsg, 1);
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
    return TRUE;
    EnableControls();
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

 //   RunTest1();
 //   RunTest2();

 

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
    m_pGclib->GCommand("SH");           // enable all axes
    m_pGclib->GCommand("SP 40000");     // speed all axes cts/sec
    m_pGclib->GCommand("PR 100000");     // move distance all axes cts
    m_pGclib->GCommand("BG");     // move distance all axes cts

    StopScanning();

    m_pGclib->GCmd("SH*");    // Set servo here
//    m_pGclib->GCmd("SHA");    // Set servo here
//    m_pGclib->GCmd("SHB");    // Set servo here
//    m_pGclib->GCmd("SHC");    // Set servo here
//    m_pGclib->GCmd("SHD");    // Set servo here

    m_pGclib->GCmd("DP*=0");  // Start position at absolute zero
 //   m_pGclib->GCmd("DPA=0");  // Start position at absolute zero
 //   m_pGclib->GCmd("DPB=0");  // Start position at absolute zero
 //   m_pGclib->GCmd("DPC=0");  // Start position at absolute zero
 //   m_pGclib->GCmd("DPD=0");  // Start position at absolute zero
    
    EnableControls();
    return SetScanSpeed(dScanSpeed);
}

void CMotionControl::RunTest1()
{
    char buf[1024]; //traffic buffer
    char* trimmed; //trimmed string pointer

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
void CMotionControl::StopScanning()
{
    m_pGclib->GCmd("ST");    // stop all motors
    m_pGclib->GMotionComplete("*"); //Wait for motion to complete
  //  m_pGclib->GMotionComplete("B"); //Wait for motion to complete
  //  m_pGclib->GMotionComplete("C"); //Wait for motion to complete
  //  m_pGclib->GMotionComplete("D"); //Wait for motion to complete
}
BOOL CMotionControl::SetScanSpeed(double speed)
{
    return SetScanSpeed(speed, speed, speed, speed);
}

BOOL CMotionControl::SetScanSpeed(double speedA, double speedB, double speedC, double speedD)
    {
    if (!IsConnected())
        return FALSE;


  //  m_pGclib->GCmd("JGA=0");  // Start jogging with 0 speed
  //  m_pGclib->GCmd("BGA");   // Begin motion on A Axis

    char strSpeed[256];
    sprintf_s(strSpeed, sizeof(strSpeed), "JGA=%g", speedA);
    m_pGclib->GCommand(strSpeed);

    sprintf_s(strSpeed, sizeof(strSpeed), "JGB=%g", speedB);
    m_pGclib->GCommand(strSpeed);

    sprintf_s(strSpeed, sizeof(strSpeed), "JGC=%g", speedC);
    m_pGclib->GCommand(strSpeed);

    sprintf_s(strSpeed, sizeof(strSpeed), "JGD=%g", speedD);
    m_pGclib->GCommand(strSpeed);

    sprintf_s(strSpeed, sizeof(strSpeed), "JG %g, %g, %g, %g", speedA, speedB, speedC, speedD);
    SendDebugMessage(_T("Downloading Program --> ") + CString(strSpeed));

    m_pGclib->GCommand("BG*");   // Begin motion on all Axis
 //   m_pGclib->GCmd("BGA");   // Begin motion on all Axis
    return TRUE;
}