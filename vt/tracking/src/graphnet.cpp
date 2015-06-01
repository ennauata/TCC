#include "graphnet.h"


using namespace std;

boost::mutex g_coutMutex;


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


CLGraphNode::~CLGraphNode()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void GraphExecuter::operator()()
{
	//check that module is initalized
	if ( !pNode_->bInitialized_ ){
		return;
	}

	pNode_->thread_init();

	try {
		while ( true )
		{
			{// scoped_lock lk

				boost::mutex::scoped_lock lk(state_mutex_);

				if (state_ == 1)
				{
					//run state, don't do anything
				}
				else if (state_ == 0 || state_ == 2) {		//if graph not running, we wait for run_conditional
					//cerr << "[" << node->execution_count_ << "]" << node->alias << " waiting_run_cond" << endl;

#ifdef GRAPHMT_DEBUG
					g_coutMutex.lock();
					cout << this->pNode_->node_name_ << " waiting for run condition" << endl;
					g_coutMutex.unlock();
#endif
					state_run_cond_.wait( lk );
#ifdef GRAPHMT_DEBUG
					g_coutMutex.lock();
					cout << this->pNode_->node_name_ << " received goahead for run condition" << endl;
					g_coutMutex.unlock();
#endif
				}
				else if (state_ == 3)
				{
#ifdef GRAPHMT_DEBUG
					g_coutMutex.lock();
					cerr << pNode_->node_name_ << " state_ == 3 - Shutting Down" << endl;
					g_coutMutex.unlock();
#endif
					return;
				}
				else
				{
#ifdef GRAPHMT_DEBUG
					g_coutMutex.lock();
					cerr << "\nError! graph state not supported! " << state_ << endl;
					g_coutMutex.unlock();
#endif
					return;
				}

			}// end scoped_lock lk

			//idea: may consider to have internal buffer to overlap computation with transfers!!
			//below weights for write buffer to open to begin its own computations!

			//Now: make sure all input edges are not empty
			for (UINT32 ui = 0; ui < pNode_->vpInEdges_.size(); ++ui)
			{
				pNode_->vpInEdges_[ui]->ready_to_read();	// will block if necessary
			}

			//Now: make sure all output edges are not full
			for (UINT32 ui = 0; ui < pNode_->vpOutEdges_.size(); ++ui)
			{
				pNode_->vpOutEdges_[ui]->ready_to_write();	// will block if necessary
			}

			pNode_->run();		//run once and write output to the edges

			//Let the edges know about the action that took place
			for (UINT32 ui = 0; ui < pNode_->vpInEdges_.size(); ++ui)
			{
				pNode_->vpInEdges_[ui]->done_read();
			}

			//Now: make sure all output edges are not full
			for (UINT32 ui = 0; ui < pNode_->vpOutEdges_.size(); ++ui)
			{
				pNode_->vpOutEdges_[ui]->done_write();
			}

			pNode_->n_counter_++;

		}// end while

	}
	catch(boost::thread_interrupted interr){


		{// scoped_lock lk

			boost::mutex::scoped_lock lk(state_mutex_);
			if (state_ == 3)
			{
#ifdef GRAPHMT_DEBUG
				g_coutMutex.lock();
				cerr << pNode_->node_name_ << " Interrupted - Shutting Down" << endl;
				g_coutMutex.unlock();
#endif
			}
			else{

#ifdef GRAPHMT_DEBUG
				g_coutMutex.lock();
				cerr << pNode_->node_name_ << " Interrupted at when state is " << state_ << endl;
				g_coutMutex.unlock();

#endif
			}

		}// end scoped_lock lk

		pNode_->thread_deinit();
		return;

	}	//end catch


}// end operator()()


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


/************************************************************************/
// Constructor
/************************************************************************/

CLGraphMT::CLGraphMT() :
				nIters_(0),
				graph_state_(0),
				bInitialized_(false),
				bThreadsInit_(false),
				bValidated_(false),
				pGraphStartEdge_(NULL),
				pGraphEndEdge_(NULL),
				time_pushpull_edges(0.0),
				time_running_modules(0.0)
{
	vp_edges.clear();
	vp_nodes.clear();
	node_execs_.clear();
};


/**
 **************************************************************************************
	@brief FUNCTION:	~CLGraphMT
	@purpose PURPOSE:	When the graph object is destructed, first stops the graph,
						then join_all (wait for all threads to quit)
	@comment COMMENT:
	@changelog CHANGELOG:
	@tested TESTED: false
	@todo TODO:
 ***************************************************************************************
 **/
