ExternalProject_Add( ${openigtlinkio_name}
  PREFIX ${external_project_dir}/${openigtlinkio_name}
  GIT_REPOSITORY "git://github.com/IGSIO/OpenIGTLinkIO.git"
  GIT_TAG master
  CMAKE_ARGS -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
             -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
             -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
             -DBUILD_TESTING:BOOL=FALSE
             -DBUILD_SHARED_LIBS:BOOL=FALSE
             -DCMAKE_CXX_FLAGS:STRING=-fPIC
             -DVTK_DIR:PATH=${IBIS_VTK_DIR}
             -DOpenIGTLink_DIR:PATH=${IBIS_OPENIGTLINK_DIR}
             -DIGTLIO_USE_GUI:BOOL=ON
             -DIGTLIO_QT_VERSION:STRING=5
             -DQt5_DIR:PATH=${Qt5_DIR}
  INSTALL_COMMAND ""
  DEPENDS ${vtk_name} ${openigtlink_name} )
