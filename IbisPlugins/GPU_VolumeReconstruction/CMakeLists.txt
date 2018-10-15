# define sources
set( PluginSrc
        gpu_volumereconstructionplugininterface.cpp
        gpu_volumereconstructionwidget.cpp
        gpu_volumereconstruction.cpp
)
set( PluginHdr gpu_volumereconstructionwidget.h gpu_volumereconstructionplugininterface.h  )
set( PluginHdrMoc gpu_volumereconstructionwidget.h gpu_volumereconstructionplugininterface.h gpu_volumereconstruction.h )
set( PluginUi gpu_volumereconstructionwidget.ui )

if ( ${OPENCL_FOUND} )
    add_subdirectory( itkVolumeReconstructionOpenCL )
endif()

IF( NOT OPENCL_FOUND )
  message( SEND_ERROR "OpenCL has not been found. This library is mandatory for the GPU Volume Reconstruction Plugin to build.\n" )
ENDIF()

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
target_link_libraries( ${PluginName} itkVolumeReconstructionOpenCL )
