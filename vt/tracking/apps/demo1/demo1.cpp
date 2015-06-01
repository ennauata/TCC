
#include <vector>
#include <iostream>
#include <string>
#include <queue>
#include <map>

#include "cl_util.h"
#include "graphnet.h"
#include "node_display.h"
#include "node_bkgnd.h"
#include "node_downsample.h"
#include "node_readvideo.h"
#include "node_draw_bb.h"
#include "node_webcam.h"
#include "node_normalize.h"
#include "node_grey.h"
#include "node_save_sequence.h"

using namespace std;


class GraphMT_FrameDownSample : public CLGraphMT {
public:

	GraphMT_FrameDownSample(string fname, string dirname) : CLGraphMT() {
		fname_ = fname;
		dirname_ = dirname;
	};

	~GraphMT_FrameDownSample(){};

	int init(){
	
		/********************************************************************/	
		// Graph 1															//
		/********************************************************************/
		
		// Create Node
		vp_nodes.clear();
		vp_nodes.push_back( new GraphNodeReadVideoFrame("read1", "ReadVideoFrame", fname_ ));
		vp_nodes.push_back( new GraphNodeNormalize("norm1"));
		vp_nodes.push_back( new GraphNodeGrey("grey1"));
		vp_nodes.push_back( new GraphNodeSaveSeq("save_seq1", dirname_));
		
		// Create Edges
		vp_edges.clear();
		vp_edges.push_back( new CLGraphEdge("read1", "norm1"));
		vp_edges.push_back( new CLGraphEdge("norm1", "grey1"));
		vp_edges.push_back( new CLGraphEdge("grey1", "save_seq1"));

		int err = link_nodes_edges();
		bInitialized_ = true;
		return err;
	};

	/// allows the interface between the graph and outside world
	int run(IN const CLInterface& input, OUT CLInterface& output)
	{
		return 0;
	}

	int validate_graph(){
		bValidated_ = true;
		return 0;
	}

private:

	string fname_;
	string dirname_;
};



int main( int argc, char *argv[] )
{
	if (argc != 3)
		return -1;


	int err;

	CLGraphMT* pgraph = new GraphMT_FrameDownSample(string(argv[1]), string(argv[2]));
		
	pgraph->construct_graph();

	g_coutMutex.lock();
	cerr << "main() After Initialization, b4 run_graph()" << endl;
	g_coutMutex.unlock();
	err = pgraph->run_graph();
	if (err < 0){
		g_coutMutex.lock();
		cerr << "Error run_graph()" << endl;
		g_coutMutex.unlock();
	} 

	sleep(5);  		//unix
	//Sleep(10*1000); 	//windows

	pgraph->stop_graph();

	delete pgraph;
	return 0;
}


