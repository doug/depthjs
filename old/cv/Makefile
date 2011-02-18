CC=g++
UNAME := $(shell uname -s)

ifeq ($(UNAME), Darwin)
$(info Detected a mac -- Assuming Homebrew install of opencv, zeromq, libusb, and libfreenect)
	PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig
	CFLAGS=-c -Wall $(shell pkg-config --cflags opencv libzmq libusb-1.0) -I/usr/local/include/libusb-1.0 -I/usr/local/include/libfreenect
	LDFLAGS=$(shell pkg-config --libs opencv libzmq) -L/usr/local/lib -lfreenect
endif

ifeq ($(UNAME), Linux)
$(info Detected Linux -- Assuming 64-bit Ubuntu install of opencv, zeromq, and libusb using a local install of libfreenect in /usr/local)
	CFLAGS=-c -Wall $(shell pkg-config --cflags opencv libzmq libusb) -I/usr/include/libusb-1.0 -I/usr/local/include/libfreenect
	LDFLAGS=$(shell pkg-config --libs opencv libzmq) -L/usr/local/lib -L/usr/local/lib64 -lfreenect
endif

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
