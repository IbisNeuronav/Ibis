/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __GlobalEventHandler_h_
#define __GlobalEventHandler_h_

class QKeyEvent;

// Virtual class that objects and plugins can reimplement to receive global events
// before they are dispatched to lower-level windows.
class GlobalEventHandler
{
public:
    // Handles keyboard event wherever the focus is in the application
    // note: return true if event is handled
    virtual bool HandleKeyboardEvent( QKeyEvent * keyEvent ) { return false; }
};

#endif
