/*
 DepthJS
 Copyright (C) 2010 Aaron Zinman, Doug Fritz, Roy Shilkrot, Greg Elliott
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.
 
 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "FreenectDevice.h"

#define LABEL_GARBAGE	0
#define LABEL_OPEN		1
#define LABEL_FIST		2
#define LABEL_THUMB		3

extern void send_event(const string& etype, const string& edata);

#include <deque>
using namespace std;

class GestureEngine {
private:
	bool running;
	
	Mat depthMat;
	Mat depthf;
	Mat rgbMat;
	Mat ownMat;
	Mat hsv;
	
	Freenect::Freenect<MyFreenectDevice> freenect;
	MyFreenectDevice* device;
	
	bool registered;
	Mat blobMaskOutput;
	Mat outC;
	Point3i midBlob;
	
	//descriptor parameters
	int startX, sizeX, num_x_reps, num_y_reps;
	double	height_over_num_y_reps,width_over_num_x_reps;
	
	
	vector<double> _d;	//the descriptor
	Mat descriptorMat;	//as a matrix
	
	CvKNearest classifier;
	
	vector<vector<double> > training_data;
	vector<int>				label_data;
	PCA pca;
	Mat labelMat, dataMat; 
	vector<float> label_counts;
	
	bool trained;
	bool loaded;
	
	int mode;
	
	int register_ctr,register_secondbloc_ctr;
	
	Point3i appear; double appearTS;
	
	Point3i lastMove;
	
	int hcr_ctr;
	vector<int> hc_stack; 
	int hc_stack_ptr;
	
	int pca_number_of_features;
	
	Vec2i mean_hue_sat_blob;
	
	std::deque<Point3i> positionQueue;

	vector<int> _refineSegments(const Mat& img, 
					Mat& mask, 
					Mat& dst, 
					vector<Point>& contour,
					vector<Point>& second_contour,
					Point3i& previous);
	
	int TrainModel();
	void SaveModelData();
	int LoadModelData(const char* filename);
	void InterpolateAndInpaint();
	void ComputeDescriptor(Scalar);
	string GetStringForGestureCode(int);
	void CheckRegistered(vector<int>&,int,Scalar);
	int GetMostLikelyGesture();
	void BiasHandColor(Mat &);
	
public:
	bool die;

	GestureEngine():	running(false),
						registered(false),
						startX(250),
						sizeX(150),
						num_x_reps(10),
						num_y_reps(10),
						height_over_num_y_reps(480/num_y_reps),
						width_over_num_x_reps(sizeX/num_x_reps),
						label_counts(vector<float>(4)),
						trained(false),
						loaded(false),
						mode(LABEL_GARBAGE),
						pca_number_of_features(25),
						die(false)
	{
		depthMat = Mat(Size(640,480),CV_16UC1);
		depthf = Mat(Size(640,480),CV_8UC1);
		rgbMat = Mat(Size(640,480),CV_8UC3,Scalar(0));
		ownMat = Mat(Size(640,480),CV_8UC3,Scalar(0));
		blobMaskOutput = Mat(Size(640,480),CV_8UC1,Scalar(0));
		
		_d = vector<double>(num_x_reps*num_y_reps);
		descriptorMat = Mat(_d);
		
		register_ctr = register_secondbloc_ctr = 0;
		registered = false;
		
		appear = Point3i(-1,-1,-1); 
		appearTS = -1;
		
		midBlob = Point3i(-1,-1,-1);
		lastMove = Point3i(-1,-1,-1);
		
		hcr_ctr = -1;
		hc_stack = vector<int>(20); 
		hc_stack_ptr = 0;
		
		mean_hue_sat_blob = Vec2i(-1,-1);
		
		//positionQueue = deque<Point3i>();
	};
	
	void RunEngine();
	bool getRunning() { return running; }
	int InitializeFreenect();
};

vector<int> GestureEngine::_refineSegments(const Mat& img, 
					   Mat& mask, 
					   Mat& dst, 
					   vector<Point>& contour,
					   vector<Point>& second_contour,
					   Point3i& previous)
{
	//    int niters = 3;
    
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
	
	vector<int> b(5,-1); //return value
    
    Mat temp;
    
	blur(mask, temp, Size(11,11));
	temp = temp > 85.0;
	
    findContours( temp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
	
	if(dst.data==NULL)
		dst = Mat::zeros(img.size(), CV_8UC1);
	else
		dst.setTo(Scalar(0));
    
    if( contours.size() == 0 )
        return b;
	
    // iterate through all the top-level contours,
    // draw each connected component with its own random color
    int idx = 0, largestComp = -1, secondlargest = -1;
    double maxWArea = 0, maxJArea = 0;
    vector<double> justarea(contours.size());
	vector<double> weightedarea(contours.size());
	Scalar color( 255 );
	Mat _blob_mask = Mat::zeros(mask.size(),CV_8UC1);
	
	//    for( ; idx >= 0; idx = hierarchy[idx][0] )
	for (; idx<contours.size(); idx++)
    {
        vector<Point>& c = contours[idx];
		Scalar _mean = mean(Mat(contours[idx]));
		justarea[idx] = fabs(contourArea(Mat(c)));
		
		double dist_to_prev = 1.0; 
		if ((previous.x >- 1)) { 
			//consider distance from last blob
			double _n = norm(Point2i(_mean[0],_mean[1])-Point2i(previous.x,previous.y));
			if(_n > 100) _n *= 10; //if dist_to_prev > 100 then it's too fast movement... penalize
			dist_to_prev = _n;
			
			//consider colors shift by L2 distance on Hue-Sat plane
			_blob_mask.setTo(Scalar(0));
			Point* pts = &(c[0]);
			int _num = (c.size());
			fillPoly(_blob_mask, (const Point**)(&pts), &_num, 1, color);
			
			//calc mean hue-sat for this blob
			Scalar _h_s_mean,_stddv; meanStdDev(img, _h_s_mean, _stddv, _blob_mask);
			Vec2i hue_sat_blob(_h_s_mean[0], _h_s_mean[1]);
			dist_to_prev *= norm(hue_sat_blob - mean_hue_sat_blob);
		}
		
		weightedarea[idx] = justarea[idx] / dist_to_prev;
    }
	for (idx = 0; idx<contours.size(); idx++) {
		if( weightedarea[idx] > maxWArea )
        {
            maxWArea = weightedarea[idx];
            largestComp = idx;
        }
	}
	for (idx = 0; idx < contours.size(); idx++) {
		if ( justarea[idx] > maxJArea && idx != largestComp ) {
			maxJArea = justarea[idx];
			secondlargest = idx;
		}
	}
	
    
	//	cout << "largest cc " << largestComp << endl;
	//   drawContours( dst, contours, largestComp, color, CV_FILLED); //, 8, hierarchy );
	//	for (idx=0; idx<contours[largestComp].size()-1; idx++) {
	//		line(dst, contours[largestComp][idx], contours[largestComp][idx+1], color, 2);
	//	
	if(largestComp >= 0) {
		int num = contours[largestComp].size();
		
		/*find top-left values
		int maxx = -INT_MAX,miny = INT_MAX;
		
		for (int i=0; i<num; i++) {
			if(contours[largestComp][i].x > maxx) maxx = contours[largestComp][i].x;
			if(contours[largestComp][i].y < miny) miny = contours[largestComp][i].y;
		}
		
		/*crop contour to 150x150 "window"*
		vector<Point> newblob;
		int maxxp150 = MAX(maxx-200,0),minyp150 = MIN(miny+170,480);
		
		circle(outC, Point(maxx,miny), 2, Scalar(0,255,0), 1);
		circle(outC, Point(maxxp150,minyp150), 2, Scalar(0,255,0), 1);
		
		for (int i=0; i<num; i++) {
			Point _p = contours[largestComp][i];
			if(_p.x > maxxp150 && _p.y < minyp150) newblob.push_back(_p);
			else newblob.push_back(Point(MAX(_p.x,maxxp150),MIN(_p.y,minyp150)));
		}
		 /**/

		vector<Point>& newblob = contours[largestComp];
		
		Point* pts = &(newblob[0]);
		num = newblob.size();
		fillPoly(dst, (const Point**)(&pts), &num, 1, color);
		
		//calc mean hue-sat for this blob
		Scalar _h_s_mean,_stddv; meanStdDev(hsv, _h_s_mean, _stddv, dst);
		mean_hue_sat_blob = Vec2i(_h_s_mean[0],_h_s_mean[1]);
		
		
		Scalar _b = mean(Mat(newblob));
		b[0] = _b[0]; b[1] = _b[1]; b[2] = depthf.at<uchar>(b[0],b[1]);	//z value
		
		b[0] += 40; b[1] -= 40;
		b[3] = justarea[largestComp];
		
		contour.clear();
		contour = newblob;
		
		second_contour.clear();
		if(secondlargest >= 0) {
			second_contour = contours[secondlargest];
			b[4] = maxJArea;
		}
		
		previous.x = b[0]; previous.y = b[1]; previous.z = b[2];
		return b;
	} else
		return b;
	
}

