#================================
# Define sources
#================================
SET( IBIS_ITK_SAMPLER_SRC
    itkImageSamplerBase.txx
    itkImageFullSampler.txx
    itkImageGridSampler.hxx
    itkImageRandomSampler.hxx
)

SET( IBIS_ITK_SAMPLER_HDR
    itkImageFullSampler.h
    itkImageGridSampler.h
    itkImageRandomSamplerBase.h
    itkImageRandomSampler.h
    itkImageSample.h
    itkImageSamplerBase.h
)

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
   ADD_LIBRARY( itkSamplers SHARED ${IBIS_ITK_SAMPLER_SRC} ${IBIS_ITK_SAMPLER_HDR} )
ELSE( BUILD_SHARED_LIBS )
   ADD_LIBRARY( itkSamplers ${IBIS_ITK_SAMPLER_SRC} ${IBIS_ITK_SAMPLER_HDR} )
ENDIF( BUILD_SHARED_LIBS )

target_link_libraries( itkSamplers ${ITK_LIBRARIES} )
##================================
## Define include dir for
## dependent projects
##================================
target_include_directories( itkSamplers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

# install
if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib itkSamplers)
    INSTALL_FILES(/include FILES ${IBIS_ITK_SAMPLER_HDR})
endif( IBIS_INSTALL_LIBS_AND_HEADERS )

