#================================
# Define sources
#================================

SET( IBIS_ITK_REGISTRATION_LIB_SRC
    itkCMAEvolutionStrategyOptimizer.cxx
    itkScaledSingleValuedNonLinearOptimizer.cxx
    itkScaledSingleValuedCostFunction.cxx
)

SET( IBIS_ITK_REGISTRATION_LIB_HDR
    itkCMAEvolutionStrategyOptimizer.h
    itkScaledSingleValuedNonLinearOptimizer.h
    itkScaledSingleValuedCostFunction.h
)

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
   ADD_LIBRARY( itkRegistration SHARED ${IBIS_ITK_REGISTRATION_LIB_SRC} ${IBIS_ITK_REGISTRATION_LIB_HDR} )
ELSE( BUILD_SHARED_LIBS )
   ADD_LIBRARY( itkRegistration ${IBIS_ITK_REGISTRATION_LIB_SRC} ${IBIS_ITK_REGISTRATION_LIB_HDR} )
ENDIF( BUILD_SHARED_LIBS )

target_link_libraries( itkRegistration ${ITK_LIBRARIES} )
#================================
# Define include dir for
# dependent projects
#================================
target_include_directories( itkRegistration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )


