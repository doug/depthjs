/*
 *  openni_backend.cpp
 *  webkit-plugin-mac
 *
 *  Created by Roy Shilkrot on 9/30/11.
 *  Copyright 2011 MIT. All rights reserved.
 *
 */

#include "openni_backend.hpp"

// Headers for OpenNI
#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnHash.h>
#include <XnLog.h>
#include <XnUSB.h>

#define VID_MICROSOFT 0x45e
#define PID_NUI_MOTOR 0x02b0

// Header for NITE
#include "XnVNite.h"

#include <iostream>

#include "ResourceRecovery.h"

#include "HandMessageListener.h"

#define GESTURE_TO_USE "Wave"

using namespace xn;
using namespace std;

#pragma mark Declarations

typedef enum
{
	IN_SESSION,
	NOT_IN_SESSION,
	QUICK_REFOCUS
} SessionState;


boost::shared_ptr<depthjspluginAPI> openni_plugin_jsapi_ptr;
void send_event(const std::string& etype, const std::string& edata) {
    //	openni_plugin_jsapi_ptr->DepthJSLog(etype);
	openni_plugin_jsapi_ptr->DepthJSEvent(etype,edata);
}
void send_log(const std::string& s) {
	openni_plugin_jsapi_ptr->DepthJSLog(s);
}

class OpenNIBackend {
public:
	OpenNIBackend():g_SessionState(NOT_IN_SESSION),running(true),terminated(false) {}
	
	void run();
	void stop();
	bool isDead();
	int init();
	
	int setKinectAngle();
    
	SessionState g_SessionState;
private:
	bool running,terminated;
	
	// OpenNI objects
	xn::Context g_Context;
	xn::ScriptNode g_ScriptNode;
	xn::DepthGenerator g_DepthGenerator;
	xn::HandsGenerator g_HandsGenerator;
	xn::GestureGenerator g_GestureGenerator;
	
	// NITE objects
	XnVSessionManager* g_pSessionManager;
	XnVFlowRouter* g_pFlowRouter;
	
	HandPointControl* g_pHandListener;
};

OpenNIBackend onib;

int openni_backend(void* _arg) { onib.run(); return 0; }
void kill_openni_backend() { onib.stop(); }
bool is_openni_backend_dead() { return onib.isDead(); }
int init_openni_backend(const boost::shared_ptr<depthjspluginAPI>& _plugin_jspai) { openni_plugin_jsapi_ptr = _plugin_jspai; return onib.init(); }


#pragma mark Implementation

#define CHECK_RC(rc, what)															\
if (rc != XN_STATUS_OK)															\
{																				\
printf("%s failed: %s\n", what, xnGetStatusString(rc));						\
std::stringstream ss; ss << what << " failed: " << xnGetStatusString(rc);	\
send_log(ss.str());															\
return rc;																	\
}

#define CHECK_ERRORS(rc, errors, what)		\
if (rc == XN_STATUS_NO_NODE_PRESENT)	\
{										\
XnChar strError[1024];				\
errors.ToString(strError, 1024);	\
printf("%s\n", strError);			\
send_log(std::string(what) + std::string(strError));\
return (rc);						\
}


// Callback for when the focus is in progress
void XN_CALLBACK_TYPE FocusProgress(const XnChar* strFocus, const XnPoint3D& ptPosition, XnFloat fProgress, void* UserCxt)
{
	send_log("OpenNIBackend: FocusProgress");
	printf("Focus progress: %s @(%f,%f,%f): %f\n", strFocus, ptPosition.X, ptPosition.Y, ptPosition.Z, fProgress);
}

