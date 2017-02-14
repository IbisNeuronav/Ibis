SET( IBIS_VTK_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vtkExtensions\;${CMAKE_CURRENT_SOURCE_DIR}/vtkMNI\;${CMAKE_CURRENT_SOURCE_DIR}/vtkQt"  )
SET( IBIS_VTK_INCLUDE_DIR ${IBIS_VTK_INCLUDE_DIR} PARENT_SCOPE  )

#================================
# Define include dir for
# dependent projects
#================================

add_subdirectory( vtkExtensions )
add_subdirectory( vtkMNI )
add_subdirectory( vtkQt )
