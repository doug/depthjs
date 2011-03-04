/*
 *  DebugLog.m
 *  DebugLog
 *
 *  Created by Karl Kraft on 3/22/09.
 *  Copyright 2009 Karl Kraft. All rights reserved.
 *
 */

#include "DLog.h"

//void _DebugLog(const char *function, int lineNumber, NSString *format,...) {
//  va_list ap;
//	
//  va_start (ap, format);
//  if (![format hasSuffix: @"\n"]) {
//    format = [format stringByAppendingString: @"\n"];
//	}
//	NSString *body =  [[NSString alloc] initWithFormat: format arguments: ap];
//	va_end (ap);
//	NSString *fileName=[[NSString stringWithUTF8String:file] lastPathComponent];
//	fprintf(stderr,"%s:%d %s",[fileName UTF8String],lineNumber,[body UTF8String]);
//	[body release];	
//}


void _DLog(const char *file, const char *function, int lineNumber, NSString *format,...){
  va_list args;
  va_start(args, format);

  NSString *body = [[NSString alloc] initWithFormat:format arguments:args];
  //NSString *filename = [[NSString stringWithFormat:@"%s",file] lastPathComponent];
	NSString *logLine = [[NSString alloc] initWithFormat:@"%s %@\n", function, body];
  va_end(args);

  [[NSFileHandle fileHandleWithStandardOutput] writeData: [logLine dataUsingEncoding: NSNEXTSTEPStringEncoding]];
  [logLine release]; 
  [body release]; 
}
