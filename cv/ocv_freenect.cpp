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

void depth_cb(freenect_device *dev, void *depth, uint32_t timestamp)
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

zmq::context_t context (1);
zmq::socket_t socket (context, ZMQ_PUB);

void send_event(const string& etype, const string& edata) {
	s_sendmore (socket, "event");
	stringstream ss;
	ss << "{\"type\":\"" << etype << "\",\"data\":{" << edata << "}}";
	s_send (socket, ss.str());
}

void send_image(const Mat& img) {
	s_sendmore(socket, "image");
	
	Mat _img;
	if(img.type() == CV_8UC1) {
		Mat _tmp; resize(img, _tmp,Size(320,240)); //better to resize the gray data and not RGB
		cvtColor(_tmp, _img, CV_GRAY2RGB);
	} else {
		resize(img, _img, Size(320,240));
	}
	
	s_send(socket, (const char*)_img.data);
}

Mat laplacian_mtx(int N, bool closed_poly) {
	Mat A = Mat::zeros(N, N, CV_64FC1);
	Mat d = Mat::zeros(N, 1, CV_64FC1);
    
    //## endpoints
	//if(closed_poly) {
	A.at<double>(0,1) = 1;
	d.at<double>(0,0) = 1;
	
	A.at<double>(N-1,N-2) = 1;
	d.at<double>(N-1,0) = 1;
	//} else {
	//      A.at<double>(0,1) = 1;
	//      d.at<double>(0,0) = 1;
	//}
    
    //## interior points
	for(int i = 1; i <= N-2; i++) {
        A.at<double>(i, i-1) = 1;
        A.at<double>(i, i+1) = 1;
        
        d.at<double>(i,0) = 0.5;
	}
    
	Mat Dinv = Mat::diag( d );
    
	return Mat::eye(N,N,CV_64FC1) - Dinv * A;
}

void calc_laplacian(Mat& X, Mat& Xlap) {
	static Mat lapX = laplacian_mtx(X.rows,false);
	if(lapX.rows != X.rows) lapX = laplacian_mtx(X.rows,false);
	
	Mat _X;
	if (X.type() != CV_64FC2) {
		X.convertTo(_X, CV_64FC2);
	} else {
		_X = X;
	}

	
	vector<Mat> v; split(_X,v);
	v[0] = v[0].t() * lapX.t();
	v[1] = v[1].t() * lapX.t();
	cv::merge(v,Xlap);
	
	Xlap = Xlap.t();
}


