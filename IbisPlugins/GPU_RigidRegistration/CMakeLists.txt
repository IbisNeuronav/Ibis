# define sources
set( PluginSrc
        gpu_rigidregistrationplugininterface.cpp
        gpu_rigidregistrationwidget.cpp
        gpu_rigidregistration.cpp
        qdebugstream.h
)
set( PluginHdrMoc gpu_rigidregistrationwidget.h gpu_rigidregistrationplugininterface.h )
set( PluginUi gpu_rigidregistrationwidget.ui )

IF( NOT OPENCL_FOUND )
  message( SEND_ERROR "OpenCL has not been found. This library is mandatory for the GPU Rigid Registration Plugin to build.\n" )
ENDIF()

include_directories( ${ITK_REGISTRATION_OPENCL_KERNEL_INCLUDE_DIR} ${ITK_REGISTRATION_OPENCL_INCLUDE_DIR} ${OPENCL_INCLUDE_DIRS} )
# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
target_link_libraries( ${PluginName} itkRegistration )
