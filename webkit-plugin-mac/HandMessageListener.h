/*
 *  HandMessageListener.h
 *  OpenniTry
 *
 *  Created by Roy Shilkrot on 9/30/11.
 *  Copyright 2011 MIT. All rights reserved.
 *
 */

#ifndef _HANDMESSAGELISTENER_H
#define _HANDMESSAGELISTENER_H

#include <XnCppWrapper.h>
#include <XnVPointControl.h>
#include <XnVFlowRouter.h>
#include <XnVSwipeDetector.h>
#include <XnVSelectableSlider1D.h>
#include <XnVSteadyDetector.h>
#include <XnVBroadcaster.h>
#include <XnVPushDetector.h>
#include <XnVWaveDetector.h>
#include <XnVSessionManager.h>
#include <XnVCircleDetector.h>

#include <sstream>
using namespace std;

extern void send_event(const string& etype, const string& edata);

class HandPointControl : public XnVPointControl {
public:
	HandPointControl(xn::DepthGenerator depthGenerator, XnVSessionManager* sessionManager):m_DepthGenerator(depthGenerator),m_SessionManager(sessionManager) {
//		m_pInnerFlowRouter = new XnVFlowRouter;
		m_pPushDetector = new XnVPushDetector;
		m_pSwipeDetector = new XnVSwipeDetector;
		m_pSwipeDetector->SetMotionSpeedThreshold(0.8);
//		m_pSteadyDetector = new XnVSteadyDetector;		
//		m_pWaveDetector = new XnVWaveDetector;
		m_pCircleDetector = new XnVCircleDetector;
		m_pCircleDetector->SetMinRadius(80);
		
//		m_pInnerFlowRouter->SetActive(m_pPushDetector);
		
		// Add the push detector and flow manager to the broadcaster
//		m_Broadcaster.AddListener(m_pInnerFlowRouter);
//		m_Broadcaster.AddListener(m_pPushDetector);
		
		// Push
		m_pPushDetector->RegisterPush(this, &Push_Pushed);
		m_pCircleDetector->RegisterCircle(this, &ACircle);
		//m_pWaveDetector->RegisterWave(this, &Wave_Waved);
		m_pSwipeDetector->RegisterSwipeLeft(this, &Swipe_Left);
		m_pSwipeDetector->RegisterSwipeRight(this, &Swipe_Right);
	}
	
	void Update(XnVMessage* pMessage)
	{
		XnVPointControl::Update(pMessage);
		//m_Broadcaster.Update(pMessage);
		m_pPushDetector->Update(pMessage);
//		m_pWaveDetector->Update(pMessage);
		m_pCircleDetector->Update(pMessage);
		m_pSwipeDetector->Update(pMessage);
	}
	
	static void XN_CALLBACK_TYPE Swipe_Left(XnFloat fVelocity, XnFloat fAngle, void* pUserCxt) {
		send_event("SwipeLeft", "");
	}
	static void XN_CALLBACK_TYPE Swipe_Right(XnFloat fVelocity, XnFloat fAngle, void* pUserCxt) {
		send_event("SwipeRight", "");
	}
	
	// Push detector
	static void XN_CALLBACK_TYPE Push_Pushed(XnFloat fVelocity, XnFloat fAngle, void* cxt)
	{
		printf("Push!\n");
		send_event("Push", "");
	}
	
	static void XN_CALLBACK_TYPE ACircle(XnFloat fTimes, XnBool bConfident, const XnVCircle* pCircle, void* cxt) {
		if(bConfident) {
			printf("Bye Bye!\n");
			((HandPointControl*)cxt)->KillSession();
		}
	}

	// Wave detector
	static void XN_CALLBACK_TYPE Wave_Waved(void* cxt)
	{
		printf("Bye Bye!\n");
		((HandPointControl*)cxt)->KillSession();
	}
	
	void KillSession() { m_SessionManager->EndSession(); }
	
	/**
	 * Handle creation of a new point
	 */
	void OnPointCreate(const XnVHandPointContext* cxt) {
		printf("** %d\n", cxt->nID);	
		send_event("Register", "");
	}
	
	/**
	 * Handle new position of an existing point
	 */
	void OnPointUpdate(const XnVHandPointContext* cxt) {
		// positions are kept in projective coordinates, since they are only used for drawing
		XnPoint3D ptProjective(cxt->ptPosition);
		
//		printf("Point (%f,%f,%f)", ptProjective.X, ptProjective.Y, ptProjective.Z);
		m_DepthGenerator.ConvertRealWorldToProjective(1, &ptProjective, &ptProjective);
//		printf(" -> (%f,%f,%f)\n", ptProjective.X, ptProjective.Y, ptProjective.Z);
		
		//move to [0->100,0->100,0->2048]
		stringstream ss;
		ss  << "\"x\":"  << (int)(100.0*ptProjective.X/640.0)
			<< ",\"y\":" << (int)(100.0*ptProjective.Y/480.0)
			<< ",\"z\":" << (int)ptProjective.Z;
		//cout << "move: " << ss.str() << endl;
		send_event("Move", ss.str());		
	}	
	
	/**
	 * Handle destruction of an existing point
	 */
	void OnPointDestroy(XnUInt32 nID) {
		printf("OnPointDestroy\n");
		send_event("Unregister", "");
	}
private:
	xn::DepthGenerator m_DepthGenerator;
	XnVSessionManager* m_SessionManager;
	
//	XnVBroadcaster m_Broadcaster;
	XnVPushDetector* m_pPushDetector;
	XnVSwipeDetector* m_pSwipeDetector;
//	XnVSteadyDetector* m_pSteadyDetector;		
//	XnVFlowRouter* m_pInnerFlowRouter;
//	
//	XnVWaveDetector* m_pWaveDetector;
	XnVCircleDetector* m_pCircleDetector;
};

#endif