# define sources
set( PluginSrc fibernavigatorplugininterface.cpp fiberobjectsettingswidget.cpp maximas.cpp fractionalAnisotropy.cpp nifti/nifti1_io.c nifti/znzlib.c)
set( PluginHdrMoc fibernavigatorplugininterface.h fiberobjectsettingswidget.h )
set( PluginHdr maximas.h fractionalAnisotropy.h nifti/config.h nifti/nifti1_io.h nifti/nifti1.h nifti/zconf.h nifti/zlib.h nifti/znzlib.h)
set( PluginUi fiberobjectsettingswidget.ui )

add_definitions( -DHAVE_ZLIB )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} z )