int GestureEngine::TrainModel() {
	cout << "train model" << endl;
	if(loaded != true) {
		dataMat = Mat(training_data.size(),_d.size(),CV_32FC1);	//descriptors as matrix rows
		for (uint i=0; i<training_data.size(); i++) {
			Mat v = dataMat(Range(i,i+1),Range::all());
			Mat(Mat(training_data[i]).t()).convertTo(v,CV_32FC1,1.0);
		}
		Mat(label_data).convertTo(labelMat,CV_32FC1);
	}
	
	try {
		pca = pca(dataMat,Mat(),CV_PCA_DATA_AS_ROW,pca_number_of_features);
		Mat dataAfterPCA;
		pca.project(dataMat,dataAfterPCA);
		
		classifier.train(&((CvMat)dataAfterPCA), &((CvMat)labelMat));
		
		trained = true;
	} catch (cv::Exception e) {
		cerr << "Can't train model: " << e.what();
		return 0;
	}
	
	return 1;
}	

void GestureEngine::SaveModelData() {
	cout << "save training data" << endl;
	//			classifier.save("knn-classifier-open-fist-thumb.yaml"); //not implemented
	dataMat = Mat(training_data.size(),_d.size(),CV_32FC1);	//descriptors as matrix rows
	for (uint i=0; i<training_data.size(); i++) {
		Mat v = dataMat(Range(i,i+1),Range::all());
		Mat(Mat(training_data[i]).t()).convertTo(v,CV_32FC1,1.0);
	}
	Mat(label_data).convertTo(labelMat,CV_32FC1);
	
	FileStorage fs;
	fs.open("data-samples-labels.yaml", CV_STORAGE_WRITE);
	if (fs.isOpened()) {
		fs << "samples" << dataMat;
		fs << "labels" << labelMat;
		loaded = true;
		fs.release();
	} else {
		cerr << "can't open saved data" << endl;
	}
}	

