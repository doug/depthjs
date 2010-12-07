CC=g++
CFLAGS=-c -Wall $(shell pkg-config --cflags opencv libzmq libusb) -I/usr/include/libusb-1.0 -I/usr/local/include/libfreenect
LDFLAGS=$(shell pkg-config --libs opencv libzmq) -L/usr/local/lib -L/usr/local/lib64 -lfreenect
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
	@rm -rf *.o
