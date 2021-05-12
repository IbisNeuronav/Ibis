# define sources
set( PluginSrc sequenceioplugininterface.cpp sequenceiowidget.cpp )
set( PluginHdrMoc sequenceiowidget.h sequenceioplugininterface.h )
set( PluginUi sequenceiowidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
