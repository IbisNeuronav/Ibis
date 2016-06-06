project( ${PluginName} )

# define sources
set( PluginSrc usacquisitionplugininterface.cpp doubleviewwidget.cpp )
set( PluginHdrMoc usacquisitionplugininterface.h doubleviewwidget.h )
set( PluginUi doubleviewwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