int GestureEngine::LoadModelData(const char* filename) {
	FileStorage fs;
	fs.open(filename, CV_STORAGE_READ);
	if (fs.isOpened()) {
		fs["samples"] >> dataMat;
		fs["labels"] >> labelMat;
		fs["startX"] >> startX;
		fs["sizeX"] >> sizeX;
		fs["num_x_reps"] >> num_x_reps;
		fs["num_y_reps"] >> num_y_reps;
		height_over_num_y_reps = 480/num_y_reps;
		width_over_num_x_reps = sizeX/num_x_reps;
		_d = vector<double>(num_x_reps*num_y_reps);
		descriptorMat = Mat(_d);
		loaded = true;
		fs.release();			
	} else {
		cerr << "can't open saved data" << endl;
		return 0;
	}
	return 1;
}	

void GestureEngine::InterpolateAndInpaint() {
	//interpolation & inpainting
	Mat _tmp,_tmp1; // = (depthMat - 400.0);          //minimum observed value is ~440. so shift a bit
	Mat(depthMat - 400.0).convertTo(_tmp1,CV_64FC1);
//	_tmp1.setTo(Scalar(2048-400.0), depthMat > 750.0);   //cut off at 600 to create a "box" where the user interacts
	
	Point minLoc; double minval,maxval;
	minMaxLoc(_tmp1, &minval, &maxval, NULL, NULL);
	_tmp1.convertTo(depthf, CV_8UC1, 255.0/maxval);
	
	Mat small_depthf; resize(depthf,small_depthf,Size(),0.2,0.2);
	cv::inpaint(small_depthf,(small_depthf == 255),_tmp1,5.0,INPAINT_TELEA);
	
	resize(_tmp1, _tmp, depthf.size());
	_tmp.copyTo(depthf, (depthf == 255));
}
	
