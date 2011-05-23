DepthJS
=======
DepthJS is a browser extension (currently Chrome & Safari) that allows the Microsoft Kinect to talk to any web page. It provides the low-level raw access to the Kinect as well as high-level hand gesture events to simplify development.

Current Status
--------------
Pre-alpha. Really; you may want to wait a bit or help contribute (for real). Hand detection is reasonably robust, but recognizing gestures is less reliable.

So far we can recognize the following:

- Presence of hand (registration)
- Removal of hand (unregistration)
- Hand movement
- Large swipe up/down/left/right

Our previous hand gesture implemented sucked. We are working (slowly) on a new implementation and associated UX.

Components
----------
DepthJS is very modular. The Kinect driver and computer vision are written on top of libfreenect & OpenCV in C++. This component can output the raw RGB image, the raw depth map (filtered for the hand), as well as the high-level events that the computer vision recognizes. A native browser plugin (think Flash) wraps this Kinect code, which directly interacts with a Javascript browser plugin.  Fortunately in Chrome extensions can contain native code, so it will be easy for anyone to install it. Safari requires a bit more work with a plugin installer & needing to go to the extension "store," if Apple will even permit this.

Event handlers in the browser extension may be placed globally, in content scripts injected into each web page, or pushed via the content script to local DOM elements written by 3rd parties.

FLOW DIAGRAM:
Kinect =====> Browser plugin/native code =====> Browser extension ===(Javascript+DOM events)==> Any web page

Platforms
---------
Right now we only support Macs. All dependencies are statically compiled and in the repo.

Linux likely automatically works, although we don't distribute pre-compiled dependencies for it (yet).

Windows seems wayyy complicated, and we are OS snobs. Perhaps once Mac & Linux are done we'll *think* about trying it. Outside code is always welcome ;)

Browsers
--------
SAFARI:
Safari needs it's own browser plugin & browser extension. webkit-plugin-mac/ contains the plugin, and the extension is in safari-extension-mac/. Unfortunately it does not like soft links, so you must in your terminal run <pre>cd safari-extension-mac/DepthJS.safariextension && ./createHardLinks.sh</pre> Build & run the Xcode project in webkit-plugin-mac, then once inside Safari, enable developer tools & extensions, and finally add the extension under safari-extension-mac/ in Extension Builder. If you click on Inspect Global Pages, you'll see output confirming if it could connect to the Kinect or not (it should be plugged in).

Safari is currently our active development browser because of XCode & GDB. It will work before Chrome at any given moment.

CHROME:
Chrome extensions support native code, which needs to be compiled. It's under npapi_plugin/ Run build-mac.sh to create it.

The chrome extension is located in chrome-extension-mac/.

Future work
-----------
In addition to the obvious improvements to our gesture recognition, we need to make the install process easier for end-users.

Eventually when it's ready it will be a 1-click install. As this is a side side project and some of us are trying to finish our PhDs *cough* "ontime," updates have been steady but infrequent.

