# define sources
set( PluginSrc
        gpuvolumereconstructionapitestplugininterface.cpp
)
set( PluginHdr gpuvolumereconstructionapitestplugininterface.h )

set( PluginHdrMoc gpuvolumereconstructionapitestplugininterface.h )

IF( NOT OPENCL_FOUND )
  message( SEND_ERROR "OpenCL has not been found. This library is mandatory for the GPU Volume Reconstruction Plugin to build.\n" )
ENDIF()


include_directories( ${GPU_VolumeReconstructionIncludeDir} )
include_directories( ${ITK_VOLUME_RECONSTRUCTION_OPENCL_KERNEL_INCLUDE_DIR} ${ITK_VOLUME_RECONSTRUCTION_OPENCL_INCLUDE_DIR} ${OPENCL_INCLUDE_DIRS} )
# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
