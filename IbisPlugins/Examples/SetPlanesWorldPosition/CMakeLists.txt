# define sources
set( PluginSrc
        setplanesworldpositionplugininterface.cpp setplanesworldpositionwidget.cpp
)
set( PluginHdr setplanesworldpositionplugininterface.h  setplanesworldpositionwidget.h )

set( PluginHdrMoc setplanesworldpositionplugininterface.h setplanesworldpositionwidget.h)

set( PluginUi setplanesworldpositionwidget.ui )


# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

