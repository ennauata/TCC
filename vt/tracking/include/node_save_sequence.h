#ifndef _NODE_SAVE_SEQ_H_
#define _NODE_SAVE_SEQ_H_

#include <iostream>

#include "graphnet.h"
#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/video/background_segm.hpp>


using namespace cv;

//this module convets the video to a sequence of images
class GraphNodeSaveSeq : public CLGraphNode
{
public:
	GraphNodeSaveSeq() : CLGraphNode()
	{}

	GraphNodeSaveSeq( std::string node_name , std::string _dirname): CLGraphNode(node_name, "Save Sequence")
	{
		dirname = _dirname;
	}

	~GraphNodeSaveSeq()
	{}
	
	
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
	std::string dirname;
};

//This function is typically called by CLGraphMT children, which gives a list of edges
//this node need to connect to
//also, this node assumes that the format, e.g. frame height, of the in_edges's CLInterface
//have been set and just to check to make sure it is corresponding
//but this function needs to set the formats of all its out_edges
int GraphNodeSaveSeq::init()
{

	//makesure the graph is consistent with what this node needs
	//check size constraints on the number of edges
	if (vpInEdges_.size() != 1)
		return -1;
	if (vpOutEdges_.size() != 0)
		return -1;
		
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
			if (vp_intf[uj]->v_images[0].cols != 320 || vp_intf[uj]->v_images[0].rows != 240 )
				return -5;
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
			vp_intf[uj]->v_images.resize(1);
			vp_intf[uj]->v_images[0].create( 240, 320, CV_8UC3 );
			vp_intf[uj]->bAllocated = true;
		}
	}

	bInitialized_ = true;
	return 0;

};

/// uses vpInEdges_ and vpOutEdges_ specified
/// This function is only called by the GraphExecuter
int GraphNodeSaveSeq::run()
{
	//we can use zero because we made sure the # of input is 1 and
	//the # of outputs is also 1
	
	const CLInterface* in_interface = vpInEdges_[0]->getInterfaceRead();
	
	vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);
	
	Mat input = in_interface->v_images[0];
	
	if(!input.empty()){
	
		string filename;
		filename = "./seq_files/" + dirname  + "/seq_" + to_string(n_counter_) + ".png";
		imwrite(filename, input, compression_params);
	}
	return 0;
};

#endif
