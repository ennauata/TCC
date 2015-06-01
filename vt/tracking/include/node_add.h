#ifndef _NODE_ADD_H_
#define _NODE_ADD_H_

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


class GraphNodeAdd : public CLGraphNode
{
public:
	GraphNodeAdd() : CLGraphNode()
	{}

	GraphNodeAdd(std::string node_name, std::string node_type_name)
				: CLGraphNode(node_name, node_type_name)
	{}

	~GraphNodeAdd()
	{}

	int thread_init(){ return 0;};
	int thread_deinit(){return 0;};
	//This function is typically called by CLGraphMT children, which gives a list of edges
	//this node need to connect to
	//also, this node assumes that the format, e.g. frame height, of the in_edges's CLInterface
	//have been set and just to check to make sure it is corresponding
	//but this function needs to set the formats of all its out_edges
	int init(){

		//check size constraints on the number of edges
		if (vpInEdges_.size() != 2)
			return -1;
		if (vpOutEdges_.size() != 1)
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


	}

	/// uses vpInEdges_ and vpOutEdges_ specified
	/// This function is only called by the GraphExecuter
	int run(){
		//we can use zero because we made sure the # of input is 1 and
		//the # of outputs is also 1
		const CLInterface* in_interface0 = vpInEdges_[0]->getInterfaceRead();
		const CLInterface* in_interface1 = vpInEdges_[1]->getInterfaceRead();
		CLInterface* out_interface =  vpOutEdges_[0]->getInterfaceWrite();

		out_interface->v_images[0] = (in_interface0->v_images[0]+in_interface1->v_images[0])/2;

		n_counter_++;
		return 0;

	}
};










#endif
