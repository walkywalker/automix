VPATH = src
CXXFLAGS	:= $(CXXFLAGS) -std=c++17 -g -I$(VPATH) -Ilib -Ilib/qm-dsp -Wunused-variable

src = $(wildcard src/*.cpp) $(wildcard src/qm/*.cpp)
obj = $(src:.cpp=.o)
pugixml_object = lib/pugixml/build/make-g++-debug-standard-c++11/src/pugixml.cpp.o

LDFLAGS = -lavutil -lpthread -lavformat -lavcodec

automix: $(obj) $(pugixml_object) fidlib.o qm-dsp libsamplerate vamp-plugin-sdk
	echo $(obj)
	$(CXX) -o $@ $(obj) $(pugixml_object) fidlib.o lib/qm-dsp/libqm-dsp.a lib/libsamplerate/build/src/libsamplerate.a lib/vamp-plugin-sdk/libvamp-sdk.a $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) automix

.PHONY: $(pugixml_object)
$(pugixml_object):
	$(MAKE) -C lib/pugixml

.PHONY: fidlib.o
fidlib.o:
	$(CC) -c lib/fidlib/fidlib.c -DT_LINUX -o fidlib.o -lm

.PHONY: qm-dsp
qm-dsp:
	cd lib/qm-dsp; $(MAKE) -f build/linux/Makefile.linux64

.PHONY: libsamplerate
libsamplerate:
	cd lib/libsamplerate; mkdir build; cd build; cmake ..;  make

.PHONY: vamp-plugin-sdk	# would be nice to get rid
vamp-plugin-sdk:
	cd lib/vamp-plugin-sdk; ./configure --disable-programs > /dev/null; make
