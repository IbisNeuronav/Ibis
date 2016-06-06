project( ${PluginName} )

include_directories( ${LandmarkRegistrationObjectIncludeDir} )

# define sources
set( PluginSrc usmanualcalibrationplugininterface.cpp usmanualcalibrationwidget.cpp )
set( PluginHdr )
set( PluginHdrMoc usmanualcalibrationwidget.h usmanualcalibrationplugininterface.h )
set( PluginUi usmanualcalibrationwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
