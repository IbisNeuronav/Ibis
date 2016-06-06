PROJECT( IGNS_ITK_REGISTRATION_OPENCL )

#================================
# Define sources
#================================
SET( IGNS_ITK_REGISTRATION_OPENCL_SRC    
    itkOpenCLUtil.cxx
    itkGPUOrientationMatchingMatrixTransformationSparseMask.hxx
    itkGPU3DRigidSimilarityMetric.h
    itkGPU2D3DOrientationMatchingMatrixTransformationSparseMask.hxx
    #itkGPUOrientationMatchingUSCalibration.hxx
    itkGPU2D3DRigidSimilarityMetric.h
    itkGPUUSCalibrationSimilarityMetric.h    
)

SET( IGNS_ITK_REGISTRATION_OPENCL_HDR    
	itkOpenCLUtil.h
	itkGPUOrientationMatchingMatrixTransformationSparseMask.h	
  itkGPU2D3DOrientationMatchingMatrixTransformationSparseMask.h 
  #itkGPUOrientationMatchingUSCalibration.h
)

set( LibCL GPUDiscreteGaussianGradientImageFilter 
          GPUOrientationMatchingMatrixTransformationSparseMaskKernel 
          GPU2D3DOrientationMatchingMatrixTransformationSparseMaskKernel 
          GPUOrientationMatchingUSCalibrationKernel 
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

include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${OPENCL_INCLUDE_DIRS})


#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
   ADD_LIBRARY( itkRegistrationOpenCL SHARED ${IGNS_ITK_REGISTRATION_OPENCL_SRC} ${IGNS_ITK_REGISTRATION_OPENCL_HDR} ${LibCLSrc} )
ELSE( BUILD_SHARED_LIBS )
   ADD_LIBRARY( itkRegistrationOpenCL ${IGNS_ITK_REGISTRATION_OPENCL_SRC} ${IGNS_ITK_REGISTRATION_OPENCL_HDR} ${LibCLSrc} )
ENDIF( BUILD_SHARED_LIBS )

target_link_libraries( itkRegistrationOpenCL ${ITK_LIBRARIES}  ${OPENCL_LIBRARIES})

# install
if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib itkRegistrationOpenCL)
    INSTALL_FILES(/include FILES ${IGNS_ITK_REGISTRATION_OPENCL_HDR})
endif( IGNS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( ITK_REGISTRATION_OPENCL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for itkRegistrationOpenCL .h files" )
MARK_AS_ADVANCED( ITK_REGISTRATION_OPENCL_INCLUDE_DIR )

SET( ITK_REGISTRATION_OPENCL_KERNEL_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}  CACHE PATH "Include directory for itkRegistrationOpenCL Kernel .h files" )
MARK_AS_ADVANCED( ITK_REGISTRATION_OPENCL_KERNEL_INCLUDE_DIR  )
