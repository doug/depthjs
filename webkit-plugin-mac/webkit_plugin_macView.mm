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
#include "ocv_freenect.hpp"

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
static volatile webkit_plugin_macView* hostPlugin = nil;

bool SendEventToBrowser(const string& _eventJson) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]; // Top-level pool
  
  NSString *eventJson = [NSString stringWithCString:_eventJson.c_str() encoding:[NSString defaultCStringEncoding]];
  DLog(@"Going to send the following eventJson nsstring: %@", eventJson);
  

  if (hostPlugin == nil) {
    DLog(@"DepthJS Plugin: Ignoring event for uninit host");
    return false;
  }
  
  id pluginContainer = [[hostPlugin pluginArguments] objectForKey:WebPlugInContainerKey];
  if (pluginContainer) {
    /* retrieve a reference to the webview */
    WebView *myWebView = [[pluginContainer webFrame] webView];
    NSString *js = [NSString stringWithFormat:@"if(DepthJS && DepthJS.npBackend)DepthJS.npBackend.receiveEvent(%@)", eventJson];
    
    [[myWebView windowScriptObject] performSelectorOnMainThread:@selector(evaluateWebScript:) withObject:js waitUntilDone:YES];
    // [[myWebView windowScriptObject] evaluateWebScript:js];
    DLog(@"Sent to javascript");
    [pool release];  // Release the objects in the pool.
    return true;
  } else {
    DLog(@"Could not find pluginContainer?!;");
    [pool release];  // Release the objects in the pool.
    return false;
  }
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
  hostPlugin = self; // TODO remove later after testing
}

- (void)webPlugInStart {
  // The plug-in usually begins drawing, playing sounds and/or animation in this method.
  // You are not required to implement this method.  It may safely be removed.
  DLog(@"webPlugInStart");
}

- (void)webPlugInStop {
  // The plug-in normally stop animations/sounds in this method.
  // You are not required to implement this method.  It may safely be removed.
  DLog(@"webPlugInStop; killing ocv");
  killOcvFreenect();
}

- (void)webPlugInDestroy {
  DLog(@"webPlugInDestroy");
  haveInitDevice = FALSE;
}

- (void)webPlugInSetIsSelected:(BOOL)isSelected {
  // This is typically used to allow the plug-in to alter its appearance when selected.
  // You are not required to implement this method.  It may safely be removed.
  DLog(@"webPlugInSetIsSelected");
}

- (id)objectForWebScript {
  // Returns the object that exposes the plug-in's interface.  The class of this object can implement
  // methods from the WebScripting informal protocol.
  // You are not required to implement this method.  It may safely be removed.
  DLog(@"objectForWebScript");
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
  ocvFreenectThread(NULL);
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


// This isn't really used... but it shows how the JavaScriptCore framework can be used if needed.
- (void) CallbackTest {
  DLog(@"CallbackTest");
  id pluginContainer = [[hostPlugin pluginArguments] objectForKey:WebPlugInContainerKey];
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
  DLog(@"DepthJS Plugin: InitDepthJS");
  
  if (!haveInitDevice) {
    DLog(@"DepthJS Plugin: Device not yet init; initing");
    int failed = initFreenect();
    haveInitDevice = !failed;
    if (haveInitDevice) {
      DLog(@"DepthJS Plugin: Successfully inited Kinect; Starting ocv thread");
      hostPlugin = self;
      [NSThread detachNewThreadSelector:@selector(ocvMainLoop) toTarget:self withObject:nil];
    } else {
      DLog(@"DepthJS Plugin: Failed to init Kinect");
    }
  } else {
    DLog(@"DepthJS Plugin: Already init, ignoring");
  }
}

- (void) ShutdownDepthJS {
  DLog(@"DepthJS Plugin: ShutdownDepthJS");
  killOcvFreenect();
  DLog(@"DepthJS Plugin: ShutdownDepthJS complete");
  haveInitDevice = false;
  if (hostPlugin == self) {
    hostPlugin = NULL;
  }
}

@end