void XN_CALLBACK_TYPE GestureIntermediateStageCompletedHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
	send_log("OpenNIBackend: GestureIntermediateStageCompletedHandler");
	printf("Gesture %s: Intermediate stage complete (%f,%f,%f)\n", strGesture, pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE GestureReadyForNextIntermediateStageHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
	send_log("OpenNIBackend: GestureReadyForNextIntermediateStageHandler");
	printf("Gesture %s: Ready for next intermediate stage (%f,%f,%f)\n", strGesture, pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE GestureProgressHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress, void* pCookie)
{
	send_log("OpenNIBackend: GestureProgressHandler");
	printf("Gesture %s progress: %f (%f,%f,%f)\n", strGesture, fProgress, pPosition->X, pPosition->Y, pPosition->Z);
}

void XN_CALLBACK_TYPE SessionStarting(const XnPoint3D& ptPosition, void* UserCxt);
void XN_CALLBACK_TYPE SessionEnding(void* UserCxt);
void XN_CALLBACK_TYPE NoHands(void* UserCxt);


// xml to initialize OpenNI
#define SAMPLE_XML_PATH "/Sample-Tracking.xml"

XnMapOutputMode QVGAMode = { 320, 240, 30 };
XnMapOutputMode VGAMode = { 640, 480, 30 };

void OpenNIBackend::run() {
    send_log("OpenNIBackend: start openni backend thread");
    printf("start openni backend thread\n");
    terminated = false;
    while (running) {
        //			XnMapOutputMode mode;
        //			g_DepthGenerator.GetMapOutputMode(mode);
        // Read next available data
        g_Context.WaitOneUpdateAll(g_DepthGenerator);
        // Update NITE tree
        g_pSessionManager->Update(&g_Context);
    }
    terminated = true;
    printf("end openni backend thread\n");
    send_log("OpenNIBackend: end openni backend thread");
}
void OpenNIBackend::stop() { printf("stopping openni backend...\n"); running = false;}
bool OpenNIBackend::isDead() { return terminated; }

int OpenNIBackend::setKinectAngle() {
    XN_USB_DEV_HANDLE dev;
    
    int angle = 20;
    
    XnStatus rc = XN_STATUS_OK;
    
    rc = xnUSBInit();
    CHECK_RC(rc,"init usb device");
    
    rc = xnUSBOpenDevice(VID_MICROSOFT, PID_NUI_MOTOR, NULL, NULL, &dev);
    CHECK_RC(rc,"open usb device");
    
    uint8_t empty[0x1];
    angle = angle * 2;
    
    rc = xnUSBSendControl(dev,
                          XN_USB_CONTROL_TYPE_VENDOR,
                          0x31,
                          (XnUInt16)angle,
                          0x0,
                          empty,
                          0x0, 0);
    CHECK_RC(rc,"send usb command");
    
    rc = xnUSBCloseDevice(dev);
    CHECK_RC(rc,"close usb device");
    
    return rc;
}

int OpenNIBackend::init() {
    send_log("OpenNIBackend: init()");
    
    running = true;
    terminated = false;
    setKinectAngle();
    
    XnStatus rc = XN_STATUS_OK;
    xn::EnumerationErrors errors;
    
    // Initialize OpenNI
    send_log("OpenNIBackend: getResourcesDirectory");
    std::string xml_file_path = getResourcesDirectory();
    send_log("XML in: "+xml_file_path);
    
    rc = g_Context.InitFromXmlFile((xml_file_path + SAMPLE_XML_PATH).c_str(), g_ScriptNode, &errors);
    CHECK_ERRORS(rc, errors, "InitFromXmlFile");
    CHECK_RC(rc, "InitFromXmlFile");
    
    rc = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
    CHECK_RC(rc, "Find depth generator");
    //		rc = g_DepthGenerator.SetMapOutputMode(QVGAMode);
    //		CHECK_RC(rc, "Set Mode");
    rc = g_Context.FindExistingNode(XN_NODE_TYPE_HANDS, g_HandsGenerator);
    CHECK_RC(rc, "Find hands generator");
    rc = g_Context.FindExistingNode(XN_NODE_TYPE_GESTURE, g_GestureGenerator);
    CHECK_RC(rc, "Find gesture generator");
    
    //	XnCallbackHandle h;
    //	if (g_HandsGenerator.IsCapabilitySupported(XN_CAPABILITY_HAND_TOUCHING_FOV_EDGE))
    //	{
    //		g_HandsGenerator.GetHandTouchingFOVEdgeCap().RegisterToHandTouchingFOVEdge(TouchingCallback, NULL, h);
    //	}
    
    XnCallbackHandle hGestureIntermediateStageCompleted, hGestureProgress, hGestureReadyForNextIntermediateStage;
    g_GestureGenerator.RegisterToGestureIntermediateStageCompleted(GestureIntermediateStageCompletedHandler, NULL, hGestureIntermediateStageCompleted);
    g_GestureGenerator.RegisterToGestureReadyForNextIntermediateStage(GestureReadyForNextIntermediateStageHandler, NULL, hGestureReadyForNextIntermediateStage);
    g_GestureGenerator.RegisterGestureCallbacks(NULL, GestureProgressHandler, NULL, hGestureProgress);
    
    g_HandsGenerator.SetSmoothing(0.1);
    
    // Create NITE objects
    g_pSessionManager = new XnVSessionManager;
    rc = g_pSessionManager->Initialize(&g_Context, "Click,Wave", "RaiseHand");
    CHECK_RC(rc, "SessionManager::Initialize");
    
    g_pSessionManager->RegisterSession(this, SessionStarting, SessionEnding, FocusProgress);
    
    g_pHandListener = new HandPointControl(g_DepthGenerator,g_pSessionManager);
    g_pFlowRouter = new XnVFlowRouter;
    g_pFlowRouter->SetActive(g_pHandListener);
    
    g_pSessionManager->AddListener(g_pFlowRouter);
    
    g_pHandListener->RegisterNoPoints(this, NoHands);
    
    // Initialization done. Start generating
    rc = g_Context.StartGeneratingAll();
    CHECK_RC(rc, "StartGenerating");
    
    stringstream ss; ss<<"openni_backend: kinect inited? " << (rc == XN_STATUS_OK);
    send_log(ss.str());
    
    return rc == XN_STATUS_OK;
}


// callback for session start
void XN_CALLBACK_TYPE SessionStarting(const XnPoint3D& ptPosition, void* UserCxt)
{
	send_log("OpenNIBackend: SessionStarting");
	printf("Session start: (%f,%f,%f)\n", ptPosition.X, ptPosition.Y, ptPosition.Z);
	((OpenNIBackend*)UserCxt)->g_SessionState = IN_SESSION;
}
// Callback for session end
void XN_CALLBACK_TYPE SessionEnding(void* UserCxt)
{
	send_log("OpenNIBackend: SessionEnding");
	printf("Session end\n");
	((OpenNIBackend*)UserCxt)->g_SessionState = NOT_IN_SESSION;
}
void XN_CALLBACK_TYPE NoHands(void* UserCxt)
{
	if (((OpenNIBackend*)UserCxt)->g_SessionState != NOT_IN_SESSION)
	{
		send_log("OpenNIBackend: NoHands");
		printf("Quick refocus\n");
		((OpenNIBackend*)UserCxt)->g_SessionState = QUICK_REFOCUS;
	}
}


