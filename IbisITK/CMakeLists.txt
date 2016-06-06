PROJECT( IGNS_ITKIBIS )

#================================
# Define include dir for
# dependent projects
#================================
SET( IGNS_ITK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory main dir containing all itk libs for the IGNS project" )
MARK_AS_ADVANCED( IGNS_ITK_INCLUDE_DIR )


#============================================================
# Look for subdirectories if specifies by user
#============================================================


add_subdirectory( itkRegistration )

add_subdirectory( itkNonRigidRegistration )

if ( ${OPENCL_FOUND} )
 	add_subdirectory( itkRegistrationOpenCL )
 	add_subdirectory( itkVolumeReconstructionOpenCL )
endif()