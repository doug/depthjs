#!/bin/sh
echo "Assuming homebrew install of libusb and libfreenect"
mkdir -p depthjs.plugin/Contents/MacOS
cp -f Info.plist depthjs.plugin/Contents
g++ -framework Cocoa -DMAC -DXP_MACOSX -framework Accelerate -framework IOKit \
  -DWEBKIT_DARWIN_SDK -Wno-write-strings -lresolv -arch i386 -bundle \
  -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 \
  -I/usr/local/include \
  -I/usr/local/include/libusb-1.0 -I/usr/local/include/libfreenect \
  `pkg-config --cflags libusb-1.0` \
  `pkg-config --cflags opencv` \
  "mac_native_libs/libfreenect_sync.a" "mac_native_libs/libfreenect.a" "mac_native_libs/libusb-1.0.a" "mac_native_libs/opencv/libopencv_calib3d_pch_dephelp.a" "mac_native_libs/opencv/libopencv_calib3d.a" "mac_native_libs/opencv/libopencv_contrib_pch_dephelp.a" "mac_native_libs/opencv/libopencv_contrib.a" "mac_native_libs/opencv/libopencv_core_pch_dephelp.a" "mac_native_libs/opencv/libopencv_core.a" "mac_native_libs/opencv/libopencv_features2d_pch_dephelp.a" "mac_native_libs/opencv/libopencv_features2d.a" "mac_native_libs/opencv/libopencv_flann_pch_dephelp.a" "mac_native_libs/opencv/libopencv_flann.a" "mac_native_libs/opencv/libopencv_gpu_pch_dephelp.a" "mac_native_libs/opencv/libopencv_gpu.a" "mac_native_libs/opencv/libopencv_haartraining_engine.a" "mac_native_libs/opencv/libopencv_highgui_pch_dephelp.a" "mac_native_libs/opencv/libopencv_highgui.a" "mac_native_libs/opencv/libopencv_imgproc_pch_dephelp.a" "mac_native_libs/opencv/libopencv_imgproc.a" "mac_native_libs/opencv/libopencv_legacy_pch_dephelp.a" "mac_native_libs/opencv/libopencv_legacy.a" "mac_native_libs/opencv/libopencv_ml_pch_dephelp.a" "mac_native_libs/opencv/libopencv_ml.a" "mac_native_libs/opencv/libopencv_objdetect_pch_dephelp.a" "mac_native_libs/opencv/libopencv_objdetect.a" "mac_native_libs/opencv/libopencv_video_pch_dephelp.a" "mac_native_libs/opencv/libopencv_video.a" \
  -lz \
  -Wall \
  -o depthjs.plugin/Contents/MacOS/depthjs \
   np_entry.cc npp_entry.cc plugin.cc depthjs.cc depthjs.mm \
   ocv_freenect.cpp bg_fg_blobs.cpp
