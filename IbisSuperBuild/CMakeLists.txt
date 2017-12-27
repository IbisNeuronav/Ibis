cmake_minimum_required(VERSION 3.5)
set( project_name IbisSuperBuild )
project( ${project_name} )

# Find Qt5
find_package( Qt5 COMPONENTS Widgets REQUIRED )

# Find out version numbers of dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeCommon/DependencyVersions.cmake)

# Build All External Dependencies except Qt (vtk, Itk, minc, etc.)
add_subdirectory( IbisDeps )

OPTION( IBIS_INSTALL_OPENCV "Install OpenCV library needed in some plugins" OFF )
# Build Ibis
INCLUDE(ExternalProject)
set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/Ibis-install )
set( VTK_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/vtk-${IBIS_VTK_LONG_VERSION}/install/lib/cmake/vtk-${IBIS_VTK_LONG_VERSION})
set( ITK_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/itk-${IBIS_ITK_LONG_VERSION}/install/lib/cmake/ITK-${IBIS_VTK_SHORT_VERSION} )
if( IBIS_INSTALL_OPENCV )
    set( OpenCV_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/opencv-${IBIS_OPENCV_LONG_VERSION}/install/share/OpenCV )
endif()
ExternalProject_Add( 
    Ibis
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Ibis 
    CMAKE_ARGS -DIBIS_USE_OPENCV=${IBIS_INSTALL_OPENCV} 
               -DOpenCV_DIR=${OpenCV_DIR} 
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT} 
               -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET} 
               -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX} 
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE} 
               -DQt5_DIR:PATH=${Qt5_DIR} 
               -DVTK_DIR:PATH=${VTK_DIR} 
               -DITK_DIR:PATH=${ITK_DIR} } )

add_dependencies( Ibis IbisDeps )
