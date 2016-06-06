PROJECT( IGNS_ITK_REGISTRATION )

#================================
# Define sources
#================================
SET( IGNS_ITK_REGISTRATION_SRC
    itkAdvancedImageToImageMetric.hxx
    itkAdvancedOrientationMatchingBetaImageToImageMetric.hxx
    itkDiscreteGaussianGradientImageFilter.txx
    itkImageFullSamplerRandomOffset.txx
    itkImageRandomSamplerSparseMaskRandomOffset.txx
    itkImageSamplerBase.txx
    itkMultiResolutionDiscreteGaussianDerivativePyramidImageFilter.txx
    itkMultiResolutionImageGradientRegistrationMethod.txx
    itkImageToVectorContainerFilter.txx
    itkVectorContainerSource.txx
    itkVectorDataContainer.txx
    itkScaledSingleValuedCostFunction.cxx
    itkMultiResolutionImageRegistrationMethod2.txx
    itkAdvancedTransform.txx
    itkReducedDimensionBSplineInterpolateImageFunction.txx
    itkAdvancedBSplineDeformableTransform.txx
    itkAdvancedBSplineDeformableTransformBase.txx
    itkOrientationMatchingBetaImageFunction.txx
    itkImageMaskSpatialObject2.txx
    itkWeightedGradientToMagnitudeImageFilter.txx
    itkCMAEvolutionStrategyOptimizer.cxx
    itkMultiOrderBSplineDecompositionImageFilter.txx
    itkImageSpatialObject2.txx
    itkScaledSingleValuedNonLinearOptimizer.cxx
    itkImageFullSampler.txx
    itkMultiResolutionPyramidImageFilter2.txx
    itkAdvancedEuler3DTransform.txx
    itkAdvancedMatrixOffsetTransformBase.txx
    itkAdvancedRigid3DTransform.txx
    itkUSCalibrationTransform.hxx
)

SET( IGNS_ITK_REGISTRATION_HDR
    itkAdvancedImageToImageMetric.h
    itkAdvancedOrientationMatchingBetaImageToImageMetric.h
    itkDiscreteGaussianGradientImageFilter.h
    itkImageFullSamplerRandomOffset.h
    itkImageRandomSamplerSparseMaskRandomOffset.h
    itkImageSample.h
    itkImageSamplerBase.h
    itkImageToVectorContainerFilter.h
    itkVectorContainerSource.h
    itkVectorDataContainer.h
    itkMultiResolutionDiscreteGaussianDerivativePyramidImageFilter.h
    itkMultiResolutionImageGradientRegistrationMethod.h
    itkScaledSingleValuedCostFunction.h
    itkMultiResolutionImageRegistrationMethod2.h
    itkAdvancedTransform.h
    itkReducedDimensionBSplineInterpolateImageFunction.h	
    itkLimiterFunctionBase.h
    itkAdvancedBSplineDeformableTransform.h
    itkAdvancedBSplineDeformableTransformBase.h
    itkImageRandomSamplerBase.h
    itkOrientationMatchingBetaImageFunction.h
    itkImageMaskSpatialObject2.h
    itkWeightedGradientToMagnitudeImageFilter.h
    itkCMAEvolutionStrategyOptimizer.h
    itkMultiOrderBSplineDecompositionImageFilter.h
    itkImageSpatialObject2.h
    itkScaledSingleValuedNonLinearOptimizer.h
    itkImageFullSampler.h
    itkMultiResolutionPyramidImageFilter2.h
    itkAdvancedEuler3DTransform.h
    itkEulerTransform.h
    itkAdvancedMatrixOffsetTransformBase.h
    itkAdvancedRigid3DTransform.h
    itkUSCalibrationTransform.h
)

SET( IGNS_ITK_REGISTRATION_LIB_SRC
    itkCMAEvolutionStrategyOptimizer.cxx
    itkScaledSingleValuedNonLinearOptimizer.cxx
    itkScaledSingleValuedCostFunction.cxx
)

SET( IGNS_ITK_REGISTRATION_LIB_HDR
    itkCMAEvolutionStrategyOptimizer.h
    itkScaledSingleValuedNonLinearOptimizer.h
    itkScaledSingleValuedCostFunction.h
)

include_directories( ${IGNS_ITK_INCLUDE_DIR} ) 


#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
   ADD_LIBRARY( itkRegistration SHARED ${IGNS_ITK_REGISTRATION_LIB_SRC} ${IGNS_ITK_REGISTRATION_LIB_HDR} )
ELSE( BUILD_SHARED_LIBS )
   ADD_LIBRARY( itkRegistration ${IGNS_ITK_REGISTRATION_LIB_SRC} ${IGNS_ITK_REGISTRATION_LIB_HDR} )
ENDIF( BUILD_SHARED_LIBS )

target_link_libraries( itkRegistration ${ITK_LIBRARIES} )

# install
if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib itkRegistration)
    INSTALL_FILES(/include FILES ${IGNS_ITK_REGISTRATION_HDR})
endif( IGNS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( ITK_REGISTRATION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for itkRegistration .h files" )
MARK_AS_ADVANCED( ITK_REGISTRATION_INCLUDE_DIR )
