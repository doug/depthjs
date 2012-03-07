DepthJS
=======
DepthJS is a browser extension (currently Chrome & Safari) that allows the Microsoft Kinect to talk to any web page. It provides the low-level raw access to the Kinect as well as high-level hand gesture events to simplify development.

Current Status
--------------
#### March 2012:
Windows plugin now works with Chrome. 
Use Firebreath to build (see instructions below), or try to use the precompiled version (but then you must rename/copy chrome-extension/manifest.json.WIN32 to chrome-extension/manifest.json).

#### January 2012:
Moving to FireBreath NPAPI plugins for Chrome + Firefox. Mac/Safari verion remains as it is (under webkit-plugin-mac and safari-extension-mac).
Mac version of the new NPAPI plugin already exists for Chrome, tested and working. You should just be able to go on to chrome://extensions and add it to your browser. Now working on Linux and Windows support.

#### September 2011:
Moving to OpenNI/NITE based backend, forsaking OpenCV for now. Gesture recognition is thus far better than what we had before.
Finger-based gestures will soon follow as a few projects parallel to DepthJS will merge in coming months.

#### Current gesture language:
- Wave to start hand tracking and get "blue pointer".
- Push to click.
- Circle to end tracking and remove "blue pointer".

Note: OpenNI & NITE should be downloaded and linked appropriately to your build.

Components
----------
DepthJS is very modular. The Kinect driver and computer vision are written on top of OpenNI and NITE. This component can output the raw RGB image, the raw depth map (filtered for the hand), as well as the high-level events that the computer vision recognizes. A native browser plugin (think Flash) wraps this Kinect code, which directly interacts with a Javascript browser plugin.  Fortunately in Chrome extensions can contain native code, so it will be easy for anyone to install it. Safari requires a bit more work with a plugin installer & needing to go to the extension "store," if Apple will even permit this.

Event handlers in the browser extension may be placed globally, in content scripts injected into each web page, or pushed via the content script to local DOM elements written by 3rd parties.

FLOW DIAGRAM:
Kinect =====> Browser plugin/native code =====> Browser extension ===(Javascript+DOM events)==> Any web page

Note: As of now we are using OpenNI/NITE for tracking and gesture rec. Download the precompiled libs from: 
http://www.openni.org/Downloads/OpenNIModules.aspx

Platforms
---------
Macs are supported. All dependencies are statically compiled and in the repo (except OpenNI/NITE who don't provide static libs).

Linux should compile nicely, although we don't distribute pre-compiled dependencies for it (yet).

Windows now works for Chrome through the FireBreath plugin, Firefox to follow, but Internet Explorer will probably not be supported (unless someone would like to create an ActiveX plugin out of the firebreath framework).

Browsers
--------
### SAFARI:
Safari needs it's own browser plugin & browser extension. webkit-plugin-mac/ contains the plugin, and the extension is in safari-extension-mac/. Unfortunately it does not like soft links, so you must in your terminal run <pre>cd safari-extension-mac/DepthJS.safariextension && ./createHardLinks.sh</pre> Build & run the Xcode project in webkit-plugin-mac, then once inside Safari, enable developer tools & extensions, and finally add the extension under safari-extension-mac/ in Extension Builder. If you click on Inspect Global Pages, you'll see output confirming if it could connect to the Kinect or not (it should be plugged in).

Safari is currently our active development browser because of XCode & GDB. It will work before Chrome at any given moment.

### CHROME:
Chrome extensions support native code, which needs to be compiled. Now under firebreat-plugin/. 
To install/compile:  (Refer to http://www.firebreath.org/display/documentation/Building+FireBreath+Plugins for instructions, they have tutorials and videos, and the process is rather simple)
Start by downloading Firebreath: http://www.firebreath.org/display/documentation/Download

(you must have CMake 2.6+ installed, OpenNI and NITE)

#### Mac building
	cd ${DEPTHJS_DIR}/firebreath-plugin/
	${FIREBREATH_DIR}/prepmac.sh . build/   # make sure you run this from the DepthJS/firebreath-plugin directory
	open build/FireBreath.xcodeproj		# at the point XCode will open, build the project and you should be all set

#### Windows building
	cd ${DEPTHJS_DIR}/firebreath-plugin/
	${FIREBREATH_DIR}/prep2010.cmd . build/	# optionally set "-DOpenNI_INCLUDE_DIRS=<...>", "-DOpenNI_LIBS=<...>", "-DNITE_INCLUDE_DIRS=<...>" and "-DNITE_LIBS=<..>"
	start build\FireBreath.sln			# at this point Visual Studio will open and you should be able to compile the plugin

(Visual Studio is a prerequisite)

The chrome extension is located in chrome-extension/.
The plugin is precompiled under chrome-extension/plugin/.

Go on chrome://extensions and use "Load upacked extension..." to manually load DepthJS as an extension (use the chrome-extension directory), and you should be good to go. Use the error console to fish for errors.

### FIREFOX:
We're in the process of creating a FF extension around the Firebreath NPAPI plugin.

Future work
-----------
In addition to the obvious improvements to our gesture recognition, we need to make the install process easier for end-users.

Most of our concern is to make everything truly cross-platform cross-browser, and make this an open-source reference implementation of a Kinect browser plugin.
One-click install is at second-priority right now, so if anyone wants to pitch in and do this - they are welcome!
