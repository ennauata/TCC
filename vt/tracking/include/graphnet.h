#ifndef _GRAPHNET_H_
#define _GRAPHNET_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <queue>
#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <Eigen/Dense>
#include "cl_util.h"


typedef unsigned int UINT32;
#define IN
#define OUT

#define TRUE 1
#define GRAPHMT_DEBUG TRUE



/**
 *********************************************************************************
	@brief CLASS/STRUCT: CLInterface

	@purpose PURPOSE: This class is basically what one module needs to be able to process
	one time step computation

	@comment COMMENT:

	@changelog CHANGELOG:

	@todo	TODO:
 *********************************************************************************
 **/
class CLInterface : private boost::noncopyable
{
public:
	CLInterface(){
		bAllocated = false;
	}

	~CLInterface(){
	}

	std::vector< cv::Mat >		v_images;
	//ublas::vector<double>		feat_;
	//ublas::matrix<double>		mat_;
	//std::vector<iBox>			boxes_;

	bool bAllocated; //whether or not the data has been allocated
};


/**
 *********************************************************************************
	@brief CLASS/STRUCT: CLGraphEdge

	@purpose   PURPOSE:	This is a base class of a graph edge which provides communication
	between various graphnodes
	@comment COMMENT:	This class should not be derived from, there are no real need of that

	@changelog CHANGELOG:

	@todo	TODO: look at how to make buffer_ private and also accessible from child of CLGraphNode
 *********************************************************************************
 **/
class CLGraphEdge : private boost::noncopyable
{
	const int static nBufferSize = 2;	//should be atleast 2 else the effect of parallelization
	//is lost
public:
	friend class CLGraphNode;
	friend class GraphExecuter;

	CLGraphEdge():
			ind_write_(0),
			ind_read_(0),
			n_buffered_(0)
	{};

	//this constructor should be used
	CLGraphEdge(std::string upstream_nodename, std::string downstream_nodename):
			ind_write_(0),
			ind_read_(0),
			n_buffered_(0)
	{
		start_nodename_ = upstream_nodename;
		end_nodename_ 	= downstream_nodename;

		buffer_.resize(nBufferSize);
		for (int i = 0; i < nBufferSize; ++i){
			buffer_[i] = new CLInterface();
		}
	};

	~CLGraphEdge(){
		for (UINT32 ui = 0; ui < buffer_.size(); ++ui){
			delete buffer_[ui];
		}
		buffer_.resize(0);
	}

	/// this function call will block if no availiable input
	bool ready_to_read(){
		{
			boost::mutex::scoped_lock lk(monitor);

			if (n_buffered_ == 0)
				cond_not_empty_.wait(lk);
		}
		return true;
	}


	/// this function makes a request for a buffer
	bool ready_to_write(){

		{
			boost::mutex::scoped_lock lk(monitor);

			if (n_buffered_ == (size_t) nBufferSize)
				cond_not_full_.wait(lk);
		}
		return true;
	}

	/// function call to indicate that one read operation have been performed
	void done_read(){
		//remember to signal all threads blocking on cond_not_full_.wait
		{
			boost::mutex::scoped_lock lk(monitor);
			n_buffered_--;
			ind_read_ = (ind_read_+1) % nBufferSize;
			cond_not_full_.notify_one();
		}

	}
	/// function call to indicate that one write operations have took place
	void done_write(){
		//remember to signal all threads blocking on cond_not_empty_.wait
		{
			boost::mutex::scoped_lock lk(monitor);
			n_buffered_++;
			ind_write_ = (ind_write_+1) % nBufferSize;
			cond_not_empty_.notify_one();
		}
	}

	//accessors to the CLInterface for current writes and reads
	const CLInterface*  getInterfaceRead(){		return buffer_[ind_read_];};
	CLInterface* 		getInterfaceWrite(){	return buffer_[ind_write_];};

	std::vector<CLInterface *> buffer_;	///< provides a buffer to store interface information
	std::string start_nodename_;		///< the nodes' names
	std::string end_nodename_;

private:
	boost::condition cond_not_full_;	///< condition to be able to write to buffer
	boost::condition cond_not_empty_;	///< condition to be able to read from buffer
	boost::mutex     monitor;			///< mutex for the condition variables

	size_t ind_write_, ind_read_;		///< where in the buffer_ to get data
	size_t n_buffered_;					///< number of "stuff" buffered as of now

	//CLGraphNode * pStart_;			///< pointer to start node
	//CLGraphNode * pEnd_;				///< pointer to end node
};



/**
 *********************************************************************************
	@brief CLASS/STRUCT: CLGraphNode

	@purpose PURPOSE:	This is a base class of a graph node which can be executed
						by a multi-threaded graph, it basically provides a wrapper
						for computing time-step based run of a module
	@comment COMMENT:

	@changelog CHANGELOG:

	@todo	TODO:
 *********************************************************************************
 **/
