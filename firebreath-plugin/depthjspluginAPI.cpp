/**********************************************************\

  Auto-generated depthjspluginAPI.cpp

\**********************************************************/

#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "global/config.h"

#include "depthjspluginAPI.h"

#include "openni_backend.hpp"

///////////////////////////////////////////////////////////////////////////////
/// @fn depthjspluginAPI::depthjspluginAPI(const depthjspluginPtr& plugin, const FB::BrowserHostPtr host)
///
/// @brief  Constructor for your JSAPI object.  You should register your methods, properties, and events
///         that should be accessible to Javascript from here.
///
/// @see FB::JSAPIAuto::registerMethod
/// @see FB::JSAPIAuto::registerProperty
/// @see FB::JSAPIAuto::registerEvent
///////////////////////////////////////////////////////////////////////////////
depthjspluginAPI::depthjspluginAPI(const depthjspluginPtr& plugin, const FB::BrowserHostPtr& host) : 
	m_plugin(plugin), 
	m_host(host)
{
    registerMethod("echo",				make_method(this, &depthjspluginAPI::echo));
    registerMethod("testEvent",			make_method(this, &depthjspluginAPI::testEvent));
    registerMethod("InitDepthJS",		make_method(this, &depthjspluginAPI::InitDepthJS));
    registerMethod("ShutdownDepthJS",	make_method(this, &depthjspluginAPI::ShutdownDepthJS));
    registerMethod("CallbackTest",		make_method(this, &depthjspluginAPI::CallbackTest));

    // Read-write property
    registerProperty("testString",
                     make_property(this,
                        &depthjspluginAPI::get_testString,
                        &depthjspluginAPI::set_testString));

    // Read-only property
    registerProperty("version",
                     make_property(this,
                        &depthjspluginAPI::get_version));
}

///////////////////////////////////////////////////////////////////////////////
/// @fn depthjspluginAPI::~depthjspluginAPI()
///
/// @brief  Destructor.  Remember that this object will not be released until
///         the browser is done with it; this will almost definitely be after
///         the plugin is released.
///////////////////////////////////////////////////////////////////////////////
depthjspluginAPI::~depthjspluginAPI()
{
}

///////////////////////////////////////////////////////////////////////////////
/// @fn depthjspluginPtr depthjspluginAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
depthjspluginPtr depthjspluginAPI::getPlugin()
{
    depthjspluginPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
}



// Read/Write property testString
std::string depthjspluginAPI::get_testString()
{
    return m_testString;
}
void depthjspluginAPI::set_testString(const std::string& val)
{
    m_testString = val;
}

// Read-only property version
std::string depthjspluginAPI::get_version()
{
    return FBSTRING_PLUGIN_VERSION;
}

// Method echo
FB::variant depthjspluginAPI::echo(const FB::variant& msg)
{
    static int n(0);
    fire_echo(msg, n++);
    return msg;
}

void depthjspluginAPI::testEvent(const FB::variant& var)
{
    fire_fired(var, true, 1);
}

void depthjspluginAPI::DepthJSLog(const std::string& s){
	// Retrieve a reference to the DOM Window
	FB::DOM::WindowPtr window = m_host->getDOMWindow();
	
	// Check if the DOM Window has an the property console
	if (window && window->getJSObject()->HasProperty("console")) {
		// Create a reference to the browswer console object
		FB::JSObjectPtr obj = window->getProperty<FB::JSObjectPtr>("console");
		
		// Invoke the "log" method on the console object
		obj->Invoke("log", FB::variant_list_of(s));
	}
}

bool depthjspluginAPI::InitDepthJS() {
	DepthJSLog("depthjspluginAPI: start DepthJS");
	return getPlugin()->RunDepthJS();
}

void depthjspluginAPI::ShutdownDepthJS() {
	DepthJSLog("depthjspluginAPI: shutdown DepthJS");
	kill_openni_backend();
}

void depthjspluginAPI::CallbackTest() {
	// Retrieve a reference to the DOM Window
	FB::DOM::WindowPtr window = m_host->getDOMWindow();
	
	// Check if the DOM Window has an the property console
	if (window && window->getJSObject()->HasProperty("DepthJS")) {
		// Create a reference to the browswer console object
		FB::JSObjectPtr obj = window->getProperty<FB::JSObjectPtr>("DepthJS");
		
		// Invoke the "log" method on the console object
		obj->Invoke("k", FB::variant_list_of("test"));
	}
}

void depthjspluginAPI::DepthJSEvent(const std::string& etype, const std::string& edata) {
	std::stringstream ss; ss<<"new message: "<<etype<<" - "<<edata;
//	DepthJSLog(ss.str());
	try {
		m_host->CallOnMainThread(boost::bind(&depthjspluginAPI::DepthJSEvent_internal,this,etype,edata));
	}
	catch (const FB::script_error&) {
		DepthJSLog("Can't CallOnMainThread");
	}
}

void depthjspluginAPI::DepthJSEvent_internal(const std::string& etype, const std::string& edata) {
	//TODO: why is this "caching" mechanism not working??
	if(!npBackendObj) { //first time init...
		FB::DOM::WindowPtr window = m_host->getDOMWindow();
		
		if (window && window->getJSObject()->HasProperty("DepthJS")) {
			FB::JSObjectPtr obj = window->getProperty<FB::JSObjectPtr>("DepthJS");
			
			if(obj && obj->HasProperty("npBackend")) {
				npBackendObj = obj->GetProperty("npBackend").cast<FB::JSObjectPtr>();
				if(!npBackendObj) { DepthJSLog("can't aqcuire npBackend obj"); return; }
				if(!npBackendObj->HasMethod("receiveEvent")) { npBackendObj.reset(); DepthJSLog("npBackend obj has no 'recieveEvent' method"); return; }
				DepthJSLog("Got the backend object");
			}
		}
	}
	if(npBackendObj) {
		std::stringstream ss;
		ss << "{\"type\":\"" << etype << "\",\"data\":{" << edata << "}}";
		npBackendObj->InvokeAsync("receiveEvent", FB::variant_list_of(ss.str()));		
	}		
}
