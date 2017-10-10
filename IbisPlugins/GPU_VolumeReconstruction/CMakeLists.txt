# define sources
set( PluginSrc
        gpu_volumereconstructionplugininterface.cpp
        gpu_volumereconstructionwidget.cpp
        gpu_volumereconstruction.cpp
)
set( PluginHdr gpu_volumereconstructionwidget.h gpu_volumereconstruction.h gpu_volumereconstructionplugininterface.h )
set( PluginHdrMoc gpu_volumereconstruction.h gpu_volumereconstructionwidget.h gpu_volumereconstructionplugininterface.h )
set( PluginUi gpu_volumereconstructionwidget.ui )

IF( NOT OPENCL_FOUND )
  message( SEND_ERROR "OpenCL has not been found. This library is mandatory for the GPU Volume Reconstruction Plugin to build.\n" )
ENDIF()

include_directories( ${ITK_VOLUME_RECONSTRUCTION_OPENCL_KERNEL_INCLUDE_DIR} ${ITK_VOLUME_RECONSTRUCTION_OPENCL_INCLUDE_DIR} ${OPENCL_INCLUDE_DIRS} )
# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
