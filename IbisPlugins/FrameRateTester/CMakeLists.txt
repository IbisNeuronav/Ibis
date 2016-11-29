project( ${PluginName} )

# define sources
set( PluginSrc frameratetesterplugininterface.cpp frameratetesterwidget.cpp )
set( PluginHdrMoc frameratetesterwidget.h frameratetesterplugininterface.h )
set( PluginUi frameratetesterwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
