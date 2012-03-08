//
//  webkit_plugin_macView.h
//  webkit-plugin-mac
//
//  Created by Aaron Zinman on 2/18/11.
//  Copyright MIT Media Lab 2011. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <WebKit/WebPlugInViewFactory.h>

@interface webkit_plugin_macView : NSView <WebPlugInViewFactory>
{
  NSDictionary *pluginArguments;
  BOOL haveInitDevice;
  NSThread *ocvThread;
}
- (NSDictionary *)pluginArguments;
- (void)setPluginArguments:(NSDictionary *)value;

@end
