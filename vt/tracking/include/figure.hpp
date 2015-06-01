
#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <utility>
#include <string>

#include <cxcore.h>
#include <cv.h>
#include <highgui.h>

#include <boost/iostreams/device/file.hpp>
#include <boost/gil/gil_all.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>


namespace ublas = boost::numeric::ublas;

void click(int event, int x, int y, int flags, void* param);

namespace cl{

class MvFigure
{
	//static const double window_width = 640.0; //fix the window to be 640
	//static const double window_height = 640.0; //fix the window to be 640
	static const double window_width;// = 640.0;
	static const double window_height;// = 640.0;
public:
	
	MvFigure();
		
	~MvFigure();
	
	
	void clear()
	{
		set_axis(-10, 10, -10, 10);
	}
	
	void set_holdon( bool b)
	{
		holdon_ = b;	
	}
	
	void set_axis(double x0, double x1, double y0, double y1);
	
	/// This function clears the image, draws the axis, and scatter_plot the points
	void plot_pts(	ublas::vector<double>& x,
					ublas::vector<double>& y,					
					CvScalar color)
	{
		isimg_ = false;
		
		if (!holdon_)
			cvSet(frame_, background_color_);
				
		if (plotaxis_)
			plot_axis();
		
		scatter_plot(x, y, color);
		
		
	}
	
	
	void plot_img( ublas::vector<double> & im)
	{
		isimg_ = true;
	}
	
	void plot_img( IplImage* img)
	{
		isimg_ = true;
		set_axis( 0, img->width, -img->height, 0);
		IplImage* tmp_img = cvCreateImage(cvSize(img->width, img->height), 8, 3);
		cvConvertImage(img, tmp_img);		
		cvResize(tmp_img, frame_);			 //rescale to size		
		cvReleaseImage(&tmp_img);
		cvShowImage(winname_.str().c_str(), frame_);
		cvWaitKey(10);				
	}
	
	/**
	 * scatter_plot the x and y onto the frame, with color
	 * 
	 */
	void scatter_plot(	ublas::vector<double>& x,
						ublas::vector<double>& y,					
						CvScalar color)
	{	
		
		for (int i = 0; i < (int)x.size(); ++i)
		{		
			double xpt = (x[i]-x0_)/x_range_*frame_->width;
						
			if (isimg_)
				y[i]= -y[i];
			
			double ypt = (y[i]-y0_)/y_range_*frame_->height;
				
			CvPoint pt = cvPoint( cvRound(xpt), cvRound(frame_->height - ypt) );
			//notice the y axis needs to be flipped
			cvCircle( frame_, pt , 3, color, 2, CV_AA);
		}
		
		cvShowImage(winname_.str().c_str(), frame_);
		cvWaitKey(10);
	}
	
	void plot_axis()
	{		
	 	//CvFont font = cvFont(1);	    	
	    //cvPutText( frame, detect_res_msgs[0].message.c_str(), cvPoint(20,20),&font, cvScalar(0,255,0) );
				
		// axis are at x = 0, y = 0;
		double x_axis_height = (1.0-(-y0_)/y_range_)*frame_->height; 
		CvPoint x_neginf = cvPoint( 0, cvRound(x_axis_height) );
		CvPoint x_inf = cvPoint(frame_->width, cvRound(x_axis_height) );		
		cvLine( frame_, x_neginf, x_inf, cvScalar(0), 0);
		
		double y_axis_width = (-x0_)/x_range_*frame_->width; 
		CvPoint y_neginf = cvPoint( cvRound(y_axis_width), frame_->height-0 );
		CvPoint y_inf = cvPoint( cvRound(y_axis_width), 0);		
		cvLine( frame_, y_neginf, y_inf, cvScalar(0), 0);	
	}
	/**
	 * Converts the clicked_pt from the frame_ 's point of view
	 * to that of the axis/frame
	 */
	void convert_pt(CvPoint clicked_pt, CvPoint2D32f& coord_pt)
	{		
		coord_pt.x = float(clicked_pt.x)/frame_->width*x_range_ + x0_;
		coord_pt.y = (1-float(clicked_pt.y)/frame_->height)*y_range_ + y0_;
		
		
		//if we are displaying an image, then, we are in the 4th quadrant,
		//and the y values are negative of what it actually is on the pixel
		if (isimg_)
			coord_pt.y *= -1;				
	}
	
	ublas::matrix<double> ginput(int num_pts);
	
	IplImage * frame_;
	
	double x0_, x1_, y0_, y1_;	///< coordinate axis boundaries
	double x_range_, y_range_;
	
	bool holdon_;
	bool plotaxis_;
	bool isimg_;		///< specifiying what the figure has is an image
	CvScalar background_color_;
	
	int id_;
	std::stringstream winname_;
};

}
