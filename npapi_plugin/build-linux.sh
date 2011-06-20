#!/bin/sh
echo "Assuming homebrew install of libusb and libfreenect"
g++ -fPIC -shared -Wno-write-strings -lresolv \
  -I/usr/local/include \
  -I/usr/local/include/libusb-1.0 -I/usr/local/include/libfreenect \
  `pkg-config --cflags libusb-1.0` \
  `pkg-config --cflags opencv` \
  `pkg-config --libs --static opencv` \
  -L/usr/local/lib \
  -lfreenect \
  -lz \
  -Wall \
  -o depthjs \
   np_entry.cc npp_entry.cc plugin.cc depthjs.cc \
   ocv_freenect.cpp bg_fg_blobs.cpp
