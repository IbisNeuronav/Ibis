
find_package(VTK REQUIRED NO_MODULE COMPONENTS RenderingImage )

# define sources
set( PluginSrc usacquisitionplugininterface.cpp doubleviewwidget.cpp )
set( PluginHdrMoc usacquisitionplugininterface.h doubleviewwidget.h )
set( PluginUi doubleviewwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

target_link_libraries( ${PluginName} ${VTK_LIBRARIES} )
