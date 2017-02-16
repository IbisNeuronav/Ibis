# define sources
set( PluginSrc contoursurfaceplugininterface.cpp generatedsurface.cpp surfacesettingswidget.cpp )
set( PluginHdrMoc contoursurfaceplugininterface.h surfacesettingswidget.h )
set( PluginHdr generatedsurface.h )
set( PluginUi surfacesettingswidget.ui )
# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
