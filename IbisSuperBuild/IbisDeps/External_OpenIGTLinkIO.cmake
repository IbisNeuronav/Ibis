set( openigtlinkio_prefix ${external_project_dir}/${openigtlinkio_name} )
ExternalProject_Add( ${openigtlinkio_name}
  PREFIX ${openigtlinkio_prefix}
  SOURCE_DIR ${openigtlinkio_prefix}/src
  BINARY_DIR ${openigtlinkio_prefix}/build
  STAMP_DIR ${openigtlinkio_prefix}/stamp
  INSTALL_COMMAND ""
  GIT_REPOSITORY "git://github.com/IGSIO/OpenIGTLinkIO.git"
  #the tag is used to mark the version that compiles and links with the first version of ibis 4.x, it will have to be removed altogether once OpenIGTLinkIO is stable
  GIT_TAG  30a04eb14e43b439178ba13130b7a2c1fb2b51c2
  CMAKE_ARGS -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
             -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
             -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
             -DBUILD_TESTING:BOOL=FALSE
             -DIGTLIO_USE_EXAMPLES:BOOL=OFF
             -DBUILD_SHARED_LIBS:BOOL=FALSE
             -DCMAKE_CXX_FLAGS:STRING=-fPIC
             -DVTK_DIR:PATH=${IBIS_VTK_DIR}
             -DOpenIGTLink_DIR:PATH=${IBIS_OPENIGTLINK_DIR}
             -DIGTLIO_USE_GUI:BOOL=ON
             -DIGTLIO_QT_VERSION:STRING=5
             -DQt5_DIR:PATH=${Qt5_DIR}
  INSTALL_COMMAND ""
  DEPENDS ${vtk_name} ${openigtlink_name} )
