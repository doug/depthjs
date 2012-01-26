#include <CoreFoundation/CoreFoundation.h>
#include "../gen/global/config.h"
#include <string>

std::string getResourcesDirectory() {
	// Get the XML file for OpenNI from the resources directory of the plugin bundle
	CFBundleRef bundleRef = CFBundleGetBundleWithIdentifier(CFSTR("com.depthjspluginLib.DepthJS Plugin"));
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundleRef);
	UInt8 cbuf[1024] = {0};
	CFURLGetFileSystemRepresentation(resourcesURL,true,cbuf,1024);
	return std::string((char*)cbuf);
}