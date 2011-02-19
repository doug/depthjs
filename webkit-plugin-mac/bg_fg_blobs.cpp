/*
 *  bg_fg_blobs.cpp
 *  OpenCVTries1
 *
 *  Created by Roy Shilkrot on 11/21/10.
 *  Copyright 2010 MIT. All rights reserved.
 *
 */

#include "bg_fg_blobs.hpp"

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

//    dilate(mask, temp, Mat(), Point(-1,-1), niters);
//    erode(temp, temp, Mat(), Point(-1,-1), niters*2);
//    dilate(temp, temp, Mat(), Point(-1,-1), niters);
    blur(mask, temp, Size(11,11));
//  imshow("temp",temp);
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
                        ((previous.x >- 1) ? (1.0 + norm(Point(_mean[0],_mean[1])-previous)) : 1.0);    //consider distance from last blob
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
//  cout << "largest cc " << largestComp << endl;
 //   drawContours( dst, contours, largestComp, color, CV_FILLED); //, 8, hierarchy );
//  for (idx=0; idx<contours[largestComp].size()-1; idx++) {
//      line(dst, contours[largestComp][idx], contours[largestComp][idx+1], color, 2);
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
/*
 void makePointsFromMask(Mat& maskm,vector<Point2f>& points, bool _add = false) {//, Mat& out) {
    if(!_add)
        points.clear();
    for (int y=0; y<maskm.rows; y+=10) {
        uchar* ptr = maskm.ptr<uchar>(y);
        for (int x=0; x<maskm.cols; x+=10) {
            if(ptr[x]>10) {
                points.push_back(Point2f(x,y));
//              if(out.data!=NULL)
//                  circle(out, Point(x,y), 2, Scalar(0,0,255), 2);
            }
        }
    }
}

void drawPoint(Mat& out,vector<Point2f>& points,Scalar color, Mat* maskm = NULL){
    for (int i=0; i<points.size(); i++) {
        if(maskm!=NULL)
            if(((uchar*)maskm->data)[i] > 0)
                circle(out,points[i],1,color,1);
        else
            circle(out,points[i],1,color,1);
    }
}


//this is a sample for foreground detection functions
int bgfg_main(int argc, char** argv)
{
    IplImage*       tmp_frame = NULL;
    CvCapture*      cap = NULL;
    bool update_bg_model = true;

    if( argc < 2 )
        cap = cvCaptureFromCAM(0);
    else
        cap = cvCaptureFromFile(argv[1]);

    if( !cap )
    {
        printf("can not open camera or video file\n");
        return -1;
    }

    tmp_frame = cvQueryFrame(cap);
    if(!tmp_frame)
    {
        printf("can not read data from the video source\n");
        return -1;
    }

    cvNamedWindow("BG", 1);
    cvNamedWindow("FG", 1);

    CvBGStatModel* bg_model = 0;
    Mat frameMat(tmp_frame);
    Mat out(frameMat.size(),CV_8UC1),
        outC(frameMat.size(),CV_8UC3);
    Mat prevImg(frameMat.size(),CV_8UC1),
        nextImg(frameMat.size(),CV_8UC1);
    vector<Point2f> prevPts,nextPts;
    vector<uchar> statusv;
    vector<float> errv;
    Rect cursor(frameMat.cols/2,frameMat.rows/2,10,10);
    int nmfr = 0; //non-motion frames counter

    for( int fr = 1;tmp_frame; tmp_frame = cvQueryFrame(cap), fr++ )
    {
        if(!bg_model)
        {
            //create BG model
            bg_model = cvCreateGaussianBGModel( tmp_frame );
            //bg_model = cvCreateFGDStatModel( tmp_frame );
            continue;
        }

        double t = (double)cvGetTickCount();
        cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );
        t = (double)cvGetTickCount() - t;
//        printf( "%d. %.1f\n", fr, t/(cvGetTickFrequency()*1000.) );
//        cvShowImage("BG", bg_model->background);
//        cvShowImage("FG", bg_model->foreground);

        Mat tmp_bg_fg(bg_model->foreground);

        vector<Point> c();
        refineSegments(frameMat,tmp_bg_fg,out,c);

        if (fr%5 == 0) {
            makePointsFromMask(out, prevPts,(fr%25 != 0));
        }

        cvtColor(frameMat, nextImg, CV_BGR2GRAY);
//      imshow("prev", prevImg);
//      imshow("next", nextImg);

        calcOpticalFlowPyrLK(prevImg, nextImg, prevPts, nextPts, statusv, errv);
        nextImg.copyTo(prevImg);


        Mat ptsM(prevPts),nptsM(nextPts);
        Mat statusM(statusv);
        Scalar means = mean(ptsM-nptsM,statusM);


        cout << "average motion of largest blob: " << means[0] << "," << means[1] << endl;

        {
            Mat _tmp; frameMat.copyTo(_tmp); //,out);
            Point mid = Point(_tmp.cols/2, _tmp.rows/2);
            line(_tmp, mid, mid+Point(means[0],0), Scalar(255,0,0), 5);
            line(_tmp, mid, mid+Point(0,means[1]), Scalar(0,255,0), 5);
//          drawPoint(_tmp,prevPts,Scalar(0,0,255)); //,Mat::ones(1, statusv.size(), CV_8UC1));
//          drawPoint(_tmp,nextPts,Scalar(255,0,0),&statusM);
            if(fabs(means[0])>2 && fabs(means[0]) < 60) {
                cursor.x -= means[0];
//              stringstream ss; ss << "Move right-left";
//              putText(_tmp, ss.str(), Point(30,30), CV_FONT_HERSHEY_PLAIN, 2.0, Scalar(255,0,255), 2);
            } else if(fabs(means[1])>2 && fabs(means[1]) < 60) {
                cursor.y -= means[1];
//              stringstream ss; ss << "Move up-down";
//              putText(_tmp, ss.str(), Point(50,50), CV_FONT_HERSHEY_PLAIN, 2.0, Scalar(255,255,0), 2);
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
            nmfr = 0;
        }

        char k = cvWaitKey(5);
        if( k == 27 ) break;
        if( k == ' ' )
            update_bg_model = !update_bg_model;
    }


    cvReleaseBGStatModel( &bg_model );
    cvReleaseCapture(&cap);

    return 0;
}
*/
