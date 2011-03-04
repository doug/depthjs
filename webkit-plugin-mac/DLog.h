/*
 *  DebugLog.h
 *  DebugLog
 *
 *  Created by Karl Kraft on 3/22/09.
 *  Copyright 2009 Karl Kraft. All rights reserved.
 *
 */

#ifndef __OPTIMIZE__

#define DLog(args...) _DLog(__FILE__,__PRETTY_FUNCTION__,__LINE__,args);

#else

#define DLog(x...)

#endif

void _DLog(const char *file, const char *function, int lineNumber, NSString *format,...);
