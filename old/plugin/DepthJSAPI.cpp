/**********************************************************\

  Auto-generated DepthJSAPI.cpp

\**********************************************************/

#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"

#include "DepthJSAPI.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <libusb.h>

#include <boost/thread/thread.hpp>

using namespace std;

#include <cv.h>
using namespace cv;

Mat depthMat,rgbMat;

///////////////////////////////////////////////////////////////////////////////
/// @fn DepthJSAPI::DepthJSAPI(DepthJSPtr plugin, FB::BrowserHostPtr host)
///
/// @brief  Constructor for your JSAPI object.  You should register your methods, properties, and events
///         that should be accessible to Javascript from here.
///
/// @see FB::JSAPIAuto::registerMethod
/// @see FB::JSAPIAuto::registerProperty
/// @see FB::JSAPIAuto::registerEvent
///////////////////////////////////////////////////////////////////////////////
DepthJSAPI::DepthJSAPI(DepthJSPtr plugin, FB::BrowserHostPtr host) : m_plugin(plugin), m_host(host), freenect_angle(0)
{
	cout << "DEPTHJS API CONSTRUBCTOR" << "\n";
	registerMethod("initKnct", make_method(this, &DepthJSAPI::initKnct));
    registerMethod("killKnct", make_method(this, &DepthJSAPI::killKnct));
	
    // Read-only property
    registerProperty("version",
                     make_property(this,
                        &DepthJSAPI::get_version));
	cout << "DONE WITH CONSTRUCOTR" << "\n";
    
}

///////////////////////////////////////////////////////////////////////////////
/// @fn DepthJSAPI::~DepthJSAPI()
///
/// @brief  Destructor.  Remember that this object will not be released until
///         the browser is done with it; this will almost definitely be after
///         the plugin is released.
///////////////////////////////////////////////////////////////////////////////
DepthJSAPI::~DepthJSAPI()
{
	cout << "DEPTHJS API DEEEEESTRCUTOR" << "\n";
}

///////////////////////////////////////////////////////////////////////////////
/// @fn DepthJSPtr DepthJSAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
DepthJSPtr DepthJSAPI::getPlugin()
{
	cout << "GETTING PLUGGIN REF" << "\n";
    DepthJSPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
	cout << "GIT PLUGIN REF" << "\n";
    return plugin;
}

void depth_cb(freenect_device *dev, void *depth, uint32_t timestamp)
{
//	pthread_mutex_lock(&buf_mutex);
//	
//	//copy to ocv buf...
//	memcpy(depthMat.data, depth, FREENECT_DEPTH_SIZE);
//	
//	got_frames++;
//	pthread_cond_signal(&frame_cond);
//	pthread_mutex_unlock(&buf_mutex);
}

void rgb_cb(freenect_device *dev, freenect_pixel *rgb, uint32_t timestamp)
{
//	pthread_mutex_lock(&buf_mutex);
//	got_frames++;
//	//copy to ocv_buf..
//	//	memcpy(rgbMat.data, rgb, FREENECT_RGB_SIZE);
//	
//	pthread_cond_signal(&frame_cond);
//	pthread_mutex_unlock(&buf_mutex);
}

void DepthJSAPI::freenect_threadfunc() {	//all this thread does is to fetch events from freenect
	cout << "IN KINECT THREAD" << "\n";
	m_host->htmlLog("freenect thread");
	while(!boost::this_thread::interruption_requested() && freenect_process_events(f_ctx) >= 0 ) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(1));
	}
	m_host->htmlLog("freenect die");
}

void DepthJSAPI::send_event(const string& etype, const string& edata) {
	//	s_sendmore (socket, "event");
	stringstream ss;
	ss << "{\"type\":\"" << etype << "\",\"data\":{" << edata << "}}";
	//	s_send (socket, ss.str());
}

///Calculating the laplacian of a 2D curve. Thanks Y.Gingold!
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
		//a feeble attempt to save up in memory allocation.. in 99.9% of the cases this if fires
	if(lapX.rows != X.rows) lapX = laplacian_mtx(X.rows,false); 
	
	Mat _X;	//handle non-64UC2 matrices
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