int main(int argc, char **argv)
{
	int res;

//	cvtColor(rgbMat, rgbMat, CV_RGB2BGR);
//	namedWindow("rgb");
//	namedWindow("depth");


	try {
		socket.bind ("tcp://*:14444");
		s_sendmore (socket, "event");
        s_send (socket, "{type:\"up\"}");
	}
	catch (zmq::error_t e) {
		cerr << "Cannot bind to socket: " <<e.what() << endl;
		return -1;
	}

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

//	CvBGStatModel* bg_model = 0;
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
//	int nmfr = 0; //non-motion frames counter
	bool update_bg_model = true;
	int fr = 1;
	int register_ctr = 0;
	bool registered = false;

	Point2i appear(-1,-1); double appearTS = -1;

	Point2i midBlob(-1,-1);
	Point2i lastMove(-1,-1);
	
//	int hc_ctr = 0;
	int hcr_ctr = -1;
	vector<int> hc_stack(20); int hc_stack_ptr = 0;
	
	while (!die) {
		fr++;

//		imshow("rgb", rgbMat);
		//Linear interpolation
		{
			Mat _tmp = (depthMat - 400.0);					//minimum observed value is ~440. so shift a bit
			_tmp.setTo(Scalar(2048), depthMat > 600.0);		//cut off at 600 to create a "box" where the user interacts
			_tmp.convertTo(depthf, CV_8UC1, 255.0/1648.0);	//values are 0-2048 (11bit), account for -400 = 1648
		}
//		{
//			stringstream ss; ss << "depth_"<<fr<<".png";
//			imwrite(ss.str(), depthf);
//		}
		//Logarithm interpolcation
//		{
//			Mat tmp,tmp1;
//			depthMat.convertTo(tmp, CV_32FC1);
//			log(tmp,tmp1);
//			tmp1.convertTo(depthf, CV_8UC1, 255.0/7.6246189861593985);
//		}
//		imshow("depth",depthf);

		/*
		//Mix grayscale and depth for bg-fg model
//		Mat rgbAndDepth,gray; cvtColor(rgbMat, gray, CV_BGR2GRAY);
//		equalizeHist(gray, gray);
//		vector<Mat> cs; //split(rgbAndDepth,cs);
//		cs.push_back(depthf);
//		cs.push_back(gray);
//		cs.push_back(Mat::zeros(depthf.size(), CV_8UC1));
//		merge(cs,rgbAndDepth);

//		imshow("rgb",rgbAndDepth);

//		IplImage rgbIplI = (IplImage)depthf;
//		IplImage rgbIplI = (IplImage)rgbAndDepth;

//		if(!bg_model)
//        {
//			CvGaussBGStatModelParams params;

//#define CV_BGFG_MOG_BACKGROUND_THRESHOLD     0.7     /* threshold sum of weights for background test /
//#define CV_BGFG_MOG_STD_THRESHOLD            2.5     /* lambda=2.5 is 99% /
//#define CV_BGFG_MOG_WINDOW_SIZE              200     /* Learning rate; alpha = 1/CV_GBG_WINDOW_SIZE /
//#define CV_BGFG_MOG_NGAUSSIANS               5       /* = K = number of Gaussians in mixture /
//#define CV_BGFG_MOG_WEIGHT_INIT              0.05
//#define CV_BGFG_MOG_SIGMA_INIT               30
//#define CV_BGFG_MOG_MINAREA                  15.f

//			params.win_size      = 100; //CV_BGFG_MOG_WINDOW_SIZE;
//			params.bg_threshold  = 0.7;
//
//			params.std_threshold = CV_BGFG_MOG_STD_THRESHOLD;
//			params.weight_init   = CV_BGFG_MOG_WEIGHT_INIT;
//
//			params.variance_init = CV_BGFG_MOG_SIGMA_INIT*CV_BGFG_MOG_SIGMA_INIT;
//			params.minArea       = CV_BGFG_MOG_MINAREA;
//			params.n_gauss       = 5;
//
//            //create BG model
////            bg_model = cvCreateGaussianBGModel( &rgbIplI , &params);
//
//            //bg_model = cvCreateFGDStatModel( &rgbIplI );
//            continue;
//        }

		//Background-forground model
//        double t = (double)cvGetTickCount();
//        cvUpdateBGStatModel( &rgbIplI, bg_model, update_bg_model ? -1 : 0 );
//        t = (double)cvGetTickCount() - t;

		if(fr == 50) update_bg_model = true;

		//        printf( "%d. %.1f\n", fr, t/(cvGetTickFrequency()*1000.) );
		//        cvShowImage("BG", bg_model->background);
		//        cvShowImage("FG", bg_model->foreground);
		//imshow("foreground", bg_model->foreground);
*/
		
		Mat tmp_bg_fg = depthf < 255; //(bg_model->foreground) & (depthf < 255); //not using bg-fg anymore
		vector<Point> ctr;
		Scalar blb = refineSegments(Mat(),tmp_bg_fg,out,ctr,midBlob); //find contours in the foreground, choose biggest

//		imshow("rgb", rgbAndDepth);

		if(blb[0]>=0 && blb[3] > 500) {
			cvtColor(depthf, outC, CV_GRAY2BGR);
//			blur(outC, outC, Size(25,25));
//			vector<Mat> chns; split(outC, chns);
//			chns[2] = chns[2] & out;
//			merge(chns,outC);

			//draw contour
			Scalar color(0,0,255);
			for (int idx=0; idx<ctr.size()-1; idx++)
				line(outC, ctr[idx], ctr[idx+1], color, 1);
			line(outC, ctr[ctr.size()-1], ctr[0], color, 1);

			//draw "major axis"
			Vec4f _line;
			Mat curve(ctr);
			fitLine(curve, _line, CV_DIST_L2, 0, 0.01, 0.01);
			line(outC, Point(blb[0]-_line[0]*70,blb[1]-_line[1]*70),
						Point(blb[0]+_line[0]*70,blb[1]+_line[1]*70),
						Scalar(255,255,0), 1);
						
			//blob center
			circle(outC, Point(blb[0],blb[1]), 50, Scalar(255,0,0), 3);
			
			//closest point to the camera
			Point minLoc; double minval;
			minMaxLoc(depthMat, &minval, NULL, &minLoc, NULL, out);
			circle(outC, minLoc, 5, Scalar(0,255,0), 3);
			vector<int> c;
			
//			cout << "min depth " << minval << endl;

			register_ctr = MIN((register_ctr + 1),60);

			if (register_ctr > 30 && !registered) {
				registered = true;
				appear.x = -1;
				cout << "register" << endl;
				send_event("Register", "");
				update_bg_model = false;
				
				lastMove.x = blb[0]; lastMove.y = blb[1];
			}

			if(registered) {
//				stringstream ss; ss << "\"x\":" << (int)floor(lastMove.x - blb[0]) << ",\"y\":" << (int)floor(lastMove.y - blb[1]);
				stringstream ss; ss << "\"x\":" << (int)floor(blb[0]*100.0/640.0) << ",\"y\":"<<(int)floor(blb[1]*100.0/480.0);
				cout << "move: " << ss.str() << endl;
				send_event("Move", ss.str());
//				lastMove.x = blb[0]; lastMove.y = blb[1];
				
				
				//---------------------- fist detection ---------------------
				//calc laplacian of curve
				vector<Point> approxCurve;	//approximate curve
				approxPolyDP(curve, approxCurve, 10.0, true);
				Mat approxCurveM(approxCurve);
				
				Mat curve_lap;
				calc_laplacian(approxCurveM, curve_lap);	//calc laplacian
				
				hcr_ctr = 0;
				for (int i=0; i<approxCurve.size(); i++) {
					double n = norm(((Point2d*)(curve_lap.data))[i]);
					if (n > 5.0) {
						//high curvature point
						circle(outC, approxCurve[i], 3, Scalar(50,155,255), 2);
						hcr_ctr++;
					}
				}
				
				
				hc_stack.at(hc_stack_ptr) = hcr_ctr;
				hc_stack_ptr = (hc_stack_ptr + 1) % hc_stack.size();
				
				Scalar _avg = mean(Mat(hc_stack));
				if ((_avg[0] - (double)hcr_ctr) > 5.0) { //a big drop in curvature
					cout << "Hand click!" << endl;
					send_event("HandClick", "");
				}
//				{
//					stringstream ss; ss << "high curve pts " << hcr_ctr << ", avg " << _avg[0];
//					putText(outC, ss.str(), Point(50,50), CV_FONT_HERSHEY_PLAIN, 2.0,Scalar(0,0,255), 2);
//				}				
			} else {
				//not registered, look for gestures
				if(appear.x<0) {
					//first appearence of blob
					appear = midBlob;
//					update_bg_model = false;
					appearTS = getTickCount();
					cout << "appear ("<<appearTS<<") " << appear.x << "," << appear.y << endl;
				} else {
					//blob was seen before, how much time passed
					double timediff = ((double)getTickCount()-appearTS)/getTickFrequency();
					if (timediff > .1 && timediff < 1.0) {
						//enough time passed from appearence
						if (appear.x - blb[0] > 100) {
							cout << "right"<<endl; appear.x = -1;
							send_event("SwipeRight", "");
							update_bg_model = true;
							register_ctr = 0;
						} else if (appear.x - blb[0] < -100) {
							cout << "left" <<endl; appear.x = -1;
							send_event("SwipeLeft", "");
							update_bg_model = true;
							register_ctr = 0;
						} else if (appear.y - blb[1] > 150) {
							cout << "up" << endl; appear.x = -1;
							send_event("SwipeUp", "");
							update_bg_model = true;
							register_ctr = 0;
						} else if (appear.y - blb[1] < -150) {
							cout << "down" << endl; appear.x = -1;
							send_event("SwipeDown", "");
							update_bg_model = true;
							register_ctr = 0;
						}
					}
					if(timediff >= 1.0) {
						cout << "a ghost..."<<endl;
						update_bg_model = true;
						//a second passed from appearence - reset 1st appear
						appear.x = -1;
						appearTS = -1;
						midBlob.x = midBlob.y = -1;
					}
				}
			}
			imshow("blob",outC);
			send_image(outC);
		} else {
			imshow("blob",depthf);
			send_image(depthf);
			register_ctr = MAX((register_ctr - 1),0);
		}

		if (register_ctr <= 15 && registered) {
			midBlob.x = midBlob.y = -1;
			registered = false;
			update_bg_model = true;
			cout << "unregister" << endl;
			send_event("Unregister", "");
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

//		if (nmfr%15 == 0) {
//			cursor.x = frameMat.cols/2;
//			cursor.y = frameMat.rows/2;
//			cursor.width = cursor.height = 10;
//			nmfr = 0;
//		}
		
		

        char k = cvWaitKey(5);
        if( k == 27 ) break;
        if( k == ' ' )
            update_bg_model = !update_bg_model;
		if (k=='s') {
			cout << "send test event" << endl;
			send_event("TestEvent", "");
		}
	}

	printf("-- done!\n");

	//destroyWindow("rgb");
	//destroyWindow("depth");

	pthread_join(ocv_thread, NULL);
	pthread_exit(NULL);
	return 0;
}
