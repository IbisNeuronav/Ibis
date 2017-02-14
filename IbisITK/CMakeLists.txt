#============================================================
# Look for subdirectories if specifies by user
#============================================================


add_subdirectory( itkRegistration )

add_subdirectory( itkNonRigidRegistration )

if ( ${OPENCL_FOUND} )
 	add_subdirectory( itkRegistrationOpenCL )
 	add_subdirectory( itkVolumeReconstructionOpenCL )
endif()