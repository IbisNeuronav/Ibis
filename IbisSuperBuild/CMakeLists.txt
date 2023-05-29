cmake_minimum_required(VERSION 3.5)
set( project_name IbisSuperBuild )
project( ${project_name} )

# Find Qt5
find_package( Qt5 COMPONENTS Widgets REQUIRED )

# Find out version numbers of dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeCommon/DependencyVersions.cmake)

include(ExternalProject)

# Optional builds
option( IBIS_BUILD_IGTL_HARDWARE_SUPPORT "Build hardware module based on OpenIGTLink and dependencies (OpenIGTLink and OpenIGTLinkIO)" ON )
option( IBIS_INSTALL_OPENCV "Install OpenCV library needed in some plugins" OFF )
option( IBIS_BUILD_ELASTIX "Build the Elastix library needed in some plugins" OFF )

# Build All External Dependencies except Qt (vtk, Itk, minc, etc.)
set( vtk_name vtk-${IBIS_VTK_LONG_VERSION} )
set( IBIS_VTK_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/${vtk_name}/build )
set( itk_name itk-${IBIS_ITK_LONG_VERSION} )
set( IBIS_ITK_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/${itk_name}/build )
set( opencv_name opencv-${IBIS_OPENCV_LONG_VERSION} )
set( IBIS_OPENCV_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/${opencv_name}/build )
set( elastix_name elastix-${IBIS_ELASTIX_LONG_VERSION} )
set( IBIS_ELASTIX_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/${elastix_name}/build )
set( openigtlink_name OpenIGTLink )
set( IBIS_OPENIGTLINK_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/${openigtlink_name}/build )
set( openigtlinkio_name OpenIGTLinkIO )
set( IBIS_OPENIGTLINKIO_DIR ${CMAKE_CURRENT_BINARY_DIR}/IbisDeps/${openigtlinkio_name}/build)
add_subdirectory( IbisDeps )

# Build Ibis
set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/Ibis-install )
set( ibis_dependencies ${vtk_name} ${itk_name} )
if( IBIS_INSTALL_OPENCV )
    list( APPEND ibis_dependencies ${opencv_name} )
endif()
if( IBIS_BUILD_ELASTIX )
    list( APPEND ibis_dependencies ${elastix_name} )
endif()
if( IBIS_BUILD_IGTL_HARDWARE_SUPPORT )
    list( APPEND ibis_dependencies ${openigtlink_name} ${openigtlinkio_name} )
endif()
ExternalProject_Add(
    Ibis
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/Ibis
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Ibis
    CMAKE_ARGS -DIBIS_USE_OPENCV=${IBIS_INSTALL_OPENCV}
               -DOpenCV_DIR=${IBIS_OPENCV_DIR}
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
               -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
               -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DCMAKE_CONFIGURATION_TYPES:STRING=${CMAKE_CONFIGURATION_TYPES}
               -DQt5_DIR:PATH=${Qt5_DIR}
               -DVTK_DIR:PATH=${IBIS_VTK_DIR}
               -DITK_DIR:PATH=${IBIS_ITK_DIR}
               -DElastix_DIR:PATH=${IBIS_ELASTIX_DIR}
               -DIBIS_USE_ELASTIX:BOOL=${IBIS_BUILD_ELASTIX}
               -DOpenIGTLink_DIR:PATH=${IBIS_OPENIGTLINK_DIR}
               -DOpenIGTLinkIO_DIR:PATH=${IBIS_OPENIGTLINKIO_DIR}
               -DIBIS_BUILD_DEFAULT_HARDWARE_MODULE:BOOL=${IBIS_BUILD_IGTL_HARDWARE_SUPPORT}
    DEPENDS ${ibis_dependencies} )
