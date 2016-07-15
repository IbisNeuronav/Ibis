PROJECT( IGNS_VTKIBIS )

#================================
# Define include dir for
# dependent projects
#================================
SET( IGNS_VTK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory main dir containing all vtk extension libs for the IGNS project" )
MARK_AS_ADVANCED( IGNS_VTK_INCLUDE_DIR )

add_subdirectory( vtkExtensions )
add_subdirectory( vtkMNI )
add_subdirectory( vtkTracker )
add_subdirectory( vtkVideo )
add_subdirectory( vtkQt )
add_subdirectory( vtkUltrasound )

