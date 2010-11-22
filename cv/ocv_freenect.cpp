/*
 *  ocv_freenect.cpp
 *  OpenCVTries1
 *
 *  Created by Roy Shilkrot on 11/19/10.
 *  Copyright 2010 MIT. All rights reserved.
 *
 */

#include "ocv_freenect.h"


#if defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <math.h>

pthread_t ocv_thread;
volatile int die = 0;

int g_argc;
char **g_argv;

int window;

pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;

Mat depthMat(Size(640,480),CV_16UC1),
	rgbMat(Size(640,480),CV_8UC3,Scalar(0));


freenect_device *f_dev;
int freenect_angle = 0;
int freenect_led;

pthread_cond_t frame_cond = PTHREAD_COND_INITIALIZER;
int got_frames = 0;

void *ocv_threadfunc(void *arg)
{
	printf("opencv thread\n");

	while (!die) {
//		pthread_mutex_lock(&buf_mutex);
		
		imshow("rgb", rgbMat);
//		imshow("depth",depthMat);
		
//		pthread_cond_signal(&frame_cond);
//		pthread_mutex_unlock(&buf_mutex);
		
		int c = waitKey(30);
		if(c == 27) die = true;
	}
	
	pthread_exit(NULL);
	return NULL;
}

uint16_t t_gamma[2048];
freenect_context *f_ctx;

void *freenect_threadfunc(void* arg) {
	cout << "freenect thread"<<endl;
	while(!die && freenect_process_events(f_ctx) >= 0 ) {}
	cout << "freenect die"<<endl;
	return NULL;
}

void depth_cb(freenect_device *dev, freenect_depth *depth, uint32_t timestamp)
{
	pthread_mutex_lock(&buf_mutex);
	
	//copy to ocv buf...
	memcpy(depthMat.data, depth, FREENECT_DEPTH_SIZE);
	
	got_frames++;
	pthread_cond_signal(&frame_cond);
	pthread_mutex_unlock(&buf_mutex);
}

void rgb_cb(freenect_device *dev, freenect_pixel *rgb, uint32_t timestamp)
{
	pthread_mutex_lock(&buf_mutex);
	got_frames++;
	//copy to ocv_buf..
	memcpy(rgbMat.data, rgb, FREENECT_RGB_SIZE);
	
	pthread_cond_signal(&frame_cond);
	pthread_mutex_unlock(&buf_mutex);
}

extern void refineSegments(const Mat& img, Mat& mask, Mat& dst);
extern void makePointsFromMask(Mat& maskm,vector<Point2f>& points, bool _add = false);
extern void drawPoint(Mat& out,vector<Point2f>& points,Scalar color, Mat* maskm = NULL);

