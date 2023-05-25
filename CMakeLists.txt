cmake_minimum_required( VERSION 3.5 )

set( CMAKE_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon )

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon ${CMAKE_MODULE_PATH} )

#=====================================================
# Find location to install the progs
#
# This MUST come before PROJECT or it does not work
#======================================================
include( ${CMAKE_COMMON_DIR}/InstallLocation.cmake )

project( ibis )

include( CMakeDependentOption )

#----------------------------------------
# Tell the compiler to use c++11 which is required with Qt >= 5.7
set( CMAKE_CXX_STANDARD 11 )
#----------------------------------------

#==================================================================
# Define a variable to hold the path of automatically compiled
# dependencies.
#==================================================================
include( ${CMAKE_COMMON_DIR}/DependencyVersions.cmake )
set( IBIS_EXTERNAL_DEPENDENCIES_DIR ${CMAKE_CURRENT_BINARY_DIR}/../IbisDeps CACHE PATH "Path where the external dependencies (itk, vtk, openCV) have been built" )

#==================================================================
# Look for QT5 (required)
#==================================================================
option( IBIS_USE_QT_MULTIMEDIA "Find and link with the Multimedia module in Qt. This is not required by Ibis but may be used by some plugins." OFF )
option( IBIS_USE_QT_NETWORK "Find and link with the Network module in Qt. This is not required by Ibis but may be used by some plugins." OFF )
set( IbisQtModules Widgets OpenGL Xml )
if( IBIS_USE_QT_MULTIMEDIA )
    list( APPEND IbisQtModules Multimedia )
endif()
if( IBIS_USE_QT_NETWORK )
    list( APPEND IbisQtModules Network )
endif()
if( UNIX AND NOT APPLE )
    list( APPEND IbisQtModules X11Extras )
endif()
find_package( Qt5 COMPONENTS ${IbisQtModules} REQUIRED )

# Qt support by Qt Company: QT 5.1.0 to 5.5.0 support ended.  5.6.0 still supported until March 2019.
# ibis was never tested with versions 5.6.0, 5.7.0 and 5.8.0. It might work with 5.8.0.
# it worked with 5.9.3

if( Qt5Widgets_FOUND )
    if( Qt5Widgets_VERSION VERSION_LESS 5.6.0 )
        message( SEND_ERROR "Qt version 5.5.x or lower is no longer supported.\nCurrently supported Qt version is 5.9 and higher.\nYou are using " ${Qt5Widgets_VERSION} "." )
    elseif( Qt5Widgets_VERSION VERSION_LESS 5.9.0 )
        message( SEND_ERROR "Currently supported Qt version is 5.9 and higher.\nYou are using " ${Qt5Widgets_VERSION} ". It may work." )
    endif()
else()
    message( SEND_ERROR "The Qt5Widgets library could not be found!" )
endif()

include_directories( ${Qt5Widgets_INCLUDE_DIRS} )
include_directories( ${Qt5Xml_INCLUDE_DIRS} )
include_directories( ${Qt5Multimedia_INCLUDE_DIRS} )

#==================================================================
# Look for VTK (required)
#==================================================================
set( AutoVtkPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/vtk-${IBIS_VTK_LONG_VERSION}/build )
find_package(VTK ${IBIS_VTK_LONG_VERSION} EXACT
    COMPONENTS GUISupportQt RenderingOpenGL2
    RenderingImage RenderingVolumeOpenGL2
    RenderingAnnotation ImagingStencil ImagingStatistics FiltersTexture
    InteractionStyle InteractionWidgets
    IOXML IOLegacy IOPLY
    PATHS ${AutoVtkPath} )

#==================================================================
# Look for ITK (required)
#==================================================================
set( AutoItkPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/itk-${IBIS_ITK_LONG_VERSION}/build )
find_package( ITK ${IBIS_ITK_LONG_VERSION} EXACT REQUIRED PATHS ${AutoItkPath} )
include( ${ITK_USE_FILE} )

