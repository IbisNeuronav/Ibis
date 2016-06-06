/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __TrackerFlags_h_
#define __TrackerFlags_h_

// Description:
// This file contains the definition of Tracker related enums
// so that it is not needed to include tracker.h in other 
// header files.
enum ToolType{ None = -1, Passive = 0, Active = 1 };
enum ToolUse{ Reference = 0, Pointer = 1, UsProbe = 2, Camera = 3, Generic = 4, NoUse = 5 };


#endif
