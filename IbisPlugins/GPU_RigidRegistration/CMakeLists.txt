# define sources
set( PluginSrc
        gpu_rigidregistrationplugininterface.cpp
        gpu_rigidregistrationwidget.cpp
        qdebugstream.h
    )
set( PluginHdrMoc 
        gpu_rigidregistrationwidget.h
        gpu_rigidregistrationplugininterface.h
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

#--------------------------------------------------
# Fetch and setup build for GPURigidRegistration lib
#--------------------------------------------------
include(FetchContent)
#FetchContent_Declare(
#  GPURigidRegistrationLib
#  GIT_REPOSITORY https://github.com/drouin-simon/GPURigidRegistrationLib.git
#  GIT_TAG fix-build-mac-remove-vtk-mni-from-lib )

FetchContent_Declare(
  GPURigidRegistrationLib
  GIT_REPOSITORY https://github.com/IbisNeuronav/GPURigidRegistrationLib.git
  GIT_TAG main )

# Don't build library's tests
set( GPURR_BUILD_TEST OFF CACHE BOOL "" FORCE)
set( GPURR_BUILD_EXECUTABLE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(GPURigidRegistrationLib)
#--------------------------------------------------

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} GPURigidRegistrationLib )
