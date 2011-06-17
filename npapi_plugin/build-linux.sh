#!/bin/sh
echo "Assuming homebrew install of libusb and libfreenect"
mkdir -p depthjs.plugin/Contents/MacOS
cp -f Info.plist depthjs.plugin/Contents
g++ -Wno-write-strings -lresolv \
  -I/usr/local/include \
  -I/usr/local/include/libusb-1.0 -I/usr/local/include/libfreenect \
  `pkg-config --cflags libusb-1.0` \
  `pkg-config --cflags opencv` \
  -lz \
  -Wall \
  -o depthjs \
   np_entry.cc npp_entry.cc plugin.cc depthjs.cc \
   ocv_freenect.cpp bg_fg_blobs.cpp