void GestureEngine::ComputeDescriptor(Scalar blb) {				
	Mat blobDepth,blobEdge; 
	depthf.copyTo(blobDepth,blobMaskOutput);
	Laplacian(blobDepth, blobEdge, 8);
	//				equalizeHist(blobEdge, blobEdge);//just for visualization
	
	Mat logPolar(depthf.size(),CV_8UC1);
	cvLogPolar(&((IplImage)blobEdge), &((IplImage)logPolar), Point2f(blb[0],blb[1]), 80.0);
	
	//				for (int i=0; i<num_x_reps+1; i++) {
	//					//verical lines
	//					line(logPolar, Point(startX+i*width_over_num_x_reps, 0), Point(startX+i*width_over_num_x_reps,479), Scalar(255), 2);
	//				}
	//				for(int i=0; i<num_y_reps+1; i++) {			
	//					//horizontal
	//					line(logPolar, Point(startX, i*height_over_num_y_reps), Point(startX+sizeX,i*height_over_num_y_reps), Scalar(255), 2);
	//				}
	
	double total = 0.0;
	
	//histogram
	for (int i=0; i<num_x_reps; i++) {
		for(int j=0; j<num_y_reps; j++) {
			Mat part = logPolar(
								Range(j*height_over_num_y_reps,(j+1)*height_over_num_y_reps),
								Range(startX+i*width_over_num_x_reps,startX+(i+1)*width_over_num_x_reps)
								);
			
//			int count = countNonZero(part); //TODO: use calcHist
//			//						part.setTo(Scalar(count/10.0)); //for debug: show the value in the image
//			
//			_d[i*num_x_reps + j] = count;
//			total += count;

			Scalar mn = mean(part);						
			_d[i*num_x_reps + j] = mn[0];
			
			
			total += mn[0];
		}
	}
	
	descriptorMat = descriptorMat / total;

	/*
	 Mat images[1] = {logPolar(Range(0,30),Range(0,30))};
	 int nimages = 1;
	 int channels[1] = {0};
	 int dims = 1;
	 float range_0[]={0,256};
	 float* ranges[] = { range_0 };
	 int histSize[1] = { 5 };
	 
	 calcHist(, <#int nimages#>, <#const int *channels#>, <#const Mat mask#>, <#MatND hist#>, <#int dims#>, <#const int *histSize#>, <#const float **ranges#>, <#bool uniform#>, <#bool accumulate#>)
	 */
	
	//				Mat _tmp(logPolar.size(),CV_8UC1);
	//				cvLogPolar(&((IplImage)logPolar), &((IplImage)_tmp),Point2f(blb[0],blb[1]), 80.0, CV_WARP_INVERSE_MAP);
	//				imshow("descriptor", _tmp);
	//				imshow("logpolar", logPolar);
	
}	

string GestureEngine::GetStringForGestureCode(int res) {
	if (res == LABEL_OPEN) {
		return "openhand";
	}
	if (res == LABEL_FIST) {
		return "theforce";
	}
	if (res == LABEL_THUMB) {
		return "Thumb";
	}
	if (res == LABEL_GARBAGE) {
		return "Garbage";
	}
	return "none";
}	

