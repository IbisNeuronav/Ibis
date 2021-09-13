#================================
# Define sources
#================================
SET( GPUWeightRegistration_SRC
    itkGPUWeightMatchingMatrixTransformationSparseMask.hxx
    itkGPU3DRigidSimilarityWeightMetric.h
    gpu_weightrigidregistration.cpp
)

SET( GPUWeightRegistration_HDR
    itkGPUWeightMatchingMatrixTransformationSparseMask.h
    gpu_weightrigidregistration.h
)

#================================
# Create custom commands to
# encode each cl file into a
# C string literal in a header
# file.
#================================
set( LibCL GPUWeightMatchingMatrixTransformationSparseMaskKernel.cl )
set( LibCLSrc )
set( LibCLHdr )
foreach (shader_file IN LISTS LibCL)
  vtk_encode_string(
    INPUT "${shader_file}"
    HEADER_OUTPUT clKernel_h
    SOURCE_OUTPUT clKernel_cxx)
  list(APPEND LibCLSrc ${clKernel_cxx})
  list(APPEND LibCLHdr ${clKernel_h})
endforeach ()

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
   ADD_LIBRARY( itkWeightRegistration SHARED ${GPUWeightRegistration_SRC} ${GPUWeightRegistration_HDR} ${LibCLSrc} )
ELSE( BUILD_SHARED_LIBS )
   ADD_LIBRARY( itkWeightRegistration ${GPUWeightRegistration_SRC} ${GPUWeightRegistration_HDR} ${LibCLSrc} )
ENDIF( BUILD_SHARED_LIBS )

target_link_libraries( itkWeightRegistration ${ITK_LIBRARIES} ${VTK_LIBRARIES} ${OPENCL_LIBRARIES} itkRegistrationOpenCL )

#================================
# Define include dir for
# dependent projects
#================================
target_include_directories( itkWeightRegistration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${OPENCL_INCLUDE_DIRS} )
set_property(TARGET itkWeightRegistration PROPERTY POSITION_INDEPENDENT_CODE ON)