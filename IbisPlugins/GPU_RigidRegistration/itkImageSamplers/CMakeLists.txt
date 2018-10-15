#================================
# Define sources
#================================
set( IBIS_ITK_SAMPLER_SRC
    itkImageSamplerBase.txx
    itkImageFullSampler.txx
    itkImageGridSampler.hxx
    itkImageRandomSampler.hxx
)

set( IBIS_ITK_SAMPLER_HDR
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
add_library( itkSamplers INTERFACE )
# TODO : Find a way to define sources that CMake likes so that files appear in the project
#target_sources( itkSamplers INTERFACE ${IBIS_ITK_SAMPLER_SRC} ${IBIS_ITK_SAMPLER_HDR} )

##================================
## Define include dir for
## dependent projects
##================================
target_include_directories( itkSamplers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )


