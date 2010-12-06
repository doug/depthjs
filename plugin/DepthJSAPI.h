/**********************************************************\

  Auto-generated DepthJSAPI.h

\**********************************************************/

#include <string>
#include <sstream>
#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "BrowserHost.h"
#include "DepthJS.h"

#include "libfreenect.h"

#ifndef H_DepthJSAPI
#define H_DepthJSAPI

class DepthJSAPI : public FB::JSAPIAuto
{
public:
    DepthJSAPI(DepthJSPtr plugin, FB::BrowserHostPtr host);
    virtual ~DepthJSAPI();

    DepthJSPtr getPlugin();

    // Read-only property ${PROPERTY.ident}
    std::string get_version();

	void initKnct();
	void killKnct();
private:
    DepthJSWeakPtr m_plugin;
    FB::BrowserHostPtr m_host;

	freenect_context *f_ctx;
	freenect_device *f_dev;
	int freenect_angle;
	int freenect_led;

	void freenect_threadfunc();
	void ocv_threadfunc();
	void send_event(const std::string& etype, const std::string& edata);
	
	boost::thread m_thread;
};

#endif // H_DepthJSAPI