/*
 Blob registartion hysterisis: 
	when count goes above higher threshold -> Register, 
	when count goes below lower threshold -> Unregister.
 */
void GestureEngine::CheckRegistered(vector<int>& blb, int recognized_gesture, Scalar mn) {
//	if(recognized_gesture != LABEL_GARBAGE) {
		register_ctr = MIN((register_ctr + 1),60);
		
		if(blb[4] > 5000)
			register_secondbloc_ctr = MIN((register_secondbloc_ctr + 1),60);
		
		if (register_ctr > 30 && !registered) { //upper threshold of hysterisis
			registered = true;
			appear.x = -1;
			lastMove.x = blb[0]; lastMove.y = blb[1]; lastMove.z = blb[2];
			positionQueue.clear();
			
			cout << "blob size " << blb[4] << endl;
			
			if(register_secondbloc_ctr < 30) {
				cout << "register pointer" << endl;
//				stringstream ss; ss << "\"mode\":\""<< GetStringForGestureCode(recognized_gesture) <<"\"";
				send_event("Register", ""); //ss.str());
				
				mode = recognized_gesture;
			} else {
				cout << "register tab swithcer" << endl;
				send_event("Register", "\"mode\":\"twohands\"");
			}
		}
		
		positionQueue.push_back(Point3i(blb[0],blb[1],(int)(mn[0] * 2.0)));
		
		if(registered) {
			stringstream ss;
			ss  << "\"x\":"  << (int)floor(blb[0]*100.0/640.0)
				<< ",\"y\":" << (int)floor(blb[1]*100.0/480.0)
				<< ",\"z\":" << (int)(mn[0] * 2.0);
			//cout << "move: " << ss.str() << endl;
			send_event("Move", ss.str());
					
//			hc_stack.at(hc_stack_ptr) = hcr_ctr;
//			hc_stack_ptr = (hc_stack_ptr + 1) % hc_stack.size();

			if (positionQueue.size() > 20) {	//store last 20 positions in the queue
				if(positionQueue.front().z - (mn[0] * 2.0) > 20) {	//compare to oldest position in queue
					cout << "Push" << endl; appear.x = -1;
					send_event("Push", "");
					positionQueue.clear();
				} else
					positionQueue.pop_front();
			}
			
			//if thumb recognized - send "hand click"
//			if (mode == LABEL_FIST && recognized_gesture == LABEL_THUMB) {
//				bool fireClick  = false;
//				if (appearTS > 0) {
//					double timediff = ((double)getTickCount()-appearTS)/getTickFrequency();
//					fireClick = (timediff > 1.0);
//				} else {
//					fireClick = true;
//				}				
//				if(fireClick) {					
//					cout << "Hand click!" << endl;
//					send_event("HandClick", "");
//					
//					appearTS = getTickCount();
//				}					
//			} else {
//				appearTS = -1;
//			}
		}
//	} else {
		if(!registered) {
			//not registered, look for gestures
//			if(appear.x<0) {
//				//first appearence of blob
//				appear = midBlob;
//				//          update_bg_model = false;
//				appearTS = getTickCount();
//				cout << "appear ("<<appearTS<<") " << appear.x << "," << appear.y << "," << appear.z << endl;
//			} else {
				//blob was seen before, how much time passed
//				double timediff = ((double)getTickCount()-appearTS)/getTickFrequency();
//				if (timediff > .2 && timediff < 1.0) {
					//enough time passed from appearence

			for(uint i=0;i<positionQueue.size()-1;i++) {
				line(outC, Point(positionQueue[i].x,positionQueue[i].y), Point(positionQueue[i+1].x,positionQueue[i+1].y), Scalar(0,0,255), 3);	
			}
			
			if (positionQueue.size() > 15) {
				appear = positionQueue.front(); 
//				line(outC, Point(appear.x,appear.y), cv::Point(blb[0],blb[1]), Scalar(0,0,255), 3);
//				if (appear.x - blb[0] > 100) {
//					cout << "right"<<endl; appear.x = -1;
//					send_event("SwipeRight", "");
//					register_ctr = 0;
//					positionQueue.clear();
//				} else 
//				if (appear.x - blb[0] < -100) {
//					cout << "left" <<endl; appear.x = -1;
//					send_event("SwipeLeft", "");
//					register_ctr = 0;
//					positionQueue.clear();
//				} else 
//				if (appear.y - blb[1] > 100) {
//					cout << "up" << endl; appear.x = -1;
//					send_event("SwipeUp", "");
//					register_ctr = 0;
//					positionQueue.clear();
//				} else 
//				if (appear.y - blb[1] < -100) {
//					cout << "down" << endl; appear.x = -1;
//					send_event("SwipeDown", "");
//					register_ctr = 0;
//					positionQueue.clear();
//				}
				positionQueue.pop_front();
			}
							
//				}
//				if(timediff >= 1.0) {
//					cout << "a ghost..."<<endl;
//					//a second passed from appearence - reset 1st appear
//					appear.x = -1;
//					appearTS = -1;
//					midBlob.x = midBlob.y = midBlob.z = -1;
//				}
//			}
		}
		
//		register_ctr = MAX((register_ctr - 1),0);
//		register_secondbloc_ctr = MAX((register_secondbloc_ctr - 1),0);
		
		if (register_ctr <= 15 && registered) {	//lower threshold of hysterisis
			midBlob.x = midBlob.y = midBlob.z = -1;
			registered = false;
			mode = -1;
			cout << "unregister" << endl;
			send_event("Unregister", "");
			positionQueue.clear();
		}		
//	}
//	send_image(outC);
}

