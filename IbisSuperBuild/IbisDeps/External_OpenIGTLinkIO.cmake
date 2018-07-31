set( IBIS_VTK_DIR ${CMAKE_CURRENT_BINARY_DIR}/vtk-${IBIS_VTK_LONG_VERSION}/install/lib/cmake/vtk-${IBIS_VTK_LONG_VERSION})
set( IBIS_OPENIGTLINK_DIR ${CMAKE_CURRENT_BINARY_DIR}/${igtl_name}/build )
ExternalProject_Add( ${igtlio_name}
  SOURCE_DIR ${external_project_dir}/${igtlio_name}/src
  BINARY_DIR ${external_project_dir}/${igtlio_name}/build
  GIT_REPOSITORY "git@github.com/IGSIO/OpenIGTLinkIO.git"
  GIT_TAG master
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${igtlio_name}/install
             -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
             -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
             -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE} 
             -DBUILD_TESTING:BOOL=FALSE
             -DBUILD_SHARED_LIBS:BOOL=FALSE
             -DCMAKE_CXX_FLAGS:STRING=-fPIC
             -DVTK_DIR:PATH=${IBIS_VTK_DIR}
             -DOpenIGTLink_DIR:PATH=${IBIS_OPENIGTLINK_DIR}
             -DIGTLIO_USE_GUI:BOOL=ON
  INSTALL_COMMAND ""
  DEPENDS ${vtk_name} ${igtl_name} )
