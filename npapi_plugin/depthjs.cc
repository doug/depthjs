/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
* ***** END LICENSE BLOCK ***** */

#include <string>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <pthread.h>
#include "plugin.h"
#include "depthjs.h"
#include "libfreenect.hpp"
#include "ocv_freenect.h"

#ifdef _WINDOWS
#include <atlenc.h>
#include <ShlObj.h>
#include <io.h>
#include <GdiPlus.h>
using namespace Gdiplus;
#define snprintf sprintf_s
#elif defined GTK
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <unistd.h>
#elif defined __APPLE__
#include <resolv.h>
#endif

class CPlugin;

class Mutex {
public:
  Mutex() {
    pthread_mutex_init( &m_mutex, NULL );
  }
  void lock() {
    pthread_mutex_lock( &m_mutex );
  }
  void unlock() {
    pthread_mutex_unlock( &m_mutex );
  }
private:
  pthread_mutex_t m_mutex;
};

class DepthJSDevice : public Freenect::FreenectDevice {
public:
  DepthJSDevice(freenect_context *_ctx, int _index)
    : Freenect::FreenectDevice(_ctx, _index), m_buffer_depth(freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT).bytes),m_buffer_video(freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB).bytes), m_gamma(2048), m_new_rgb_frame(false), m_new_depth_frame(false)
  {
    for( unsigned int i = 0 ; i < 2048 ; i++) {
      float v = i/2048.0;
      v = std::pow(v, 3)* 6;
      m_gamma[i] = v*6*256;
    }
  }
  //~MyFreenectDevice(){}
  // Do not call directly even in child
  void VideoCallback(void* _rgb, uint32_t timestamp) {
    m_rgb_mutex.lock();
    uint8_t* rgb = static_cast<uint8_t*>(_rgb);
    std::copy(rgb, rgb+getVideoBufferSize(), m_buffer_video.begin());
    m_new_rgb_frame = true;
    m_rgb_mutex.unlock();
  };
  // Do not call directly even in child
  void DepthCallback(void* _depth, uint32_t timestamp) {
    m_depth_mutex.lock();
    uint16_t* depth = static_cast<uint16_t*>(_depth);
    std::copy(depth, depth+getDepthBufferSize(), m_buffer_depth.begin());
    m_new_depth_frame = true;
    m_depth_mutex.unlock();
  }

  bool getRGB(std::vector<uint8_t> &buffer) {
    m_rgb_mutex.lock();
    if(m_new_rgb_frame) {
      buffer.swap(m_buffer_video);
      m_new_rgb_frame = false;
      m_rgb_mutex.unlock();
      return true;
    } else {
      m_rgb_mutex.unlock();
      return false;
    }
  }

  bool getDepth(std::vector<uint16_t> &buffer) {
    m_depth_mutex.lock();
    if(m_new_depth_frame) {
      buffer.swap(m_buffer_depth);
      m_new_depth_frame = false;
      m_depth_mutex.unlock();
      return true;
    } else {
      m_depth_mutex.unlock();
      return false;
    }
  }

private:
  std::vector<uint16_t> m_buffer_depth;
  std::vector<uint8_t> m_buffer_video;
  std::vector<uint16_t> m_gamma;
  Mutex m_rgb_mutex;
  Mutex m_depth_mutex;
  bool m_new_rgb_frame;
  bool m_new_depth_frame;
};

static Freenect::Freenect* libfreenect = NULL;
static DepthJSDevice* device = NULL;
static volatile bool haveInitDevice = false;
static volatile ScriptablePluginObject* pluginHost = NULL;

/*
static void InvokeCallback(NPP npp, NPObject* callback, const char* param) {
  NPVariant npParam;
  STRINGZ_TO_NPVARIANT(param, npParam);
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  npnfuncs->invokeDefault(npp, callback, &npParam, 1, &result);
}

static void InvokeCallback(NPP npp, NPObject* callback, bool param) {
  NPVariant npParam;
  BOOLEAN_TO_NPVARIANT(param, npParam);
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  npnfuncs->invokeDefault(npp, callback, &npParam, 1, &result);
}
*/

static int instance_count = 0;

static bool setupDevice() {
  std::cout << "DepthJS Plugin: Initalizing libfreenect singleton" << "\n";
  libfreenect = new Freenect::Freenect();
  int deviceCount = libfreenect->deviceCount();
  std::cout << "DepthJS Plugin: Found " << deviceCount << " devices" << "\n";
  if (deviceCount > 0) {
    std::cout << "DepthJS Plugin: Connecting to first device" << "\n";
    device = &(libfreenect->createDevice<DepthJSDevice>(0));
    device->startVideo();
    device->startDepth();
    device->setLed(LED_GREEN);
    std::cout << "DepthJS Plugin: Setup device & started video & depth capture" << "\n";
    return true;
  } else {
    std::cout << "DepthJS Plugin: No devices found, shutting libfreenect." << "\n";
    delete libfreenect;
    libfreenect = NULL;
    return false;
  }
}