int GestureEngine::InitializeFreenect() {
	try {
		device = &freenect.createDevice(0);
		device->startVideo();
		device->startDepth();
		device->setTiltDegrees(10.0);
	}
	catch (std::runtime_error e) {
		return 0;
	}
  /*
	if(!LoadModelData(data)) return 0;
	if(!TrainModel()) return 0;
	*/
  
	return 1;
}

int GestureEngine::GetMostLikelyGesture() {
	Mat results(1,1,CV_32FC1);
	Mat samples; Mat(Mat(_d).t()).convertTo(samples,CV_32FC1);
	Mat samplesAfterPCA = pca.project(samples);
	
	classifier.find_nearest(&((CvMat)samplesAfterPCA), 1, &((CvMat)results));
	
	Mat lc(label_counts); lc *= 0.9;
	label_counts[(int)((float*)results.data)[0]] += 0.1;
	Point maxLoc;
	minMaxLoc(lc, NULL, NULL, NULL, &maxLoc);
	
	for (int i=0; i<4; i++) {
		rectangle(outC, Point(50+i*20,50), Point(50+(i+1)*20,50+50*label_counts[i]), Scalar(255), CV_FILLED);
	}	
	
	return maxLoc.y;
}	

void GestureEngine::BiasHandColor(Mat &blobMaskInput) 		//(very simple) bias with hand color
{
	Mat _col_p(hsv.size(),CV_32FC1);
	int jump = 5;
	for (int x=0; x < hsv.cols; x+=jump) {
		for (int y=0; y < hsv.rows; y+=jump) {
			Mat _i = hsv(Range(y,MIN(y+jump,hsv.rows-1)),Range(x,MIN(x+jump,hsv.cols-1)));
			Scalar hsv_mean = mean(_i);
			Vec2i u; u[0] = hsv_mean[0]; u[1] = hsv_mean[1];
			Vec2i v; v[0] = 120; v[1] = 110;
			rectangle(_col_p, Point(x,y), Point(x+jump,y+jump), Scalar(1.0-MIN(norm(u-v)/105.0,1.0)), CV_FILLED);
		}
	}
	//			hsv = hsv - Scalar(0,0,255);
	Mat _t = (Mat_<double>(2,3) << 1, 0, 15,    0, 1, -20);
	Mat col_p(_col_p.size(),CV_32FC1);
	warpAffine(_col_p, col_p, _t, col_p.size());
	GaussianBlur(col_p, col_p, Size(11.0,11.0), 2.5);
	imshow("hand color",col_p);
	imshow("rgb",rgbMat);
	
	Mat blobMaskInput_32FC1; blobMaskInput.convertTo(blobMaskInput_32FC1, CV_32FC1, 1.0/255.0);
	blobMaskInput_32FC1 = blobMaskInput_32FC1.mul(col_p, 1.0);
	blobMaskInput_32FC1.convertTo(blobMaskInput, CV_8UC1, 255.0);
	
	blobMaskInput = blobMaskInput > 128;
	
	imshow("blob bias", blobMaskInput);
}


