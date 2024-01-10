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
               -DBUILD_PERF_TESTS:BOOL=OFF
               -DBUILD_SHARED_LIBS:BOOL=OFF
               -DBUILD_WITH_STATIC_CRT:BOOL=OFF
               -DBUILD_TESTS:BOOL=OFF
               -DWITH_TIFF:BOOL=OFF
               -DWITH_JASPER:BOOL=OFF
               -DWITH_OPENEXR:BOOL=OFF
               -DBUILD_DOCS:BOOL=OFF
               -DBUILD_FAT_JAVA_LIB:BOOL=OFF
               -DWITH_CUDA:BOOL=OFF
               -DWITH_VTK:BOOL=OFF
               -DWITH_EIGEN:BOOL=OFF
               -DWITH_1394:BOOL=OFF
               -DWITH_GSTREAMER:BOOL=OFF
               -DWITH_GTK:BOOL=OFF
               -DWITH_ITT:BOOL=OFF
               -DBUILD_opencv_python2:BOOL=OFF
               -DBUILD_opencv_python3:BOOL=OFF
               -DBUILD_opencv_python_bindings_generator:BOOL=OFF
               -DBUILD_opencv_python_tests:BOOL=OFF
               -DOPENCV_PYTHON_SKIP_DETECTION=ON
               -DBUILD_opencv_apps:BOOL=OFF
               -DBUILD_JAVA:BOOL=OFF
               -DBUILD_PACKAGE:BOOL=OFF
                )
