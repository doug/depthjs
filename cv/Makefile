CC=g++
CFLAGS=-c -Wall $(shell pkg-config --cflags opencv) -I../../libfreenect/c/include -I../../libusb-7da756e/libusb/ $(shell pkg-config --cflags libzmq)
LDFLAGS=$(shell pkg-config --libs opencv libzmq) -L../../libfreenect/c/lib -lfreenect
SOURCES=ocv_freenect.cpp bg_fg_blobs.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ocv_freenect

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	@rm -rf $(EXECUTABLE)
