set( itk_prefix ${external_project_dir}/${itk_name} )
ExternalProject_Add( ${itk_name}
    PREFIX ${itk_prefix}
    SOURCE_DIR ${itk_prefix}/src
    BINARY_DIR ${itk_prefix}/build
    STAMP_DIR ${itk_prefix}/stamp
    INSTALL_COMMAND ""
    GIT_REPOSITORY https://github.com/IbisNeuronav/ITK.git
    GIT_TAG 2e610a9ca92c7363ab65d3f85baff95ae1fb54a6
    GIT_TAG v${IBIS_ITK_LONG_VERSION}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${itk_name}/install
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
               -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DBUILD_TESTING:BOOL=FALSE
               -DBUILD_EXAMPLES:BOOL=FALSE
               -DITK_USE_GPU:BOOL=TRUE
               -DITKV3_COMPATIBILITY:BOOL=TRUE
               -DModule_ITKIOMINC=ON
               -DModule_ITKReview=ON )
