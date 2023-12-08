set( itk_prefix ${external_project_dir}/${itk_name} )
ExternalProject_Add( ${itk_name}
    PREFIX ${itk_prefix}
    SOURCE_DIR ${itk_prefix}/src
    BINARY_DIR ${itk_prefix}/build
    STAMP_DIR ${itk_prefix}/stamp
    INSTALL_COMMAND ""
    GIT_REPOSITORY https://github.com/IbisNeuronav/ITK.git
    GIT_TAG "399bffe6b87b127b943386b34e669208334874de"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${itk_name}/install
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
               -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DITK_BUILD_DEFAULT_MODULES:BOOL=FALSE
               -DITKGroup_IO:BOOL=TRUE
               -DBUILD_TESTING:BOOL=FALSE
               -DBUILD_EXAMPLES:BOOL=FALSE
               -DITK_USE_GPU:BOOL=TRUE
               -DITKV4_COMPATIBILITY:BOOL=TRUE
			   -DITK_LEGACY_REMOVE=FALSE
               -DModule_ITKIOMINC=ON
               -DModule_ITKReview=ON )
