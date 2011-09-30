/*
 *  openni_backend.h
 *  webkit-plugin-mac
 *
 *  Created by Roy Shilkrot on 9/30/11.
 *  Copyright 2011 MIT. All rights reserved.
 *
 */

#ifndef _OPENNI_BACKEND_HPP
#define _OPENNI_BACKEND_HPP

int openni_backend(void* _arg);
void kill_openni_backend();
bool is_openni_backend_dead();
int init_openni_backend();

#endif