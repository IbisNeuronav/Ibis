# define sources
set( PluginSrc imagefilterexampleplugininterface.cpp imagefilterexamplewidget.cpp )
set( PluginHdrMoc imagefilterexamplewidget.h imagefilterexampleplugininterface.h )
set( PluginUi imagefilterexamplewidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
