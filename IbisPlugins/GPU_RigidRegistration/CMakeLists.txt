# define sources
set( PluginSrc
        gpu_rigidregistrationplugininterface.cpp
        gpu_rigidregistrationwidget.cpp
        gpu_rigidregistration.cpp
        qdebugstream.h
    )
set( PluginHdrMoc 
        gpu_rigidregistrationwidget.h
        gpu_rigidregistrationplugininterface.h
        gpu_rigidregistration.h
    )
set( PluginUi gpu_rigidregistrationwidget.ui )

IF( NOT OPENCL_FOUND )
  message( SEND_ERROR "OpenCL has not been found. This library is mandatory for the GPU Rigid Registration Plugin to build.\n" )
ENDIF()

IF( NOT ITK_USE_GPU )
  message( SEND_ERROR "GPU_RigidRegistration plugin cannot build if ITK was built without ITK_USE_GPU enabled.\n" )
ENDIF()

IF( NOT IBIS_USE_ELASTIX )
  message( SEND_ERROR "GPU_RigidRegistration plugin require Elastix library. Please enable IBIS_USE_ELASTIX.\n" )
ENDIF()

add_subdirectory( itkRegistrationOpenCL )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} itkRegistrationOpenCL )
