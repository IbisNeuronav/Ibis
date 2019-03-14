# define sources
set( PluginSrc
        applytransformtoobjectplugininterface.cpp applytransformtoobjectwidget.cpp
)
set( PluginHdr applytransformtoobjectplugininterface.h  applytransformtoobjectwidget.h )

set( PluginHdrMoc applytransformtoobjectplugininterface.h applytransformtoobjectwidget.h)

set( PluginUi applytransformtoobjectwidget.ui )


# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

