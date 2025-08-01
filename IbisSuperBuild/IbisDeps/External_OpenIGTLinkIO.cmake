set( openigtlinkio_prefix ${external_project_dir}/${openigtlinkio_name} )
cmake_path(GET Qt6_DIR PARENT_PATH Qt6_root)
ExternalProject_Add( ${openigtlinkio_name}
  PREFIX ${openigtlinkio_prefix}
  SOURCE_DIR ${openigtlinkio_prefix}/src
  BINARY_DIR ${openigtlinkio_prefix}/build
  STAMP_DIR ${openigtlinkio_prefix}/stamp
  INSTALL_COMMAND ""
  GIT_REPOSITORY "https://github.com/IbisNeuronav/OpenIGTLinkIO.git"
  GIT_TAG  master
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
             -DIGTLIO_QT_VERSION:STRING=6
             -DQt6_DIR:PATH=${Qt6_DIR}
             -DQt6CoreTools_DIR:PATH=${Qt6_root}/Qt6CoreTools
             -DQt6GuiTools_DIR:PATH=${Qt6_root}/Qt6GuiTools
             -DQt6QmlTools_DIR:PATH=${Qt6_root}/Qt6QmlTools
  INSTALL_COMMAND ""
  DEPENDS ${vtk_name} ${openigtlink_name} )
