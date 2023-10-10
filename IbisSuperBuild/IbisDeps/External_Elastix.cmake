set( elastix_prefix ${external_project_dir}/${elastix_name} )

if( SUPERBUILD_BUILD_MINIMAL_ELASTIX )
    ExternalProject_Add( ${elastix_name}
        PREFIX ${elastix_prefix}
        SOURCE_DIR ${elastix_prefix}/src
        BINARY_DIR ${elastix_prefix}/build
        STAMP_DIR ${elastix_prefix}/stamp
        INSTALL_COMMAND ""
        GIT_REPOSITORY "https://github.com/SuperElastix/elastix.git"
        GIT_TAG ${IBIS_ELASTIX_LONG_VERSION}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${elastix_name}/install
                -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
                -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                -DCMAKE_CXX_FLAGS=-std=c++11
                -DBUILD_TESTING:BOOL=FALSE
                -DITK_DIR:PATH=${IBIS_ITK_DIR}
                -DELASTIX_BUILD_EXECUTABLE:BOOL=FALSE
                # Set what we want:
                -DUSE_CMAEvolutionStrategy:BOOL=TRUE
                -DUSE_FullSampler:BOOL=TRUE
                -DUSE_GridSampler:BOOL=TRUE
                -DUSE_RandomSampler:BOOL=TRUE
                # Turn everything else off:
                -DUSE_AdaptiveStochasticGradientDescent:BOOL=OFF
                -DUSE_AdvancedAffineTransformElastix:BOOL=OFF
                -DUSE_AdvancedBSplineTransform:BOOL=OFF
                -DUSE_AdvancedKappaStatisticMetric:BOOL=OFF
                -DUSE_AdvancedMattesMutualInformationMetric:BOOL=OFF
                -DUSE_AdvancedMeanSquaresMetric:BOOL=OFF
                -DUSE_AdvancedNormalizedCorrelationMetric:BOOL=OFF
                -DUSE_AffineDTITransformElastix:BOOL=OFF
                -DUSE_AffineLogStackTransform:BOOL=OFF
                -DUSE_AffineLogTransformElastix:BOOL=OFF
                -DUSE_BSplineInterpolator:BOOL=OFF
                -DUSE_BSplineInterpolatorFloat:BOOL=OFF
                -DUSE_BSplineResampleInterpolator:BOOL=OFF
                -DUSE_BSplineResampleInterpolatorFloat:BOOL=OFF
                -DUSE_BSplineStackTransform:BOOL=OFF
                -DUSE_BSplineTransformWithDiffusion:BOOL=OFF
                -DUSE_ConjugateGradient:BOOL=OFF
                -DUSE_ConjugateGradientFRPR:BOOL=OFF
                -DUSE_CorrespondingPointsEuclideanDistanceMetric:BOOL=OFF
                -DUSE_DeformationFieldTransform:BOOL=OFF
                -DUSE_DisplacementMagnitudePenalty:BOOL=OFF
                -DUSE_DistancePreservingRigidityPenalty:BOOL=OFF
                -DUSE_EulerStackTransform:BOOL=OFF
                -DUSE_EulerTransformElastix:BOOL=OFF
                -DUSE_FiniteDifferenceGradientDescent:BOOL=OFF
                -DUSE_FixedGenericPyramid:BOOL=OFF
                -DUSE_FixedRecursivePyramid:BOOL=OFF
                -DUSE_FixedShrinkingPyramid:BOOL=OFF
                -DUSE_FixedSmoothingPyramid:BOOL=OFF
                -DUSE_FullSearch:BOOL=OFF
                -DUSE_GradientDifferenceMetric:BOOL=OFF
                -DUSE_KNNGraphAlphaMutualInformationMetric:BOOL=OFF
                -DUSE_LinearInterpolator:BOOL=OFF
                -DUSE_LinearResampleInterpolator:BOOL=OFF
                -DUSE_MissingStructurePenalty:BOOL=OFF
                -DUSE_MovingGenericPyramid:BOOL=OFF
                -DUSE_MovingRecursivePyramid:BOOL=OFF
                -DUSE_MovingShrinkingPyramid:BOOL=OFF
                -DUSE_MovingSmoothingPyramid:BOOL=OFF
                -DUSE_MultiBSplineTransformWithNormal:BOOL=OFF
                -DUSE_MultiInputRandomCoordinateSampler:BOOL=OFF
                -DUSE_MultiMetricMultiResolutionRegistration:BOOL=OFF
                -DUSE_MultiResolutionRegistration:BOOL=OFF
                -DUSE_MultiResolutionRegistrationWithFeatures:BOOL=OFF
                -DUSE_MutualInformationHistogramMetric:BOOL=OFF
                -DUSE_MyStandardResampler:BOOL=OFF
                -DUSE_NearestNeighborInterpolator:BOOL=OFF
                -DUSE_NearestNeighborResampleInterpolator:BOOL=OFF
                -DUSE_NormalizedGradientCorrelationMetric:BOOL=OFF
                -DUSE_NormalizedMutualInformationMetric:BOOL=OFF
                -DUSE_OpenCLFixedGenericPyramid:BOOL=OFF
                -DUSE_OpenCLMovingGenericPyramid:BOOL=OFF
                -DUSE_OpenCLResampler:BOOL=OFF
                -DUSE_PCAMetric:BOOL=OFF
                -DUSE_PCAMetric2:BOOL=OFF
                -DUSE_PatternIntensityMetric:BOOL=OFF
                -DUSE_PolydataDummyPenalty:BOOL=OFF
                -DUSE_Powell:BOOL=OFF
                -DUSE_QuasiNewtonLBFGS:BOOL=OFF
                -DUSE_RSGDEachParameterApart:BOOL=OFF
                -DUSE_RandomCoordinateSampler:BOOL=OFF
                -DUSE_RandomSamplerSparseMask:BOOL=OFF
                -DUSE_RayCastInterpolator:BOOL=OFF
                -DUSE_RayCastResampleInterpolator:BOOL=OFF
                -DUSE_RecursiveBSplineTransform:BOOL=OFF
                -DUSE_ReducedDimensionBSplineInterpolator:BOOL=OFF
                -DUSE_ReducedDimensionBSplineResampleInterpolator:BOOL=OFF
                -DUSE_RegularStepGradientDescent:BOOL=OFF
                -DUSE_SimilarityTransformElastix:BOOL=OFF
                -DUSE_Simplex:BOOL=OFF
                -DUSE_SimultaneousPerturbation:BOOL=OFF
                -DUSE_SplineKernelTransform:BOOL=OFF
                -DUSE_StandardGradientDescent:BOOL=OFF
                -DUSE_StatisticalShapePenalty:BOOL=OFF
                -DUSE_SumOfPairwiseCorrelationCoefficientsMetric:BOOL=OFF
                -DUSE_SumSquaredTissueVolumeDifferenceMetric:BOOL=OFF
                -DUSE_TransformBendingEnergyPenalty:BOOL=OFF
                -DUSE_TransformRigidityPenalty:BOOL=OFF
                -DUSE_TranslationStackTransform:BOOL=OFF
                -DUSE_TranslationTransformElastix:BOOL=OFF
                -DUSE_VarianceOverLastDimensionMetric:BOOL=OFF
                -DUSE_ViolaWellsMutualInformationMetric:BOOL=OFF
                -DUSE_WeightedCombinationTransformElastix:BOOL=OFF
        DEPENDS ${itk_name} )
else()
    ExternalProject_Add( ${elastix_name}
        PREFIX ${elastix_prefix}
        SOURCE_DIR ${elastix_prefix}/src
        BINARY_DIR ${elastix_prefix}/build
        STAMP_DIR ${elastix_prefix}/stamp
        INSTALL_COMMAND ""
        GIT_REPOSITORY "https://github.com/SuperElastix/elastix.git"
        GIT_TAG ${IBIS_ELASTIX_LONG_VERSION}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${elastix_name}/install
                -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
                -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                -DCMAKE_CXX_FLAGS=-std=c++11
                -DBUILD_TESTING:BOOL=FALSE
                -DITK_DIR:PATH=${IBIS_ITK_DIR}
                -DELASTIX_BUILD_EXECUTABLE:BOOL=FALSE
                -DUSE_CMAEvolutionStrategy:BOOL=TRUE
        DEPENDS ${itk_name} )
endif()
