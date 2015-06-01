#include "figure.hpp"


int gFigureNum = 1;

const double cl::MvFigure::window_height = 640.0;
const double cl::MvFigure::window_width = 640.0;

volatile bool gLeftClicked = false;
volatile bool gRightClicked = false;
CvPoint gClickedPt;


void click(int event, int x, int y, int flags, void* param)
{	
	if ( event == CV_EVENT_FLAG_LBUTTON )
	{
		std::cout<<"pt: "<<x<<","<<y<<std::endl;		
		gClickedPt = cvPoint(x, y);
		gLeftClicked = true;
	}
	if( event == CV_EVENT_FLAG_RBUTTON )
	{
		gRightClicked = true;
	}
}

namespace cl{


	MvFigure::MvFigure()
	{	
		frame_ = NULL;
		clear();		
		holdon_ = false;
		plotaxis_ = true;
		isimg_ = false;
		background_color_ = cvScalar(255,255,255);
		id_ = gFigureNum;
		gFigureNum++;


		//create window
		//winname_ = "";
		winname_ << id_;
		cvNamedWindow(winname_.str().c_str());

	}

	MvFigure::~MvFigure()
	{
		cvDestroyWindow(winname_.str().c_str());
		if (frame_ != NULL)
			cvReleaseImage(&frame_);
	}

	void MvFigure::set_axis(double x0, double x1, double y0, double y1)
	{
		x0_ = x0;
		x1_ = x1;
		y0_ = y0;
		y1_ = y1;
		x_range_ = x1_-x0_;
		y_range_ = y1_-y0_;

		double height_to_width_ratio = y_range_/x_range_;	
		if (frame_ != NULL)
			cvReleaseImage(&frame_);
		frame_ = cvCreateImage(cvSize(cvRound(window_width), cvRound(window_width*height_to_width_ratio)),8,3);
		cvSet(frame_, background_color_);
	}


	/**
	* 
	* 
	* @note the matrix consists of [x y; x y ...] of points clicked
	* 
	*/	
	ublas::matrix<double> MvFigure::ginput(int num_pts)
	{	
		std::cerr << "Click " << num_pts << " Points!" << std::endl;
		ublas::matrix<double> pts(num_pts, 2);

		cvSetMouseCallback(winname_.str().c_str(), click);

		//for drawing
		ublas::vector<double> x(1);
		ublas::vector<double> y(1);

		for (int i = 0; i < num_pts; ++i)
		{
			while(gLeftClicked == false)
			{
				cvWaitKey(5);
			}
			gLeftClicked = false;

			CvPoint2D32f coord_pt;
			convert_pt(gClickedPt, coord_pt);
			row(pts,i)[0] = coord_pt.x; 
			row(pts,i)[1] = coord_pt.y;

			std::cout << row(pts, i) << std::endl;

			//draw the point to screen

			x[0] = coord_pt.x;
			y[0] = coord_pt.y;
			scatter_plot(x,y, CV_RGB(0,0,0) );	

		}

		return pts;		
	}
}