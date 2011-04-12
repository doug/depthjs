//
//  webkit_plugin_macView.m
//  webkit-plugin-mac
//
//  Created by Aaron Zinman on 2/18/11.
//  Copyright MIT Media Lab 2011. All rights reserved.
//

#import "webkit_plugin_macView.h"
#import <stdlib.h>
#import <string.h>
#import <JavaScriptCore/JavaScriptCore.h>
//#include "ocv_freenect.hpp"
#include "gesture_engine.hpp"

// PRIVATE METHODS ---------------------------------------------------------------------------------

@interface webkit_plugin_macView (JsExposed)
- (void) InitDepthJS;
- (void) ShutdownDepthJS;
- (void) CallbackTest;
@end

@interface webkit_plugin_macView (Internal)
- (id)_initWithArguments:(NSDictionary *)arguments;
- (void) ocvMainLoop;
@end


// BRIDGE BACK FROM C++ LAND -----------------------------------------------------------------------
static webkit_plugin_macView* hostPlugin = nil;

bool SendEventToBrowser(const string& _eventJson) {
  if (hostPlugin == nil) {
    return false;
  }

  BOOL success = false;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]; // Top-level pool
  id pluginContainer = [[hostPlugin pluginArguments] objectForKey:WebPlugInContainerKey];
  if (pluginContainer != nil) {
    WebView *myWebView = [[pluginContainer webFrame] webView];
    NSString *eventJson = [NSString stringWithCString:_eventJson.c_str() encoding:[NSString defaultCStringEncoding]];
    NSString *js = [NSString stringWithFormat:@"if(DepthJS && DepthJS.npBackend)DepthJS.npBackend.receiveEvent(%@)", eventJson];
    [[myWebView windowScriptObject] performSelectorOnMainThread:@selector(evaluateWebScript:) withObject:js waitUntilDone:NO];
    DLog(@"[DepthJS] Sent: %@", eventJson);
    success = true;
  } else {
    DLog(@"[DepthJS] Could not find pluginContainer?!;");
    return false;
  }
  [pool release];  // Release the objects in the pool.
  return success;
}

@implementation webkit_plugin_macView

// WebPlugInViewFactory protocol
// The principal class of the plug-in bundle must implement this protocol.

+ (NSView *)plugInViewWithArguments:(NSDictionary *)newArguments {
  NSView *v = [[[self alloc] _initWithArguments:newArguments] autorelease];
  return v;
}


/* accessors for pluginArguments */
- (NSDictionary *)pluginArguments {
  return [[pluginArguments retain] autorelease];
}

- (void)setPluginArguments:(NSDictionary *)value {
  if (pluginArguments != value) {
    [pluginArguments release];
    pluginArguments = [value copy];
    DLog(@"just set args: %@", pluginArguments);
  }
}

- (void) dealloc {
  [self setPluginArguments: nil];
  [super dealloc];
}

// WebPlugIn informal protocol

- (void)webPlugInInitialize {
  // This method will be only called once per instance of the plug-in object, and will be called
  // before any other methods in the WebPlugIn protocol.
  // You are not required to implement this method.  It may safely be removed.
  DLog(@"webPlugInInitialize");
  haveInitDevice = NO;
  ocvThread = nil;
  if (hostPlugin == nil) {
    hostPlugin = self;
  }
}

- (void)webPlugInDestroy {
  DLog(@"webPlugInDestroy");
  [self ShutdownDepthJS];
  DLog(@"webPlugInDestroy finished");
}

- (id)objectForWebScript {
  return self;
}

