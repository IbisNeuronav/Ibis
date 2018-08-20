set( openigtlink_prefix ${external_project_dir}/${openigtlink_name} )
ExternalProject_Add( ${openigtlink_name}
    PREFIX ${openigtlink_prefix}
    SOURCE_DIR ${openigtlink_prefix}/src
    BINARY_DIR ${openigtlink_prefix}/build
    STAMP_DIR ${openigtlink_prefix}/stamp
    INSTALL_COMMAND ""
    GIT_REPOSITORY "git://github.com/openigtlink/OpenIGTLink.git"
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${external_project_dir}/${openigtlink_name}/install
               -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
               -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DBUILD_SHARED_LIBS:BOOL=OFF
               -DCMAKE_CXX_FLAGS:STRING=-fPIC
               -DBUILD_EXAMPLES:BOOL=OFF
               -DBUILD_TESTING:BOOL=OFF
               -DOpenIGTLink_SUPERBUILD:BOOL=OFF
               -DOpenIGTLink_PROTOCOL_VERSION_2:BOOL=OFF
               -DOpenIGTLink_PROTOCOL_VERSION_3:BOOL=ON
               -DOpenIGTLink_ENABLE_VIDEOSTREAMING:BOOL=ON
    INSTALL_COMMAND "" )