Scalar refineSegments(const Mat& img, 
					  Mat& mask, 
					  Mat& dst, 
					  vector<Point>& contour,
					  Point2i& previous)
{
	//    int niters = 3;
    
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    
    Mat temp;
    
	//    dilate(mask, temp, Mat(), Point(-1,-1), niters);
	//    erode(temp, temp, Mat(), Point(-1,-1), niters*2);
	//    dilate(temp, temp, Mat(), Point(-1,-1), niters);
	blur(mask, temp, Size(21,21));
	temp = temp > 85.0;
    
    findContours( temp, contours, /*hierarchy,*/ CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
	
	if(dst.data==NULL)
		dst = Mat::zeros(img.size(), CV_8UC1);
	else
		dst.setTo(Scalar(0));
    
    if( contours.size() == 0 )
        return Scalar(-1,-1);
	
    // iterate through all the top-level contours,
    // draw each connected component with its own random color
    unsigned int idx = 0, largestComp = 0;
    double maxArea = 0;
    
	//    for( ; idx >= 0; idx = hierarchy[idx][0] )
	for (; idx<contours.size(); idx++)
    {
        const vector<Point>& c = contours[idx];
		Scalar _mean = mean(Mat(contours[idx]));
        double area = fabs(contourArea(Mat(c))) * 
		((previous.x >- 1) ? 1.0 / (1.0 + norm(Point(_mean[0],_mean[1])-previous)) : 1.0);	//consider distance from last blob
        if( area > maxArea )
        {
            maxArea = area;
            largestComp = idx;
        }
    }
    Scalar color( 255 );
	//	cout << "largest cc " << largestComp << endl;
	//   drawContours( dst, contours, largestComp, color, CV_FILLED); //, 8, hierarchy );
	//	for (idx=0; idx<contours[largestComp].size()-1; idx++) {
	//		line(dst, contours[largestComp][idx], contours[largestComp][idx+1], color, 2);
	//	
	int num = contours[largestComp].size();
	Point* pts = &(contours[largestComp][0]);
	fillPoly(dst, (const Point**)(&pts), &num, 1, color);
	
	Scalar b = mean(Mat(contours[largestComp]));
	b[3] = maxArea;
	
	contour.clear();
	contour = contours[largestComp];
	
	previous.x = b[0]; previous.y = b[1];
	
	return b;
}



void DepthJSAPI::ocv_threadfunc() {
	Mat depthf;
	
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
	bool update_bg_model = true;
	int fr = 1;
	int register_ctr = 0;
	bool registered = false;
	
	Point2i appear(-1,-1); double appearTS = -1;
	
	Point2i midBlob(-1,-1);
	Point2i lastMove(-1,-1);
	
	int hcr_ctr = -1;
	vector<int> hc_stack(20); int hc_stack_ptr = 0;
	
	while (!boost::this_thread::interruption_requested()) {
		fr++;
		
		//		imshow("rgb", rgbMat);
		//Linear interpolation
		{
			Mat _tmp = (depthMat - 400.0);					//minimum observed value is ~440. so shift a bit
			_tmp.setTo(Scalar(2048), depthMat > 600.0);		//cut off at 600 to create a "box" where the user interacts
			_tmp.convertTo(depthf, CV_8UC1, 255.0/1648.0);	//values are 0-2048 (11bit), account for -400 = 1648
		}
		
		//		{ //saving the frames to files for debug
		//			stringstream ss; ss << "depth_"<<fr<<".png";
		//			imwrite(ss.str(), depthf);
		//		}
		
		//Logarithm interpolation - try it!, It should be more "sensitive" for closer depths
		//		{
		//			Mat tmp,tmp1;
		//			depthMat.convertTo(tmp, CV_32FC1);
		//			log(tmp,tmp1);
		//			tmp1.convertTo(depthf, CV_8UC1, 255.0/7.6246189861593985);
		//		}
		//		imshow("depth",depthf);
		
		
		Mat tmp_bg_fg = depthf < 255;	//anything not white is "real" depth
		vector<Point> ctr;
		Scalar blb = refineSegments(Mat(),tmp_bg_fg,out,ctr,midBlob); //find contours in the foreground, choose biggest
		
		if(blb[0]>=0 && blb[3] > 500) {
			cvtColor(depthf, outC, CV_GRAY2BGR);
			
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
				stringstream ss; ss << "\"x\":" << (int)floor(blb[0]*100.0/640.0) << ",\"y\":"<<(int)floor(blb[1]*100.0/480.0);
				cout << "move: " << ss.str() << endl;
				send_event("Move", ss.str());
				
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
				if ((_avg[0] - (double)hcr_ctr) > 5.0) { //a big drop in curvature = hand fisted?
					cout << "Hand click!" << endl;
					send_event("HandClick", "");
				}
				//				{	//some debug on screen..
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
//			imshow("blob",outC);
//			send_image(outC);
		} else {
//			imshow("blob",depthf);
//			send_image(depthf);
			register_ctr = MAX((register_ctr - 1),0);
		}
		
		if (register_ctr <= 15 && registered) {
			midBlob.x = midBlob.y = -1;
			registered = false;
			update_bg_model = true;
			cout << "unregister" << endl;
			send_event("Unregister", "");
		}
		
		boost::this_thread::sleep(boost::posix_time::milliseconds(5));
//        char k = cvWaitKey(5);
//        if( k == 27 ) break;
//        if( k == ' ' )
//            update_bg_model = !update_bg_model;
//		if (k=='s') {
//			cout << "send test event" << endl;
//			send_event("TestEvent", "");
//		}
	}
	
	m_host->htmlLog("ocv done!\n");
}

// Read-only property version
std::string DepthJSAPI::get_version()
{
    return "CURRENT_VERSION";
}

void DepthJSAPI::killKnct() {
	cout << "KILLING KINECT" << "\n";
	
	m_thread.interrupt();
	
	freenect_stop_rgb(f_dev);
	freenect_stop_depth(f_dev);
	freenect_shutdown(f_ctx);
}

void DepthJSAPI::initKnct() {
	//setup Freenect...
	cout << "HELLO BIT HES" << "\n";
	if (freenect_init(&f_ctx, NULL) < 0) {
		m_host->htmlLog("freenect_init() failed");
		return;
	}
	
	freenect_set_log_level(f_ctx, FREENECT_LOG_ERROR);
	
	int nr_devices = freenect_num_devices (f_ctx);
	{
		stringstream ss; ss << "Number of devices found: " << nr_devices;
		m_host->htmlLog(ss.str());
	}
	
	int user_device_number = 0;
	
	if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
		m_host->htmlLog("Could not open device\n");
		return;
	}
	
	freenect_set_tilt_degs(f_dev,freenect_angle);
	freenect_set_led(f_dev,LED_RED);
	freenect_set_depth_callback(f_dev, depth_cb);
	freenect_set_rgb_callback(f_dev, rgb_cb);
	freenect_set_rgb_format(f_dev, FREENECT_FORMAT_RGB);
	freenect_set_depth_format(f_dev, FREENECT_FORMAT_11_BIT);
	
	freenect_start_depth(f_dev);
	freenect_start_rgb(f_dev);
	
	m_thread = boost::thread(&DepthJSAPI::freenect_threadfunc,this);
}