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

set( LibCL
    GPUWeightMatchingMatrixTransformationSparseMaskKernel
)

#================================
# Create custom commands to
# encode each cl file into a
# C string literal in a header
# file.
#================================
foreach( file ${LibCL} )
  set( clKernel_src ${CMAKE_CURRENT_SOURCE_DIR}/${file}.cl )
  set( clKernel_cxx ${CMAKE_CURRENT_BINARY_DIR}/${file}.cxx )
  set( clKernel_h ${CMAKE_CURRENT_BINARY_DIR}/${file}.h )
  add_custom_command(
    OUTPUT ${clKernel_cxx} ${clKernel_h}
    DEPENDS ${clKernel_src}
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${clKernel_cxx} ${clKernel_src} ${file} --build-header " " vtkSystemIncludes.h )
  set( LibCLSrc ${LibCLSrc} ${clKernel_src} ${clKernel_h} ${clKernel_cxx} )
  set( LibCLHdr ${LibCLHdr} ${clKernel_h} )
endforeach(file)

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
   ADD_LIBRARY( itkWeightRegistration SHARED ${GPUWeightRegistration_SRC} ${GPUWeightRegistration_HDR} ${LibCLSrc} )
ELSE( BUILD_SHARED_LIBS )
   ADD_LIBRARY( itkWeightRegistration ${GPUWeightRegistration_SRC} ${GPUWeightRegistration_HDR} ${LibCLSrc} )
ENDIF( BUILD_SHARED_LIBS )

target_link_libraries( itkWeightRegistration ${ITK_LIBRARIES} ${OPENCL_LIBRARIES} itkRegistrationOpenCL )

#================================
# Define include dir for
# dependent projects
#================================
target_include_directories( itkWeightRegistration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${OPENCL_INCLUDE_DIRS} )
set_property(TARGET itkWeightRegistration PROPERTY POSITION_INDEPENDENT_CODE ON)