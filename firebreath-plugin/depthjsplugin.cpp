/**********************************************************\

  Auto-generated depthjsplugin.cpp

  This file contains the auto-generated main plugin object
  implementation for the DepthJS Plugin project

\**********************************************************/

#include "depthjspluginAPI.h"

#include "depthjsplugin.h"

#include "openni_backend.hpp"

///////////////////////////////////////////////////////////////////////////////
/// @fn depthjsplugin::StaticInitialize()
///
/// @brief  Called from PluginFactory::globalPluginInitialize()
///
/// @see FB::FactoryBase::globalPluginInitialize
///////////////////////////////////////////////////////////////////////////////
void depthjsplugin::StaticInitialize()
{
    // Place one-time initialization stuff here; As of FireBreath 1.4 this should only
    // be called once per process
}

///////////////////////////////////////////////////////////////////////////////
/// @fn depthjsplugin::StaticInitialize()
///
/// @brief  Called from PluginFactory::globalPluginDeinitialize()
///
/// @see FB::FactoryBase::globalPluginDeinitialize
///////////////////////////////////////////////////////////////////////////////
void depthjsplugin::StaticDeinitialize()
{
    // Place one-time deinitialization stuff here. As of FireBreath 1.4 this should
    // always be called just before the plugin library is unloaded
}

///////////////////////////////////////////////////////////////////////////////
/// @brief  depthjsplugin constructor.  Note that your API is not available
///         at this point, nor the window.  For best results wait to use
///         the JSAPI object until the onPluginReady method is called
///////////////////////////////////////////////////////////////////////////////
depthjsplugin::depthjsplugin()
{
}

///////////////////////////////////////////////////////////////////////////////
/// @brief  depthjsplugin destructor.
///////////////////////////////////////////////////////////////////////////////
depthjsplugin::~depthjsplugin()
{
    // This is optional, but if you reset m_api (the shared_ptr to your JSAPI
    // root object) and tell the host to free the retained JSAPI objects then
    // unless you are holding another shared_ptr reference to your JSAPI object
    // they will be released here.
    releaseRootJSAPI();
    m_host->freeRetainedObjects();
}

void depthjsplugin::onPluginReady()
{
    // When this is called, the BrowserHost is attached, the JSAPI object is
    // created, and we are ready to interact with the page and such.  The
    // PluginWindow may or may not have already fire the AttachedEvent at
    // this point.
	boost::shared_ptr<depthjspluginAPI> djs_jsapi = FB::ptr_cast<depthjspluginAPI>(createJSAPI());
	djs_jsapi->DepthJSLog("depthjsplugin::onPluginReady");
	kinect_status = init_openni_backend(djs_jsapi);
}

void depthjsplugin::shutdown()
{
    // This will be called when it is time for the plugin to shut down;
    // any threads or anything else that may hold a shared_ptr to this
    // object should be released here so that this object can be safely
    // destroyed. This is the last point that shared_from_this and weak_ptr
    // references to this object will be valid
	kill_openni_backend();
}

bool depthjsplugin::RunDepthJS() {
	if(!kinect_status) return false; //initialization failed.
	
	boost::shared_ptr<depthjspluginAPI> djs_jsapi = FB::ptr_cast<depthjspluginAPI>(createJSAPI());
	djs_jsapi->DepthJSLog("depthjsplugin::RunDepthJS");
	
	boost::thread thrd(boost::bind(openni_backend,(void*)0));
	
	djs_jsapi->DepthJSLog("depthjsplugin::RunDepthJS - thread running?");
	return thrd.joinable();
}

///////////////////////////////////////////////////////////////////////////////
/// @brief  Creates an instance of the JSAPI object that provides your main
///         Javascript interface.
///
/// Note that m_host is your BrowserHost and shared_ptr returns a
/// FB::PluginCorePtr, which can be used to provide a
/// boost::weak_ptr<depthjsplugin> for your JSAPI class.
///
/// Be very careful where you hold a shared_ptr to your plugin class from,
/// as it could prevent your plugin class from getting destroyed properly.
///////////////////////////////////////////////////////////////////////////////
FB::JSAPIPtr depthjsplugin::createJSAPI()
{
    // m_host is the BrowserHost
    return boost::make_shared<depthjspluginAPI>(FB::ptr_cast<depthjsplugin>(shared_from_this()), m_host);
}

bool depthjsplugin::onMouseDown(FB::MouseDownEvent *evt, FB::PluginWindow *)
{
    //printf("Mouse down at: %d, %d\n", evt->m_x, evt->m_y);
    return false;
}

bool depthjsplugin::onMouseUp(FB::MouseUpEvent *evt, FB::PluginWindow *)
{
    //printf("Mouse up at: %d, %d\n", evt->m_x, evt->m_y);
    return false;
}

bool depthjsplugin::onMouseMove(FB::MouseMoveEvent *evt, FB::PluginWindow *)
{
    //printf("Mouse move at: %d, %d\n", evt->m_x, evt->m_y);
    return false;
}
bool depthjsplugin::onWindowAttached(FB::AttachedEvent *evt, FB::PluginWindow *)
{
    // The window is attached; act appropriately
    return false;
}

bool depthjsplugin::onWindowDetached(FB::DetachedEvent *evt, FB::PluginWindow *)
{
    // The window is about to be detached; act appropriately
    return false;
}

