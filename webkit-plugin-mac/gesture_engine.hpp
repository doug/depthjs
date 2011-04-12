/*
 DepthJS
 Copyright (C) 2010 Aaron Zinman, Doug Fritz, Roy Shilkrot, Greg Elliott
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.
 
 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *  gesture_engine.hpp
 *  webkit-plugin-mac
 *
 *  Created by Roy Shilkrot on 3/6/11.
 *
 */

#ifndef _GESTURE_ENGINE_HPP
#define _GESTURE_ENGINE_HPP


int gesture_engine(void* _arg);
void kill_gesture_engine();
bool is_gesture_engine_dead();
int init_gesture_engine();

#endif