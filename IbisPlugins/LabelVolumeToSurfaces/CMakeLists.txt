project( ${PluginName} )

# define sources
set( PluginSrc labelvolumetosurfacesplugininterface.cpp )
set( PluginHdrMoc labelvolumetosurfacesplugininterface.h )
set( PluginHdr  )
set( PluginUi )
# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
