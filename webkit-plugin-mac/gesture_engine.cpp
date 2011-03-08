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

class GestureEngine {
private:
	bool running;
	
	Mat depthMat;
	Mat depthf;
	Mat rgbMat;
	Mat ownMat;
	
	Freenect::Freenect<MyFreenectDevice> freenect;
	MyFreenectDevice* device;
	
	bool registered;
	Mat blobMaskOutput;
	Mat outC;
	Point midBlob;
	
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
	
	Point2i appear; double appearTS;
	
	Point2i lastMove;
	
	int hcr_ctr;
	vector<int> hc_stack; 
	int hc_stack_ptr;
	
	int pca_number_of_features;

	Scalar _refineSegments(const Mat& img, 
					Mat& mask, 
					Mat& dst, 
					vector<Point>& contour,
					vector<Point>& second_contour,
						   Point2i& previous);
	int TrainModel();
	void SaveModelData();
	int LoadModelData(const char* filename);
	void InterpolateAndInpaint();
	void ComputeDescriptor(Scalar);
	string GetStringForGestureCode(int);
	void CheckRegistered(Scalar,int);
	int GetMostLikelyGesture();
	
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
						die(false),
						mode(LABEL_GARBAGE),
						pca_number_of_features(15)
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
		
		appear = Point2i(-1,-1); 
		appearTS = -1;
		
		midBlob = Point2i(-1,-1);
		lastMove = Point2i(-1,-1);
		
		hcr_ctr = -1;
		hc_stack = vector<int>(20); 
		hc_stack_ptr = 0;
	};
	
	void RunEngine();
	bool getRunning() { return running; }
	int InitializeFreenect(const char* );
};

