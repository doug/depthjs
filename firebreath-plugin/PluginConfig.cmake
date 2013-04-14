#/**********************************************************\ 
#
# Auto-Generated Plugin Configuration file
# for DepthJS Plugin
#
#\**********************************************************/

set(PLUGIN_NAME "depthjsplugin")
set(PLUGIN_PREFIX "DJP")
set(COMPANY_NAME "mitmedialab")

# ActiveX constants:
set(FBTYPELIB_NAME depthjspluginLib)
set(FBTYPELIB_DESC "depthjsplugin 1.0 Type Library")
set(IFBControl_DESC "depthjsplugin Control Interface")
set(FBControl_DESC "depthjsplugin Control Class")
set(IFBComJavascriptObject_DESC "depthjsplugin IComJavascriptObject Interface")
set(FBComJavascriptObject_DESC "depthjsplugin ComJavascriptObject Class")
set(IFBComEventSource_DESC "depthjsplugin IFBComEventSource Interface")
set(AXVERSION_NUM "1")

# NOTE: THESE GUIDS *MUST* BE UNIQUE TO YOUR PLUGIN/ACTIVEX CONTROL!  YES, ALL OF THEM!
set(FBTYPELIB_GUID a2420d9e-3906-547c-b726-b2dad0aefee9)
set(IFBControl_GUID df2aaf09-8562-53b9-90eb-9b1bc920d6e9)
set(FBControl_GUID d76e88da-5167-551c-9608-ace40abf22d4)
set(IFBComJavascriptObject_GUID 5ff9b773-f7e9-50e3-b62a-e39c2933d46e)
set(FBComJavascriptObject_GUID be9a7083-405b-5562-8a69-b166312230bf)
set(IFBComEventSource_GUID 0344798c-8962-508e-9331-f9285d69a83d)

# these are the pieces that are relevant to using it from Javascript
set(ACTIVEX_PROGID "mitmedialab.depthjsplugin")
set(MOZILLA_PLUGINID "depthjs.media.mit.edu/depthjsplugin")

# strings
set(FBSTRING_CompanyName "MIT Media Lab")
set(FBSTRING_FileDescription "The native plugin part of the DepthJS project")
set(FBSTRING_PLUGIN_VERSION "1.0.0.0")
set(FBSTRING_LegalCopyright "Copyright 2012 MIT Media Lab")
set(FBSTRING_PluginFileName "np${PLUGIN_NAME}.dll")
set(FBSTRING_ProductName "DepthJSPlugin")
set(FBSTRING_FileExtents "")
set(FBSTRING_PluginName "DepthJSPlugin")
set(FBSTRING_MIMEType "application/x-depthjsplugin")

# Uncomment this next line if you're not planning on your plugin doing
# any drawing:

#set (FB_GUI_DISABLED 1)

# Mac plugin settings. If your plugin does not draw, set these all to 0
set(FBMAC_USE_QUICKDRAW 0)
set(FBMAC_USE_CARBON 1)
set(FBMAC_USE_COCOA 1)
set(FBMAC_USE_COREGRAPHICS 1)
set(FBMAC_USE_COREANIMATION 0)
set(FBMAC_USE_INVALIDATINGCOREANIMATION 0)

# If you want to register per-machine on Windows, uncomment this line
#set (FB_ATLREG_MACHINEWIDE 1)
