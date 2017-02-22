CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

cmake_policy(SET CMP0003 OLD)

SET(CMAKE_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon ${CMAKE_MODULE_PATH})

#=====================================================
# Find location to install the progs
#
# This MUST come before PROJECT or it does not work
INCLUDE(${CMAKE_COMMON_DIR}/InstallLocation.cmake)
#======================================================

PROJECT( ibis )

INCLUDE(CMakeDependentOption)

#==================================================================
# Define a variable to hold the path of automatically compiled
# dependencies.
#==================================================================
include(${CMAKE_COMMON_DIR}/DependencyVersions.cmake)
set( IBIS_EXTERNAL_DEPENDENCIES_DIR ${CMAKE_CURRENT_BINARY_DIR}/../ibisExternalDependencies CACHE PATH "Path where the external dependencies (itk, vtk, openCV) have been built" )

#==================================================================
# Look for QT5 (required)
#==================================================================
find_package( Qt5 COMPONENTS Widgets OpenGL Xml REQUIRED )
include_directories( ${Qt5Widgets_INCLUDE_DIRS} )
include_directories( ${Qt5Xml_INCLUDE_DIRS} )

#==================================================================
# Look for VTK (required)
#==================================================================
set( AutoVtkPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/vtk-${IBIS_VTK_LONG_VERSION}/install/lib/cmake/vtk-${IBIS_VTK_SHORT_VERSION} )
find_package(VTK ${IBIS_VTK_LONG_VERSION} EXACT COMPONENTS vtkGUISupportQt vtkRenderingOpenGL vtkRenderingVolumeOpenGL vtkRenderingImage
    vtkRenderingAnnotation vtkImagingStencil vtkImagingStatistics vtkFiltersTexture
    vtkInteractionStyle vtkInteractionWidgets
    vtkIOXML vtkIOLegacy NO_MODULE
    PATHS ${AutoVtkPath} )
include(${VTK_USE_FILE})

#==================================================================
# Look for ITK (required)
#==================================================================
set( AutoItkPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/itk-${IBIS_ITK_LONG_VERSION}/install/lib/cmake/ITK-${IBIS_ITK_SHORT_VERSION} )
find_package( ITK ${IBIS_ITK_SHORT_VERSION} EXACT REQUIRED PATHS ${AutoItkPath} )
include( ${ITK_USE_FILE} )

#==================================================================
# Look for OpenCV (required by some plugins, but not the main app)
#==================================================================
if( DEFINED OpenCV_DIR )
    set( AutoOpenCVPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/opencv-${IBIS_OPENCV_LONG_VERSION}/install/share/OpenCV )
    find_package( OpenCV REQUIRED PATHS ${AutoOpenCVPath} )
    include_directories( ${OpenCV_INCLUDE_DIRS} )
endif()

#==================================================================
# Include linux include directories, needed for kernel 2.6.x+
#==================================================================
include_directories(/usr/src/linux/include)

#==================================================================
# Create options to build or not the different dependent projects.
#==================================================================
if( NOT APPLE )
    OPTION( IBIS_BUILD_DEFAULT_HARDWARE_MODULE "Build default hardware module on Linux." ON )
endif()
OPTION( IBIS_BUILD_IGSIO_HARDWARE_MODULE "Build hardware module that allows connection to OpenIGTLinkServers" OFF )
OPTION( IBIS_INSTALL_LIBS_AND_HEADERS "make install will install all libraries and headers if this option is ON. Otherwise, only executables are installed." OFF )

find_package ( OpenCL QUIET )
include(${CMAKE_COMMON_DIR}/OpenCLMacros.cmake)

set( VTK_ENCODESTRING_EXE ${VTK_INSTALL_PREFIX}/bin/vtkEncodeString-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION} )

#==================================================================
# Build SVL (Simple Vector Library). This lib may be used everywhere
# including in vtk extension classes.
add_subdirectory( svl )
include_directories( ${svlIncludeDir} )
#==================================================================
add_subdirectory( IbisITK )
add_subdirectory( IbisVTK )
if( DEFINED OpenCV_DIR )
    add_subdirectory( IbisOpenCV )
endif()

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
include_directories( ${Ibis_BINARY_DIR} )

add_subdirectory( IbisLib )
set( HardwareModulesLibs "" )
if( IBIS_BUILD_DEFAULT_HARDWARE_MODULE )
    add_subdirectory( IbisHardware )
endif()
if( IBIS_BUILD_IGSIO_HARDWARE_MODULE )
    add_subdirectory( IbisHardwareIGSIO )
endif()
add_subdirectory( IbisPlugins )
add_subdirectory( Ibis )