Scalar GestureEngine::_refineSegments(const Mat& img, 
					   Mat& mask, 
					   Mat& dst, 
					   vector<Point>& contour,
					   vector<Point>& second_contour,
					   Point2i& previous)
{
	//    int niters = 3;
    
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    
    Mat temp;
    
	blur(mask, temp, Size(11,11));
	temp = temp > 85.0;
	
    findContours( temp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
	
	if(dst.data==NULL)
		dst = Mat::zeros(img.size(), CV_8UC1);
	else
		dst.setTo(Scalar(0));
    
    if( contours.size() == 0 )
        return Scalar(-1,-1);
	
    // iterate through all the top-level contours,
    // draw each connected component with its own random color
    int idx = 0, largestComp = -1, secondlargest = -1;
    double maxWArea = 0, maxJArea = 0;
    vector<double> justarea(contours.size());
	vector<double> weightedarea(contours.size());
	
	//    for( ; idx >= 0; idx = hierarchy[idx][0] )
	for (; idx<contours.size(); idx++)
    {
        const vector<Point>& c = contours[idx];
		Scalar _mean = mean(Mat(contours[idx]));
		justarea[idx] = fabs(contourArea(Mat(c)));
		weightedarea[idx] = fabs(contourArea(Mat(c))) / 
		((previous.x >- 1) ? (1.0 + norm(Point(_mean[0],_mean[1])-previous)) : 1.0);	//consider distance from last blob
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
	
    Scalar color( 255 );
	//	cout << "largest cc " << largestComp << endl;
	//   drawContours( dst, contours, largestComp, color, CV_FILLED); //, 8, hierarchy );
	//	for (idx=0; idx<contours[largestComp].size()-1; idx++) {
	//		line(dst, contours[largestComp][idx], contours[largestComp][idx+1], color, 2);
	//	
	if(largestComp >= 0) {
		int num = contours[largestComp].size();
		Point* pts = &(contours[largestComp][0]);
		fillPoly(dst, (const Point**)(&pts), &num, 1, color);
		
		Scalar b = mean(Mat(contours[largestComp]));
		b[2] = justarea[largestComp];
		
		contour.clear();
		contour = contours[largestComp];
		
		second_contour.clear();
		if(secondlargest >= 0) {
			second_contour = contours[secondlargest];
			b[3] = maxJArea;
		}
		
		previous.x = b[0]; previous.y = b[1];
		return b;
	} else
		return Scalar(-1,-1);
	
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
			
			int count = countNonZero(part); //TODO: use calcHist
			//						part.setTo(Scalar(count/10.0)); //for debug: show the value in the image
			
			_d[i*num_x_reps + j] = count;
			total += count;
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
		return "Open hand";
	}
	if (res == LABEL_FIST) {
		return "Fist";
	}
	if (res == LABEL_THUMB) {
		return "Thumb";
	}
	if (res == LABEL_GARBAGE) {
		return "Garbage";
	}
	return "none";
}	

void GestureEngine::CheckRegistered(Scalar blb, int recognized_gesture) {
	register_ctr = MIN((register_ctr + 1),60);
	
	if(blb[3] > 5000)
		register_secondbloc_ctr = MIN((register_secondbloc_ctr + 1),60);
	
	if (register_ctr > 30 && !registered) {
		registered = true;
		appear.x = -1;
		lastMove.x = blb[0]; lastMove.y = blb[1];
		
		cout << "blob size " << blb[2] << endl;
		
		if(register_secondbloc_ctr < 30) {
			cout << "register pointer" << endl;
			stringstream ss; ss << "\"mode\":\""<< GetStringForGestureCode(recognized_gesture) <<"\"";
			send_event("Register", ss.str());
		} else {
            cout << "register tab swithcer" << endl;
            send_event("Register", "\"mode\":\"twohands\"");
		}
	}
	
	if(registered) {
		stringstream ss;
		ss  << "\"x\":"  << (int)floor(blb[0]*100.0/640.0)
			<< ",\"y\":" << (int)floor(blb[1]*100.0/480.0)
			<< ",\"z\":" << 100; //(int)(mn[0] * 2.0);
		//cout << "move: " << ss.str() << endl;
		send_event("Move", ss.str());
				
		hc_stack.at(hc_stack_ptr) = hcr_ctr;
		hc_stack_ptr = (hc_stack_ptr + 1) % hc_stack.size();
		
		//if thumb recognized - send "hand click"
		if (recognized_gesture == LABEL_THUMB) {
			cout << "Hand click!" << endl;
            send_event("HandClick", "");
		}
	} else {
		//not registered, look for gestures
		if(appear.x<0) {
            //first appearence of blob
            appear = midBlob;
            //          update_bg_model = false;
            appearTS = getTickCount();
            cout << "appear ("<<appearTS<<") " << appear.x << "," << appear.y << endl;
		} else {
            //blob was seen before, how much time passed
            double timediff = ((double)getTickCount()-appearTS)/getTickFrequency();
            if (timediff > .2 && timediff < 1.0) {
				//enough time passed from appearence
				line(outC, appear, cv::Point(blb[0],blb[1]), Scalar(0,0,255), 3);
				if (appear.x - blb[0] > 100) {
					cout << "right"<<endl; appear.x = -1;
					send_event("SwipeRight", "");
					register_ctr = 0;
				} else if (appear.x - blb[0] < -100) {
					cout << "left" <<endl; appear.x = -1;
					send_event("SwipeLeft", "");
					register_ctr = 0;
				} else if (appear.y - blb[1] > 100) {
					cout << "up" << endl; appear.x = -1;
					send_event("SwipeUp", "");
					register_ctr = 0;
				} else if (appear.y - blb[1] < -100) {
					cout << "down" << endl; appear.x = -1;
					send_event("SwipeDown", "");
					register_ctr = 0;
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
//	send_image(outC);
}

int GestureEngine::InitializeFreenect(const char* data) {
	try {
		device = &freenect.createDevice(0);
		device->startVideo();
		device->startDepth();		
	}
	catch (std::runtime_error e) {
		return 0;
	}
	if(!LoadModelData(data)) return 0;
	if(!TrainModel()) return 0;
	
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
	return maxLoc.y;
}	

void GestureEngine::RunEngine() {
	
	running = true;
	
    while (!die) {
    	device->getVideo(rgbMat);
    	device->getDepth(depthMat);

		InterpolateAndInpaint();
		
		cvtColor(depthf, outC, CV_GRAY2BGR);
		
		Mat blobMaskInput = depthf < 120; //take closer values
		vector<Point> ctr,ctr2;

		//closest point to the camera
		Point minLoc; double minval,maxval;
		minMaxLoc(depthf, &minval, &maxval, &minLoc, NULL, blobMaskInput);
		circle(outC, minLoc, 5, Scalar(0,255,0), 3);
		
		blobMaskInput = depthf < (minval + 18);
		
		Scalar blb = _refineSegments(Mat(),blobMaskInput,blobMaskOutput,ctr,ctr2,midBlob); //find contours in the foreground, choose biggest
		/////// blb :
		//blb[0] = x, blb[1] = y, blb[2] = 1st blob size, blb[3] = 2nd blob size.
		if(blb[0]>=0 && blb[2] > 500) { //1st blob detected, and is big enough
			//cvtColor(depthf, outC, CV_GRAY2BGR);
			
			Scalar mn,stdv;
			meanStdDev(depthf,mn,stdv,blobMaskInput);
			
			//cout << "min: " << minval << ", max: " << maxval << ", mean: " << mn[0] << endl;
			
			//now refining blob by looking at the mean depth value it has...
			blobMaskInput = depthf < (mn[0] + stdv[0]*.5);
			
			blb = _refineSegments(Mat(),blobMaskInput,blobMaskOutput,ctr,ctr2,midBlob);
			
			imshow("blob", blobMaskOutput);
			
			if(blb[0] >= 0 && blb[2] > 300) {
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

				if(trained) {
					ComputeDescriptor(blb);
					int gesture_code = GetMostLikelyGesture();
					
					{ //debug
						stringstream ss; ss << "prediction: " << GetStringForGestureCode(gesture_code);
						putText(outC, ss.str(), Point(20,50), CV_FONT_HERSHEY_PLAIN, 3.0, Scalar(0,0,255), 2);
					}
					
					CheckRegistered(blb, gesture_code);
				}
			}
		}
				
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

int init_gesture_engine(const char* data) { return ge.InitializeFreenect(data); }