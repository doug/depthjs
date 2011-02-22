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
#include "ocv_freenect.hpp"

static volatile bool haveInitDevice = false;

static volatile webkit_plugin_macView* hostPlugin = nil;

bool SendEventToBrowser(const string& _eventJson) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]; // Top-level pool
  
  NSString *eventJson = [NSString stringWithCString:_eventJson.c_str() encoding:[NSString defaultCStringEncoding]];
  NSLog(@"Going to send the following eventJson nsstring: %@", eventJson);
  

  if (hostPlugin == nil) {
    NSLog(@"DepthJS Plugin: Ignoring event for uninit host");
    return false;
  }
  
  id pluginContainer = [[hostPlugin pluginArguments] objectForKey:WebPlugInContainerKey];
  if (pluginContainer) {
    /* retrieve a reference to the webview */
    WebView *myWebView = [[pluginContainer webFrame] webView];
    NSString *js = [NSString stringWithFormat:@"if(DepthJS && DepthJS.npBackend)DepthJS.npBackend.receiveEvent(%@)", eventJson];
    // [[myWebView windowScriptObject] evaluateWebScript:js];
    NSLog(@"Sent to javascript");
    [pool release];  // Release the objects in the pool.
    return true;
  } else {
    NSLog(@"Could not find pluginContainer?!;");
    [pool release];  // Release the objects in the pool.
    return false;
  }
}


@interface webkit_plugin_macView (Internal)
- (id)_initWithArguments:(NSDictionary *)arguments;
@end

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
  NSLog(@"webPlugInInitialize");
  haveInitDevice = NO;
}

- (void)webPlugInStart {
  // The plug-in usually begins drawing, playing sounds and/or animation in this method.
  // You are not required to implement this method.  It may safely be removed.
  NSLog(@"webPlugInStart");
}

- (void)webPlugInStop {
  // The plug-in normally stop animations/sounds in this method.
  // You are not required to implement this method.  It may safely be removed.
  NSLog(@"webPlugInStop");
}

- (void)webPlugInDestroy {
  // Perform cleanup and prepare to be deallocated.
  // You are not required to implement this method.  It may safely be removed.
  NSLog(@"webPlugInDestroy");
  haveInitDevice = FALSE;
  
}

- (void)webPlugInSetIsSelected:(BOOL)isSelected {
  // This is typically used to allow the plug-in to alter its appearance when selected.
  // You are not required to implement this method.  It may safely be removed.
  NSLog(@"webPlugInSetIsSelected");
}

- (id)objectForWebScript {
  // Returns the object that exposes the plug-in's interface.  The class of this object can implement
  // methods from the WebScripting informal protocol.
  // You are not required to implement this method.  It may safely be removed.
  NSLog(@"objectForWebScript");
  return self;
}

- (NSString *)webScriptNameForSelector:(SEL)selector {
  if(selector == @selector(InitDepthJS)) {
    return @"InitDepthJS";
  } else if (selector == @selector(ShutdownDepthJS)) {
    return @"ShutdownDepthJS";
  }
  return nil;
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector {
  NSLog(@"isSelectorExcludedFromWebScript");
  if(selector == @selector(InitDepthJS) ||
     selector == @selector(ShutdownDepthJS)) {
    NSLog(@"NO");
    return NO;
  }
  NSLog(@"YES");
  return YES;
}

- (void)finalizeForWebScript {
  NSLog(@"finalizeForWebScript");
}

- (void) InitDepthJS {
  NSLog(@"DepthJS Plugin: InitDepthJS");
  
  if (!haveInitDevice) {
    NSLog(@"DepthJS Plugin: Device not yet init; initing");
    haveInitDevice = launchOcvFreenect() == 0;
    if (haveInitDevice) {
      NSLog(@"DepthJS Plugin: Successfully inited Kinect");
      hostPlugin = self;
    } else {
      NSLog(@"DepthJS Plugin: Failed to init Kinect");
    }
  } else {
    NSLog(@"DepthJS Plugin: Already init, ignoring");
  }
}

- (void) ShutdownDepthJS {
  NSLog(@"DepthJS Plugin: ShutdownDepthJS");
  killOcvFreenect();
  NSLog(@"DepthJS Plugin: ShutdownDepthJS complete");
  haveInitDevice = false;
  if (hostPlugin == self) {
    hostPlugin = NULL;
  }
}

@end

@implementation webkit_plugin_macView (Internal)

- (id)_initWithArguments:(NSDictionary *)newArguments {
  if (!(self = [super initWithFrame:NSZeroRect]))
    return nil;
    
  [self setPluginArguments: newArguments];
  return self;
}

@end
