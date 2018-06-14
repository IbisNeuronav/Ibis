#================================
# Define include dir for
# dependent projects
#================================

add_subdirectory( vtkExtensions )
target_include_directories( vtkExtensions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vtkExtensions
   ${CMAKE_CURRENT_SOURCE_DIR}/vtkMNI ${CMAKE_CURRENT_SOURCE_DIR}/vtkQt )

add_subdirectory( vtkMNI )
add_subdirectory( vtkQt )

