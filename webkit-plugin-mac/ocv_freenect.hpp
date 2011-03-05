/*
 *  ocv_freenect.h
 *  OpenCVTries1
 *
 *  Created by Roy Shilkrot on 11/19/10.
 *  Copyright 2010 MIT. All rights reserved.
 *
 */

#ifndef __DEPTHJS_OCV_FREENECT_HPP__
#define __DEPTHJS_OCV_FREENECT_HPP__

#include <vector>
#include <stack>

using namespace std;

#include <cv.h>
#include <highgui.h>
#include <cvaux.h>

using namespace cv;

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "libusb.h"
#include "libfreenect.h"

#include <pthread.h>

int initFreenect();
void* ocvFreenectThread(void *arg);
void killOcvFreenect();
BOOL isDead();

#endif // __DEPTHJS_OCV_FREENECT_HPP__