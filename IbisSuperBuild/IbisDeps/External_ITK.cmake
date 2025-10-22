set( itk_prefix ${external_project_dir}/${itk_name} )
if( ${IBIS_BUILD_OPENCV} )
    set( zlib_lib_name "" )
    if( WIN32 )
        set( zlib_lib_debug_name "DEBUG/zlibstatic.lib" )
        set( zlib_lib_release_name "RELEASE/zlibstatic.lib" )
    else()
        set( zlib_lib_debug_name "libz.a" )
        set( zlib_lib_release_name "libz.a" )
    endif()
    ExternalProject_Add( ${itk_name}
        PREFIX ${itk_prefix}
        SOURCE_DIR ${itk_prefix}/src
        BINARY_DIR ${itk_prefix}/build
        STAMP_DIR ${itk_prefix}/stamp
        INSTALL_COMMAND ""
        GIT_REPOSITORY https://github.com/IbisNeuronav/ITK.git
        GIT_TAG "v${IBIS_ITK_LONG_VERSION}"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${itk_name}/install
                -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
                -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                -DITK_BUILD_DEFAULT_MODULES:BOOL=OFF
                -DITKGroup_IO:BOOL=ON
                -DBUILD_TESTING:BOOL=OFF
                -DBUILD_EXAMPLES:BOOL=OFF
                -DITK_USE_GPU:BOOL=ON
                -DITKV4_COMPATIBILITY:BOOL=OFF
                -DITK_LEGACY_REMOVE:BOOL=ON
                -DITK_USE_SYSTEM_ZLIB:BOOL=ON
                -DZLIB_INCLUDE_DIR:STRING=${opencv_prefix}/build/3rdparty/zlib-ng
                -DZLIB_LIBRARY_DEBUG:STRING=${opencv_prefix}/build/lib/${zlib_lib_debug_name}
                -DZLIB_LIBRARY_RELEASE:STRING=${opencv_prefix}/build/lib/${zlib_lib_release_name}
                -DModule_ITKIOMINC:BOOL=ON
                -DModule_ITKReview:BOOL=ON
        DEPENDS ${opencv_name} )
else()
    ExternalProject_Add( ${itk_name}
        PREFIX ${itk_prefix}
        SOURCE_DIR ${itk_prefix}/src
        BINARY_DIR ${itk_prefix}/build
        STAMP_DIR ${itk_prefix}/stamp
        INSTALL_COMMAND ""
        GIT_REPOSITORY https://github.com/IbisNeuronav/ITK.git
        GIT_TAG "v${IBIS_ITK_LONG_VERSION}"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${itk_name}/install
                -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
                -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                -DITK_BUILD_DEFAULT_MODULES:BOOL=OFF
                -DITKGroup_IO:BOOL=ON
                -DBUILD_TESTING:BOOL=OFF
                -DBUILD_EXAMPLES:BOOL=OFF
                -DITK_USE_GPU:BOOL=ON
                -DITKV4_COMPATIBILITY:BOOL=OFF
                -DITK_LEGACY_REMOVE:BOOL=ON
                -DModule_ITKIOMINC:BOOL=ON
                -DModule_ITKReview:BOOL=ON
    )
endif()
