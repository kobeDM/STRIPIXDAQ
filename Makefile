# CPPFLAGS += `root-config --cflags`
# LDLIBS   += `root-config --libs`

ROOTFLAGS= -pthread -Wno-deprecated-declarations -m64 -I${ROOTSYS}/include -I./ShUtil
CXXFLAGS=$(ROOTFLAGS) -std=c++11
CXXFLAGS += $(shell root-config --cflags)
ROOTLIBS= $(shell root-config --libs)

all: image-stripix-ver3argpara drawWaveform

image-stripix-ver3argpara: image-stripix-ver3argpara.o ShUtil/src/ShStyle.o
	g++ $(CXXFLAGS) -o image-stripix-ver3argpara image-stripix-ver3argpara.o ShUtil/src/ShStyle.o $(ROOTLIBS) -lgcc


drawWaveform: drawWaveform.o ShUtil/src/ShStyle.o
	g++ $(CXXFLAGS) -o drawWaveform drawWaveform.o ShUtil/src/ShStyle.o $(ROOTLIBS) -lgcc


%.o: %.C
	g++ -Wall $(ROOTLIBS) -I. -I${ROOTSYS}/include -I./ShUtil -O4 -c -o $@ $<

%.o: %.cc
	g++ -Wall $(ROOTLIBS) -I. -I${ROOTSYS}/include -I./ShUtil -O4 -c -o $@ $<

clean::
	rm -f image-stripix-ver3argpara drawWaveform *.o ShUtil/src/*.o
