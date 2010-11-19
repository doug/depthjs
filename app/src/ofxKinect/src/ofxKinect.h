#pragma once

#include "ofConstants.h"
#include "ofTexture.h"
#include "ofGraphics.h"
#include "ofTypes.h"

#include "ofxThread.h"
#include "ofxVectorMath.h"

#include <libusb.h>
#include "libfreenect.h"

class ofxKinect : public ofBaseVideo, protected ofxThread{

	public :

		ofxKinect();
		virtual ~ofxKinect();

		/// is the current frame new?
		bool isFrameNew() {return true;}
        
		/// open the connection and start grabbing images
		bool open();
        
		/// close the connection and stop grabbing images
		void close();
        
		/// initialize resources, must be called before open()
		bool init(bool bTexture=true);
        
		/// clear resources
		void clear();
	
		float getDistanceAt(int x, int y);
		float getDistanceAt(const ofPoint & p);

		ofColor	getColorAt(int x, int y);
		ofColor getColorAt(const ofPoint & p);

		ofColor getCalibratedColorAt(int x, int y);
		ofColor getCalibratedColorAt(const ofPoint & p);

		ofxMatrix4x4 getRGBDepthMatrix();
		void setRGBDepthMatrix(const ofxMatrix4x4 & matrix);
		
		/// get the pixels of the most recent rgb frame
		unsigned char	* getPixels();
		
		/// get the pixels of the most recent depth frame
		unsigned char 	* getDepthPixels();		// grey scale values
		unsigned short	* getRawDepthPixels();	// raw 11 bit values
		
		// get the rgb pixels corrected to match the depth frame
		unsigned char * getCalibratedRGBPixels();

		/// get the distance in centimeters to a given point
		float* getDistancePixels();
		
		/// get the rgb texture
		ofTexture &		getTextureReference();
		
		/// get the greyscale depth texture
		ofTexture &		getDepthTextureReference();
		
		void 			setVerbose(bool bTalkToMe);
        
		void 			setUseTexture(bool bUse);
		void 			draw(float x, float y, float w, float h);
		void 			draw(float x, float y);
		
		void 			drawDepth(float x, float y, float w, float h);
		void 			drawDepth(float x, float y);
		
		/**
			\brief	updates the pixel buffers and textures
				
				make sure to call this to update to the latetst incoming frames
		*/
		void update();

		float 			getHeight();
		float 			getWidth();

		const static int	width = 640;
		const static int	height = 480;

	protected:

		bool					bUseTexture;
		ofTexture				depthTex;			// the depth texture
		ofTexture 				rgbTex;				// the RGB texture
		bool 					bVerbose;
		bool 					bGrabberInited;
		
		unsigned char *			depthPixels;
		unsigned char *			rgbPixels;
		unsigned char *			calibratedRGBPixels;
		
		unsigned short *		depthPixelsRaw;
		float * 				distancePixels;
        
    private:

		libusb_device_handle* kinectDev;	// kinect device handle
		
		unsigned short *	depthPixelsBack;	// depth back
		unsigned char *		rgbPixelsBack;		// rgb back
		
		bool bNeedsUpdate;
		bool bUpdateTex;
		
		ofxMatrix4x4		rgbDepthMatrix;

		// libfreenect callbacks
		static void grabDepthFrame(uint16_t *buf, int width, int height);
		static void grabRgbFrame(uint8_t *buf, int width, int height);
    
		// thread function
		void threadedFunction();
};

