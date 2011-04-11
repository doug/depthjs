#include "FreenectDevice.h"

Scalar refineSegments(const Mat& img, 
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
		//find top-left values
		int maxx = -INT_MAX,miny = INT_MAX;
		int num = contours[largestComp].size();
		for (int i=0; i<num; i++) {
			if(contours[largestComp][i].x > maxx) maxx = contours[largestComp][i].x;
			if(contours[largestComp][i].y < miny) miny = contours[largestComp][i].y;
		}
		
		//crop contour to 150x150 "window"
		vector<Point> newblob = contours[largestComp];
//		int maxxp150 = MAX(maxx-200,0),minyp150 = MIN(miny+170,480);
//				
//		for (int i=0; i<num; i++) {
//			Point _p = contours[largestComp][i];
//			if(_p.x > maxxp150 && _p.y < minyp150) newblob.push_back(_p);
//				else newblob.push_back(Point(MAX(_p.x,maxxp150),MIN(_p.y,minyp150)));
//		}
		
		Point* pts = &(newblob[0]);
		num = newblob.size();
		fillPoly(dst, (const Point**)(&pts), &num, 1, color);
		
		Scalar b = mean(Mat(newblob)); //center of the blob
		b[0] += 40; b[1] -= 40;
		b[2] = justarea[largestComp];
		
		contour.clear();
		contour = newblob;
		
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

#define LABEL_GARBAGE	0
#define LABEL_OPEN		1
#define LABEL_FIST		2
#define LABEL_THUMB		3


int main(int argc, char **argv) {
	bool die(false);
	string filename("snapshot");
	string suffix(".png");
	int i_snap(0),iter(0);
	
	Mat depthMat(Size(640,480),CV_16UC1);
	Mat depthf  (Size(640,480),CV_8UC1);
	Mat rgbMat(Size(640,480),CV_8UC3,Scalar(0));
	Mat ownMat(Size(640,480),CV_8UC3,Scalar(0));
	
	Freenect::Freenect<MyFreenectDevice> freenect;
	MyFreenectDevice& device = freenect.createDevice(0);
	
	device.setTiltDegrees(10.0);
	
	bool registered  = false;
	Mat blobMaskOutput = Mat::zeros(Size(640,480),CV_8UC1),outC;
	Point midBlob;
	
	int startX = 200, sizeX = 180, num_x_reps = 18, num_y_reps = 48;
	double	height_over_num_y_reps = 480/num_y_reps,
			width_over_num_x_reps = sizeX/num_x_reps;
	
	
	vector<double> _d(num_x_reps * num_y_reps); //the descriptor
	Mat descriptorMat(_d);

//	CvNormalBayesClassifier classifier;	//doesnt work
	CvKNearest classifier;
//	CvSVM classifier;	//doesnt work
//	CvBoost classifier;	//only good for 2 classes
//	CvDTree classifier;
	
	
	vector<vector<double> > training_data;
	vector<int>				label_data;
	PCA pca;
	Mat labelMat, dataMat; 
	vector<float> label_counts(4);
	
	bool trained = false, loaded = false;
 	
	device.startVideo();
	device.startDepth();
    while (!die) {
    	device.getVideo(rgbMat);
    	device.getDepth(depthMat);
//        cv::imshow("rgb", rgbMat);
    	depthMat.convertTo(depthf, CV_8UC1, 255.0/2048.0);
				
        cv::imshow("depth",depthf);
		
		//interpolation & inpainting
		{
			Mat _tmp,_tmp1; // = (depthMat - 400.0);          //minimum observed value is ~440. so shift a bit
			Mat(depthMat - 400.0).convertTo(_tmp1,CV_64FC1);
			_tmp.setTo(Scalar(2048), depthMat > 750.0);   //cut off at 600 to create a "box" where the user interacts
//			_tmp.convertTo(depthf, CV_8UC1, 255.0/1648.0);  //values are 0-2048 (11bit), account for -400 = 1648

			//quadratic interpolation
//			cv::pow(_tmp,2.0,_tmp1);
//			_tmp1 = _tmp1 * 4.0;
			
//			try {
//				cv:log(_tmp,_tmp1);
//			}
//			catch (cv::Exception e) {
//				cerr << e.what() << endl;
//				exit(0);
//			}
			
			Point minLoc; double minval,maxval;
			minMaxLoc(_tmp1, &minval, &maxval, NULL, NULL);
			_tmp1.convertTo(depthf, CV_8UC1, 255.0/maxval);
			
			Mat small_depthf; resize(depthf,small_depthf,Size(),0.2,0.2);
			cv::inpaint(small_depthf,(small_depthf == 255),_tmp1,5.0,INPAINT_TELEA);
			
			resize(_tmp1, _tmp, depthf.size());
			_tmp.copyTo(depthf, (depthf == 255));
		}

		{
//			Mat smallDepth = depthf; //cv::resize(depthf,smallDepth,Size(),0.5,0.5);
//			Mat edges; //Laplacian(smallDepth, edges, -1, 7, 1.0);
//			Sobel(smallDepth, edges, -1, 1, 1, 7);
			//medianBlur(edges, edges, 11);
//			for (int x=0; x < edges.cols; x+=20) {
//				for (int y=0; y < edges.rows; y+=20) {
//					//int nz = countNonZero(edges(Range(y,MIN(y+20,edges.rows-1)),Range(x,MIN(x+20,edges.cols-1))));
//					Mat _i = edges(Range(y,MIN(y+20,edges.rows-1)),Range(x,MIN(x+20,edges.cols-1)));
//					medianBlur(_i, _i, 7);
//					//rectangle(edges, Point(x,y), Point(x+20,y+20), Scalar(nz), CV_FILLED);
//				}
//			}
		
//			imshow("edges", edges);
		}
				
		cvtColor(depthf, outC, CV_GRAY2BGR);
		
		Mat blobMaskInput = depthf < 120; //anything not white is "real" depth, TODO: inpainting invalid data
		vector<Point> ctr,ctr2;

		//closest point to the camera
		Point minLoc; double minval,maxval;
		minMaxLoc(depthf, &minval, &maxval, &minLoc, NULL, blobMaskInput);
		circle(outC, minLoc, 5, Scalar(0,255,0), 3);
		
		blobMaskInput = depthf < (minval + 18);
		
		Scalar blb = refineSegments(Mat(),blobMaskInput,blobMaskOutput,ctr,ctr2,midBlob); //find contours in the foreground, choose biggest
//		if (blobMaskOutput.data != NULL) {
//			imshow("first", blobMaskOutput);
//		}
		/////// blb :
		//blb[0] = x, blb[1] = y, blb[2] = 1st blob size, blb[3] = 2nd blob size.
		

		
		if(blb[0]>=0 && blb[2] > 500) { //1st blob detected, and is big enough
			//cvtColor(depthf, outC, CV_GRAY2BGR);
			
			Scalar mn,stdv;
			meanStdDev(depthf,mn,stdv,blobMaskInput);
			
			//cout << "min: " << minval << ", max: " << maxval << ", mean: " << mn[0] << endl;
			
			//now refining blob by looking at the mean depth value it has...
			blobMaskInput = depthf < (mn[0] + stdv[0]);
			
			//(very simple) bias with hand color
			{
				Mat hsv; cvtColor(rgbMat, hsv, CV_RGB2HSV);
				Mat _col_p(hsv.size(),CV_32FC1);
				int jump = 5;
				for (int x=0; x < hsv.cols; x+=jump) {
					for (int y=0; y < hsv.rows; y+=jump) {
						Mat _i = hsv(Range(y,MIN(y+jump,hsv.rows-1)),Range(x,MIN(x+jump,hsv.cols-1)));
						Scalar hsv_mean = mean(_i);
						Vec2i u; u[0] = hsv_mean[0]; u[1] = hsv_mean[1];
						Vec2i v; v[0] = 120; v[1] = 110;
						rectangle(_col_p, Point(x,y), Point(x+jump,y+jump), Scalar(1.0-MIN(norm(u-v)/125.0,1.0)), CV_FILLED);
					}
				}
				//			hsv = hsv - Scalar(0,0,255);
				Mat _t = (Mat_<double>(2,3) << 1, 0, 15,    0, 1, -20);
				Mat col_p(_col_p.size(),CV_32FC1);
				warpAffine(_col_p, col_p, _t, col_p.size());
				GaussianBlur(col_p, col_p, Size(11.0,11.0), 2.5);
				imshow("hand color",col_p);
				//			imshow("rgb",rgbMat);
				Mat blobMaskInput_32FC1; blobMaskInput.convertTo(blobMaskInput_32FC1, CV_32FC1, 1.0/255.0);
				blobMaskInput_32FC1 = blobMaskInput_32FC1.mul(col_p, 1.0);
				blobMaskInput_32FC1.convertTo(blobMaskInput, CV_8UC1, 255.0);
				
				blobMaskInput = blobMaskInput > 128;
				
				imshow("blob bias", blobMaskInput);
			}
			
			
			blb = refineSegments(Mat(),blobMaskInput,blobMaskOutput,ctr,ctr2,midBlob);
			
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
				
				{
					Mat hsv; cvtColor(rgbMat, hsv, CV_RGB2HSV);
					Scalar hsv_mean,hsv_stddev; meanStdDev(hsv, hsv_mean, hsv_stddev, blobMaskOutput);
					stringstream ss; ss << hsv_mean[0] << "," << hsv_mean[1] << "," << hsv_mean[2];
					putText(outC, ss.str(), Point(blb[0],blb[1]), CV_FONT_HERSHEY_PLAIN, 1.0, Scalar(0,255,255));
				}
				
				
				Mat blobDepth,blobEdge; 
				depthf.copyTo(blobDepth,blobMaskOutput);
				Laplacian(blobDepth, blobEdge, 8);
//				equalizeHist(blobEdge, blobEdge);//just for visualization
				
				Mat logPolar(depthf.size(),CV_8UC1);
				cvLogPolar(&((IplImage)blobEdge), &((IplImage)logPolar), Point2f(blb[0],blb[1]), 80.0);
				
//				for (int i=0; i<num_x_reps+1; i++) {
//					//verical lines
//					line(logPolar, Point(startX+i*width_over_num_x_reps, 0), Point(startX+i*width_over_num_x_reps,479), Scalar(255), 1);
//				}
//				for(int i=0; i<num_y_reps+1; i++) {			
//					//horizontal
//					line(logPolar, Point(startX, i*height_over_num_y_reps), Point(startX+sizeX,i*height_over_num_y_reps), Scalar(255), 1);
//				}
				
				double total = 0.0;
				
				//histogram
				for (int i=0; i<num_x_reps; i++) {
					for(int j=0; j<num_y_reps; j++) {
						Mat part = logPolar(
										Range(j*height_over_num_y_reps,(j+1)*height_over_num_y_reps),
										 Range(startX+i*width_over_num_x_reps,startX+(i+1)*width_over_num_x_reps)
										 );
						
//						int count = countNonZero(part); //TODO: use calcHist
//						_d[i*num_x_reps + j] = count;

						Scalar mn = mean(part);						
//						part.setTo(Scalar(mn[0])); //for debug: show the value in the image
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
		}
		
		if(trained) {
			Mat results(1,1,CV_32FC1);
			Mat samples; Mat(Mat(_d).t()).convertTo(samples,CV_32FC1);
			
			Mat samplesAfterPCA = samples; //pca.project(samples);
			
			classifier.find_nearest(&((CvMat)samplesAfterPCA), 1, &((CvMat)results));
//			((float*)results.data)[0] = classifier.predict(&((CvMat)samples))->value;
			
			Mat lc(label_counts); lc *= 0.9;
			
//			label_counts[(int)((float*)results.data)[0]] *= 0.9;
			label_counts[(int)((float*)results.data)[0]] += 0.1;
			Point maxLoc;
			minMaxLoc(lc, NULL, NULL, NULL, &maxLoc);
			int res = maxLoc.y;
			
			stringstream ss; ss << "prediction: ";
			if (res == LABEL_OPEN) {
				ss << "Open hand";
			}
			if (res == LABEL_FIST) {
				ss << "Fist";
			}
			if (res == LABEL_THUMB) {
				ss << "Thumb";
			}
			if (res == LABEL_GARBAGE) {
				ss << "Garbage";
			}
			putText(outC, ss.str(), Point(20,50), CV_FONT_HERSHEY_PLAIN, 3.0, Scalar(0,0,255), 2);
		}
		
		stringstream ss; ss << "samples: " << training_data.size();
		putText(outC, ss.str(), Point(30,outC.rows - 30), CV_FONT_HERSHEY_PLAIN, 2.0, Scalar(0,0,255), 1);
		
		imshow("blobs", outC);
		
		char k = cvWaitKey(5);
		if( k == 27 ){
			break;
		}
		if( k == 8 ) {
			std::ostringstream file;
			file << filename << i_snap << suffix;
			cv::imwrite(file.str(),rgbMat);
			i_snap++;
		}
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
			//train model
			cout << "train model" << endl;
			if(loaded != true) {
				dataMat = Mat(training_data.size(),_d.size(),CV_32FC1);	//descriptors as matrix rows
				for (uint i=0; i<training_data.size(); i++) {
					Mat v = dataMat(Range(i,i+1),Range::all());
					Mat(Mat(training_data[i]).t()).convertTo(v,CV_32FC1,1.0);
				}
				Mat(label_data).convertTo(labelMat,CV_32FC1);
			}
			
//			pca = pca(dataMat,Mat(),CV_PCA_DATA_AS_ROW,15);
			Mat dataAfterPCA = dataMat;
//			pca.project(dataMat,dataAfterPCA);
			
			classifier.train(&((CvMat)dataAfterPCA), &((CvMat)labelMat));
			
			trained = true;
		}
//		if(k=='p' && trained) {
//			//predict
//			Mat results(1,1,CV_32FC1);
//			Mat samples(1,64,CV_32FC1); Mat(Mat(_d).t()).convertTo(samples,CV_32FC1);
//			classifier.find_nearest(&((CvMat)samples), 1, &((CvMat)results));
//			cout << "prediction: " << ((float*)results.data)[0] << endl;
//		}
		if(k=='s') {
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

				fs << "startX" << startX;
				fs << "sizeX" << sizeX;
				fs << "num_x_reps" << num_x_reps;
				fs << "num_y_reps" << num_y_reps;
				
				loaded = true;
				fs.release();
			} else {
				cerr << "can't open saved data" << endl;
			}
		}
		if(k=='l') {
			cout << "try to load training data" << endl;
			FileStorage fs;
			fs.open("data-samples-labels.yaml", CV_STORAGE_READ);
			if (fs.isOpened()) {
				fs["samples"] >> dataMat;
				fs["labels"] >> labelMat;
				fs["startX"] >> startX;
				fs["sizeX"] >> sizeX;
				fs["num_x_reps"] >> num_x_reps;
				fs["num_y_reps"] >> num_y_reps;
				height_over_num_y_reps = 480/num_y_reps;
				width_over_num_x_reps = sizeX/num_x_reps;
				
				loaded = true;
				fs.release();			
			} else {
				cerr << "can't open saved data" << endl;
			}
		}
    }
	
   	device.stopVideo();
	device.stopDepth();
	return 0;
}
