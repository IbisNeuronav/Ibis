# define sources
set( PluginSrc recordtrackingplugininterface.cpp recordtrackingwidget.cpp )
set( PluginHdrMoc recordtrackingwidget.h recordtrackingplugininterface.h )
set( PluginUi recordtrackingwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