CLGraphMT::~CLGraphMT()
{
	stop_graph();

#ifdef GRAPHMT_DEBUG
	g_coutMutex.lock();
	cerr << "~CLGraphMT - b4 join_all" << endl;
	g_coutMutex.unlock();
#endif
	thrds_.join_all();

#ifdef GRAPHMT_DEBUG
	g_coutMutex.lock();
	cerr << "~CLGraphMT - after join_all" << endl;
	g_coutMutex.unlock();
#endif

	//kill all GraphExecuter objects
	for (UINT32 ui = 0; ui < vp_nodes.size(); ++ui)
	{
		delete node_execs_[ui];

		if ( vp_nodes[ui] != NULL )
			delete vp_nodes[ui];
	}
	for (UINT32 ui = 0; ui < vp_edges.size(); ++ui)
	{
		delete vp_edges[ui];
	}

	if ( pGraphStartEdge_ != NULL)
		delete pGraphStartEdge_;

	if ( pGraphEndEdge_ != NULL)
		delete pGraphEndEdge_;
}

/**
 **************************************************************************************
	@brief FUNCTION: construct_graph
	@purpose PURPOSE:	called to construct a new multi-threaded graph and make sure it's working
						properly
	@comment COMMENT:	every main that needs to use a GraphMT child object need to call this function
						before calling run_graph
	@changelog CHANGELOG:
	@tested TESTED: false
	@todo TODO:
 ***************************************************************************************
 **/
int CLGraphMT::construct_graph()
{
	int err = init();
	if (err != 0){
		cerr << "GraphMT Failed to Init() err:" << err << endl;
		return -1;
	}

	err = validate_graph();
	if (err != 0){
		cerr << "GraphMT Failed to be Validated err:"  << err  << endl;
		return -1;
	}

	err = init_threads();
	if (err != 0){
		cerr << "GraphMT Failed to init threads err:"  << err  << endl;
		return -1;
	}

	return 0;
}


//this function tries to link the nodes with the edges and also do some error checking
//should be called inside of CLGraphMT::init(), after initializing the nodes and edges
int CLGraphMT::link_nodes_edges(){

	//first make a maps of node names to their node pointer

	if (vp_edges.size() < 1)
		return -1;

	if (vp_nodes.size() < 1)
		return -2;

	nodes_map.clear();
	std::map<std::string, CLGraphNode*>::iterator it;

	for (unsigned int i = 0; i < vp_nodes.size(); ++i){
		nodes_map[vp_nodes[i]->node_name_] = vp_nodes[i];

		vp_nodes[i]->vpInEdges_.clear();
		vp_nodes[i]->vpOutEdges_.clear();
	}

	for (unsigned int i = 0; i < vp_edges.size(); ++i){

		//find nodes which this edge has inbound connection from
		std::string name = vp_edges[i]->start_nodename_;
		it = nodes_map.find(name);
		if ( it==nodes_map.end()){
			return -3;
		}else{
			it->second->vpOutEdges_.push_back( vp_edges[i] );
		}

		//find nodes which this edge has outbound connection to
		name = vp_edges[i]->end_nodename_;
		it = nodes_map.find(name);
		if ( it==nodes_map.end()){
			return -4;
		}else{
			it->second->vpInEdges_.push_back( vp_edges[i] );
		}
	}

	for (unsigned int i = 0; i < vp_nodes.size(); ++i){
		int err= vp_nodes[i]->init();
		if (err != 0)
			return -(i+1)*100+err;
	}

	return 0;
}


//initializes and creates the threads for every module in the graph
int CLGraphMT::init_threads()
{
	if (!bInitialized_)
		return -1;

	node_execs_.resize(vp_nodes.size());

	for (UINT32 ui = 0; ui < vp_nodes.size(); ++ui)
	{
		node_execs_[ui] = new GraphExecuter( vp_nodes[ui] );
		thrds_.create_thread( boost::ref(*(node_execs_[ui])));
	}

	bThreadsInit_ = true;
	return 0;
}


/**
 **************************************************************************************
	@brief FUNCTION:	run_graph
	@purpose PURPOSE:	set the graph to run state and motion and notify all threads blocking
						to wait for the graph to be put into run state
	@comment COMMENT:
	@changelog CHANGELOG:
	@tested TESTED: false
	@todo TODO:
 ***************************************************************************************
 **/
