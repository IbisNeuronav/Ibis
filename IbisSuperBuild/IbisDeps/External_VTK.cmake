ExternalProject_Add( ${vtk_name}
    SOURCE_DIR ${external_project_dir}/${vtk_name}/src
    BINARY_DIR ${external_project_dir}/${vtk_name}/build
    GIT_REPOSITORY "https://github.com/Kitware/VTK.git"
    GIT_TAG v${IBIS_VTK_LONG_VERSION}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${vtk_name}/install
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
               -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DQT_QMAKE_EXECUTABLE:PATH=${QT_QMAKE_EXECUTABLE}
               -DBUILD_TESTING:BOOL=FALSE
               -DBUILD_SHARED_LIBS:BOOL=FALSE
               -DCMAKE_CXX_FLAGS:STRING=-fPIC
               -DVTK_QT_VERSION:STRING=5
               -DModule_vtkGUISupportQt:BOOL=ON
               -DQt5_DIR:PATH=${Qt5_DIR}
               -DVTK_LEGACY_REMOVE:BOOL=ON
               -DVTK_RENDERING_BACKEND:STRING=OpenGL )
