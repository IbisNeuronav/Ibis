#================================
# Define subprojects
#================================

add_subdirectory( vtkExtensions )
target_include_directories( vtkExtensions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vtkExtensions ${CMAKE_CURRENT_SOURCE_DIR}/vtkMNI ${CMAKE_CURRENT_SOURCE_DIR}/vtkQt )

#================================
# Define interface for dependent project headers
#================================
add_library( ibisVTKHeaders INTERFACE  )
target_compile_definitions( ibisVTKHeaders INTERFACE LIBRARY_HEADER_ONLY )
target_include_directories( ibisVTKHeaders INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vtkExtensions ${CMAKE_CURRENT_SOURCE_DIR}/vtkMNI ${CMAKE_CURRENT_SOURCE_DIR}/vtkQt )

add_subdirectory( vtkMNI )
add_subdirectory( vtkQt )
