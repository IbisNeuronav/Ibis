# define sources
set( PluginSrc usmanualcalibrationplugininterface.cpp usmanualcalibrationwidget.cpp )
set( PluginHdr )
set( PluginHdrMoc usmanualcalibrationwidget.h usmanualcalibrationplugininterface.h )
set( PluginUi usmanualcalibrationwidget.ui )

set( PluginModelFiles calibrationPhantomModel.ply )
configure_file( ${PluginModelFiles} ${CMAKE_CURRENT_BINARY_DIR}/${PluginModelFiles} COPYONLY )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} LandmarkRegistrationObject )
