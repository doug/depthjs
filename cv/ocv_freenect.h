/*
 *  ocv_freenect.h
 *  OpenCVTries1
 *
 *  Created by Roy Shilkrot on 11/19/10.
 *  Copyright 2010 MIT. All rights reserved.
 *
 */

#include <vector>

using namespace std;

//#include <opencv2/opencv.hpp>
#include <cv.h>
#include <highgui.h>
#include <cvaux.h>

using namespace cv;

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <libusb.h>
#include "libfreenect.h"

#include <pthread.h>

#include "zhelpers.hpp"
