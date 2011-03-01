DepthJS
=======
DepthJS is a browser extension (currently Chrome) that allows the Microsoft Kinect to talk to any web page. It provides the low-level raw access to the Kinect as well as high-level hand gesture events to simplify development.

Current Status
--------------
Everything is VERY alpha. Hand detection and tracking are reasonably robust, but recognizing gestures is less reliable.

So far we can recognize the following:

- Presence of hand (registration)
- Removal of hand (unregistration)
- Hand movement
- Quick open and close of hand ("hand click")
- Large swipe up/down/left/right

We want to recognize:

- Two hands
- Pushing and pulling
- Z-depth in general
- Individual fingers
- Finger-based hand gestures (e.g. peace sign, etc)

Getting Started
---------------

Right now, you'll want to be on Chrome, on OSX, then:

- Plug in your Kinect to your computer via USB
- Download this code from http://github.com/doug/depthjs
- Install the chrome extension
  - Bring up the extensions management page by clicking the wrench icon and choosing *Tools* > *Extensions*.
  - If *Developer mode* has a + by it, click the + to add developer information to the page. The + changes to a -, and more buttons and information appear.
  - Click the *Load unpacked extension* button. A file dialog appears.
  - In the file dialog, navigate to your *depthjs/chrome-extension-mac* and click *OK*.
- Open a new web page (it only affects new pages)  
- Have fun!

What the camera sees...

- When the extension starts up, it opens 3 windows, blob, first, and second
- first & second show the limits of the depth of field the camera is paying attention to, you should try to make it so that your hand is in both first & second and not much else
- blob shows you everything the camera can see as well as a blue circle around where it thinks your hand is.

After opening a new page in Chrome,

- Pull your hand back so that both first & second windows are blank and there is no circle in the blob window
- Then bring your hand forward until it gets an outline in the blob window, and pause for a second until the blue circle appears
- You should then see a blue circle on the web site that will track your movements
- To click, just close your hand into a fist!


Components
----------
DepthJS is very modular. The Kinect driver and computer vision are written on top of Open Frameworks and OpenCV in C++. This component can output the raw RGB image, the raw depth map (filtered for the hand), as well as the high-level events that the computer vision recognizes. The three outputs are pumped out on three separate 0MQ TCP sockets. Next, a Torando web server (written in Python) takes the 0MQ data and wraps it into a WebSocket, which is what enables the web browser extension to receive the data. Finally a pure javascript-based extension connects to the WebSocket server to receive the events. Event handlers may be placed globally, in content scripts injected into each web page, or pushed via the content script to local DOM elements written by 3rd parties.


BACKEND:
Kinect ====(libfreekinect)====> OpenFramework+OpenCV =====(0MQ)====> Tornado

FRONTEND:
Torando <====(WebSocket)==== DepthJS extension ====(Javascript+DOM events)====> Any web page

Future work
-----------
In addition to the obvious improvements to our gesture recognition, we need to make the install process easier for end-users.

We like that the backend is very modular, so that anything (not just DepthJS) can connect via 0MQ or WebSockets to the C++ Kinect+OpenCV backend.

However, this is not a user friendly setup. We want to eventually integrate the entire driver+backend into an NPAPI-based native code extension such that we can distribute the extension "batteries-included" as a one-click install. Eventually it should be hosted on the various web browser (Safari, Firefox, and Chome) extension galleries.
