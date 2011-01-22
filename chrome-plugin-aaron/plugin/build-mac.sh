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
  -L/usr/local/lib -lfreenect \
  -Wall \
  -o depthjs.plugin/Contents/MacOS/depthjs \
   np_entry.cc npp_entry.cc plugin.cc depthjs.cc depthjs.mm
