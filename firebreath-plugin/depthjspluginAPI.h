/**********************************************************\

  Auto-generated depthjspluginAPI.h

\**********************************************************/

#include <string>
#include <sstream>
#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "BrowserHost.h"
#include "depthjsplugin.h"
#include "DOM/Window.h"

#ifndef H_depthjspluginAPI
#define H_depthjspluginAPI

class depthjspluginAPI : public FB::JSAPIAuto
{
public:
    depthjspluginAPI(const depthjspluginPtr& plugin, const FB::BrowserHostPtr& host);
    virtual ~depthjspluginAPI();

    depthjspluginPtr getPlugin();

    // Read/Write property ${PROPERTY.ident}
    std::string get_testString();
    void set_testString(const std::string& val);

    // Read-only property ${PROPERTY.ident}
    std::string get_version();

    // Method echo
    FB::variant echo(const FB::variant& msg);
    
    // Event helpers
    FB_JSAPI_EVENT(fired, 3, (const FB::variant&, bool, int));
    FB_JSAPI_EVENT(echo, 2, (const FB::variant&, const int));
    FB_JSAPI_EVENT(notify, 0, ());

    // Method test-event
    void testEvent(const FB::variant& s);

	
	bool InitDepthJS();
	void ShutdownDepthJS();
	void CallbackTest();
	
	void DepthJSLog(const std::string& s);
	
	void DepthJSEvent(const std::string& etype, const std::string& emessage);
private:
	void DepthJSEvent_internal(const std::string& etype, const std::string& emessage);
	FB::JSObjectPtr npBackendObj; //DepthJS backend link
	
    depthjspluginWeakPtr m_plugin;
    FB::BrowserHostPtr m_host;

    std::string m_testString;
};

#endif // H_depthjspluginAPI