- (NSString *)webScriptNameForSelector:(SEL)selector {
  if(selector == @selector(InitDepthJS)) {
    return @"InitDepthJS";
  } else if (selector == @selector(ShutdownDepthJS)) {
    return @"ShutdownDepthJS";
  } else if (selector == @selector(CallbackTest)) {
    return @"CallbackTest";
  }
  return nil;
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector {
  DLog(@"isSelectorExcludedFromWebScript");
  if(selector == @selector(InitDepthJS) ||
     selector == @selector(ShutdownDepthJS) ||
     selector == @selector(CallbackTest)) {
    DLog(@"NO");
    return NO;
  }
  DLog(@"YES");
  return YES;
}

- (void)finalizeForWebScript {
  DLog(@"finalizeForWebScript");
}

@end

@implementation webkit_plugin_macView (Internal)

- (id)_initWithArguments:(NSDictionary *)newArguments {
  if (!(self = [super initWithFrame:NSZeroRect]))
    return nil;
    
  [self setPluginArguments: newArguments];
  return self;
}

- (void) ocvMainLoop {
  ocvThread = [NSThread currentThread];
  [ocvThread setName:@"ocvMainLoop"];
//  ocvFreenectThread(NULL);
	gesture_engine(NULL);
}

@end

@implementation webkit_plugin_macView (JsExposed)

NSString* jsRefToTypeString(JSContextRef& ctx, JSValueRef& t) {
  switch (JSValueGetType(ctx, t)) {
    case kJSTypeNull:
      return @"null";
    case kJSTypeUndefined:
      return @"undefined";
    case kJSTypeBoolean:
      return @"boolean";
    case kJSTypeNumber:
      return @"number";
    case kJSTypeString:
      return @"string";
    case kJSTypeObject:
      return @"object";
    default:
      return [NSString stringWithFormat:@"Unknown type: %d", t];
  }
}

- (void) CallbackTest {
  id pluginContainer = [[self pluginArguments] objectForKey:WebPlugInContainerKey];
  WebView *myWebView = [[pluginContainer webFrame] webView];
  JSObjectRef globalObj = [[myWebView windowScriptObject] JSObject];
  JSValueRef exception;
  DLog(@"starting stuff to call DepthJS.k()");
  
  // JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
  JSContextRef ctx = [[myWebView mainFrame] globalContext];
  JSValueRef depthJsRef = JSObjectGetProperty(ctx, globalObj, JSStringCreateWithCFString((CFStringRef)@"DepthJS"), &exception);;
  DLog(@"depthjs ref type %s", jsRefToTypeString(ctx, depthJsRef));
  JSObjectRef depthJsObj = JSValueToObject(ctx, depthJsRef, &exception);
  JSValueRef funcRef = JSObjectGetProperty(ctx, depthJsObj, JSStringCreateWithCFString((CFStringRef)@"k"), &exception);
  DLog(@"depthjs.k ref type %s", jsRefToTypeString(ctx, funcRef));
  JSObjectRef funcObj = JSValueToObject(ctx, funcRef, &exception);
  
  JSValueRef result = JSObjectCallAsFunction(ctx, funcObj, NULL, 0, NULL, &exception);
  if (result == NULL) {
    DLog(@"got back null");
  } else {
    DLog(@"got back type %s", jsRefToTypeString(ctx, result));
  }
  DLog(@"done");
}

- (void) InitDepthJS {
  DLog(@"[DepthJS] InitDepthJS");
  // If the windowScriptObject is not first referenced from this thread, we will later get a thread
  // exception. Even though this doesn't do anything. Yah. I have no idea either.
  id pluginContainer = [[hostPlugin pluginArguments] objectForKey:WebPlugInContainerKey];
  WebView *myWebView = [[pluginContainer webFrame] webView];
  [myWebView windowScriptObject];
  
  if (!haveInitDevice) {
    DLog(@"[DepthJS] Device not yet init; initing");
    hostPlugin = self;
    int success = init_gesture_engine();
    haveInitDevice = success;
    if (haveInitDevice) {
      DLog(@"[DepthJS] Successfully inited Kinect; Starting ocv thread");
      [NSThread detachNewThreadSelector:@selector(ocvMainLoop) toTarget:self withObject:nil];
    } else {
      DLog(@"[DepthJS] Failed to init Kinect");
      hostPlugin = nil;
    }
  } else {
    DLog(@"[DepthJS] Already init, ignoring");
  }
}

- (void) ShutdownDepthJS {
  DLog(@"[DepthJS] ShutdownDepthJS");
  haveInitDevice = false;
  if (hostPlugin == self) {
    hostPlugin = NULL;
//    killOcvFreenect();
	kill_gesture_engine();
    if (ocvThread != nil) [ocvThread cancel];
    ocvThread = nil;
    while (!is_gesture_engine_dead()) {
      [NSThread sleepForTimeInterval:0.01];
    }
  }
  DLog(@"[DepthJS] ShutdownDepthJS complete");
}

@end