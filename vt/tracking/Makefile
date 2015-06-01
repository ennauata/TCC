LOCAL_HOME = /usr/local
SDK_HOME = /Users/nelson/Desktop/WorkSpaces/TCCSpace/vt/tracking

CC = g++
CPPFLAGS = -fPIC -c -Wall -D_GNU_SOURCE -pthread -fexceptions -std=c++11
COPTIMFLAGS += -O3 -funroll-loops -msse2
LDFLAGS = -shared  												#used with g++

INCHEADER1 = $(LOCAL_HOME)/eigen3.2								# Include Eigen
INCHEADER2 = $(LOCAL_HOME)/include 								# Include Opencv, Boost ...
INCHEADER3 = $(SDK_HOME)/include 								# Include Graph nodes and algorithms
INCALLHEADER = -I$(INCHEADER1) -I$(INCHEADER2) -I$(INCHEADER3)

SRCDIR = $(SDK_HOME)/src
OBJDIR = $(SDK_HOME)/obj
LIBDIR = $(SDK_HOME)/lib
BINDIR = $(SDK_HOME)/bin

INCLIBS += -L$(LOCAL_HOME)/lib -Wl,-rpath,$(LOCAL_HOME)/lib		
LIBS += -lopencv_core -lopencv_imgproc  -lopencv_highgui -lboost_thread-mt -lboost_system-mt -lopencv_features2d -lopencv_video

SRC = BackgroundSubtractorSuBSENSE.cpp
	
LIBSOURCES = $(patsubst %.cpp, $(SRCDIR)/%.cpp, $(SRC))
LIBOBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))

all: objs subs
	#ln -s $(LIBDIR)/libsubs.so $(LIBDIR)/libsubs.so
	
objs: $(LIBSOURCES) 
	$(CC) $(CPPFLAGS) $(INCALLHEADER) -o $(LIBOBJS) $(LIBSOURCES) 

subs: $(LIBOBJS)
	$(CC) $(LDFLAGS) -o $(LIBDIR)/libsubs.so $(LIBOBJS) $(INCLIBS) $(LIBS) $(LIBDIR)/libbsp.so $(LIBDIR)/liblbsp.so
	
install:

clean :	
	#rm -f $(OBJDIR)/*.o
	#rm -f $(LIBDIR)/*.so