static void _sendEventInBrowserThread(void *data) {
  std::cout << "DepthJS Plugin: In browser thread" << "\n";
  char* eventJson = static_cast<char*>(data);

  if (pluginHost == NULL) {
    std::cerr << "DepthJS Plugin: Ignoring event for uninit host: " << eventJson << "\n";
    free(eventJson);
    return;
  }

  if (npnfuncs == NULL) {
    std::cerr << "DepthJS Plugin: Somehow npnfuncs is NULL!?" << "\n";
    free(eventJson);
    return;
  }

  stringstream ss;
  ss << "javascript:if(DepthJS && DepthJS.npBackend)DepthJS.npBackend.receiveEvent(" << eventJson << ")";
  std::cout << "DepthJS Plugin [browser thread]: Calling " << ss.str() << "\n";
  npnfuncs->geturl(pluginHost->npp, ss.str().c_str(), NULL);
  free(eventJson);
}

bool SendEventToBrowser(const string& eventJson) {
  if (pluginHost == NULL) {
    std::cerr << "DepthJS Plugin: Ignoring event for uninit host: " << eventJson << "\n";
    return false;
  }

  if (npnfuncs == NULL) {
    std::cerr << "DepthJS Plugin: Somehow npnfuncs is NULL!?" << "\n";
    return false;
  }

  char *eventJsonCStr = static_cast<char*>(malloc(sizeof(char) * (eventJson.length() + 1)));
  size_t length = eventJson.copy(eventJsonCStr, eventJson.length(), 0);
  eventJsonCStr[length] = '\0';

  npnfuncs->pluginthreadasynccall(
      pluginHost->npp,
      _sendEventInBrowserThread,
      eventJsonCStr);

  return true;
}

bool InitDepthJS(ScriptablePluginObject* obj, const NPVariant* args,
          unsigned int argCount, NPVariant* result) {
  std::cout << "DepthJS Plugin: InitDepthJS" << "\n";
  instance_count++;

  if (!haveInitDevice) { //libfreenect == NULL) {
    std::cout << "DepthJS Plugin: Device not yet init; initing" << "\n";
    haveInitDevice = launchOcvFreenect() == 0; // setupDevice();
    if (haveInitDevice) {
      std::cout << "DepthJS Plugin: Successfully inited Kinect" << "\n";
      pluginHost = obj;
    } else {
      std::cerr << "DepthJS Plugin: Failed to init Kinect" << "\n";
    }
  } else {
    std::cout << "DepthJS Plugin: Already init, ignoring" << "\n";
  }
  BOOLEAN_TO_NPVARIANT(haveInitDevice, *result);
  return true;
}

bool GetDepth(ScriptablePluginObject* obj, const NPVariant* args,
              unsigned int argCount, NPVariant* result) {
  static std::vector<uint8_t> depth(640*480*4);
  return true;
}

bool GetRGB(ScriptablePluginObject* obj, const NPVariant* args,
            unsigned int argCount, NPVariant* result) {
  static std::vector<uint8_t> rgb(640*480*4);
  return true;
}

void ShutdownDepthJS() {
  std::cout << "DepthJS Plugin: ShutdownDepthJS" << "\n";
  /*
  if (device != NULL) {
    std::cout << "DepthJS Plugin: removing device" << "\n";
    device->setLed(LED_BLINK_GREEN);
    device->stopVideo();
    device->stopDepth();
    std::cout << "DepthJS Plugin: stopped video & depth" << "\n";
    libfreenect->deleteDevice(0); // this will call its destructor
    device = NULL;
  }
  if (libfreenect != NULL) {
    std::cout << "DepthJS Plugin: removing libfreenect" << "\n";
    delete libfreenect;
    libfreenect = NULL;
  }
  */
  killOcvFreenect();
  std::cout << "DepthJS Plugin: ShutdownDepthJS complete" << "\n";
  haveInitDevice = false;
  pluginHost = NULL;
}

bool ShutdownDepthJS(ScriptablePluginObject* obj, const NPVariant* args,
          unsigned int argCount, NPVariant* result) {
  ShutdownDepthJS();
  BOOLEAN_TO_NPVARIANT(true, *result);
  return true;
}