void GestureEngine::RunEngine() {
	
	running = true;
	
    while (!die) {
    	device->getVideo(rgbMat);
    	device->getDepth(depthMat);
		cvtColor(rgbMat, hsv, CV_RGB2HSV);
		
		InterpolateAndInpaint();
		
		cvtColor(depthf, outC, CV_GRAY2BGR);
		
		Mat blobMaskInput = depthf < 30; //take closer values
		vector<Point> ctr,ctr2;

		//closest point to the camera
		Point minLoc; double minval,maxval;
		minMaxLoc(depthf, &minval, &maxval, &minLoc, NULL, blobMaskInput);
		circle(outC, minLoc, 5, Scalar(0,255,0), 3);
		
		blobMaskInput = depthf < (minval + 20);

		BiasHandColor(blobMaskInput);
		
		vector<int> blb = _refineSegments(depthf,blobMaskInput,blobMaskOutput,ctr,ctr2,midBlob); //find contours in the foreground, choose biggest
		/////// blb :
		//blb[0] = x, blb[1] = y, blb[2] = 1st blob size, blb[3] = 2nd blob size.
		if(blb[0]>=0 && blb[3] > 500) { //1st blob detected, and is big enough
			//cvtColor(depthf, outC, CV_GRAY2BGR);
			
			Scalar mn,stdv;
			meanStdDev(depthf,mn,stdv,blobMaskInput);
			blb[2] = mn[0]; //average depth of blob
			
			/*{	//trying a single gaussian skin-color model
				Mat samples = Mat::zeros(countNonZero(blobMaskInput),2,CV_32FC1);
				Mat_<float>& samplesM = (Mat_<float>&)samples;
				int count = 0;
				for (int x=0; x<blobMaskInput.cols; x++) {
					for (int y=0; y<blobMaskInput.rows; y++) {
						if(blobMaskInput.at<uchar>(y,x) > 0) {
							Vec3b HSVv = hsv.at<Vec3b>(y,x);
							//samples(Range(count,count+1),Range::all()) 
//							samples.row(count) += (Mat_<float>(1,2) << (float)HSVv[0] , (float)HSVv[1]);
							samplesM(count,0) = (float)HSVv[0];
							samplesM(count,1) = (float)HSVv[1];
							count++;
						}
					}
				}
				Scalar _mean(mean(samples.col(0))[0],mean(samples.col(1))[0]);
				samples = samples - _mean;
				Mat cov(2,2,CV_32FC1);
				for(int i=0;i<count;i++) {
					Mat sample = samples.row(i);
					Mat sTs = sample.t() * sample;
					addWeighted(sTs, 1.0/(double)count, cov, 1.0, 0.0, cov);
				}
				
				Mat_<float> X = (Mat_<float>(1,2) << 100,100); 
				Mat_<float> X_bar = (Mat_<float>(1,2) << _mean[0],_mean[1]);
				Mat_<float> X_m_X_bar = X - X_bar; 
				Mat inv_cov = cov.inv();
				double alpha = (1.0/(double)count) * (1.0/(2.0*CV_PI)) * 1.0/sqrt(determinant(cov));
//				Mat inexpM = (X_m_X_bar * inv_cov * X_m_X_bar.t());
//				double inexp = inexpM.at<float>(0,0);
//				double p = alpha * exp(-1.0/2.0 * inexp);
				
				vector<Mat> hsvv(3); split(hsv,hsvv);
				Mat imFlat(hsv.rows*hsv.cols,2,CV_32FC1); 
				hsvv[0].reshape(1,hsvv[0].rows*hsvv.cols).convertTo(imFlat.col(0),CV_32FC1);
				hsvv[1].reshape(1,hsvv[1].rows*hsvv.cols).convertTo(imFlat.col(1),CV_32FC1);
				
				cout << p << endl;
			}			*/	
			
			//cout << "min: " << minval << ", max: " << maxval << ", mean: " << mn[0] << endl;
			
			//now refining blob by looking at the mean depth value it has...
//			blobMaskInput = depthf < (mn[0] + stdv[0]*.5);
//			
//			blb = _refineSegments(Mat(),blobMaskInput,blobMaskOutput,ctr,ctr2,midBlob);
//			
////			imshow("blob", blobMaskOutput);
//			
//			if(blb[0] >= 0 && blb[2] > 300) {
				//draw contour
				Scalar color(0,0,255);
				for (int idx=0; idx<ctr.size()-1; idx++)
					line(outC, ctr[idx], ctr[idx+1], color, 1);
				line(outC, ctr[ctr.size()-1], ctr[0], color, 1);
				
				if(ctr2.size() > 0) {	//second blob detected
					Scalar color2(255,0,255);
					for (int idx=0; idx<ctr2.size()-1; idx++)
						line(outC, ctr2[idx], ctr2[idx+1], color2, 2);
					line(outC, ctr2[ctr2.size()-1], ctr2[0], color2, 2);
				}
								
				//blob center
				circle(outC, Point(blb[0],blb[1]), 50, Scalar(255,0,0), 3);

//				if(trained) {
//					ComputeDescriptor(blb);
//					int gesture_code = GetMostLikelyGesture();
//					
//					{ //debug
//						stringstream ss; ss << "prediction: " << GetStringForGestureCode(gesture_code);
//						putText(outC, ss.str(), Point(20,50), CV_FONT_HERSHEY_PLAIN, 3.0, Scalar(0,0,255), 2);
//					}
					
					CheckRegistered(blb, LABEL_GARBAGE, mn);
//				}
//			}
		} else {
			register_ctr = MAX((register_ctr - 1),0);
			register_secondbloc_ctr = MAX((register_secondbloc_ctr - 1),0);
			positionQueue.clear();
		}

		
//		stringstream ss; ss << "samples: " << dataMat.rows;
//		putText(outC, ss.str(), Point(30,outC.rows - 30), CV_FONT_HERSHEY_PLAIN, 2.0, Scalar(0,0,255), 1);

		imshow("blobs", outC);
		
		char k = cvWaitKey(5);
		if( k == 27 ){
			break;
		}
		/*
		if (k == 'g') {
			//put into training as 'garbage'
			training_data.push_back(_d);
			label_data.push_back(LABEL_GARBAGE);
			cout << "learn grabage" << endl;
		}
		if(k == 'o') {
				//put into training as 'open'
			training_data.push_back(_d);
			label_data.push_back(LABEL_OPEN);
			cout << "learn open" << endl;
		}
		if(k == 'f') {
			//put into training as 'fist'
			training_data.push_back(_d);
			label_data.push_back(LABEL_FIST);
			cout << "learn fist" << endl;
		}
		if(k == 'h') {
			//put into training as 'thumb'
			training_data.push_back(_d);
			label_data.push_back(LABEL_THUMB);
			cout << "learn thumb" << endl;
		}
		if (k=='t') {
			TrainModel();
		}
		if(k=='s') {
			SaveModelData();
		}
		if(k=='l') {
			LoadModelData();
		}
		 */
    }
	
   	device->stopVideo();
	device->stopDepth();
	
	running = false;
}

GestureEngine ge;

void* gesture_engine(void* _arg) {

	ge.RunEngine();
 	
}

void kill_gesture_engine() {
	ge.die = true;
}

bool is_gesture_engine_dead() { return !ge.getRunning(); }

int init_gesture_engine() { return ge.InitializeFreenect(); }
