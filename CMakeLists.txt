CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

cmake_policy(SET CMP0003 OLD)

SET(CMAKE_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon ${CMAKE_MODULE_PATH})

#=====================================================
# Find location to install the progs
#
# This MUST come before PROJECT or it does not work
INCLUDE(${CMAKE_COMMON_DIR}/InstallLocation.cmake)
#======================================================

PROJECT( Ibis )

INCLUDE(CMakeDependentOption)

#==================================================================
# Define a variable to hold the path of automatically compiled
# dependencies.
#==================================================================
set( IBIS_EXTERNAL_DEPENDENCIES_DIR ${CMAKE_CURRENT_BINARY_DIR}/../ibisExternalDependencies CACHE PATH "Path where the external dependencies (itk, vtk, openCV) have been built" )

#==================================================================
# Look for QT5 (required)
#==================================================================
find_package( Qt5 COMPONENTS Widgets Xml REQUIRED )
include_directories( ${Qt5Widgets_INCLUDE_DIRS} )
include_directories( ${Qt5Xml_INCLUDE_DIRS} )

#==================================================================
# Look for VTK (required)
#==================================================================
set( AutoVtkPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/vtk-6.2/install/lib/cmake/vtk-6.2 )
find_package(VTK 6.2 REQUIRED PATHS ${AutoVtkPath} )
# packages needed: vtkCommon vtkHybrid vtkWidgets QVTK vtkRenderingOpenGL vtkInteractionStyle vtkRenderingVolumeOpenGL
#if(NOT VTK_USE_RENDERING)
#  message(FATAL_ERROR "${PROJECT_NAME} requires VTK_USE_RENDERING.")
#endif(NOT VTK_USE_RENDERING)
#if( NOT VTK_USE_QT )
#  message( FATAL_ERROR "${PROJECT_NAME} requires vtk to be build with qt support. Make sure #VTK_USE_QT is ON when building VTK" )
#endif( NOT VTK_USE_QT )
include(${VTK_USE_FILE})

#==================================================================
# Look for ITK (required)
#==================================================================
set( AutoItkPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/itk-4.5.2/install/lib/cmake/ITK-4.5 )
find_package( ITK REQUIRED PATHS ${AutoItkPath} )
include( ${ITK_USE_FILE} )

#==================================================================
# Look for OpenCV (required by some plugins, but not the main app)
#==================================================================
set( AutoOpenCVPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/opencv-3.0.0/install/share/OpenCV )
find_package( OpenCV REQUIRED PATHS ${AutoOpenCVPath} )
include_directories( ${OpenCV_INCLUDE_DIRS} )

#==================================================================
# Include linux include directories, needed for kernel 2.6.x+
#==================================================================
include_directories(/usr/src/linux/include)

#==================================================================
# Create options to build or not the different dependent projects.
#==================================================================
OPTION( IGNS_BUILD_VTK_TRACKER "Build lib vtkTracker" ON )
OPTION( IGNS_BUILD_VTK_EXTENSIONS "Build lib vtkExtensions" ON )
OPTION( IGNS_BUILD_VTKIBIS_VOLUME_RENDERING "Build lib vtkIbisVolumeRendering" ON )
OPTION( IGNS_BUILD_VTK_VIDEO "Build lib vtkVideo" ON )
OPTION( IGNS_BUILD_VTK_QT "Build lib vtkQt" ON )
OPTION( IGNS_BUILD_VTK_ULTRASOUND "Build lib vtkUltrasound" ON )
OPTION( IGNS_INSTALL_LIBS_AND_HEADERS "make install will install all libraries and headers if this option is ON. Otherwise, only executables are installed." OFF )

find_package ( OpenCL QUIET )
include(${CMAKE_COMMON_DIR}/OpenCLMacros.cmake)

CMAKE_DEPENDENT_OPTION( IGNS_BUILD_IBIS "Build ibis application" ON "IGNS_BUILD_IBISLIB" ON )
CMAKE_DEPENDENT_OPTION( IGNS_BUILD_IBIS_PLUGINS "Build Ibis plugins" ON "IGNS_BUILD_IBIS" ON )

set( VTK_ENCODESTRING_EXE ${VTK_INSTALL_PREFIX}/bin/vtkEncodeString-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION} )

#==================================================================
# Build SVL (Simple Vector Library). This lib may be used everywhere
# including in vtk extension classes.
add_subdirectory( svl )
include_directories( ${svlIncludeDir} )
#==================================================================
add_subdirectory( IbisITK )
add_subdirectory( IbisVTK )

# define version and build info
set( IBIS_MAJOR_VERSION 3 )
set( IBIS_MINOR_VERSION 0 )
set( IBIS_PATCH_VERSION 0 )
set( IBIS_VERSION_QUALIFIER "Dev" )                    # Dev, Alpha, Beta or nothing for official releases
if( CMAKE_BUILD_TYPE MATCHES "Debug" )
    set( IBIS_BUILD_QUALIFIER "Debug" )
endif( CMAKE_BUILD_TYPE MATCHES "Debug" )

# include the output directory, where the svnversion.h file is generated
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# a custom target that is always built
add_custom_target(gitversion ALL
    DEPENDS version_git_hash)
# creates githash.h using cmake script
add_custom_command(OUTPUT version_git_hash
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon/getgithash.cmake)

# githash.h is a generated file
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/githash.h
    PROPERTIES GENERATED TRUE
    HEADER_FILE_ONLY TRUE)


# configure version file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h )

# needed for projects to include version.h
include_directories( ${IGNS_BINARY_DIR} )


add_subdirectory( IbisLib )
add_subdirectory( IbisHardware )
add_subdirectory( IbisPlugins )
add_subdirectory( Ibis )