class CLGraphNode : private boost::noncopyable
{
	friend class GraphExecuter;
public:
	CLGraphNode() :
		bInitialized_(false),
		n_counter_(0)
	{};

	CLGraphNode(std::string node_name, std::string node_type_name) :
		node_name_(node_name),
		node_type_name_(node_type_name),
		bInitialized_(false),
		n_counter_(0)
	{};

	//#####################Function need to be Implemented######################
	virtual ~CLGraphNode();

	//#####################Function need to be Implemented######################
	/// this function is for initializing the CLInterface edges according to what they want
	virtual int init() = 0;

	//#####################Function need to be Implemented######################
	/// uses vpInEdges_ and vpOutEdges_ specified
	/// only called by the GraphExecuter
	virtual int run() = 0;

	//#####################Function need to be Implemented######################
	/// this function will be called by the GraphExecuter thread upon thread creation
	virtual int thread_init() = 0;

	//#####################Function need to be Implemented######################
	/// this function will be called by the GraphExecuter thread upon thread interruption
	virtual int thread_deinit() = 0;

	/// this function builds a map for 1 to 1 mapping between edge name and CLGraphEdge*
	//int BuildEdgesMap();

	/// finds the edge connecting to a node with name "name" and return that edge
	/// returns -1 for name not found
	//int getEdgePtrFromNodeName(std::string name, CLGraphEdge* &);

	std::vector<CLGraphEdge*> vpInEdges_;				///< contains all edges it needs input from
	std::vector<CLGraphEdge*> vpOutEdges_;				///< contains all edges it sends data out to

	//std::map<std::string, unsigned int> InEdgesMap_;	///< map of module name to index in vpInEdges_
	//std::map<std::string, unsigned int> OutEdgesMap_;	///< map of module name to index in vpOutEdges_

	std::string node_name_;			///< specific name e.g. videointerface1
	std::string node_type_name_;	///< name of the type of node for debugging purposes, e.g. videointerface

protected:

	bool bInitialized_;
	int n_counter_;
};

/**
 *********************************************************************************
	@brief CLASS/STRUCT: GraphExecuter

	@purpose PURPOSE:	This class provides a wrapper object which can be passed
						to boost::threads' create_thread() function
						the thread executes the operator() function
	@comment COMMENT:

	@changelog CHANGELOG:

	@todo	TODO:
 *********************************************************************************
 **/
class GraphExecuter : private boost::noncopyable
{
public:
	GraphExecuter() :
		pNode_( NULL ),
		state_(0)
	{};

	GraphExecuter(CLGraphNode* pNode) :
		pNode_( pNode ),
		state_(0)
	{};

	/// function executed by a thread
	void operator()();

	CLGraphNode* pNode_;				///< pointer to the node

	boost::mutex state_mutex_;
	boost::condition state_run_cond_;	///< condition that its ok to run
	int state_;			// 0 == stopped, 1 == running, 2 == paused, 3 == shutdown(flush)
};



class CLGraphMT : private boost::noncopyable
{
public:

	CLGraphMT();
	//#####################Function need to be Implemented######################
	virtual ~CLGraphMT();

	//#####################Function need to be Implemented######################
	/// for initilizing the graph to anything
	virtual int init() = 0;

	//#####################Function need to be Implemented######################
	/// allows the interface between the graph and outside world
	virtual int run(IN const CLInterface& input, OUT CLInterface& output) = 0;

	//#####################Function need to be Implemented######################
	/// make sure the graph is initialized properly
	virtual int validate_graph() = 0;

	int construct_graph();
	int link_nodes_edges();

	int init_threads();

	/// set the graph in different states
	int run_graph();
	int stop_graph();
	int pause_graph();
	int flush_graph();

protected:

	boost::thread_group thrds_;

	int nIters_;		/// iterations of which the graph ran
	int graph_state_;   /// 0 == stopped, 1 == running, 2 == paused, 3 == shutdown
	bool bInitialized_;
	///< whether or not the graph's nodes or edges are initialized(linked together)
	///< basically whether or not init() function is called

	bool bThreadsInit_; ///< whether or not the threads for each node have been initialized
	//is init_threads() called?

	bool bValidated_;   ///< whether or not the graph is linked wrong
	//is validate() called?

	CLGraphEdge * pGraphStartEdge_;		///< for interfacing with outside world
	CLGraphEdge * pGraphEndEdge_;		///< for interfacing with outside world

	std::vector<CLGraphEdge*> vp_edges;
	std::vector<CLGraphNode*> vp_nodes;
	std::vector<GraphExecuter*> node_execs_;

	std::map<std::string, CLGraphNode*> nodes_map;

public:
	//for timing benchmarks
	double time_pushpull_edges;
	double time_running_modules;
};

#endif
