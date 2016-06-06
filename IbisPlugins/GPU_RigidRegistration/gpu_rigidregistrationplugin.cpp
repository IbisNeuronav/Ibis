#include "cameracalibrationplugininterface.h"

extern "C" PluginInterface * GetPluginInterface()
{
    return new CameraCalibrationPluginInterface;   
}
