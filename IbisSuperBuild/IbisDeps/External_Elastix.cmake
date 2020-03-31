set( elastix_prefix ${external_project_dir}/${elastix_name} )
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
               -DBUILD_TESTING:BOOL=FALSE
               -DITK_DIR:PATH=${IBIS_ITK_DIR}
               -DELASTIX_BUILD_EXECUTABLE:BOOL=FALSE
               -DUSE_CMAEvolutionStrategy:BOOL=TRUE
    DEPENDS ${itk_name} )