#==================================================================
# Look for OpenCV (required by some plugins, but not the main app)
#==================================================================
option( IBIS_USE_OPENCV "Use OpenCV library" OFF )
if( IBIS_USE_OPENCV )
    set( AutoOpenCVPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/opencv-${IBIS_OPENCV_LONG_VERSION}/build )
    find_package( OpenCV ${IBIS_OPENCV_LONG_VERSION} REQUIRED EXACT PATHS ${AutoOpenCVPath} )
endif()

#==================================================================
# Look for Elastix (required by some plugins, but not the main app)
#==================================================================
option( IBIS_USE_ELASTIX "Use Elastix library" OFF )
if( IBIS_USE_ELASTIX )
    set( AutoElastixPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/elastix-${IBIS_ELASTIX_LONG_VERSION}/build )
    find_package( Elastix ${IBIS_ELASTIX_LONG_VERSION} REQUIRED EXACT PATHS ${AutoElastixPath} )
    include( ${ELASTIX_USE_FILE} )
    # Elastix 4.9 does not seem to include CMAEvolutionStrategy directory, we need to do it manually
    include_directories( ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/elastix-${IBIS_ELASTIX_LONG_VERSION}/src/Components/Optimizers/CMAEvolutionStrategy )
endif()

#==================================================================
# Create options to build or not the different dependent projects.
#==================================================================
option( IBIS_BUILD_DEFAULT_HARDWARE_MODULE "Build hardware module based on OpenIGTLink and dependencies (OpenIGTLink and OpenIGTLinkIO)" ON )

find_package ( OpenCL QUIET )
include(${CMAKE_COMMON_DIR}/OpenCLMacros.cmake)

#==================================================================
# Build SVL (Simple Vector Library). This lib may be used everywhere
# including in vtk extension classes.
add_subdirectory( svl )
#==================================================================
add_subdirectory( IbisVTK )
if( IBIS_USE_OPENCV )
    add_subdirectory( IbisOpenCV )
endif()



# define version and build info
set( IBIS_MAJOR_VERSION 4 )
set( IBIS_MINOR_VERSION 0 )
set( IBIS_PATCH_VERSION 0 )
set( IBIS_VERSION_QUALIFIER "Dev" )                    # Dev, Alpha, Beta or nothing for official releases
if( CMAKE_BUILD_TYPE MATCHES "Debug" )
    set( IBIS_BUILD_QUALIFIER "Debug" )
endif( CMAKE_BUILD_TYPE MATCHES "Debug" )

# Track state of local git repository
include(${CMAKE_COMMON_DIR}/gitwatcher.cmake)

# configure version file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h )

add_subdirectory( IbisLib )
set( HardwareModulesLibs "" )
if( IBIS_BUILD_DEFAULT_HARDWARE_MODULE )
    add_subdirectory( IbisHardwareIGSIO )
endif()
add_subdirectory( IbisPlugins )
add_subdirectory( Ibis )

#
# Build the documentation
#
option( IBIS_BUILD_DOCUMENTATION "Build ibis documentation using Doxygen" OFF )
if( IBIS_BUILD_DOCUMENTATION )
    option( DOXYGEN_SHORT_NAMES "Build Doxygen of VTK using short file names" OFF )
    option( DOXYGEN_GENERATE_HTMLHELP "Generate HTML help (CHM) files" ON )
    if( DOXYGEN_SHORT_NAMES )
        set( DOXYGEN_SHORT_NAMES YES )
    else()
        set( DOXYGEN_SHORT_NAMES NO )
    endif()
    if( DOXYGEN_GENERATE_HTMLHELP )
        set( DOXYGEN_GENERATE_HTMLHELP YES )
    else()
        set( DOXYGEN_GENERATE_HTMLHELP NO )
    endif()
    find_package(Doxygen REQUIRED)
    if( DOXYGEN_FOUND )
        set( DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in )
        set( DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxygen/Doxyfile )
        # configure Doxygen file
        configure_file( ${DOXYGEN_IN} ${DOXYGEN_OUT} )
        # note the option ALL which allows to build the docs together with the application
        add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    else( DOXYGEN_FOUND )
        message( "Doxygen needs to be installed to generate documentation" )
    endif()
endif()