#!/bin/sh
echo "Assuming homebrew install of libusb and libfreenect"
mkdir -p depthjs.plugin/Contents/MacOS
cp -f Info.plist depthjs.plugin/Contents
g++ -framework Cocoa -DMAC -DXP_MACOSX \
  -DWEBKIT_DARWIN_SDK -Wno-write-strings -lresolv -arch i386 -bundle \
  -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 \
  -I/usr/local/include \
  -I/usr/local/include/libusb-1.0 -I/usr/local/include/libfreenect \
  `pkg-config --cflags libusb-1.0` \
  `pkg-config --cflags opencv` \
  -Lmac_native_libs \
  -Lmac_native_libs/opencv \
  -lfreenect \
  -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann \
  -Wall \
  -o depthjs.plugin/Contents/MacOS/depthjs \
   np_entry.cc npp_entry.cc plugin.cc depthjs.cc depthjs.mm \
   ocv_freenect.cpp bg_fg_blobs.cpp
