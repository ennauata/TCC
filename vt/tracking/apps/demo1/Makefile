MATLAB_HOME = /pkgs/matlab-7.14
LOCAL_HOME = /usr/local
SDK_HOME = /Users/nelson/Desktop/WorkSpaces/TCCSpace/vt/tracking

# Define C compiler
CC            = gcc

# Define C++ compiler
CCC	          = /usr/bin/g++

# Define C compiler options
CFLAGS        = -Wall -c -O2

# Define C++ compiler options
CCCFLAGS      = -Wall -c -O2 -D__STDC_CONSTANT_MACROS 

# Define Includes
CPPFLAGS =  -std=c++11
CPPFLAGS += -I$(SDK_HOME)/include				# Include graph nodes and algorithms 
CPPFLAGS += -I$(LOCAL_HOME)/include  			# Include OpenCv,Boost ...
CPPFLAGS += -I$(LOCAL_HOME)/eigen3.2			# Include Eigen
CPPFLAGS += -I$(MATLAB_HOME)/extern/include		# Include Matlab
				
# Define location of OpenGL and GLU libraries along with their lib names
GL_LIBS       = -L/System/Library/Frameworks/ -L$(LOCAL_HOME)/lib/ -framework GLUT -framework openGL \
                               -lboost_thread-mt -lboost_system-mt -framework Accelerate


# Define location of X Windows libraries,  and the X11 library names
XLIBS         = -L/usr/X11R6/lib -L/usr/lib -lX11 


# Define the location of the destination directory for the executable file
DEST	      = .

# Define flags that should be passed to the linker
LDFLAGS	      =

# Define libraries to be linked with
LIBS	      = $(GL_LIBS) -lm $(XLIBS) -ldl
LIBS 		  += -L$(LOCAL_HOME)/lib/ -Wl,-rpath,$(LOCAL_HOME)/lib/ \
				-L$(SDK_HOME)/lib/ -Wl,-rpath,$(SDK_HOME)/lib/ \
				-lclvt -lpbas -lbsp -llbsp -lsubs -lavformat -lavcodec -lavutil -lavdevice -lavfilter -lpthread -lz -lbz2 -lm \
				-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_objdetect -lopencv_video \
				-lfftw3 -lboost_thread-mt -lbgs
								 
				#-L$(MATLAB_HOME)/bin/glnxa64 -Wl,-rpath,$(MATLAB_HOME)/bin/glnxa64 \
				# -lmx -lmex -lmat

# Define linker
LINKER	      = g++

# Define all object files to be the same as CPPSRCS but with all the .cpp and .c suffixes replaced with .o
OBJ           = $(CPPSRCS:.cpp=.o) $(CSRCS:.c=.o)

# Define all C source files here
#CSRCS         =  ee.c

# Define name of target executable
PROGRAM	      = demo1

# Define all C++ source files here
CPPSRCS       = demo1.cpp

##############################################################################
# Define additional rules that make should know about in order to compile our
# files.                                        
##############################################################################

# Define default rule if Make is run without arguments
all : $(PROGRAM) 

# Define rule for compiling all C++ files
%.o : %.cpp graphnet.h
	$(CCC) $(CCCFLAGS) $(CPPFLAGS) $*.cpp
	
# Define rule for compiling all C files
%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $*.c
	
# Define rule for creating executable
$(PROGRAM) :	$(OBJ)
		@echo -n "Loading $(PROGRAM) ... "
		$(LINKER) -o $(PROGRAM) $(OBJ) $(LIBS) 
		@echo "done"


		
# Define rule to clean up directory by removing all object, temp and core
# files along with the executable
clean :
	@rm -f $(OBJ) *~ $(PROGRAM)
