# define sources
set( PluginSrc landmarkregistrationobjectplugininterface.cpp landmarkregistrationobject.cpp landmarktransform.cpp landmarkregistrationobjectsettingswidget.cpp landmarkregistrationobjectwidget.cpp )
set( PluginHdr landmarktransform.h )
set( PluginHdrMoc landmarkregistrationobjectplugininterface.h landmarkregistrationobject.h landmarkregistrationobjectsettingswidget.h landmarkregistrationobjectwidget.h )
set( PluginUi landmarkregistrationobjectsettingswidget.ui landmarkregistrationobjectwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
