/*
 *  ocv_freenect.cpp
 *  OpenCVTries1
 *
 *  Created by Roy Shilkrot on 11/19/10.
 *  Copyright 2010 MIT. All rights reserved.
 *
 */

#define LIBUSB_DEBUG 5

#include "ocv_freenect.h"


//#if defined(__APPLE__)
//#include <GLUT/glut.h>
//#include <OpenGL/gl.h>
//#include <OpenGL/glu.h>
//#else
//#include <GL/glut.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
//#endif

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

extern  Scalar refineSegments(const Mat& img, 
							  Mat& mask, 
							  Mat& dst, 
							  vector<Point>& contour,
							  Point2i& previous);
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
	
	freenect_set_log_level(f_ctx, FREENECT_LOG_ERROR);
	
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
	int register_ctr = 0;
	bool registered = false;
	
	Point2i appear(-1,-1); double appearTS = -1;
	
	Point2i midBlob(-1,-1);
	
	while (!die) {
		fr++;
		
//		imshow("rgb", rgbMat);
		depthMat.convertTo(depthf, CV_8UC1, 255.0/2048.0);
//		{
//			Mat tmp,tmp1;
//			depthMat.convertTo(tmp, CV_32FC1);
//			log(tmp,tmp1);
//			tmp1.convertTo(depthf, CV_8UC1, 255.0/7.6246189861593985);
//		}
//		imshow("depth",depthf);
		
		Mat rgbAndDepth,gray; cvtColor(rgbMat, gray, CV_BGR2GRAY);
		equalizeHist(gray, gray);
		vector<Mat> cs; //split(rgbAndDepth,cs);
		cs.push_back(depthf);
		cs.push_back(gray);
		cs.push_back(Mat::zeros(depthf.size(), CV_8UC1));
		merge(cs,rgbAndDepth);
		
//		imshow("rgb",rgbAndDepth);
		
//		IplImage rgbIplI = (IplImage)depthf;
		IplImage rgbIplI = (IplImage)rgbAndDepth;
		
		if(!bg_model)
        {
			CvGaussBGStatModelParams params;

//#define CV_BGFG_MOG_BACKGROUND_THRESHOLD     0.7     /* threshold sum of weights for background test */
//#define CV_BGFG_MOG_STD_THRESHOLD            2.5     /* lambda=2.5 is 99% */
//#define CV_BGFG_MOG_WINDOW_SIZE              200     /* Learning rate; alpha = 1/CV_GBG_WINDOW_SIZE */
//#define CV_BGFG_MOG_NGAUSSIANS               5       /* = K = number of Gaussians in mixture */
//#define CV_BGFG_MOG_WEIGHT_INIT              0.05
//#define CV_BGFG_MOG_SIGMA_INIT               30
//#define CV_BGFG_MOG_MINAREA                  15.f
			
			params.win_size      = 100; //CV_BGFG_MOG_WINDOW_SIZE;
			params.bg_threshold  = 0.7;
			
			params.std_threshold = CV_BGFG_MOG_STD_THRESHOLD;
			params.weight_init   = CV_BGFG_MOG_WEIGHT_INIT;
			
			params.variance_init = CV_BGFG_MOG_SIGMA_INIT*CV_BGFG_MOG_SIGMA_INIT;
			params.minArea       = CV_BGFG_MOG_MINAREA;
			params.n_gauss       = 5;
			
            //create BG model
            bg_model = cvCreateGaussianBGModel( &rgbIplI , &params);

            //bg_model = cvCreateFGDStatModel( &rgbIplI );
            continue;
        }
        
        double t = (double)cvGetTickCount();
        cvUpdateBGStatModel( &rgbIplI, bg_model, update_bg_model ? -1 : 0 );
        t = (double)cvGetTickCount() - t;
		
//		if(fr == 125) update_bg_model = false;
		
		//        printf( "%d. %.1f\n", fr, t/(cvGetTickFrequency()*1000.) );
		//        cvShowImage("BG", bg_model->background);
		//        cvShowImage("FG", bg_model->foreground);
		imshow("foreground", bg_model->foreground);
		
		Mat tmp_bg_fg = (bg_model->foreground) & (depthf < 255);
		vector<Point> ctr;
		Scalar blb = refineSegments(Mat(),tmp_bg_fg,out,ctr,midBlob); //find contours in the foreground, choose biggest
		
//		imshow("rgb", rgbAndDepth);
		
		if(blb[0]>=0 && blb[3] > 500) {
			cvtColor(depthf, outC, CV_GRAY2BGR);
//			blur(outC, outC, Size(25,25));
//			vector<Mat> chns; split(outC, chns);
//			chns[2] = chns[2] & out;
//			merge(chns,outC);
			
			Scalar color(0,0,255);
			for (int idx=0; idx<ctr.size()-1; idx++) 
				line(outC, ctr[idx], ctr[idx+1], color, 2);
			line(outC, ctr[ctr.size()-1], ctr[0], color, 2);

			Vec4f _line;
			fitLine(Mat(ctr), _line, CV_DIST_L2, 0, 0.01, 0.01);
			line(outC, Point(blb[0]-_line[0]*70,blb[1]-_line[1]*70), 
						Point(blb[0]+_line[0]*70,blb[1]+_line[1]*70),
						Scalar(255,255,0), 1);

			//find farthest point on fitted line
//			double angle = Vec2d(1,0).dot(Vec2d(_line[0],_line[1]));
//			cout << "blob area " << blb[3] << ", angle " << angle << endl;
//			Mat rotMat = getRotationMatrix2D(Point(0,0), angle, 1.0);
//			Mat transformed(1,ctr.size(),CV_32FC2);
//			transform(Mat(ctr),transformed,rotMat);
//			vector<Mat> splitted; split(transformed,splitted);
//			Point maxLoc;
//			minMaxLoc(splitted[0],NULL, NULL, NULL, &maxLoc);
//			cout << "point: " << maxLoc.x << "," << maxLoc.y << endl;
//			circle(outC, Point(ctr[maxLoc.y].x,ctr[maxLoc.y].y), 5, Scalar(0,0,255), 3);

			circle(outC, Point(blb[0],blb[1]), 5, Scalar(0,255,0), 3);
			//ellipse(outC, Point(blb[0],blb[1]), Size(100,50), angle*180, 0.0, 360.0, Scalar(255,0,0), 2);
			circle(outC, Point(blb[0],blb[1]), 50, Scalar(255,0,0), 3);
			
			imshow("blob",outC);
			register_ctr = MIN((register_ctr + 1),60);
			
			if (register_ctr > 40 && !registered) {
				registered = true;
				appear.x = -1;
				cout << "register" << endl;
			}
			
			if(registered) {
				cout << "move: " << blb[0] << "," << blb[1] << endl;
			} else {
				//not registered, look for gestures
				if(appear.x<0) {
					//first appearence of blob
					appear = midBlob;
					appearTS = getTickCount();
					cout << "appear ("<<appearTS<<") " << appear.x << "," << appear.y << endl;
				} else {
					//blob was seen before, how much time passed
					double timediff = ((double)getTickCount()-appearTS)/getTickFrequency();
					if (timediff > .1 && timediff < 1.0) { 
						//enough time passed from appearence
						if (appear.x - blb[0] > 100) {
							cout << "right"<<endl; appear.x = -1;
						} else if (appear.x - blb[0] < -100) {
							cout << "left" <<endl; appear.x = -1;
						} else if (appear.y - blb[1] > 100) {
							cout << "up" << endl; appear.x = -1;
						} else if (appear.y - blb[1] < -100) {
							cout << "down" << endl; appear.x = -1;
						}						
					}
					if(timediff >= 1.0) {
						cout << "a ghost..."<<endl;
						//a second passed from appearence - reset 1st appear
						appear.x = -1;
						appearTS = -1;
						midBlob.x = midBlob.y = -1;
					}
				}
			}			
		} else {
			imshow("blob",depthf);
			register_ctr = MAX((register_ctr - 1),0);
		}
		
		if (register_ctr == 0 && registered) {
			midBlob.x = midBlob.y = -1;
			registered = false;
			cout << "unregister" << endl;
		}

		
		/*
		if (fr%5 == 0) {	//every 5 frames add more points to track
			makePointsFromMask(out, prevPts,(fr%25 != 0));//clear points every 25 frame
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
		imshow("depth change",depthf & (depthf < 254) & out);
		imshow("just depth",depthf);
		depthf.copyTo(prevDepth);
		
//		cout << "average motion of largest blob: " << 
//					means[0] << "," << 
//					means[1] << "," <<
//					depthChange[0] <<
//					endl;
		
		{
			Mat _tmp; frameMat.copyTo(_tmp); //,out);
			Point mid = Point(_tmp.cols/2, _tmp.rows/2);
			line(_tmp, mid, mid+Point(means[0],0), Scalar(255,0,0), 5);
			line(_tmp, mid, mid+Point(0,means[1]), Scalar(0,255,0), 5);
			drawPoint(_tmp,prevPts,Scalar(0,0,255)); //,Mat::ones(1, statusv.size(), CV_8UC1));
			drawPoint(_tmp,nextPts,Scalar(255,0,0),&statusM);
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
		 */
		
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
	return 0;
}
