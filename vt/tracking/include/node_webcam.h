#ifndef _NODE_WEBCAM_H_
#define _NODE_WEBCAM_H_

#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "graphnet.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/objdetect/objdetect.hpp"



extern boost::mutex g_coutMutex;


class GraphNodeWebcam : public CLGraphNode
{
public:

	GraphNodeWebcam() : CLGraphNode()
	{
		pCapture = NULL;
	}

	GraphNodeWebcam(std::string node_name, std::string node_type_name)
	: CLGraphNode(node_name, node_type_name)
	{
		pCapture = NULL;
	}

	~GraphNodeWebcam()
	{
		delete pCapture;
	}

	int thread_init(){ return 0;};
	int thread_deinit(){return 0;};

	//This function is typically called by CLGraphMT children, which gives a list of edges
	//this node need to connect to
	//also, this node assumes that the format, e.g. frame height, of the in_edges's CLInterface
	//have been set and just to check to make sure it is corresponding
	//but this function needs to set the formats of all its out_edges
	int init();

	/// uses vpInEdges_ and vpOutEdges_ specified
	/// This function is only called by the GraphExecuter
	int run();

private:

	cv::VideoCapture* pCapture;
};



//This function is typically called by CLGraphMT children, which gives a list of edges
//this node need to connect to
//also, this node assumes that the format, e.g. frame height, of the in_edges's CLInterface
//have been set and just to check to make sure it is corresponding
//but this function needs to set the formats of all its out_edges
int GraphNodeWebcam::init()
{
	//check size constraints on the number of edges
	if (vpInEdges_.size() != 0)
		return -1;
	if (vpOutEdges_.size() < 1)
		return -2;

	//now, check to see if the input Interface's formats are good
	for (UINT32 ui = 0 ; ui < vpInEdges_.size(); ++ui)
	{
		std::vector<CLInterface*>  vp_intf = vpInEdges_[ui]->buffer_;

		//format checks here if necessary
		for (UINT32 uj = 0; uj < vp_intf.size(); ++uj)
		{
			if (!vp_intf[uj]->bAllocated)
				return -3;
		}
	}

	//now, set the output's Interface's formats
	for (UINT32 ui = 0 ; ui < vpOutEdges_.size(); ++ui)
	{
		std::vector<CLInterface*>  vp_intf = vpOutEdges_[ui]->buffer_;

		//sets the formats
		for (UINT32 uj = 0; uj < vp_intf.size(); ++uj)
		{
			vp_intf[uj]->v_images.resize(1);
			vp_intf[uj]->v_images[0].create( 240, 320, CV_8UC3 );
			vp_intf[uj]->bAllocated = true;
		}
	}

	//Node specific initialization

	pCapture = new cv::VideoCapture(0);

	if(!pCapture->isOpened())  // check if we succeeded
        return -1;
	bInitialized_ = true;
	return 0;
};

/// uses vpInEdges_ and vpOutEdges_ specified
/// This function is only called by the GraphExecuter
int GraphNodeWebcam::run()
{
	//IplImage* p_image = cvQueryFrame( pCapture );
	cv::Mat frame;
	*pCapture >> frame;
	if (frame.cols*frame.rows > 0){

		cv::resize(frame,frame,  cv::Size(320,240));

		for (UINT32 ui = 0; ui < vpOutEdges_.size(); ++ui)
		{
			CLInterface* out_interface =  vpOutEdges_[ui]->getInterfaceWrite();
			frame.copyTo( out_interface->v_images[0] );

			//cvCopy(p_image, out_interface->vpImages_[0]);
			//cvSet(out_interface->vpImages_[0], CV_RGB(0,255,0));
		}
	}
	n_counter_++;
	return 0;
};






#endif
