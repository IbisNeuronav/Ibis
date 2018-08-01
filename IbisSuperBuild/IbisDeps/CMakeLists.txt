cmake_minimum_required(VERSION 3.5)
set( project_name IbisDeps )
project( ${project_name} )

set( external_project_dir ${CMAKE_CURRENT_BINARY_DIR} )

# Get and build vtk
include( External_VTK.cmake )

# Get and build itk
include( External_ITK.cmake )

# Get and build OpenCV
if( IBIS_INSTALL_OPENCV )
    include( External_OpenCV.cmake )
endif()

# Get and build OpenIGTLink and OpenIGTLinkIO
if( IBIS_BUILD_IGTL_HARDWARE_SUPPORT )
  include( External_OpenIGTLink.cmake )
  include( External_OpenIGTLinkIO.cmake )
endif()
