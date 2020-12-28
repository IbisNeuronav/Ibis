set( opencv_prefix ${external_project_dir}/${opencv_name} )
ExternalProject_Add( ${opencv_name}
    PREFIX ${opencv_prefix}
    SOURCE_DIR ${opencv_prefix}/src
    BINARY_DIR ${opencv_prefix}/build
    STAMP_DIR ${opencv_prefix}/stamp
    INSTALL_COMMAND ""
    GIT_REPOSITORY "https://github.com/opencv/opencv.git"
    GIT_TAG ${IBIS_OPENCV_LONG_VERSION}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${opencv_name}/install
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
               -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DBUILD_PERF_TESTS:BOOL=FALSE
               -DBUILD_SHARED_LIBS:BOOL=FALSE
               -DBUILD_WITH_STATIC_CRT:BOOL=FALSE
               -DBUILD_TESTS:BOOL=FALSE
               -DWITH_TIFF:BOOL=FALSE
               -DWITH_JASPER:BOOL=FALSE
               -DBUILD_DOCS:BOOL=FALSE
               -DBUILD_FAT_JAVA_LIB:BOOL=FALSE
               -DWITH_CUDA:BOOL=FALSE
               -DWITH_VTK:BOOL=FALSE
               -DWITH_EIGEN:BOOL=OFF
               -DWITH_1394:BOOL=OFF
               -DWITH_GSTREAMER:BOOL=OFF
               -DWITH_GTK:BOOL=OFF
               -DWITH_ITT:BOOL=OFF
               -DBUILD_opencv_python2:BOOL=FALSE
               -DBUILD_opencv_python3:BOOL=FALSE
               -DBUILD_opencv_apps:BOOL=FALSE
               -DBUILD_JAVA:BOOL=FALSE
               -DBUILD_PACKAGE:BOOL=FALSE
                )
