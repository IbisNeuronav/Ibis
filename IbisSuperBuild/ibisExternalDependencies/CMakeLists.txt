cmake_minimum_required(VERSION 3.5)
set( project_name IbisExternalDependencies )
project( ${project_name} )

set( external_project_dir ${CMAKE_CURRENT_BINARY_DIR} )

INCLUDE(ExternalProject)

# Get and build vtk
set( vtk_name vtk-${IBIS_VTK_LONG_VERSION} )
ExternalProject_Add( ${vtk_name} 
    SOURCE_DIR ${external_project_dir}/${vtk_name}/src
    BINARY_DIR ${external_project_dir}/${vtk_name}/build
    GIT_REPOSITORY "https://github.com/Kitware/VTK.git"
    GIT_TAG v${IBIS_VTK_LONG_VERSION}
    BUILD_COMMAND   make
    INSTALL_COMMAND make install 
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${vtk_name}/install
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT} -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DQT_QMAKE_EXECUTABLE:PATH=${QT_QMAKE_EXECUTABLE} -DBUILD_TESTING:BOOL=FALSE -DBUILD_SHARED_LIBS:BOOL=FALSE
               -DCMAKE_CXX_FLAGS:STRING=-fPIC -DVTK_QT_VERSION:STRING=5 -DModule_vtkGUISupportQt:BOOL=ON -DQt5_DIR:PATH=${Qt5_DIR}
               -DVTK_LEGACY_REMOVE:BOOL=ON
  )

# Get and build itk
set( itk_name itk-${IBIS_ITK_LONG_VERSION} )
ExternalProject_Add( ${itk_name} 
    SOURCE_DIR ${external_project_dir}/${itk_name}/src
    GIT_REPOSITORY "https://github.com/InsightSoftwareConsortium/ITK.git"
    GIT_TAG v${IBIS_ITK_LONG_VERSION}
	BINARY_DIR ${external_project_dir}/${itk_name}/build
    BUILD_COMMAND   make
    INSTALL_COMMAND make install 
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${itk_name}/install -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT} -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE} -DBUILD_TESTING:BOOL=FALSE -DBUILD_EXAMPLES:BOOL=FALSE -DITKV3_COMPATIBILITY:BOOL=TRUE -DModule_ITKIOMINC=ON -DModule_ITKReview=ON
  )

# Get and build OpenCV
set( opencv_name opencv-${IBIS_OPENCV_LONG_VERSION} )
ExternalProject_Add( ${opencv_name} 
    SOURCE_DIR ${external_project_dir}/${opencv_name}/src
    GIT_REPOSITORY "https://github.com/opencv/opencv.git"
    GIT_TAG ef0811018229c584c21dafa1bf9555c9f56a1d4e
    BINARY_DIR ${external_project_dir}/${opencv_name}/build
    BUILD_COMMAND   make
    INSTALL_COMMAND make install 
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${opencv_name}/install -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT} -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE} -DBUILD_PERF_TESTS:BOOL=FALSE -DBUILD_SHARED_LIBS:BOOL=FALSE -DBUILD_TESTS:BOOL=FALSE -DWITH_TIFF:BOOL=FALSE -DWITH_JASPER:BOOL=FALSE -DBUILD_DOCS:BOOL=FALSE -DBUILD_FAT_JAVA_LIB:BOOL=FALSE -DWITH_CUDA:BOOL=FALSE
  )

add_custom_target( IbisExternalDependencies ALL )
add_dependencies( IbisExternalDependencies ${vtk_name} )
add_dependencies( IbisExternalDependencies ${itk_name} )
add_dependencies( IbisExternalDependencies ${opencv_name} )

