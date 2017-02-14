# define sources
set( PluginSrc stereotacticframeplugininterface.cpp stereotacticframewidget.cpp )
set( PluginHdrMoc stereotacticframewidget.h stereotacticframeplugininterface.h )
set( PluginUi stereotacticframewidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
