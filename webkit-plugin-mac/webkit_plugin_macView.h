//
//  webkit_plugin_macView.h
//  webkit-plugin-mac
//
//  Created by Aaron Zinman on 2/18/11.
//  Copyright MIT Media Lab 2011. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface webkit_plugin_macView : NSView <WebPlugInViewFactory>
{
  BOOL haveInitDevice;
  NSDictionary *pluginArguments;
}
- (NSDictionary *)pluginArguments;
- (void)setPluginArguments:(NSDictionary *)value;
  
- (void) InitDepthJS;
- (void) ShutdownDepthJS;
          
@end
