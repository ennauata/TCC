#ifndef _NODE_DRAW_BB_H_
#define _NODE_DRAW_BB_H_

#include <fstream>
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

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

extern boost::mutex g_coutMutex;

using namespace std;
using namespace boost;


struct obj_bb{
	int start;
	int end;

	int id;   //0, 1, 2 ...
	std::string type; //"person"

	std::vector<std::vector<float> > positions;
};

struct video_bb{
	int nFrame;
	int maxObj;

	std::vector<obj_bb> persons;
};


video_bb read_bb( std::string & fn)
{

	// populate tree structure pt
    using boost::property_tree::ptree;
    ptree pt;
    read_xml(fn, pt);

    video_bb boxes;

    boxes.nFrame = pt.get<int>("root.nFrame");
    boxes.maxObj = pt.get<int>("root.maxObj");


    // traverse pt
    BOOST_FOREACH( ptree::value_type const& v, pt.get_child("root") ) {

        if( v.first == "obj" ) {

            obj_bb o;
            o.id = v.second.get<int>("<xmlattr>.id");
            o.type = v.second.get<std::string>("<xmlattr>.type");
            o.start = v.second.get<int>("start");
            o.end = v.second.get<int>("end");

            BOOST_FOREACH(ptree::value_type const& v2, v.second ){

            	if( v2.first == "pos" ) {
            		std::string pos_str = v2.second.data();

            		vector<float> pos4;
            		char_separator<char> sep(", ");
            		tokenizer<char_separator<char> > tokens(pos_str, sep);

            		for ( tokenizer<char_separator<char> >::iterator it = tokens.begin();
            				it != tokens.end(); ++it){
            			pos4.push_back( atof( it->c_str() ) );
            		}
            		assert(pos4.size()==4);
            		o.positions.push_back(pos4);
            	}
            }

            assert(o.positions.size() == o.end-o.start+1);

            boxes.persons.push_back(o);
        }
    }


    assert(boxes.persons.size() == boxes.maxObj);

    return boxes;
}



class GraphNodeDrawBB : public CLGraphNode
{
public:

	GraphNodeDrawBB() : CLGraphNode()
	{

	}

	GraphNodeDrawBB(std::string node_name, std::string node_type_name,
			std::string _xml_fname)
	: CLGraphNode(node_name, node_type_name)
	{
		xml_fname = _xml_fname;
	}

	~GraphNodeDrawBB()
	{
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

	std::string xml_fname;
	video_bb boxes;

	int num_frames;

};



//This function is typically called by CLGraphMT children, which gives a list of edges
//this node need to connect to
//also, this node assumes that the format, e.g. frame height, of the in_edges's CLInterface
//have been set and just to check to make sure it is corresponding
//but this function needs to set the formats of all its out_edges
int GraphNodeDrawBB::init()
{
	//check size constraints on the number of edges
	if (vpInEdges_.size() != 1)
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
			if (vp_intf[uj]->v_images[0].depth() != CV_8U || vp_intf[uj]->v_images[0].channels() !=3)
				return -6;
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

	//load xml file
	boxes = read_bb( xml_fname );

	fprintf(stderr, "DrawBB: nFrame:%d  maxObj:%d persons:%ld\n", boxes.nFrame, boxes.maxObj, boxes.persons.size());

	num_frames = 0;
	bInitialized_ = true;
	return 0;
};

/// uses vpInEdges_ and vpOutEdges_ specified
/// This function is only called by the GraphExecuter
int GraphNodeDrawBB::run()
{

	const CLInterface* in_interface = vpInEdges_[0]->getInterfaceRead();

	int framenum = num_frames + 1;

	for (UINT32 ui = 0; ui < vpOutEdges_.size(); ++ui)
	{
		CLInterface* out_interface =  vpOutEdges_[ui]->getInterfaceWrite();
		in_interface->v_images[0].copyTo( out_interface->v_images[0] );

		BOOST_FOREACH(obj_bb bb, boxes.persons ){

			if (framenum >= bb.start &&  framenum <= bb.end){

				float x = bb.positions[framenum-bb.start][0];
				float y = bb.positions[framenum-bb.start][1];

				float width = bb.positions[framenum-bb.start][2];
				float height = bb.positions[framenum-bb.start][3];

				cv::Rect r(x, y, width, height);
				cv::rectangle(  out_interface->v_images[0], r, cv::Scalar( 10, 220, 0 ) );
			}
		}

		//cvCopy(p_image, out_interface->vpImages_[0]);
		//cvSet(out_interface->vpImages_[0], CV_RGB(0,255,0));
	}

	num_frames++;

	return 0;
};





#endif
