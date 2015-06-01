#ifndef _NODE_DISPLAY_H_
#define _NODE_DISPLAY_H_


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


class GraphNodeDisplay : public CLGraphNode
{
public:
	GraphNodeDisplay() : CLGraphNode()
	{
	}
	GraphNodeDisplay(std::string node_name, std::string node_type_name)
	: CLGraphNode(node_name, node_type_name)
	{
		sprintf(winname, "%s:%s", this->node_type_name_.c_str(), this->node_name_.c_str() );
		cvNamedWindow(winname, CV_WINDOW_NORMAL);
		cv::moveWindow(winname, numwin*400, 100);

		numwin++;
	}

	~GraphNodeDisplay()
	{
		cvDestroyWindow(winname);
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

	char winname[100];

	static int numwin;
};
int GraphNodeDisplay::numwin = 0;


//This function is typically called by CLGraphMT children, which gives a list of edges
//this node need to connect to
//also, this node assumes that the format, e.g. frame height, of the in_edges's CLInterface
//have been set and just to check to make sure it is corresponding
//but this function needs to set the formats of all its out_edges
int GraphNodeDisplay::init()
{
	//makesure the graph is consistent with what this node needs
	//check size constraints on the number of edges
	if (vpInEdges_.size() != 1)
		return -1;
	if (vpOutEdges_.size() != 0)
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
			if (vp_intf[uj]->v_images.size() != 1)
				return -4;
			if (vp_intf[uj]->v_images[0].depth() != CV_8U || vp_intf[uj]->v_images[0].channels() !=3)
				return -6;
		}
	}

	//now, set the output's Interface's formats
	for (UINT32 ui = 0 ; ui < vpOutEdges_.size(); ++ui)
	{
		std::vector<CLInterface*> vp_intf = vpOutEdges_[ui]->buffer_;
		//sets the formats
		for (UINT32 uj = 0; uj < vp_intf.size(); ++uj)
		{
		}
	}


	bInitialized_ = true;
	return 0;

};

/// uses vpInEdges_ and vpOutEdges_ specified
/// This function is only called by the GraphExecuter
int GraphNodeDisplay::run()
{
	//we can use zero because we made sure the # of input is 1 and
	//the # of outputs is 0
	const CLInterface* in_interface = vpInEdges_[0]->getInterfaceRead();

	g_coutMutex.lock();
	if(!in_interface->v_images[0].empty()){
		cv::imshow(winname, in_interface->v_images[0]);
	}
	cv::waitKey( 10 );
	g_coutMutex.unlock();
	//usleep(20*1000);

	n_counter_++;
	return 0;
};





#endif
