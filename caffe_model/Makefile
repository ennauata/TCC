LOCAL_HOME = /usr
CAFFE= /home/nelson/caffe
SDK_HOME = /home/nelson/TCC/caffe_model

OBJ_DIR = $(SDK_HOME)/obj
SRC_DIR = $(SDK_HOME)/src

# Define C++ compiler
CCC	          = /usr/bin/g++

# Define C compiler options
CFLAGS        = -Wall -c -O2

# Define C++ compiler options
CCCFLAGS      = -Wall -c -O2 -D__STDC_CONSTANT_MACROS 

# Define Includes
CPPFLAGS =  -std=c++11
CPPFLAGS += -I$(SDK_HOME)/include		

# Include proto
CPPFLAGS += -I$(CAFFE)/build/include/caffe/proto/

# Define libraries to be linked with
LIBS	      =  -L$(LOCAL_HOME)/lib/ -lgflags -lprotobuf -lleveldb -lglog -llmdb -L/$(CAFFE)/build/lib -lcaffe

# Define linker
LINKER	      = g++

# Define all object files to be the same as CPPSRCS but with all the .cpp and .c suffixes replaced with .o
OBJ           =  $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPPSRCS))

# Define name of target executable
PROGRAM	      = $(SDK_HOME)/bin/convert_data

# Define all C++ source files here
CPPSRCS       = $(SRC_DIR)/convert_data.cpp

##############################################################################
# Define additional rules that make should know about in order to compile our
# files.                                        
##############################################################################

# Define default rule if Make is run without arguments
all : $(PROGRAM) 

# Define rule for compiling all C++ files
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	$(CCC) $(CCCFLAGS) $(CPPFLAGS) -c $< -o $@
	
# Define rule for creating executable
$(PROGRAM) :	$(OBJ)
		@echo -n "Loading $(PROGRAM) ... "
		$(LINKER) -o $(PROGRAM) $(OBJ) $(LIBS) 
		@echo "done"

# Define rule to clean up directory by removing all object, temp and core
# files along with the executable
clean :
	@rm -f $(OBJ) *~ $(PROGRAM)
