cmake_minimum_required(VERSION 2.8)
set( project_name IbisSuperBuild )
project( ${project_name} )

# Find Qt5
find_package( Qt5 COMPONENTS Widgets REQUIRED )

# Build All External Dependencies except Qt (vtk, Itk, minc, etc.)
add_subdirectory( ibisExternalDependencies )

# Build Ibis
INCLUDE(ExternalProject)
set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/Ibis-install )
set( VTK_DIR ${CMAKE_CURRENT_BINARY_DIR}/ibisExternalDependencies/vtk-6.2/install/lib/cmake/vtk-6.2)
set( ITK_DIR ${CMAKE_CURRENT_BINARY_DIR}/ibisExternalDependencies/itk-4.5.2/install/lib/cmake/ITK-4.5 )
set( OpenCV_DIR ${CMAKE_CURRENT_BINARY_DIR}/ibisExternalDependencies/opencv-3.1.0/install/share/OpenCV )
ExternalProject_Add( Ibis
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Ibis
    BUILD_COMMAND   make
    INSTALL_COMMAND make install 
	  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX} -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE} -DQt5_DIR:PATH=${Qt5_DIR} -DVTK_DIR:PATH=${VTK_DIR} -DITK_DIR:PATH=${ITK_DIR} -DOpenCV_DIR:PATH=${OpenCV_DIR}
  )


add_dependencies( Ibis IbisExternalDependencies )
