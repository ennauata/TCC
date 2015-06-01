#ifndef _CL_UTIL_H_
#define _CL_UTIL_H_


#include "stdio.h"
#include <vector>
#include <cstdlib>
#include <stdint.h>
#include <cfloat>
#include "assert.h"


//if we are compiling a mex file, use mxASSERT instead of the default cASSERT
#ifdef MATLAB_MEX_FILE

	#include "mex_util.h"

	inline void clASSERT(bool b, const char* msg){
		mxASSERT(b, msg);
	}

	inline void clPrintf( char* msg ){
		mexPrintf( msg );
		mexEvalString("drawnow;");
	}

#else
	inline void clASSERT(bool b, const char* msg){

		if (!b){
			char errmsg[1000];
			sprintf( errmsg, "\nAssertion Failed:%s\n", msg);
			printf( "%s", errmsg );
			exit(-1);
		}
	}

	inline void clPrintf( char* msg ){
		printf( "%s", msg );
	}

	//to make sure error code returned by cuda function calls are 0
	#ifndef clCheckErr
	#define clCheckErr( err ) \
		{ int my_err_code = (err); \
		if ( my_err_code != 0){ \
			char errmsg[1000];\
			sprintf(errmsg, "\n@@@@@\nCUDA function call failed![%d] %s:%d\n@@@@@\n", my_err_code, __FILE__, __LINE__);\
			clPrintf( errmsg ); \
			exit(-1); \
		} }
	#endif
#endif

// short form for clCheckErr
#define cr( err ) \
	clCheckErr(err)


#endif