int main(int argc, char **argv)
{
	int res;
	
//	cvtColor(rgbMat, rgbMat, CV_RGB2BGR);
//	namedWindow("rgb");
//	namedWindow("depth");
	
	printf("Kinect camera test\n");
	
	int i;
	for (i=0; i<2048; i++) {
		float v = i/2048.0;
		v = powf(v, 3)* 6;
		t_gamma[i] = v*6*256;
	}
	
	g_argc = argc;
	g_argv = argv;
	
	if (freenect_init(&f_ctx, NULL) < 0) {
		printf("freenect_init() failed\n");
		return 1;
	}
	
	freenect_set_log_level(f_ctx, FREENECT_LOG_INFO);
	
	int nr_devices = freenect_num_devices (f_ctx);
	printf ("Number of devices found: %d\n", nr_devices);
	
	int user_device_number = 0;
	if (argc > 1)
		user_device_number = atoi(argv[1]);
	
	if (nr_devices < 1)
		return 1;
	
	if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
		printf("Could not open device\n");
		return 1;
	}
	
	
	freenect_set_tilt_degs(f_dev,freenect_angle);
	freenect_set_led(f_dev,LED_RED);
	freenect_set_depth_callback(f_dev, depth_cb);
	freenect_set_rgb_callback(f_dev, rgb_cb);
	freenect_set_rgb_format(f_dev, FREENECT_FORMAT_RGB);
	freenect_set_depth_format(f_dev, FREENECT_FORMAT_11_BIT);
	
	freenect_start_depth(f_dev);
	freenect_start_rgb(f_dev);

	res = pthread_create(&ocv_thread, NULL, freenect_threadfunc, NULL);
	if (res) {
		printf("pthread_create failed\n");
		return 1;
	}
	
	Mat depthf; //(depthMat.size(),CV_32FC1,Scalar(0));

	CvBGStatModel* bg_model = 0;
	Mat frameMat(rgbMat);
	Mat out(frameMat.size(),CV_8UC1),
		outC(frameMat.size(),CV_8UC3);
	Mat prevImg(frameMat.size(),CV_8UC1),
		nextImg(frameMat.size(),CV_8UC1),
		prevDepth(depthMat.size(),CV_8UC1);
	vector<Point2f> prevPts,nextPts;
	vector<uchar> statusv;
	vector<float> errv;
	Rect cursor(frameMat.cols/2,frameMat.rows/2,10,10);
	int nmfr = 0; //non-motion frames counter
	bool update_bg_model = true;
	int fr = 1;
	
	while (!die) {
		fr++;
		
//		imshow("rgb", rgbMat);
		depthMat.convertTo(depthf, CV_8UC1, 255.0/2048.0);
//		imshow("depth",depthf);			
		IplImage rgbIplI = (IplImage)depthf;
		
		if(!bg_model)
        {
            //create BG model
            bg_model = cvCreateGaussianBGModel( &rgbIplI );
            //bg_model = cvCreateFGDStatModel( tmp_frame );
            continue;
        }
        
        double t = (double)cvGetTickCount();
        cvUpdateBGStatModel( &rgbIplI, bg_model, update_bg_model ? -1 : 0 );
        t = (double)cvGetTickCount() - t;
		//        printf( "%d. %.1f\n", fr, t/(cvGetTickFrequency()*1000.) );
		//        cvShowImage("BG", bg_model->background);
		//        cvShowImage("FG", bg_model->foreground);
		imshow("bg", bg_model->foreground);
		
		Mat tmp_bg_fg(bg_model->foreground);
		refineSegments(frameMat,tmp_bg_fg,out);
		
		if (fr%5 == 0) {	//every 5f add more points to track
			makePointsFromMask(out, prevPts,(fr%25 != 0));//clear points every 25f
		}
		
		cvtColor(frameMat, nextImg, CV_BGR2GRAY);
//		imshow("prev", prevImg);
//		imshow("next", nextImg);
		
		calcOpticalFlowPyrLK(prevImg, nextImg, prevPts, nextPts, statusv, errv);
		nextImg.copyTo(prevImg);
		
		Mat ptsM(prevPts),nptsM(nextPts);
		Mat statusM(statusv);
		Scalar means = mean(ptsM-nptsM,statusM);
		
		Scalar depthChange = mean(depthf-prevDepth,out);
		imshow("depth change",depthf & out);
		depthf.copyTo(prevDepth);
		
		cout << "average motion of largest blob: " << 
					means[0] << "," << 
					means[1] << "," <<
					depthChange[0] <<
					endl;
		
		{
			Mat _tmp; frameMat.copyTo(_tmp); //,out);
			Point mid = Point(_tmp.cols/2, _tmp.rows/2);
			line(_tmp, mid, mid+Point(means[0],0), Scalar(255,0,0), 5);
			line(_tmp, mid, mid+Point(0,means[1]), Scalar(0,255,0), 5);
//			drawPoint(_tmp,prevPts,Scalar(0,0,255)); //,Mat::ones(1, statusv.size(), CV_8UC1));
//			drawPoint(_tmp,nextPts,Scalar(255,0,0),&statusM);
			if(fabs(means[0])>2 && fabs(means[0]) < 60) {
				cursor.x -= means[0];
				//				stringstream ss; ss << "Move right-left";
				//				putText(_tmp, ss.str(), Point(30,30), CV_FONT_HERSHEY_PLAIN, 2.0, Scalar(255,0,255), 2);
			} else if(fabs(means[1])>2 && fabs(means[1]) < 60) {
				cursor.y -= means[1];
				//				stringstream ss; ss << "Move up-down";
				//				putText(_tmp, ss.str(), Point(50,50), CV_FONT_HERSHEY_PLAIN, 2.0, Scalar(255,255,0), 2);
			} else if (depthChange[0] > 50) {
				cursor.width += depthChange[0]/10;
				cursor.height += depthChange[0]/10;
			} else {
				nmfr++;
			}
			
			rectangle(_tmp, cursor, Scalar(0,0,255), 2);
			imshow("out", _tmp);
		}
		prevPts = nextPts;
		
		if (nmfr%15 == 0) {
			cursor.x = frameMat.cols/2;
			cursor.y = frameMat.rows/2;
			cursor.width = cursor.height = 10;
			nmfr = 0;
		}
		
        char k = cvWaitKey(5);
        if( k == 27 ) break;
        if( k == ' ' )
            update_bg_model = !update_bg_model;
	}
	
	printf("-- done!\n");
	
	destroyWindow("rgb");
	destroyWindow("depth");
	
	pthread_join(ocv_thread, NULL);
	pthread_exit(NULL);
}
