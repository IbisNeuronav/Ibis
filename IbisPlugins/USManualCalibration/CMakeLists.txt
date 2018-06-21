# define sources
set( PluginSrc usmanualcalibrationplugininterface.cpp usmanualcalibrationwidget.cpp )
set( PluginHdr )
set( PluginHdrMoc usmanualcalibrationwidget.h usmanualcalibrationplugininterface.h )
set( PluginUi usmanualcalibrationwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} LandmarkRegistrationObject )
