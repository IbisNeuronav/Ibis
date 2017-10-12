include_directories( ${GPU_VolumeReconstructionIncludeDir} )
include_directories( ${ITK_VOLUME_RECONSTRUCTION_OPENCL_KERNEL_INCLUDE_DIR} ${ITK_VOLUME_RECONSTRUCTION_OPENCL_INCLUDE_DIR} ${OPENCL_INCLUDE_DIRS} )

# define sources
set( PluginSrc usacquisitionplugininterface.cpp doubleviewwidget.cpp )
set( PluginHdrMoc usacquisitionplugininterface.h doubleviewwidget.h )
set( PluginUi doubleviewwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
