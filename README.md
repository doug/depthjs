DepthJS
=======
DepthJS is an open-source browser extension and plugin (currently working for Chrome) that allows the Microsoft Kinect to talk to any web page. It provides the low-level raw access to the Kinect as well as high-level hand gesture events to simplify development.

Current Status
--------------
#### April 2013:
Updated to Chrome extension standards: https://developer.chrome.com/extensions/ 

#### March 2012:
Windows plugin now works with Chrome. 
Use Firebreath to build (see instructions below), or try to use the precompiled version (but then you must rename/copy chrome-extension/manifest.json.WIN32 to chrome-extension/manifest.json).

#### January 2012:
Moving to FireBreath NPAPI plugins for Chrome + Firefox. Mac/Safari version remains as it is (under webkit-plugin-mac and safari-extension-mac).
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

Note: As of now we are using OpenNI/NITE for tracking and gesture rec. 

Platforms
---------
Macs are supported. All dependencies are statically compiled and in the repo (except OpenNI/NITE who don't provide static libs).

Linux should compile nicely, although we don't distribute pre-compiled dependencies for it (yet).

Windows now works for Chrome through the FireBreath plugin, Firefox to follow, but Internet Explorer will probably not be supported (unless someone would like to create an ActiveX plugin out of the firebreath framework).

Prerequisites
--------
Download the (historic) OpenNI libs from: http://www.openni.org/openni-sdk/openni-sdk-history-2/. Get OpenNI, Sensor driver and NiTE.
Download the SensorKinect (if you plan on using Kinect) lib from: https://github.com/avin2/SensorKinect/tree/unstable/Bin.
Mac/Linux: Make sure you have the following directory existing and writeable:
	/var/lib/ni/
	
Install each OpenNI (in order) library using the install script. 
Mac/Linux: You may want to change the install directories to /usr/local/* instead of /usr/*.
Check that all the modules are registered:

	# niReg -l
	
	566 INFO       New log started on 2013-04-14 08:05:29
	600 INFO       OpenNI version is 1.5.2 (Build 23)-MacOSX (Dec 28 2011 17:54:41)
	608 INFO       --- Filter Info --- Minimum Severity: NONE
	OpenNI version is 1.5.2.23.

	Registered modules:

	(compiled with OpenNI 1.5.2.23):
	Script: OpenNI/OpenNI/1.5.2.23
	/usr/local/lib/libXnVFeatures_1_4_1.dylib (compiled with OpenNI 1.3.2.3):
		Scene: PrimeSense/XnVSceneAnalyzer/1.4.1.2
		...
	/usr/local/lib/libXnVHandGenerator_1_4_1.dylib (compiled with OpenNI 1.3.2.3):
		Gesture: PrimeSense/XnVGestureGenrator/1.4.1.2
		...
	/usr/local/lib/libnimMockNodes.dylib (compiled with OpenNI 1.5.2.23):
		ProductionNode: OpenNI/Mock/1.5.2.23
		...
	/usr/local/lib/libnimCodecs.dylib (compiled with OpenNI 1.5.2.23):
		Codec: OpenNI/16zP/1.5.2.23
		...

	/usr/local/lib/libnimRecorder.dylib (compiled with OpenNI 1.5.2.23):
		Recorder: OpenNI/Recorder/1.5.2.23
		...

	/usr/local/lib/libXnDeviceSensorV2.dylib (compiled with OpenNI 1.3.2.3):
		Device: PrimeSense/SensorV2/5.0.3.4
		...

	/usr/local/lib/libXnDeviceFile.dylib (compiled with OpenNI 1.3.2.3):
		Player: PrimeSense/File/5.0.3.4

	/usr/local/lib/libXnDeviceSensorV2KM.dylib (compiled with OpenNI 1.3.2.3):
		Device: PrimeSense/SensorKinect/5.0.3.4
		...

If everything is installed properly you should be able to run the NiTE examples.


Browsers
--------
### CHROME:
Chrome extensions support native code, which needs to be compiled. Now under firebreath-plugin/. 
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

### SAFARI:
Safari is no longer the active development browser, because in 10.7 apple changed the policy for NPAPI plugins so they cannot be run as a singleton in the background.

History: Safari needs it's own browser plugin & browser extension. webkit-plugin-mac/ contains the plugin, and the extension is in safari-extension-mac/. Unfortunately it does not like soft links, so you must in your terminal run <pre>cd safari-extension-mac/DepthJS.safariextension && ./createHardLinks.sh</pre> Build & run the Xcode project in webkit-plugin-mac, then once inside Safari, enable developer tools & extensions, and finally add the extension under safari-extension-mac/ in Extension Builder. If you click on Inspect Global Pages, you'll see output confirming if it could connect to the Kinect or not (it should be plugged in).

### FIREFOX:
Help us create a FF extension around the Firebreath NPAPI plugin!

Future work
-----------
In addition to the obvious improvements to our gesture recognition, we need to make the install process easier for end-users.

Most of our concern is to make everything truly cross-platform cross-browser, and make this an open-source reference implementation of a Kinect browser plugin.
One-click install is at second-priority right now, so if anyone wants to pitch in and do this - they are welcome!
