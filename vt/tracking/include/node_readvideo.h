#ifndef _NODE_READVIDEO_H_
#define _NODE_READVIDEO_H_

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


class GraphNodeReadVideoFrame : public CLGraphNode
{
public:

	GraphNodeReadVideoFrame() : CLGraphNode()
	{
		pCapture = NULL;
	}

	GraphNodeReadVideoFrame(std::string node_name, std::string node_type_name,
			std::string _fname)
	: CLGraphNode(node_name, node_type_name)
	{
		pCapture = NULL;
		fname = _fname;
	}

	~GraphNodeReadVideoFrame()
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
	bool is_finished;
	std::string fname;
	cv::VideoCapture* pCapture;

};



//This function is typically called by CLGraphMT children, which gives a list of edges
//this node need to connect to
//also, this node assumes that the format, e.g. frame height, of the in_edges's CLInterface
//have been set and just to check to make sure it is corresponding
//but this function needs to set the formats of all its out_edges
int GraphNodeReadVideoFrame::init()
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
	pCapture = new cv::VideoCapture( fname.c_str() );

	if(!pCapture->isOpened())  // check if we succeeded
        return -1;

	bInitialized_ = true;
	is_finished =   false;
	return 0;
};

/// uses vpInEdges_ and vpOutEdges_ specified
/// This function is only called by the GraphExecuter
int GraphNodeReadVideoFrame::run()
{
	//IplImage* p_image = cvQueryFrame( pCapture );
	cv::Mat frame;
	
	if(!is_finished){
		*pCapture >> frame;

		if (pCapture->get(CV_CAP_PROP_POS_FRAMES) == pCapture->get(CV_CAP_PROP_FRAME_COUNT))
		{
			g_coutMutex.lock();
			fprintf(stderr, "\nGraphNodeReadVideoFrame: Finished\n" );
			g_coutMutex.unlock();
			is_finished = true;
			return 0;
		}

		for (UINT32 ui = 0; ui < vpOutEdges_.size(); ++ui)
		{	
			CLInterface* out_interface =  vpOutEdges_[ui]->getInterfaceWrite();
			frame.copyTo( out_interface->v_images[0] );
		}
	}
	else{
		for (UINT32 ui = 0; ui < vpOutEdges_.size(); ++ui)
		{	
			CLInterface* out_interface =  vpOutEdges_[ui]->getInterfaceWrite();
			out_interface->v_images[0].release();
		}
	}
	
	return 0;
};





#endif
