PROJECT( IGNS_VTKIBIS )

#================================
# Define include dir for
# dependent projects
#================================
SET( IGNS_VTK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory main dir containing all vtk extension libs for the IGNS project" )
MARK_AS_ADVANCED( IGNS_VTK_INCLUDE_DIR )

#============================================================
# Look for subdirectories if specifies by user
#============================================================

if( IGNS_BUILD_VTK_EXTENSIONS )
    add_subdirectory( vtkExtensions )
endif()

#============================================================
# Always include vtkMNI which contains tools to read/write some MNI file formats
#============================================================
add_subdirectory( vtkMNI )

#if( IGNS_BUILD_VTK_MINC )
#    add_subdirectory( vtkMinc )
#endif()

if( IGNS_BUILD_VTK_TRACKER )
    add_subdirectory( vtkTracker )
endif()

if( IGNS_BUILD_VTK_VIDEO )
    add_subdirectory( vtkVideo )
endif()

if( IGNS_BUILD_VTK_QT )
    add_subdirectory( vtkQt )
endif()

if( IGNS_BUILD_VTK_ULTRASOUND )
    add_subdirectory( vtkUltrasound )
endif()

if( IGNS_BUILD_VTKIBIS_VOLUME_RENDERING )
    add_subdirectory( vtkIbisVolumeRendering )
endif()

