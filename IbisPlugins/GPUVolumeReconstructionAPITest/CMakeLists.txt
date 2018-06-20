# define sources
set( PluginSrc
        gpuvolumereconstructionapitestplugininterface.cpp
)
set( PluginHdr gpuvolumereconstructionapitestplugininterface.h )

set( PluginHdrMoc gpuvolumereconstructionapitestplugininterface.h )

IF( NOT OPENCL_FOUND )
  message( SEND_ERROR "OpenCL has not been found. This library is mandatory for the GPU Volume Reconstruction Plugin to build.\n" )
ENDIF()


# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} itkVolumeReconstructionOpenCLHeaders GPU_VolumeReconstructionHeaders )

