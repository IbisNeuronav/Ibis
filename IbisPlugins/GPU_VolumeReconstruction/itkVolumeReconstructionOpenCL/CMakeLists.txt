#================================
# Define sources
#================================
SET( IBIS_ITK_VOLUME_RECONSTRUCTION_OPENCL_SRC
    itkGPUVolumeReconstruction.hxx
)

SET( IBIS_ITK_VOLUME_RECONSTRUCTION_OPENCL_HDR
  	itkGPUVolumeReconstruction.h
)

#================================
# Create custom commands to
# encode each cl file into a
# C string literal in a header
# file.
#================================
set( LibCL GPUVolumeReconstructionKernel.cl )
set( LibCLSrc )
set( LibCLHdr )
foreach (shader_file IN LISTS LibCL)
  vtk_encode_string(
    INPUT         "${shader_file}"
    HEADER_OUTPUT clKernel_h
    SOURCE_OUTPUT clKernel_cxx)
  list(APPEND LibCLSrc ${clKernel_cxx})
  list(APPEND LibCLHdr ${clKernel_h})
endforeach ()

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
   ADD_LIBRARY( itkVolumeReconstructionOpenCL SHARED ${IBIS_ITK_VOLUME_RECONSTRUCTION_OPENCL_SRC} ${IBIS_ITK_VOLUME_RECONSTRUCTION_OPENCL_HDR} ${LibCLSrc} )
ELSE( BUILD_SHARED_LIBS )
   ADD_LIBRARY( itkVolumeReconstructionOpenCL ${IBIS_ITK_VOLUME_RECONSTRUCTION_OPENCL_SRC} ${IBIS_ITK_VOLUME_RECONSTRUCTION_OPENCL_HDR} ${LibCLSrc} )
ENDIF( BUILD_SHARED_LIBS )

target_link_libraries( itkVolumeReconstructionOpenCL ${ITK_LIBRARIES}  ${OPENCL_LIBRARIES})
#================================
# Define include dir for
# dependent projects
#================================
target_include_directories( itkVolumeReconstructionOpenCL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${OPENCL_INCLUDE_DIRS} )