int CLGraphMT::run_graph( )
{
	if (!bInitialized_ || !bThreadsInit_ || !bValidated_O
		return -1;

	if( (vp_nodes.size() == 0) ||	(vp_edges.size() == 0)
			|| (UINT32) thrds_.size() != node_execs_.size())
		return -1;

	//now, need to set all node_execs_ 's states to run
	for (UINT32 ui = 0; ui < node_execs_.size(); ++ui)
	{
		boost::mutex::scoped_lock lk( node_execs_[ui]->state_mutex_);
		node_execs_[ui]->state_ = 1;
		node_execs_[ui]->state_run_cond_.notify_one();
	}

#ifdef GRAPHMT_DEBUG
	g_coutMutex.lock();
	cerr << "@@@@@@@CLGraphMT@@@@@@@@ GraphMT Running" << endl;
	g_coutMutex.unlock();
#endif

	return 0;
}


/**
 **************************************************************************************
	@brief FUNCTION:	stop_graph
	@purpose PURPOSE:	to change the states of the graph to 0 (stop)
	@comment COMMENT:	also calls interrupt_all() which will interrupt all threads which
						are stopped at an interruption point, which includes threads are
						blocked on a condition etc.. see Boost.Thread documentation
	@changelog CHANGELOG:
	@tested TESTED: false
	@todo TODO:
 ***************************************************************************************
 **/
int CLGraphMT::stop_graph()
{
	if( (vp_nodes.size() == 0) ||	(vp_edges.size() == 0) || !bInitialized_
			|| (UINT32) thrds_.size() != node_execs_.size())
		return -1;

	//now, need to set all node_execs_ 's states to stop
	for (UINT32 ui = 0; ui < node_execs_.size(); ++ui)
	{
		boost::mutex::scoped_lock lk( node_execs_[ui]->state_mutex_);
		node_execs_[ui]->state_ = 0;
	}

	thrds_.interrupt_all();

#ifdef GRAPHMT_DEBUG
	g_coutMutex.lock();
	cerr << "@@@@@@@CLGraphMT@@@@@@@@ GraphMT Stopped" << endl;
	g_coutMutex.unlock();
#endif
	return 0;
}


int CLGraphMT::pause_graph()
{
	if( (vp_nodes.size() == 0) ||	(vp_edges.size() == 0) || !bInitialized_
			|| (UINT32) thrds_.size() != node_execs_.size())
		return -1;

	//now, need to set all node_execs_ 's states to stop
	for (UINT32 ui = 0; ui < node_execs_.size(); ++ui)
	{
		boost::mutex::scoped_lock lk( node_execs_[ui]->state_mutex_);
		node_execs_[ui]->state_ = 2;
	}

#ifdef GRAPHMT_DEBUG
	g_coutMutex.lock();
	cerr << "@@@@@@@CLGraphMT@@@@@@@@ GraphMT Paused" << endl;
	g_coutMutex.unlock();
#endif
	return 0;
}

/**
 **************************************************************************************
	@brief FUNCTION:	flush_graph
	@purpose PURPOSE:	the purpose of this function is to flush all the graph's output
						with input stopped, the node will finish processing whatever is on
						their input buffers and then the graph should stop
	@comment COMMENT:
	@changelog CHANGELOG:
	@tested TESTED: false
	@todo TODO:			implement it
 ***************************************************************************************
 **/
int CLGraphMT::flush_graph()
{
	return 0;
}

/*
	//rungraph() should be called prior to invoking this function
	int CLGraphMT::run( MvInterface& input, MvInterface& output)
	{
		if( (nodes_.size() == 0) ||	(edges_.size() == 0)|| v_threads_.size() == 0 || !initialized_ || !valid_ )
			return -1;

		if (start_node_->incoming_edge_.size() != 1 || start_node_->incoming_edge_[0] != &edges_[graphstart_edge_ind_])
		{
			cerr << "Error First Module doesn't contain src edge as the incoming edge";
			return -1;
		}

		pthread_mutex_lock( &run_mutex_);
		if (graph_state_ != 1) { //if graph not running, we wait for run_conditional
			cerr << "run is called when graph is not running" << endl;
			pthread_mutex_unlock( &run_mutex_);
			return -1;
		}
		pthread_mutex_unlock( &run_mutex_);

#ifndef NDEBUG
		cerr << "[*]run() func" << " trying lock pushedge: graphstart edge" << endl;
#endif

		pthread_mutex_lock( &v_edge_mutexes_[graphstart_edge_ind_] );

		if (v_edge_buffer_state_[graphstart_edge_ind_] == 2) //buffer is full
		{
#ifndef NDEBUG
			cerr << "run() func" << " waiting to pushedge:" << " graphstart edge" << endl;
#endif

			//block if edge buffer is full
			pthread_cond_wait( &v_edge_buffer_unfull_[graphstart_edge_ind_] , &v_edge_mutexes_[graphstart_edge_ind_] );
			//will unblock when state change to 0 or 1
		}

		int res = edges_[graphstart_edge_ind_].push_mt( input );
		assert(res > 0);
		if (res > 0) //did something
		{
			//after push, 2 conditions
			int past_buf_state = v_edge_buffer_state_[graphstart_edge_ind_] ;
			//1 buffer is full - set flag
			v_edge_buffer_state_[graphstart_edge_ind_] = res;

			//2 buffer not empty anymore, wake anythread waiting for input
			if (past_buf_state == 0 && v_edge_buffer_state_[graphstart_edge_ind_]  != 0) {
				pthread_cond_signal( &v_edge_ready_cond_[graphstart_edge_ind_] );
			}
		}

		pthread_mutex_unlock( &v_edge_mutexes_[graphstart_edge_ind_] );


		//after pushing into the graphstart_edge_
		//we check to graph from graphend_edge_

		output.clear();

#ifndef NDEBUG
		cerr << "[*]run() func" << " trying lock pulledge: graphend edge" << endl;
#endif

		pthread_mutex_lock( &v_edge_mutexes_[graphend_edge_ind_]);

		//if the buffer is zero then we just don't give output anything,
		//we shouldn't block when there are no output, that would make multi thread
		//meanlingless
		if (v_edge_buffer_state_[graphend_edge_ind_] != 0) //buffer is NOT empty
		{
			res = edges_[graphend_edge_ind_].pull_mt(output);
			assert(res < 2);
			if (res < 2) //after pulling 2 conditions:
			{
				//	int past_buf_state = v_edge_buffer_state_[graphend_edge_ind_];
				//1 buffer is empty - set flag
				v_edge_buffer_state_[graphend_edge_ind_] = res;
				//2 buffer not full anymore - wake any blocked threads wanting to push
				//NOTE: state can jump from 2 to 0, i.e. from Full to empty!!!
				//if (past_buf_state == 2 && v_edge_buffer_state_[graphend_edge_ind_] != 2) {
				pthread_cond_signal( &v_edge_buffer_unfull_[graphend_edge_ind_]);
				//}

			}
		}

		pthread_mutex_unlock( &v_edge_mutexes_[graphend_edge_ind_]);

		return 0;
	}




	bool MvGraphMT::validate_graph()
	{
		//traverse graph and ensure there are no loops
		//make sure all the graph nodes are connected in one

		if (start_node_ == NULL || end_node_ == NULL)
		{
			valid_ = false;
			cerr << "MvGraphMT Not Validated, missing start and/or end node ptr" << endl;
			return false;
		}

		if ((start_node_->incoming_edge_.size() == 0 && start_node_->module_type_string != "video_source") ||
			(start_node_->incoming_edge_.size() != 1 || start_node_->incoming_edge_[0] != &edges_[graphstart_edge_ind_]))
		{
			cerr << "MvGraphMT Not Validated, start_node_ doesn't have src edge or start_node_ is not video_source" << endl;
			valid_ = false;
			return false;
		}

		map< string, bool > linked_nodes; //create a map
		for (size_t i = 0; i < nodes_.size(); ++i) {
			linked_nodes[nodes_[i].module_type_string] = false;
		}

		MvGraphNode* traversenode;
		queue< MvGraphNode* > searchQ;
		searchQ.push( start_node_ );

		while ( !searchQ.empty() )
		{
			traversenode = searchQ.front();
			searchQ.pop();

			for (uint32_t i = 0; i < traversenode->outgoing_edge_.size(); ++i)
			{

				if (traversenode->outgoing_edge_[i]->end_ != NULL)
				{
					searchQ.push( traversenode->outgoing_edge_[i]->end_ );
				}
				else if (traversenode != end_node_)
				{
					cerr << "MvGraphMT Not Validated, outgoingedge pointed to NULL, node other than end_node_" << endl;
					valid_ = false;
					return false;
				}
			}

			//now toggle a specific module as visited
			map<string, bool>::iterator it_found = linked_nodes.lower_bound( traversenode->module_type_string );
			if (it_found != linked_nodes.end() && !(linked_nodes.key_comp()( traversenode->module_type_string,it_found->first)))
			{
				it_found->second = true;
			}
		}

		map<string, bool>::const_iterator it;
		for ( it = linked_nodes.begin(); it != linked_nodes.end(); ++it)
		{
			if (it->second == false) {
				valid_ = false;
				cerr << "MvGraphMT Not Validated, have loose modules lying around!!" << endl;
				return false;
			}
		}

		cerr << "MvGraphMT Validated!!" << endl;
		valid_ = true;
		return true;
	}
 */
