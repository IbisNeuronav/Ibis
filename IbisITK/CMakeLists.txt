#============================================================
# Look for subdirectories if specifies by user
#============================================================

#SET( IBIS_IMAGE_SAMPLER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/itkImageSamplers )

add_subdirectory( itkRegistration )

add_subdirectory( itkNonRigidRegistration )

add_subdirectory( itkImageSamplers )

if ( ${OPENCL_FOUND} )
 	add_subdirectory( itkRegistrationOpenCL )
 	add_subdirectory( itkVolumeReconstructionOpenCL )
endif()
