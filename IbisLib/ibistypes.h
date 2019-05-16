/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __IbisTypes_h_
#define __IbisTypes_h_

enum InteractorStyle { InteractorStyleTerrain = 0, InteractorStyleTrackball = 1, InteractorStyleJoystick = 2 };
enum VIEWTYPES {SAGITTAL_VIEW_TYPE = 0, CORONAL_VIEW_TYPE = 1, TRANSVERSE_VIEW_TYPE = 2, THREED_VIEW_TYPE = 3};
enum MAINVIEWIDS {SAGITTAL_VIEW_ID = -5, CORONAL_VIEW_ID = -4, THREED_VIEW_ID = -3, TRANSVERSE_VIEW_ID = -2};
enum TrackerToolState{ Ok, Missing, OutOfVolume, OutOfView, HighError, Disabled, Undefined };
enum IbisPluginTypes{ IbisPluginTypeTool = 0, IbisPluginTypeObject = 1, IbisPluginTypeGlobalObject = 2, IbisPluginTypeImportExport = 3, IbisPluginTypeHardwareModule = 4, IbisPluginTypeGenerator = 5 };

#endif
