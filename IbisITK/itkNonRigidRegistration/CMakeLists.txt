#================================
# Define sources
#================================
SET( IBIS_ITK_NONRIGID_REGISTRATION_SRC
    itkCannyEdgeDetectionImageFilter2.hxx  
    itkGradientOrientationAlignmentCostFunction.hxx  
    itkVelocityFieldTransformCustom.hxx
    itkCombinationGOACostFunction.hxx      
    itkRescaledGaussianGradientImageFilter.hxx
    itkGOAGradientAscent.hxx               
    itkRescaledImageFilter.hxx
    itkSymbaRegistrationMethod.hxx
    itkVectorLinearInterpolateImageFunction2.hxx    
    vnl_sd_matrix_tools.hxx    
    itkGradientOrientationDifferenceFilter.hxx    
)

SET( IBIS_ITK_NONRIGID_REGISTRATION_HDR
    itkCannyEdgeDetectionImageFilter2.h  
    itkGradientOrientationAlignmentCostFunction.h  
    itkVelocityFieldTransformCustom.h
    itkCombinationGOACostFunction.h      
    itkRescaledGaussianGradientImageFilter.h
    itkGOAGradientAscent.h               
    itkRescaledImageFilter.h
    itkSymbaRegistrationMethod.h
    itkVectorLinearInterpolateImageFunction2.h    
    vnl_sd_matrix_tools.h   
    itkGradientOrientationDifferenceFilter.h
)

# install
if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_FILES(/include FILES ${IBIS_ITK_NONRIGID_REGISTRATION_HDR})
endif( IBIS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( ITK_NONRIGID_REGISTRATION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for itkNonRigidRegistration .h files" )
MARK_AS_ADVANCED( ITK_NONRIGID_REGISTRATION_INCLUDE_DIR )

